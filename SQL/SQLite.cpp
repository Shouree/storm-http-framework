#include "stdafx.h"
#include "SQLite.h"
#include "Exception.h"

namespace sql {


	/////////////////////////////////////
	//			  Statement			   //
	/////////////////////////////////////

	SQLite_Statement::SQLite_Statement(const SQLite *database, Str *str) {
		db = database;
		result = false;
		lastId = 0;
		lastChanges = 0;
		error = null;

		int ok = sqlite3_prepare_v2(db->raw(), str->utf8_str(), -1, &stmt, null);
		if (ok != SQLITE_OK) {
			Str *msg = new (this) Str((wchar *)sqlite3_errmsg16(db->raw()));
			throw new (this) SQLError(msg);
		}
	}

	SQLite_Statement::~SQLite_Statement() {
		finalize();
	}

	void SQLite_Statement::reset() {
		if (result)
			sqlite3_reset(stmt);
		result = false;
		error = null;
	}

	void SQLite_Statement::bind(Nat pos, Str *str) {
		reset();
		sqlite3_bind_text(stmt, pos + 1, str->utf8_str(), -1, SQLITE_TRANSIENT);
	}

	void SQLite_Statement::bind(Nat pos, Bool b) {
		reset();
		sqlite3_bind_int(stmt, pos + 1, b ? 1 : 0);
	}

	void SQLite_Statement::bind(Nat pos, Int i) {
		reset();
		sqlite3_bind_int(stmt, pos + 1, i);
	}

	void SQLite_Statement::bind(Nat pos, Long l) {
		reset();
		sqlite3_bind_int64(stmt, pos + 1, l);
	}

	void SQLite_Statement::bind(Nat pos, Double d) {
		reset();
		sqlite3_bind_double(stmt, pos + 1, d);
	}

	void SQLite_Statement::bindNull(Nat pos) {
		reset();
		sqlite3_bind_null(stmt, pos + 1);
	}

	void SQLite_Statement::execute() {
		reset();

		invalidateIterators();

		int r = sqlite3_step(stmt);

		if (r == SQLITE_DONE) {
			// No data. We're done.
			lastId = (Int)sqlite3_last_insert_rowid(db->raw());
			lastChanges = sqlite3_changes(db->raw());
			sqlite3_reset(stmt);
			result = false;
		} else if (r == SQLITE_ROW) {
			// We have data!
			result = true;
		} else {
			Str *msg = new (this) Str((wchar*)sqlite3_errmsg16(db->raw()));
			throw new (this) SQLError(msg);
		}
	}

	void SQLite_Statement::finalize() {
		if (stmt) {
			sqlite3_finalize(stmt);
			stmt = null;
			result = false;
		}
	}

	void SQLite_Statement::done() {
		reset();
	}

	Row *SQLite_Statement::fetch() {
		if (error) {
			Str *msg = error;
			error = null;
			throw new (this) SQLError(msg);
		}

		// No result, don't do anything.
		if (!result)
			return null;

		Engine &e = engine();
		int num_column = sqlite3_column_count(stmt);
		Array<Variant> *row = new (this) Array<Variant>();
		row->reserve(Nat(num_column));

		for (int i = 0; i < num_column; i++){
			switch (sqlite3_column_type(stmt, i)) {
			case SQLITE3_TEXT:
				row->push(Variant(new (this)Str((wchar *)sqlite3_column_text16(stmt, i)), e));
				break;
			case SQLITE_INTEGER:
				row->push(Variant(Long(sqlite3_column_int64(stmt, i)), e));
				break;
			case SQLITE_FLOAT:
				row->push(Variant(sqlite3_column_double(stmt, i), e));
				break;
			case SQLITE_NULL:
				row->push(Variant());
				break;
			default:
				assert(false, L"Unknown column type from SQLite!");
				break;
			}
		}


		// Go to the next row.
		int r = sqlite3_step(stmt);
		if (r == SQLITE_DONE) {
			sqlite3_reset(stmt);
			result = false;
		} else if (r != SQLITE_ROW) {
			error = new (this) Str((wchar *)sqlite3_errmsg16(db->raw()));
			sqlite3_reset(stmt);
			result = false;
		}

		return new (this) Row(row);
	}

