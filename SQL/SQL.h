#pragma once
#include "Core/Maybe.h"
#include "Core/Array.h"
#include "Core/Variant.h"
#include "Row.h"
#include "Schema.h"
#include "QueryStr.h"

namespace sql {

	/**
	 * A prepared statement associated with a database connection.
	 *
	 * Abstract base class that is overridden by different database implementations.
	 */
	class Statement : public Object {
		STORM_ABSTRACT_CLASS;
	public:
		STORM_CTOR Statement();

		// Bind parameters.
		virtual void STORM_FN bind(Nat pos, Str *str) ABSTRACT;
		virtual void STORM_FN bind(Nat pos, Bool b) ABSTRACT;
		virtual void STORM_FN bind(Nat pos, Int i) ABSTRACT;
		virtual void STORM_FN bind(Nat pos, Long i) ABSTRACT;
		virtual void STORM_FN bind(Nat pos, Float f) ABSTRACT;
		virtual void STORM_FN bind(Nat pos, Double d) ABSTRACT;

		// Bind a null value.
		virtual void STORM_FN bindNull(Nat pos) ABSTRACT;

		/**
		 * Type describing the result of executing a statement. Acts like an iterator.
		 *
		 * The iterator is invalidated as soon as the parent statement object is modified in any
		 * way. At that time, the iterator acts as if the end of the iteration was reached.
		 *
		 * The database implementation may opt to stream data from the database until the iterator
		 * is destroyed. This implies that the database might be locked until this happens. It is
		 * possible to manually dispose of any resources in the database by calling 'finalize'.
		 */
		class Result {
			STORM_VALUE;
		public:
			Result(Statement *owner);
			Result(const Result &other);
			Result &operator =(const Result &other);
			~Result();

			// Get the next row.
			Maybe<Row> STORM_FN next();

			// Get the last inserted row ID.
			Int STORM_FN lastRowId() const;

			// Get the number of changes made when INSERT, UPDATE or DELETE was executed.
			Nat STORM_FN changes() const;

			// Finalize the result prematurely.
			void STORM_FN finalize();

			// To allow use in for-each loops:
			Result STORM_FN iter() { return *this; }

			// Deep copy: to issue an error.
			void STORM_FN deepCopy(CloneEnv *env);

		private:
			// Owning statement.
			Statement *owner;

			// Query sequence number.
			Nat sequence;
		};

		// Execute the prepared statement with the currently bound parameters. Returns an iterator
		// that can be used to retrieve the result. Any subsequent modification of the statement
		// (e.g. modifying bound parameters or calling 'execute' again) invalidates the returned
		// iterator.
		virtual Result STORM_FN execute() ABSTRACT;

		// Finalize (dispose of) the statement. It will not be usable again.
		// Finalize is called automatically by the destructor of derived classes,
		// but since finalization by the GC may not happen instantly, calling
		// finalize manually can make resource reclamation quicker.
		virtual void STORM_FN finalize() ABSTRACT;

	protected:
		// Called by derived classes to invalidate existing Result instances.
		void STORM_FN invalidateResult();

		// Called by the implementation whenever the current result can be disposed.
		virtual void STORM_FN disposeResult() ABSTRACT;

		// Called by iterators to get the next row.
		virtual Maybe<Row> STORM_FN nextRow() ABSTRACT;

		// Called by iterators to get the last row id.
		virtual Int STORM_FN lastRowId() ABSTRACT;

		// Called by iterators to get the number of changes.
		virtual Nat STORM_FN changes() ABSTRACT;

	private:
		// Number of Result objects that are currently alive. When it reaches zero, we can end the
		// current query even if not all rows were extracted.
		Nat resultAliveCount;

		// Query sequence number. Used to invalidate iterators for previous executions of the
		// statement.
		Nat resultSequence;
	};

	// Wrappers that allow binding maybe types conveniently.
	// Note: These can not be member functions since the string overload
	// would clash in C++.
	void STORM_FN bind(Statement *to, Nat pos, MAYBE(Str *) str);
	void STORM_FN bind(Statement *to, Nat pos, Maybe<Bool> b);
	void STORM_FN bind(Statement *to, Nat pos, Maybe<Int> i);
	void STORM_FN bind(Statement *to, Nat pos, Maybe<Long> l);
	void STORM_FN bind(Statement *to, Nat pos, Maybe<Float> f);
	void STORM_FN bind(Statement *to, Nat pos, Maybe<Double> d);


	/**
	 * A database connection.
	 *
	 * A connection to some database. This is the generic interface that the rest of the database
	 * system utilizes.
	 */
	class DBConnection : public Object {
		STORM_ABSTRACT_CLASS;
	public:
		// Create a prepared statement. Throws an error if the statement is invalid.
		virtual Statement *STORM_FN prepare(Str *query) ABSTRACT;
		virtual Statement *STORM_FN prepare(QueryStr *query) ABSTRACT;

		// Closes the connection to the database.
		virtual void STORM_FN close() ABSTRACT;

		// Returns all names of tables in SQLite connection in an Array of Str.
		virtual Array<Str*> *STORM_FN tables() ABSTRACT;

		// Returns a Schema for a particular table.
		virtual MAYBE(Schema *) STORM_FN schema(Str *table) ABSTRACT;


		/**
		 * Peculiarities of the database, that the database library might need to consider:
		 */

		// Does this database implicitly assign IDs to the first primary key? I.e., can we omit
		// autoincrement in this case?
		virtual Bool STORM_FN implicitAutoIncrement() const;

		// Does this database support full ALTER TABLE syntax?
		virtual Bool STORM_FN fullAlterTable() const;
	};

}
