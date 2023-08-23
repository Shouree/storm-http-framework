#include "stdafx.h"
#include "MariaDB.h"
#include "Value.h"
#include "Exception.h"
#include "Core/Convert.h"

namespace sql {

	MySQL::MySQL(ConnectionType c, Str *user, MAYBE(Str *) password, Str *database)
		: MariaDBBase(c, user, password, database) {}

	MariaDB::MariaDB(ConnectionType c, Str *user, MAYBE(Str *) password, Str *database)
		: MariaDBBase(c, user, password, database) {}

	MariaDBBase::MariaDBBase(ConnectionType c, Str *user, MAYBE(Str *) password, Str *database) {
		handle = mysql_init(null);

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

			if (!mysql_real_connect(
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
			mysql_close(handle);
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

		const char *msg = mysql_error(handle);
		if (!msg)
			return;

		throw new (this) SQLError(new (this) Str(toWChar(engine(), msg)));
	}




	MariaDBBase::Stmt::Stmt(MariaDBBase *owner, Str *query) : owner(owner) {
		stmt = mysql_stmt_init(owner->handle);
		if (!stmt)
			owner->throwError();

		try {

			// TODO: Seems like MySQL/MariaDB does not like quoting table names with "

			if (mysql_stmt_prepare(stmt, query->utf8_str(), -1)) {
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

		const char *error = mysql_stmt_error(stmt);
		if (!error)
			return;

		throw new (this) SQLError(new (this) Str(toWChar(engine(), error)));
	}

	void MariaDBBase::Stmt::finalize() {
		if (stmt) {
			columns = null;
			colCount = 0;
			mysql_stmt_close(stmt);
			stmt = null;
		}
	}

	void MariaDBBase::Stmt::reset() {
		if (stmt) {
			columns = null;
			colCount = 0;
			mysql_stmt_free_result(stmt);
			mysql_stmt_reset(stmt);
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

		if (mysql_stmt_execute(stmt))
			throwError();

		lastChanges = mysql_stmt_affected_rows(stmt);

		MYSQL_RES *metadata = mysql_stmt_result_metadata(stmt);
		colCount = mysql_num_fields(metadata);
		// columns = mysql_fetch_fields(metadata);
		TODO(L"free_result here will likely invalidate columns, so we need to re-write ValueSet!");
		mysql_free_result(metadata);

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
		if (!columns)
			return Maybe<Row>();

		TODO(L"We don't want to re-allocate the Value_Set and re-bind it all the time!");
		TODO(L"If there are multiple concurrent fetches, we need to do something intelligent.");

		ValueSet output(colCount);

		// Set type of values in 'output'.
		for (Nat i = 0; i < colCount; i++) {
			if (IS_NUM(columns[i].type)) {
				// Treat all integers as Long.
				if (columns[i].flags & UNSIGNED_FLAG)
					output[i].set_uint(0);
				else
					output[i].set_int(0);
			} else if (columns[i].type == MYSQL_TYPE_STRING
					|| columns[i].type == MYSQL_TYPE_VAR_STRING
					|| columns[i].type == MYSQL_TYPE_BLOB) {
				// Set length to 0 to ask for the real size.
				output[i].set_string(0);
			} else {
				StrBuf *msg = new (this) StrBuf();
				*msg << S("Unknown column type: ") << columns[i].type << S("!");
				throw new (this) SQLError(msg->toS());
			}
		}

		// Bind the result. TODO: We can probably get away with doing this only once.
		if (mysql_stmt_bind_result(stmt, output.data()))
			throwError();

		// Fetch the next row.
		int result = mysql_stmt_fetch(stmt);
		if (result == 1) {
			throwError();
		} else if (result == MYSQL_NO_DATA) {
			// End of the query!
			reset();
			return Maybe<Row>();
		}
		// TODO: if status is MYSQL_DATA_TRUNCATED, we need to check lengths. We do this always in this impl.

		// Extract the results.
		Row::Builder builder = Row::builder(engine(), colCount);

		for (Nat i = 0; i < colCount; i++) {
			if (IS_NUM(columns[i].type)) {
				if (columns[i].flags & UNSIGNED_FLAG)
					builder.push(Long(output[i].get_uint())); // TODO: Maybe support unsigned also?
				else
					builder.push(Long(output[i].get_int()));
			} else if (columns[i].type == MYSQL_TYPE_STRING
					|| columns[i].type == MYSQL_TYPE_VAR_STRING
					|| columns[i].type == MYSQL_TYPE_BLOB) {
				Value &val = output[i];
				size_t sz = val.is_truncated();
				if (sz) {
					// Re-fetch if it was truncated.
					val.set_string(sz);
					mysql_stmt_fetch_column(stmt, output.data() + i, i, 0);
				}

				builder.push(new (this) Str(toWChar(engine(), val.get_string().c_str())));
			} else {
				StrBuf *msg = new (this) StrBuf();
				*msg << S("Unknown column type: ") << columns[i].type << S("!");
				throw new (this) SQLError(msg->toS());
			}
		}

		return Maybe<Row>(Row(builder));
	}

}
