#include "stdafx.h"
#include "MariaDB.h"
#include "Value.h"
#include "Exception.h"
#include "Core/Convert.h"

namespace sql {

	// Number of bytes to allocate for strings as an initial guess. If most strings fit here, we
	// avoid back and forth to the library. We don't want to waste too much memory, however.
	static const size_t DEFAULT_STRING_SIZE = 32;

	MySQL::MySQL(Host c, Str *user, MAYBE(Str *) password, Str *database)
		: MariaDBBase(c, user, password, database) {}

	MariaDB::MariaDB(Host c, Str *user, MAYBE(Str *) password, Str *database)
		: MariaDBBase(c, user, password, database) {}

	MariaDBBase::MariaDBBase(Host c, Str *user, MAYBE(Str *) password, Str *database) {
		currentFetching = null;
		handle = createDriver(engine());
		api = handle->methods->api;

		// Set charset. utf8 is the default for MariaDB, but if the default ever changes it is better to be explicit.
		(*api->mysql_options)(handle, MYSQL_SET_CHARSET_NAME, "utf8mb4");

		try {
			const char *host = null;
			const char *pipe = null;
			unsigned int port = 0;

			if (Address *addr = c.isSocket()) {
				host = addr->withPort(0)->toS()->utf8_str();
				port = addr->port();
			} else if (Str *local = c.isLocal()) {
				host = null;
				pipe = local->utf8_str();
			} else {
				host = null;
				pipe = null;
			}

			if (!(*api->mysql_real_connect)(
					handle,
					host,
					user->utf8_str(),
					password ? password->utf8_str() : null,
					database->utf8_str(),
					port,
					pipe,
					0)) {
				throwError();
			}
		} catch (...) {
			close();
			throw;
		}
	}

	MariaDBBase::~MariaDBBase() {
		close();
	}

	void MariaDBBase::close() {
		if (handle) {
			destroyDriver(handle);
			handle = null;
		}
	}

	Statement *MariaDBBase::prepare(Str *query) {
		return new (this) Stmt(this, query);
	}

	Statement *MariaDBBase::prepare(QueryStr *query) {
		return prepare(query->generate(new (this) Visitor()));
	}

	MariaDBBase::Visitor::Visitor() {}

	void MariaDBBase::Visitor::name(StrBuf *to, Str *name) {
		*to << S("`") << name << S("`");
	}

	void MariaDBBase::Visitor::type(StrBuf *to, QueryStr::Type type, Nat size) {
		if (size != Nat(-1))
			TODO(L"Support explicit sizes!");

		switch (type) {
		case QueryStr::text:
			*to << S("TEXT");
			break;
		case QueryStr::integer:
			*to << S("INTEGER");
			break;
		case QueryStr::real:
			*to << S("FLOAT");
			break;
		default:
			assert(false, L"Unknown DB type!");
			break;
		}
	}

	void MariaDBBase::Visitor::autoincrement(StrBuf *to) {
		*to << S("AUTO_INCREMENT");
	}

	Array<Str *> *MariaDBBase::tables() {
		Statement *query = prepare(new (this) Str(S("SHOW TABLES;")));
		Array<Str *> *result = new (this) Array<Str *>();

		Statement::Result queryResult = query->execute();
		for (Maybe<Row> row = queryResult.next(); row.any(); row = queryResult.next()) {
			result->push(row.value().getStr(0));
		}

		query->finalize();

		return result;
	}

	static void addWord(StrBuf *to, Bool &first, const wchar *word) {
		if (!first)
			*to << S(" ");
		first = false;
		*to << word;
	}

