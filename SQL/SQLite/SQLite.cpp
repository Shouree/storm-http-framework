#include "stdafx.h"
#include "SQLite.h"
#include "Exception.h"
#include "String.h"
#include "Core/Set.h"

namespace sql {

	/**
	 * The base SQLite connection.
	 */

	SQLite::SQLite(Url *str) {
		int rc = sqlite3_open16(str->format()->c_str(), &db);
		if (rc) {
			StrBuf *msg = new (this) StrBuf();
			*msg << S("Unable to open database: ") << str->format()
				 << S(". Error code: ") << rc;
			throw new (this) InternalError(msg->toS());
		}
	}

	SQLite::SQLite() {
		int rc = sqlite3_open(":memory:", &db);
		if (rc) {
			StrBuf *msg = new (this) StrBuf();
			*msg << S("Unable to open in-memory database: ") << rc;
			throw new (this) InternalError(msg->toS());
		}
	}

	SQLite::~SQLite() {
		close();
	}

	Bool SQLite::implicitAutoIncrement() const {
		return true;
	}

	Bool SQLite::fullAlterTable() const {
		return false;
	}

	void SQLite::close() {
		if (db) {
			sqlite3_close(db);
			db = null;
		}
	}

	Statement *SQLite::prepare(Str *query) {
		return new (this) Stmt(this, query);
	}

	Statement *SQLite::prepare(QueryStr *query) {
		return prepare(query->generate(new (this) Visitor()));
	}


	SQLite::Visitor::Visitor() {}

	void SQLite::Visitor::type(StrBuf *to, QueryType type) {
		if (type.sameType(QueryType::text())) {
			*to << S("TEXT");
		} else if (type.sameType(QueryType::integer())) {
			*to << S("INTEGER");
		} else if (type.sameType(QueryType::real())) {
			*to << S("REAL");
		} else {
			throw new (this) SQLError(TO_S(this, S("Unknown type: ") << type << S(".")));
		}
	}