	Int SQLite_Statement::lastRowId() const {
		return lastId;
	}

	Nat SQLite_Statement::changes() const {
		return lastChanges;
	}

	////////////////////////////////////
	//			   SQLite			  //
	////////////////////////////////////

	SQLite::~SQLite() {
		close();
	}

	SQLite::SQLite(Url * str) {
		int rc = sqlite3_open16(str -> format() -> c_str(), &db);

		if (rc)
			throw new (this) InternalError(TO_S(this, S("Can't open database: ") << rc));
	}

	SQLite::SQLite() {
		int rc = sqlite3_open(":memory:", &db);

		if (rc)
			throw new (this) InternalError(TO_S(this, S("Can't open database: ") << rc));
	}

	Statement * SQLite::prepare(Str *str) {
		return new (str) SQLite_Statement(this, str);
	}

	void SQLite::close() {
		sqlite3_close(db);
	}

	sqlite3 *SQLite::raw() const {
		return db;
	}

	Array<Str*>* SQLite::tables(){
		Str * str = new (this) Str(L"SELECT name FROM sqlite_master WHERE type IN ('table', 'view') AND name NOT LIKE 'sqlite%' ORDER BY 1");

		Array<Str*>* names = new (this) Array<Str*>;

		Statement * stmt = prepare(str);
		stmt->execute();

		Row * name;
		Statement::Iter i = stmt->iter();
		while (name = i.next())
			names->push(name->getStr(0));

		stmt->finalize();
		return names;
	}

	static bool isWS(wchar c) {
		switch (c) {
		case ' ':
		case '\n':
		case '\r':
		case '\t':
			return true;
		default:
			return false;
		}
	}

	static bool isSpecial(wchar c) {
		switch (c) {
		case '(':
		case ')':
		case ',':
		case '.':
		case ';':
			return true;
		default:
			return false;
		}
	}

	static const wchar *skipWS(const wchar *at) {
		for (; *at; at++) {
			if (!isWS(*at))
				break;
		}
		return at;
	}

	static const wchar *nextToken(const wchar *at) {
		if (*at == '"') {
			// Identifier. Quite easy, no escape sequences need to be taken into consideration (I think).
			for (at++; *at; at++) {
				if (*at == '"')
					return at + 1;
			}
		} else if (*at == '\'') {
			// String literal. '' is used as an "escape", otherwise easy.
			for (at++; *at; at++) {
				if (*at == '\'') {
					if (at[1] == '\'') {
						// Escaped, go on.
						at++;
					} else {
						// End. We're done.
						return at + 1;
					}
				}
			}
		} else if (isSpecial(*at)) {
			return at + 1;
		} else {
			// Some keyword or an unquoted literal.
			for (; *at; at++) {
				if (isWS(*at) || isSpecial(*at))
					return at;
			}
		}

		return at;
	}

	static bool cmp(const wchar *begin, const wchar *end, const wchar *str) {
		for (; begin != end; begin++, str++) {
			if (*str == 0)
				return false;
			if (*str != *begin)
				return false;
		}

		return *str == 0;
	}

	static Str *identifier(Engine &e, const wchar *begin, const wchar *end) {
		if (*begin == '"')
			return new (e) Str(begin + 1, end - 1);
		else
			return new (e) Str(begin, end);
	}

	static bool next(const wchar *&begin, const wchar *&end) {
		begin = skipWS(end);
		if (*begin == 0)
			return false;
		end = nextToken(begin);
		return true;
	}

	// Include any parameters to the type in a SQL statement.
	static const wchar *parseType(const wchar *typeend) {
		const wchar *begin = typeend;
		const wchar *end = typeend;
		next(begin, end);
		if (!cmp(begin, end, S("("))) {
			// No parameter!
			return typeend;
		}

		// Find an end paren.
		do {
			next(begin, end);
		} while (cmp(begin, end, S(")")));

		return end;
	}