	MAYBE(Schema *) MariaDBBase::schema(Str *table) {
		Statement *columnQuery;
		Statement *indexQuery;
		{
			QueryStrBuilder *colBuilder = new (this) QueryStrBuilder();
			colBuilder->put(S("SHOW COLUMNS FROM "));
			colBuilder->name(table);
			colBuilder->put(S(";"));
			columnQuery = prepare(colBuilder->build());

			QueryStrBuilder *indexBuilder = new (this) QueryStrBuilder();
			indexBuilder->put(S("SHOW INDEX FROM "));
			indexBuilder->name(table);
			indexBuilder->put(S(";"));
			indexQuery = prepare(indexBuilder->build());
		}

		try {
			Array<Schema::Column *> *columns = new (this) Array<Schema::Column *>();
			Array<Str *> *pk = new (this) Array<Str *>();

			Statement::Result queryResult = columnQuery->execute();
			// Columns are: field, type, null, key, default, extra
			for (Maybe<Row> row = queryResult.next(); row.any(); row = queryResult.next()) {
				Row &v = row.value();

				Str *colName = v.getStr(0);

				StrBuf *attrs = new (this) StrBuf();
				Bool first = true;

				if (*v.getStr(2) == S("YES"))
					addWord(attrs, first, S("NOT NULL"));

				if (*v.getStr(3) == S("PRI"))
					pk->push(v.getStr(0));

				if (!v.isNull(4)) {
					addWord(attrs, first, S("DEFAULT "));
					Variant def = at(engine(), v, 4);
					if (def.has(StormInfo<Str *>::type(engine()))) {
						*attrs << S("'");
						Str *s = (Str *)def.getObject();
						for (Str::Iter i = s->begin(); i != s->end(); ++i) {
							if (i.v() == Char('\''))
								*attrs << S("''");
							else
								*attrs << i.v();
						}
						*attrs << S("'");
					} else {
						*attrs << def;
					}
				}

				if (v.getStr(5)->any()) {
					if (!first)
						*attrs << S(" ");
					*attrs << v.getStr(5);
				}

				columns->push(new (this) Schema::Column(colName, v.getStr(1), attrs->toS()));
			}

			Array<Schema::Index *> *indices = new (this) Array<Schema::Index *>();
			Map<Str *, Nat> *nameMap = new (this) Map<Str *, Nat>();

			queryResult = indexQuery->execute();
			// Columns are: table, non_unique, key_name, seq_in_index, column_name, collation, cardinality, ...
			for (Maybe<Row> row = queryResult.next(); row.any(); row = queryResult.next()) {
				Row &v = row.value();

				Str *name = v.getStr(2);
				// Don't add the index for the primary key.
				if (*name == S("PRIMARY"))
					continue;

				Nat id = nameMap->get(name, indices->count());
				if (id >= indices->count()) {
					indices->push(new (this) Schema::Index(name, new (this) Array<Str *>()));
					nameMap->put(name, id);
				}

				indices->at(id)->columns->push(v.getStr(4));
			}

			return new (this) Schema(table, columns, pk, indices);
		} catch (SQLError *e) {
			// Check if the error was that the table does not exist.
			if (e->code.any() && e->code.value() == ER_NO_SUCH_TABLE)
				return null;

			// Otherwise, re-throw the exception.
			throw e;
		}
	}

	void MariaDBBase::throwError() {
		if (!handle)
			return;

		unsigned int code = (*api->mysql_errno)(handle);
		if (code == 0)
			return;

		const char *msg = (*api->mysql_error)(handle);
		throw new (this) SQLError(new (this) Str(toWChar(engine(), msg)), Maybe<Nat>(code));
	}

	void MariaDBBase::startFetch(Stmt *stmt) {
		clearFetch();
		currentFetching = stmt;
	}

	void MariaDBBase::stopFetch(Stmt *stmt) {
		if (currentFetching == stmt)
			currentFetching = null;
	}

	void MariaDBBase::clearFetch() {
		if (currentFetching) {
			currentFetching->fetchAll();
			// Should be cleared by 'fetchAll'.
			assert(currentFetching == null);
			currentFetching = null;
		}
	}

	Int MariaDBBase::queryLastRowId() {
		if (!lastRowIdQuery) {
			lastRowIdQuery = new (this) Stmt(this, new (this) Str(S("SELECT LAST_INSERT_ID();")));
		}

		Statement::Result result = lastRowIdQuery->execute();
		Int id = result.next().value().getInt(0);
		result.finalize();
		return id;
	}


	/**
	 * The statement.
	 */

