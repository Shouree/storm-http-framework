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

	Statement *MariaDBBase::prepare(QueryString *query) {
		return prepare(query->generate(new (this) Visitor()));
	}

	MariaDBBase::Visitor::Visitor() {}

	void MariaDBBase::Visitor::name(StrBuf *to, Str *name) {
		*to << S("`") << name << S("`");
	}

	void MariaDBBase::Visitor::type(StrBuf *to, QueryString::Type type, Nat size) {
		if (size != Nat(-1))
			TODO(L"Support explicit sizes!");

		switch (type) {
		case QueryString::text:
			*to << S("TEXT");
			break;
		case QueryString::integer:
			*to << S("INTEGER");
			break;
		case QueryString::real:
			*to << S("FLOAT");
			break;
		default:
			assert(false, L"Unknown DB type!");
			break;
		}
	}

	Array<Str *> *MariaDBBase::tables() {
		TODO(L"Implement me!");
		return new (this) Array<Str *>();
	}

	MAYBE(Schema *) MariaDBBase::schema(Str *table) {
		TODO(L"Implement me!");
		return null;
	}

	void MariaDBBase::throwError() {
		if (!handle)
			return;

		const char *msg = (*api->mysql_error)(handle);
		if (!msg)
			return;

		throw new (this) SQLError(new (this) Str(toWChar(engine(), msg)));
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
		: owner(owner), lastId(-1),
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

		const char *error = (*owner->api->mysql_stmt_error)(stmt);
		if (!error)
			return;

		throw new (this) SQLError(new (this) Str(toWChar(engine(), error)));
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

		if (stmt) {
			(*owner->api->mysql_stmt_free_result)(stmt);
			(*owner->api->mysql_stmt_reset)(stmt);
		}

		owner->stopFetch(this);
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