	static Str *identifierBefore(Engine &e, const wchar *&begin, const wchar *&end, const wchar *token) {
		const wchar *nameBegin = begin;
		const wchar *nameEnd = end;
		while (next(begin, end)) {
			if (cmp(begin, end, token))
				break;

			nameBegin = begin;
			nameEnd = end;
		}

		return identifier(e, nameBegin, nameEnd);
	}

	static Array<Str *> *parsePK(Engine &e, const wchar *&oBegin, const wchar *&oEnd) {
		const wchar *begin = oBegin;
		const wchar *end = oEnd;

		if (!cmp(begin, end, S("PRIMARY")))
			return null;

		next(begin, end);
		if (!cmp(begin, end, S("KEY")))
			return null;

		next(begin, end);
		if (!cmp(begin, end, S("(")))
			return null;

		Array<Str *> *cols = new (e) Array<Str *>();
		while (next(begin, end)) {
			cols->push(identifier(e, begin, end));

			next(begin, end);
			if (cmp(begin, end, S(")")))
				break;
			if (!cmp(begin, end, S(",")))
				throw new (e) InternalError(S("Failed parsing primary key string."));
		}

		oBegin = begin;
		oEnd = end;
		return cols;
	}

	Schema *SQLite::schema(Str *table) {
		// Note: There is PRAGMA table_info(table); that we could use. It does not seem like we get
		// information on other constraints from there though.

		// Tables:

		Str *query = new (this) Str(S("SELECT sql FROM sqlite_master WHERE type = 'table' AND name = ?;"));
		Statement *prepared = prepare(query);
		prepared->bind(0, table);
		prepared->execute();
		Row *row = prepared->iter().next();
		if (!row)
			return null;

		const wchar *data = row->getStr(0)->c_str();
		const wchar *begin = data;
		const wchar *end = data;

		prepared->finalize();

		// Find the "(" and remember the name in the previous token.
		Str *tableName = identifierBefore(engine(), begin, end, S("("));

		Array<Str *> *pk = null;
		Array<Schema::Column *> *cols = new (this) Array<Schema::Column *>();
		while (next(begin, end)) {
			if (pk = parsePK(engine(), begin, end)) {
				next(begin, end);
			} else {
				Str *colName = identifier(engine(), begin, end);
				next(begin, end);
				end = parseType(end);
				Str *typeName = identifier(engine(), begin, end);
				Str *attributes = null;

				next(begin, end);
				if (cmp(begin, end, S(",")) || cmp(begin, end, S(")"))) {
					attributes = new (this) Str();
				} else {
					const wchar *attrStart = begin;
					const wchar *attrEnd = begin;
					do {
						attrEnd = end;
						next(begin, end);
					} while (*begin != 0 && !cmp(begin, end, S(")")) && !cmp(begin, end, S(",")));
					attributes = new (this) Str(attrStart, attrEnd);
				}

				cols->push(new (this) Schema::Column(colName, typeName, attributes));
			}

			if (cmp(begin, end, S(")")))
				break;
		}

		// Indices:

		Array<Schema::Index *> *indices = new (this) Array<Schema::Index *>();

		query = new (this) Str(S("SELECT sql FROM sqlite_master WHERE type = 'index' AND tbl_name = ? AND sql IS NOT NULL;"));
		prepared = prepare(query);
		prepared->bind(0, table);
		prepared->execute();
		Statement::Iter iter = prepared->iter();
		while (Row *row = iter.next()) {
			const wchar *data = row->getStr(0)->c_str();
			const wchar *begin = data;
			const wchar *end = data;

			Str *name = identifierBefore(engine(), begin, end, S("ON"));
			Array<Str *> *cols = new (this) Array<Str *>();

			while (next(begin, end))
				if (cmp(begin, end, S("(")))
					break;

			while (next(begin, end)) {
				cols->push(identifier(engine(), begin, end));

				next(begin, end);
				if (cmp(begin, end, S(")")))
					break;
			}

			*indices << new (this) Schema::Index(name, cols);
		}

		prepared->finalize();

		return new (this) Schema(tableName, cols, pk, indices);
	}

}