	static void freeBinds(Nat &count, MYSQL_BIND *&binds, Value *&values) {
		if (values) {
			for (Nat i = 0; i < count; i++)
				values[i].~Value();

			free(values);
			values = null;
		}

		if (binds) {
			free(binds);
			binds = null;
		}

		count = 0;
	}

	static void allocBinds(Nat count, MYSQL_BIND *&binds, Value *&values) {
		binds = (MYSQL_BIND *)calloc(count, sizeof(MYSQL_BIND));
		values = (Value *)calloc(count, sizeof(Value));

		for (Nat i = 0; i < count; i++)
			new (values + i) Value(binds + i);
	}

	MariaDBBase::Stmt::Stmt(MariaDBBase *owner, Str *query)
		: owner(owner), stmtHasData(false), lastId(-1),
		  paramCount(0), paramBinds(null), paramValues(0),
		  resultCount(0), resultBinds(null), resultValues(0) {

		// We need access to the data stream now!
		owner->clearFetch();

		stmt = (*owner->api->mysql_stmt_init)(owner->handle);
		if (!stmt)
			owner->throwError();

		try {
			if ((*owner->api->mysql_stmt_prepare)(stmt, query->utf8_str(), -1)) {
				throwError();
			}

			paramCount = (*owner->api->mysql_stmt_param_count)(stmt);
			if (paramCount > 0)
				allocBinds(paramCount, paramBinds, paramValues);

		} catch (...) {
			finalize();
			throw;
		}
	}

	MariaDBBase::Stmt::~Stmt() {
		finalize();
	}

	void MariaDBBase::Stmt::throwError() {
		if (!stmt)
			return;

		unsigned int code = (*owner->api->mysql_stmt_errno)(stmt);
		if (code == 0)
			return;

		const char *error = (*owner->api->mysql_stmt_error)(stmt);
		throw new (this) SQLError(new (this) Str(toWChar(engine(), error)), Maybe<Nat>(code));
	}

	void MariaDBBase::Stmt::finalize() {
		freeBinds(paramCount, paramBinds, paramValues);
		freeBinds(resultCount, resultBinds, resultValues);
		buffer = null;

		if (stmt) {
			(*owner->api->mysql_stmt_close)(stmt);
			stmt = null;
		}

		owner->stopFetch(this);
	}

	void MariaDBBase::Stmt::reset() {
		freeBinds(resultCount, resultBinds, resultValues);
		buffer = null;

		owner->stopFetch(this);

		if (stmt && stmtHasData) {
			// Note: If any other query is currently fetching data, the driver will crash if we call
			// reset in the middle of the fetch, even for an unrelated query...
			owner->clearFetch();

			(*owner->api->mysql_stmt_free_result)(stmt);
			(*owner->api->mysql_stmt_reset)(stmt);
			stmtHasData = false;
		}
	}

	void MariaDBBase::Stmt::bind(Nat pos, Str *str) {
		if (pos < paramCount)
			paramValues[pos].setString(str);
	}

	void MariaDBBase::Stmt::bind(Nat pos, Bool b) {
		if (pos < paramCount)
			paramValues[pos].setInt(b ? 1 : 0);
	}

	void MariaDBBase::Stmt::bind(Nat pos, Int i) {
		if (pos < paramCount)
			paramValues[pos].setInt(i);
	}

	void MariaDBBase::Stmt::bind(Nat pos, Long l) {
		if (pos < paramCount)
			paramValues[pos].setInt(l);
	}

	void MariaDBBase::Stmt::bind(Nat pos, Float f) {
		if (pos < paramCount)
			paramValues[pos].setFloat(f);
	}

	void MariaDBBase::Stmt::bind(Nat pos, Double d) {
		if (pos < paramCount)
			paramValues[pos].setFloat(d);
	}

	void MariaDBBase::Stmt::bindNull(Nat pos) {
		if (pos < paramCount)
			paramValues[pos].setNull();
	}

