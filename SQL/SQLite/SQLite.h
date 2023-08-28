#pragma once
#include "SQLite/sqlite3.h"
#include "Core/Io/Url.h"
#include "Core/Array.h"
#include "SQL.h"

namespace sql {

	/**
	 * Implementation of a connection to SQLite databases.
	 */
	class SQLite : public DBConnection {
		STORM_CLASS;
	public:
		// Create a connection to an SQLite database in a file. The Url has to refer to a file that
		// resides in the filesystem.
		STORM_CTOR SQLite(Url *str);

		// Create a connection to a temporary, in-memory database.
		STORM_CTOR SQLite();

		// Destroy. Calls 'close'.
		virtual ~SQLite();

		// Prepare a statement.
		Statement *STORM_FN prepare(Str *query) override;
		Statement *STORM_FN prepare(QueryStr *query) override;

		// Close the connection.
		void STORM_FN close() override;

		// Returns all names of tables in SQLite connection in an Array of strings.
		Array<Str*> *STORM_FN tables() override;

		// Returns a Schema for SQLite connection.
		MAYBE(Schema *) STORM_FN schema(Str *table);

	private:
		// The SQLite3 object.
		UNKNOWN(PTR_NOGC) sqlite3 *db;

	public:

		/**
		 * Prepared statements in SQLite.
		 */
		class Stmt : public Statement {
			STORM_CLASS;
		public:
			// Create. Called by SQLite class.
			Stmt(SQLite *owner, Str *statement);

			// Destroy.
			virtual ~Stmt();

			// Bind parameters.
			virtual void STORM_FN bind(Nat pos, Str *str) override;
			virtual void STORM_FN bind(Nat pos, Bool b) override;
			virtual void STORM_FN bind(Nat pos, Int i) override;
			virtual void STORM_FN bind(Nat pos, Long i) override;
			virtual void STORM_FN bind(Nat pos, Float f) override;
			virtual void STORM_FN bind(Nat pos, Double d) override;
			virtual void STORM_FN bindNull(Nat pos) override;

			// Execute.
			Statement::Result STORM_FN execute() override;

			// Finalize the statement.
			void STORM_FN finalize() override;

		protected:
			// Dispose results, re-create the statement.
			void STORM_FN disposeResult() override;

			// Get the next row.
			Maybe<Row> STORM_FN nextRow() override;

			// Get the last row id.
			Int STORM_FN lastRowId() override { return lastId; }

			// Get the number of changed rows.
			Nat STORM_FN changes() override { return lastChanges; }

		private:
			// Owner.
			SQLite *owner;

			// Prepared statement.
			UNKNOWN(PTR_NOGC) sqlite3_stmt *stmt;

			// Last row id.
			Int lastId;

			// Last number of changed rows.
			Nat lastChanges;

			// Is the statement in a clean state?
			Bool isClean;

			// Do we have a row of results ready?
			Bool hasRow;

			// Do we have more results to read using step?
			Bool moreRows;

			// Ensure that the statement is ready for manipulation.
			void reset();
		};


		/**
		 * Visitor to transform query strings into proper queries.
		 */
		class Visitor : public QueryStr::Visitor {
			STORM_CLASS;
		public:
			STORM_CTOR Visitor();
			void STORM_FN type(StrBuf *to, QueryStr::Type type, Nat size) override;
		};
	};

}
