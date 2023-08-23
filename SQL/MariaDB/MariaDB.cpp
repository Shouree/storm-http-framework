#include "stdafx.h"
#include "MariaDB.h"
#include "Value.h"
#include "Exception.h"
#include "Core/Convert.h"

namespace sql {

	MySQL::MySQL(Host c, Str *user, MAYBE(Str *) password, Str *database)
		: MariaDBBase(c, user, password, database) {}

	MariaDB::MariaDB(Host c, Str *user, MAYBE(Str *) password, Str *database)
		: MariaDBBase(c, user, password, database) {}

	MariaDBBase::MariaDBBase(Host c, Str *user, MAYBE(Str *) password, Str *database) {
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




	MariaDBBase::Stmt::Stmt(MariaDBBase *owner, Str *query) : owner(owner) {
		stmt = (*owner->api->mysql_stmt_init)(owner->handle);
		if (!stmt)
			owner->throwError();

		try {
			if ((*owner->api->mysql_stmt_prepare)(stmt, query->utf8_str(), -1)) {
				throwError();
			}

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
		for (Nat i = 0; i < colCount; i++)
			colValues[i].~Value();

		free(colValues);
		colValues = null;

		free(colBind);
		colBind = null;

		colCount = 0;

		if (stmt) {
			(*owner->api->mysql_stmt_close)(stmt);
			stmt = null;
		}
	}

	void MariaDBBase::Stmt::reset() {
		for (Nat i = 0; i < colCount; i++)
			colValues[i].~Value();

		free(colValues);
		colValues = null;

		free(colBind);
		colBind = null;

		colCount = 0;

		if (stmt) {
			(*owner->api->mysql_stmt_free_result)(stmt);
			(*owner->api->mysql_stmt_reset)(stmt);
		}
	}

	void MariaDBBase::Stmt::bind(Nat pos, Str *str) {}
	void MariaDBBase::Stmt::bind(Nat pos, Bool b) {}
	void MariaDBBase::Stmt::bind(Nat pos, Int i) {}
	void MariaDBBase::Stmt::bind(Nat pos, Long l) {}
	void MariaDBBase::Stmt::bind(Nat pos, Float f) {}
	void MariaDBBase::Stmt::bind(Nat pos, Double d) {}
	void MariaDBBase::Stmt::bindNull(Nat pos) {}

	Statement::Result MariaDBBase::Stmt::execute() {
		reset();

		if ((*owner->api->mysql_stmt_execute)(stmt))
			throwError();

		lastChanges = (*owner->api->mysql_stmt_affected_rows)(stmt);

		MYSQL_RES *metadata = (*owner->api->mysql_stmt_result_metadata)(stmt);
		colCount = (*owner->api->mysql_num_fields)(metadata);

		MYSQL_FIELD *columns = (*owner->api->mysql_fetch_fields)(metadata);
		colBind = (MYSQL_BIND *)calloc(colCount, sizeof(MYSQL_BIND));
		colValues = (Value *)calloc(colCount, sizeof(Value));

		for (Nat i = 0; i < colCount; i++) {
			new (colValues + i) Value(colBind + i);

			if (IS_NUM(columns[i].type)) {
				// Treat all integers as 'Long'.
				if (columns[i].flags & UNSIGNED_FLAG)
					colValues[i].setUInt(0);
				else
					colValues[i].setInt(0);
			} else if (columns[i].type == MYSQL_TYPE_STRING
					|| columns[i].type == MYSQL_TYPE_VAR_STRING
					|| columns[i].type == MYSQL_TYPE_BLOB) {
				// Set length to 0 to ask for the real size.
				colValues[i].setString(0);
			}
		}

		(*owner->api->mysql_free_result)(metadata);

		// Bind the result now.
		if ((*owner->api->mysql_stmt_bind_result)(stmt, colBind))
			throwError();

		return Result(this);
	}

	Int MariaDBBase::Stmt::lastRowId() {
		// Need to execute query: SELECT LAST_INSERT_ID() to the DB.
		assert(false, L"Not implemented yet!");
		return -1;
	}

	void MariaDBBase::Stmt::disposeResult() {
		reset();
	}

	Maybe<Row> MariaDBBase::Stmt::nextRow() {
		if (!colValues)
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
		Row::Builder builder = Row::builder(engine(), colCount);

		for (Nat i = 0; i < colCount; i++) {
			Value &v = colValues[i];

			if (v.isNull()) {
				builder.pushNull();
			} else if (v.isInt()) {
				builder.push(Long(v.getInt()));
			} else if (v.isUInt()) {
				builder.push(Long(v.getUInt())); // TODO: Maybe support unsigned values also?
			} else if (v.isString()) {
				size_t sz = v.isTruncated();
				if (sz) {
					// Re-fetch to see if it was truncated.
					v.setString(sz);
					(*owner->api->mysql_stmt_fetch_column)(stmt, colBind + i, i, 0);
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

}