	Statement::Result MariaDBBase::Stmt::execute() {
		reset();

		// Make sure no other statement is fetching results now.
		owner->clearFetch();

		if (paramBinds) {
			(*owner->api->mysql_stmt_bind_param)(stmt, paramBinds);
		}

		stmtHasData = true;
		if ((*owner->api->mysql_stmt_execute)(stmt))
			throwError();

		lastChanges = Nat((*owner->api->mysql_stmt_affected_rows)(stmt));

		MYSQL_RES *metadata = (*owner->api->mysql_stmt_result_metadata)(stmt);

		if (metadata) {
			resultCount = (*owner->api->mysql_num_fields)(metadata);

			MYSQL_FIELD *columns = (*owner->api->mysql_fetch_fields)(metadata);
			allocBinds(resultCount, resultBinds, resultValues);

			for (Nat i = 0; i < resultCount; i++) {
				switch (columns[i].type) {
				case MYSQL_TYPE_STRING:
				case MYSQL_TYPE_VAR_STRING:
				case MYSQL_TYPE_BLOB:
					// Allocate some size. We ask for the real size later on.
					resultValues[i].setString(DEFAULT_STRING_SIZE);
					break;

				case MYSQL_TYPE_FLOAT:
				case MYSQL_TYPE_DOUBLE:
					resultValues[i].setFloat(0);
					break;

				default:
					if (IS_NUM(columns[i].type)) {
						// Treat all integers as 'Long'.
						if (columns[i].flags & UNSIGNED_FLAG)
							resultValues[i].setUInt(0);
						else
							resultValues[i].setInt(0);
					}
					break;
				}
			}

			(*owner->api->mysql_free_result)(metadata);

			// Bind the result now.
			if ((*owner->api->mysql_stmt_bind_result)(stmt, resultBinds))
				throwError();

			owner->startFetch(this);

		} else {
			// No result, the metadata returned null.
			reset();

			// This means it is interesting to query for a row id!
			lastId = owner->queryLastRowId();
		}

		return Result(this);
	}

	Int MariaDBBase::Stmt::lastRowId() {
		return lastId;
	}

	void MariaDBBase::Stmt::disposeResult() {
		reset();
	}

	Maybe<Row> MariaDBBase::Stmt::nextRow() {
		if (buffer) {
			if (buffer->empty()) {
				reset();
				return Maybe<Row>();
			}

			Row r = buffer->last();
			buffer->pop();
			return Maybe<Row>(r);
		}

		if (!resultValues)
			return Maybe<Row>();

		// Fetch the next row.
		int result = (*owner->api->mysql_stmt_fetch)(stmt);
		if (result == 1) {
			throwError();
		} else if (result == MYSQL_NO_DATA) {
			// End of the query!
			reset();
			return Maybe<Row>();
		}

		// Note: If we get MYSQL_DATA_TRUNCATED, we know that something was truncated. We don't need
		// to check specifically for this here, since we always check string lengths.

		// Extract the results.
		Row::Builder builder = Row::builder(engine(), resultCount);

		for (Nat i = 0; i < resultCount; i++) {
			Value &v = resultValues[i];

			if (v.isNull()) {
				builder.pushNull();
			} else if (v.isInt()) {
				builder.push(Long(v.getInt()));
			} else if (v.isUInt()) {
				builder.push(Long(v.getUInt())); // TODO: Maybe support unsigned values also?
			} else if (v.isFloat()) {
				builder.push(v.getFloat());
			} else if (v.isString()) {
				size_t sz = v.isTruncated();
				if (sz) {
					// Re-fetch to see if it was truncated.
					v.setString(sz);
					(*owner->api->mysql_stmt_fetch_column)(stmt, resultBinds + i, i, 0);
				}

				builder.push(v.getString(engine()));
			} else {
				StrBuf *msg = new (this) StrBuf();
				*msg << S("Unknown column type for column ") << i << S("!");
				throw new (this) SQLError(msg->toS());
			}
		}

		return Maybe<Row>(Row(builder));
	}

	void MariaDBBase::Stmt::fetchAll() {
		Array<Row> *b = new (this) Array<Row>();
		while (true) {
			Maybe<Row> r = nextRow();
			if (r.any())
				b->push(r.value());
			else
				break;
		}
		b->reverse();

		// Not really necessary, since 'nextRow' calls reset() when we reach the end. But here for
		// clarity as it does not hurt.
		reset();

		buffer = b;
	}

}