	Array<Str *> *SQLite::tables() {
		Str *str = new (this) Str(S("SELECT name FROM sqlite_master WHERE type IN ('table', 'view') AND name NOT LIKE 'sqlite%'"));
		Array<Str *> *names = new (this) Array<Str*>();

		Statement *stmt = prepare(str);
		Statement::Result r = stmt->execute();

		while (true) {
			Maybe<Row> row = r.next();
			if (row.empty())
				break;
			names->push(row.value().getStr(0));
		}

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
		return compareNoCase(begin, end, str);
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

	static Set<Str *> *parsePK(Engine &e, const wchar *&oBegin, const wchar *&oEnd) {
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

		Set<Str *> *cols = new (e) Set<Str *>();
		while (next(begin, end)) {
			cols->put(identifier(e, begin, end));

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

	static bool nextIf(const wchar *&begin, const wchar *&end, const wchar *find) {
		if (!cmp(begin, end, find))
			return false;

		next(begin, end);
		return true;
	}

	static bool nextIf(const wchar *&begin, const wchar *&end, const wchar *find1, const wchar *find2) {
		const wchar *tBegin = begin;
		const wchar *tEnd = end;

		if (!cmp(tBegin, tEnd, find1))
			return false;

		next(tBegin, tEnd);
		if (!cmp(tBegin, tEnd, find2))
			return false;

		next(tBegin, tEnd);
		begin = tBegin;
		end = tEnd;
		return true;
	}

	Schema *SQLite::schema(Str *table) {
		// Note: There is PRAGMA table_info(table); that we could use. It does not seem like we get
		// information on other constraints from there though.

		// Tables:

		Str *query = new (this) Str(S("SELECT sql FROM sqlite_master WHERE type = 'table' AND name = ?;"));
		Statement *prepared = prepare(query);
		prepared->bind(0, table);
		Statement::Result result = prepared->execute();
		Maybe<Row> row = result.next();
		if (row.empty())
			return null;

		const wchar *data = row.value().getStr(0)->c_str();
		const wchar *begin = data;
		const wchar *end = data;

		prepared->finalize();

		// Find the "(" and remember the name in the previous token.
		Str *tableName = identifierBefore(engine(), begin, end, S("("));

		Set<Str *> *pk = null;
		Array<Schema::Column *> *cols = new (this) Array<Schema::Column *>();
		while (next(begin, end)) {
			if (pk = parsePK(engine(), begin, end)) {
				next(begin, end);
			} else {
				Str *colName = identifier(engine(), begin, end);
				next(begin, end);
				end = parseType(end);
				Str *typeName = identifier(engine(), begin, end);

				Schema::Column *col = new (this) Schema::Column(colName, QueryType::parse(typeName));
				if (col->type.empty())
					col->unknown = typeName;

				next(begin, end);
				while (!cmp(begin, end, S(",")) && !cmp(begin, end, S(")"))) {
					if (nextIf(begin, end, S("UNIQUE"))) {
						col->attributes |= Schema::unique;
					} else if (nextIf(begin, end, S("PRIMARY"), S("KEY"))) {
						col->attributes |= Schema::primaryKey;
					} else if (nextIf(begin, end, S("NOT"), S("NULL"))) {
						col->attributes |= Schema::notNull;
					} else if (nextIf(begin, end, S("AUTOINCREMENT")) || nextIf(begin, end, S("AUTO_INCREMENT"))) {
						col->attributes |= Schema::autoIncrement;
					} else if (nextIf(begin, end, S("DEFAULT"))) {
						col->defaultValue = new (this) Str(begin, end);
						next(begin, end);
					} else {
						Str *here = new (this) Str(begin, end);
						next(begin, end);

						if (col->unknown)
							col->unknown = TO_S(this, col->unknown << S(" ") << here);
						else
							col->unknown = here;
					}
				}

				cols->push(col);
			}

			if (cmp(begin, end, S(")")))
				break;
		}

		// Update columns to store primary key information.
		if (pk) {
			for (Nat i = 0; i < cols->count(); i++) {
				Schema::Column *col = cols->at(i);
				if (pk->has(col->name))
					col->attributes |= Schema::primaryKey;
			}
		}


		// Indices:

		Array<Schema::Index *> *indices = new (this) Array<Schema::Index *>();

		query = new (this) Str(S("SELECT sql FROM sqlite_master WHERE type = 'index' AND tbl_name = ? AND sql IS NOT NULL;"));
		prepared = prepare(query);
		prepared->bind(0, table);
		Statement::Result iter = prepared->execute();
		while (true) {
			Maybe<Row> row = iter.next();
			if (row.empty())
				break;
			const wchar *data = row.value().getStr(0)->c_str();
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

		return new (this) Schema(tableName, cols, indices);
	}


	/**
	 * Statements.
	 */

	SQLite::Stmt::Stmt(SQLite *owner, Str *statement) : owner(owner), lastId(0), lastChanges(0) {
		int status = sqlite3_prepare_v2(owner->db, statement->utf8_str(), -1, &stmt, null);
		if (status != SQLITE_OK)
			throw new (this) SQLError(new (this) Str((wchar *)sqlite3_errmsg16(owner->db)));
		isClean = true;
		hasRow = false;
	}

	SQLite::Stmt::~Stmt() {
		finalize();
	}

	void SQLite::Stmt::finalize() {
		if (stmt) {
			invalidateResult();
			sqlite3_finalize(stmt);
			stmt = null;
			moreRows = false;
			hasRow = false;
		}
	}

	void SQLite::Stmt::reset() {
		if (isClean)
			return;

		invalidateResult();
		sqlite3_reset(stmt);
		isClean = true;
		hasRow = false;
		moreRows = false;
	}

	void SQLite::Stmt::bind(Nat pos, Str *str) {
		reset();
		sqlite3_bind_text(stmt, pos + 1, str->utf8_str(), -1, SQLITE_TRANSIENT);
	}

	void SQLite::Stmt::bind(Nat pos, Bool b) {
		reset();
		sqlite3_bind_int(stmt, pos + 1, b ? 1 : 0);
	}

	void SQLite::Stmt::bind(Nat pos, Int i) {
		reset();
		sqlite3_bind_int(stmt, pos + 1, i);
	}

	void SQLite::Stmt::bind(Nat pos, Long i) {
		reset();
		sqlite3_bind_int64(stmt, pos + 1, i);
	}

	void SQLite::Stmt::bind(Nat pos, Float f) {
		reset();
		sqlite3_bind_double(stmt, pos + 1, f);
	}

	void SQLite::Stmt::bind(Nat pos, Double d) {
		reset();
		sqlite3_bind_double(stmt, pos + 1, d);
	}

	void SQLite::Stmt::bindNull(Nat pos) {
		reset();
		sqlite3_bind_null(stmt, pos + 1);
	}

	Statement::Result SQLite::Stmt::execute() {
		reset();

		int r = sqlite3_step(stmt);
		isClean = false;

		if (r == SQLITE_DONE) {
			// No data. We are done!
			lastId = (Int)sqlite3_last_insert_rowid(owner->db);
			lastChanges = sqlite3_changes(owner->db);
			// Call reset here already, but act as if we need to call reset again later to make
			// iterators behave correctly.
			sqlite3_reset(stmt);
		} else if (r == SQLITE_ROW) {
			// We have data!
			hasRow = true;
			moreRows = true;
		} else {
			throw new (this) SQLError(new (this) Str((wchar *)sqlite3_errmsg16(owner->db)));
		}

		return Result(this);
	}

	void SQLite::Stmt::disposeResult() {
		reset();
	}

	Maybe<Row> SQLite::Stmt::nextRow() {
		// We are already done!
		if (!moreRows)
			return Maybe<Row>();

		if (hasRow) {
			// We already have a row (i.e. execute was just called). Simply mark it as consumed.
			hasRow = false;
		} else {
			// No row. Call step.
			int r = sqlite3_step(stmt);
			if (r == SQLITE_DONE) {
				// Release resources now, even though it is not strictly needed.
				sqlite3_reset(stmt);
				moreRows = false;
				// We don't set "isClean" to false to make iterators continue to work.
				return Maybe<Row>();
			} else if (r != SQLITE_ROW) {
				moreRows = false;
				throw new (this) SQLError(new (this) Str((wchar *)sqlite3_errmsg16(owner->db)));
			}
		}

		int cols = sqlite3_column_count(stmt);
		if (cols <= 0)
			cols = 0;

		Row::Builder builder = Row::builder(engine(), Nat(cols));
		for (int i = 0; i < cols; i++) {
			switch (sqlite3_column_type(stmt, i)) {
			case SQLITE3_TEXT:
				builder.push(new (this) Str((const wchar *)sqlite3_column_text16(stmt, i)));
				break;
			case SQLITE_INTEGER:
				builder.push(Long(sqlite3_column_int64(stmt, i)));
				break;
			case SQLITE_FLOAT:
				builder.push(Double(sqlite3_column_double(stmt, i)));
				break;
			case SQLITE_NULL:
				builder.pushNull();
				break;
			default:
			{
				StrBuf *msg = new (this) StrBuf();
				*msg << S("Unsupported column type from SQLite: ") << sqlite3_column_type(stmt, i);
				throw new (this) SQLError(msg->toS());
			}
			}
		}

		return Maybe<Row>(Row(builder));
	}

}
