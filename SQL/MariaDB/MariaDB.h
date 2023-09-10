#pragma once
#include "Driver.h"
#include "Host.h"
#include "Core/Io/Url.h"
#include "Core/Net/Address.h"
#include "Core/Array.h"
#include "Value.h"
#include "SQL.h"

namespace sql {

	/**
	 * Base class for MySQL-derivatives.
	 */
	class MariaDBBase : public DBConnection {
		STORM_CLASS;
	public:
		// Destructor of an Mariadb database connection. Calls MariaDB:close().
		virtual ~MariaDBBase();

		// Prepares a statement for execution.
		Statement *STORM_FN prepare(Str *query) override;
		Statement *STORM_FN prepare(QueryStr *query) override;

		// Closes the connection.
		void STORM_FN close() override;

		// Returns all names of tables in MariaDB connection in an Array of Str.
		// NOT IMPLEMENTED.
		Array<Str*> *STORM_FN tables() override;

		// Returns a Schema for MariaDB connection.
		MAYBE(Schema *) STORM_FN schema(Str *table);

		// Getter for the contained MYSQL handle.
		MYSQL *raw() const;

		class Stmt;

	protected:
		// Create the connection.
		STORM_CTOR MariaDBBase(Host c, Str *user, MAYBE(Str *) password, Str *database);

		// Throw an error if one is present.
		void throwError();

		// Called by Stmt: Inform us that we a statement wishes to start fetching data.
		void startFetch(Stmt *stmt);

		// Called by Stmt: Inform us that a statement is no longer fetching data.
		void stopFetch(Stmt *stmt);

		// Called by Stmt: Ask any current fetching statement to stop fetching data.
		void clearFetch();

		// Find the last row id.
		Int queryLastRowId();

	private:
		// Handle to the database.
		UNKNOWN(PTR_NOGC) MYSQL *handle;

		// Pointer to the API block of the handle for easy access.
		UNKNOWN(PTR_NOGC) st_mariadb_api *api;

		// Current statement that has not yet stored its results. We use this to tell it to finish
		// result extraction when we need to execute another statement. Might be null.
		Stmt *currentFetching;

		// Cached statement for querying the last row id.
		Stmt *lastRowIdQuery;

	public:

		/**
		 * A class representing a prepared statement statement.
		 */
		class Stmt : public Statement {
			STORM_CLASS;
		public:
			// Constructor of an MariaDB_Statement. Takes an MariaDB database and an Str.
			STORM_CTOR Stmt(MariaDBBase *database, Str *query);

			// Destructor of an MariaDB_Statement. Calls MariaDB_Statement:finalize().
			virtual ~Stmt();

			// Bind parameters.
			void STORM_FN bind(Nat pos, Str *str) override;
			void STORM_FN bind(Nat pos, Bool b) override;
			void STORM_FN bind(Nat pos, Int i) override;
			void STORM_FN bind(Nat pos, Long l) override;
			void STORM_FN bind(Nat pos, Float f) override;
			void STORM_FN bind(Nat pos, Double d) override;
			void STORM_FN bindNull(Nat pos) override;

			// Executes an MariaDB statement.
			Statement::Result STORM_FN execute() override;

			// Finalize the statement.
			void STORM_FN finalize() override;

			// Called by MariaDBBase to store the remainder of the data set in the class, so that
			// other queries can be made through the established database connection.
			void STORM_FN fetchAll();

		protected:
			// Dispose of the results.
			void STORM_FN disposeResult() override;

			// Get the next row.
			Maybe<Row> STORM_FN nextRow() override;

			// Get the last row id.
			Int STORM_FN lastRowId() override;

			// Get the number of changed rows.
			Nat STORM_FN changes() override { return lastChanges; }

		private:
			// Owner.
			MariaDBBase *owner;

			// The prepared statement.
			UNKNOWN(PTR_NOGC) MYSQL_STMT *stmt;

			// Does the statement contain any results? I.e. do we need to reset it?
			Bool stmtHasData;

			// Number of changes from last execute.
			Nat lastChanges;

			// Last row id. Fetched if we detect that there are no "regular" results, which
			// indicates an insert statement.
			Int lastId;

			// Array of rows that we have buffered. In reverse order, so it is easy to pop them all. Might be null.
			Array<Row> *buffer;


			/**
			 * Bound buffers for the parameters:
			 *
			 * Arrays are allocated with malloc since the library has pointers into them (at least
			 * "values", but maybe also to "resultBind").
			 */

			Nat paramCount;
			UNKNOWN(PTR_NOGC) MYSQL_BIND *paramBinds;
			UNKNOWN(PTR_NOGC) Value *paramValues;


			/**
			 * Bound buffers for the result:
			 *
			 * Arrays are allocated with malloc since the library has pointers into them (at least
			 * "values", but maybe also to "resultBind").
			 */

			Nat resultCount;
			UNKNOWN(PTR_NOGC) MYSQL_BIND *resultBinds;
			UNKNOWN(PTR_NOGC) Value *resultValues;


			// Resets a prepared statement on client and server to state after prepare if needed.
			void reset();

			// Throw error.
			void throwError();
		};


		/**
		 * Visitor to transform query strings.
		 */
		class Visitor : public QueryStr::Visitor {
			STORM_CLASS;
		public:
			STORM_CTOR Visitor();
			void STORM_FN name(StrBuf *to, Str *name) override;
			void STORM_FN type(StrBuf *to, QueryType type) override;
			void STORM_FN autoincrement(StrBuf *to) override;
		};
	};


	/**
	 * Connection to MySQL databases.
	 *
	 * Note: The distinction between MySQL and MariaDB is mostly for future-proofing any differences
	 * that might arise eventually. They are currently compatible enough for this difference to be
	 * irrelevant.
	 */
	class MySQL : public MariaDBBase {
		STORM_CLASS;
	public:
		// Connect.
		STORM_CTOR MySQL(Host c, Str *user, MAYBE(Str *) password, Str *database);
	};


	/**
	 * Connection to MariaDB databases.
	 *
	 * Note: The distinction between MySQL and MariaDB is mostly for future-proofing any differences
	 * that might arise eventually. They are currently compatible enough for this difference to be
	 * irrelevant.
	 */
	class MariaDB : public MariaDBBase {
		STORM_CLASS;
	public:
		// Connect.
		STORM_CTOR MariaDB(Host c, Str *user, MAYBE(Str *) password, Str *database);
	};

}
