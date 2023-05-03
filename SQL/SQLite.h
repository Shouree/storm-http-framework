#pragma once
#include "SQLite/sqlite3.h"
#include "Core/Io/Url.h"
#include "Core/Array.h"
#include "Core/Variant.h"
#include "SQL.h"

namespace sql {

	class SQLite;

	/**
	 * A class representing a SQLite statement.
	 */
	class SQLite_Statement : public Statement {
		STORM_CLASS;
	public:
		// Constructor of an SQLite_Statement. Takes an SQLite database and an Str.
		STORM_CTOR SQLite_Statement(const SQLite *database, Str *str);

		// Destructor of an SQLite_Statement. Calls SQLite_Statement:finalize().
		~SQLite_Statement();


		// Binds for all relevant data types in SQLite. If a statement includes question marks,
		// these are used to bind one string to a question mark.
		// pos specifies which question mark is to be replaced by matched data type.
		void STORM_FN bind(Nat pos, Str *str) override;
		void STORM_FN bind(Nat pos, Bool b) override;
		void STORM_FN bind(Nat pos, Int i) override;
		void STORM_FN bind(Nat pos, Long l) override;
		void STORM_FN bind(Nat pos, Double d) override;
		void STORM_FN bindNull(Nat pos) override;

		// Executes an SQLite statement, returns true if execute was successful.
		void STORM_FN execute() override;

		// Calls SQLite3_finalize on SQLite_Statement and cleans member variables.
		void STORM_FN finalize() override;

		// Fetches a new row and returns nullptr if result is false or SQLITE_DONE-flag is set.
		MAYBE(Row *) STORM_FN fetch() override;

		// Called when fetching is done, releases locks from the database.
		void STORM_FN done() override;

		// Get the last row id.
		Int STORM_FN lastRowId() const override;

		// Get number of changes.
		Nat STORM_FN changes() const override;

	private:
		// The prepared statement.
		UNKNOWN(PTR_NOGC) sqlite3_stmt *stmt;

		// Owner.
		const SQLite *db;

		// Any result to produce from the statement?
		// If 'true' means that we need to reset the statement before executing it again.
		Bool result;

		// Number of changes from last execute.
		Nat lastChanges;

		// Last row id.
		Int lastId;

		// Any error for the next call to "execute"?
		Str *error;

		// Reset if needed.
		void reset();
	};

	/**
	 * Database class specificly for SQLite.
	 */
	class SQLite : public DBConnection {
		STORM_CLASS;
	public:

		// Destructor of an SQLite database connection. Calls SQLite:close().
		virtual ~SQLite();

		// Constructors of an SQLite database connection. either connected through a file
		// on given URL or is created in memory.
		STORM_CTOR SQLite(Url * str);
		STORM_CTOR SQLite();

		// Returns an SQLite_Statement given an Str str.
		Statement * STORM_FN prepare(Str *str) override;

		// Calls sqlite3_close(db).
		void STORM_FN close() override;

		// Returns all names of tables in SQLite connection in an Array of Str.
		Array<Str*> *STORM_FN tables() override;

		// Getter for member variable db.
		sqlite3 * raw() const;

		// Returns a Schema for SQLite connection.
		MAYBE(Schema *) STORM_FN schema(Str *str);

	private:
		sqlite3 * db;
	};

}
