#pragma once
#include "Core/Array.h"
#include "Core/Variant.h"
#include "Schema.h"

namespace sql {

    /**
     * This is representing the result of a Row from a SQL SELECT statement query.
	 *
	 * TODO: What about null values?
     */
    class Row : public Object {
        STORM_CLASS;
    public:
        STORM_CTOR Row();
        STORM_CTOR Row(Array<Variant> * v);

        //If column is of type Str, use this to get the result. (TEXT or VARCHAR in sqlite3)
        Str* STORM_FN getStr(Nat idx);

        //If column is of type Int, use this to get the result. (INTEGER in sqlite3)
        Int STORM_FN getInt(Nat idx);

		// Get a 64-bit integer.
		Long STORM_FN getLong(Nat idx);

        //If column is of type Double, use this to get the result. (REAL in sqlite3)
        Double STORM_FN getDouble(Nat idx);

		// Is a particular column null?
		Bool STORM_FN isNull(Nat idx);

    private:
        Array<Variant> * v;
    };

    /**
     * A pure virtual Statement class.
	 *
     * This class specifies which functions have to be made if you're creating your own SQL language in Storm.
     */
    class Statement : public Object {
        STORM_ABSTRACT_CLASS;
    public:
        Statement();

		// Execute this prepared statement. Throws on error.
        virtual void STORM_FN execute() ABSTRACT;

		// Bind parameters.
        virtual void STORM_FN bind(Nat pos, Str *str) ABSTRACT;
		virtual void STORM_FN bind(Nat pos, Bool b) ABSTRACT;
        virtual void STORM_FN bind(Nat pos, Int i) ABSTRACT;
		virtual void STORM_FN bind(Nat pos, Long i) ABSTRACT;
        virtual void STORM_FN bind(Nat pos, Double d) ABSTRACT;

		// Bind a null value.
		virtual void STORM_FN bindNull(Nat pos) ABSTRACT;

		// Finalize (dispose of) the statement. It will not be usable again.
        virtual void STORM_FN finalize() ABSTRACT;

		// Fetch a row.
        virtual MAYBE(Row *) STORM_FN fetch() ABSTRACT;

		// Fetch a single row. Equivalent to calling fetch once, and then calling done.
		MAYBE(Row *) STORM_FN fetchOne();

		// Indicate that we are not interested in additional results. This allows
		// the underlying implementation to release resources required to fetch
		// any remaining rows (e.g. releasing memory, releasing locks, etc.)
		virtual void STORM_FN done() ABSTRACT;

		// Get the last row id. TODO: Make it Long.
        virtual Int STORM_FN lastRowId() const ABSTRACT;

		// Get the number of changes made when the statement was executed. Assuming the statement
		// was one of INSERT, UPDATE or DELETE.
		virtual Nat STORM_FN changes() const ABSTRACT;

        class Iter {
            STORM_VALUE;
        public:
            Iter(Statement *stmt);
			Iter(const Iter &other);
			Iter &operator =(const Iter &other);
			~Iter();

			// Get the next element.
            MAYBE(Row *) STORM_FN next();

			// To allow use in foreach-loops.
			Iter STORM_FN iter() { return *this; }

        private:
			// Owning statement.
            Statement *owner;

			// Generation for the iterator.
			Nat gen;
        };

		// Iterator interface. Mixing usage of the iterator interface and the 'fetch' interface
		// might not work well, since the iterators make sure to execute 'done' whenever the last
		// iterator is destroyed.
        Iter STORM_FN iter();

	protected:
		// Called by derived classes to inform that a new query has been executed, and existing
		// iterators should be invalidated.
		void STORM_FN invalidateIterators();

	private:
		// Number of iterators that are currently alive. This allows calling 'finish' when all
		// iterators have been destroyed.
		Nat iteratorLive;

		// Iterator generation. Set to an integer whenever a new query is executed. This allows
		// iterators to detect when they have been kept alive for too long, so that such mistakes
		// can be detected.
		Nat iteratorGen;
    };


    /**
	 * A database connection.
	 *
	 * A connection to some database. This is the generic interface that the rest of the database
	 * system utilizes.
	 */
	class DBConnection : public Object {
        STORM_ABSTRACT_CLASS;
    public:
		// Returns an SQLite_Statement given an Str str.
        virtual Statement * STORM_FN prepare(Str * str) ABSTRACT;

		// Calls sqlite3_close(db).
        virtual void STORM_FN close() ABSTRACT;

		// Returns all names of tables in SQLite connection in an Array of Str.
        virtual Array<Str*>* STORM_FN tables() ABSTRACT;

		// Returns a Schema for the connection.
        virtual MAYBE(Schema *) STORM_FN schema(Str * str) ABSTRACT;
    };

}
