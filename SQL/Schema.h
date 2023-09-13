#pragma once
#include "Core/Array.h"
#include "Core/Str.h"
#include "Utils/Bitmask.h"
#include "Type.h"

namespace sql {

	/**
	 * The Schema class contains the structure from a table within your database.
	 * To get the Schema one would need to call the schema() function in a database class.
	 */
	class Schema : public Object {
		STORM_CLASS;
	public:
		/**
		 * Attributes for a column.
		 */
		enum Attributes {
			// No attributes.
			none,
			// Primary key.
			primaryKey = 0x01,
			// Not null.
			notNull = 0x02,
			// Unique.
			unique = 0x04,
			// Autoincrement.
			autoIncrement = 0x08,
		};

		/**
		 * Schema contains the content class, which holds the name, datatype and all attributes for
		 * a given row.  This row is specified in the getRow function from the Schema class.
		 */
		class Column : public Object {
			STORM_CLASS;
		public:
			// Constructors of the Column class.
			STORM_CTOR Column(Str *name, QueryType dt);
			STORM_CTOR Column(Str *name, QueryType dt, Attributes attributes);

			// Name of the column.
			Str *name;

			// Datatype of the column.
			QueryType type;

			// Attribtes.
			Attributes attributes;

			// Default value, if any.
			MAYBE(Str *) STORM_NAME(defaultValue, default);

			// Unknown parts of the declaration. If 'type' is VOID, then the type is here also.
			MAYBE(Str *) unknown;

		protected:
			// To string.
			virtual void STORM_FN toS(StrBuf *to) const override;
		};

		/**
		 * An index on a particular table.
		 */
		class Index : public Object {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR Index(Str *name, Array<Str *> *columns);

			// Name of the index.
			Str *name;

			// Columns indexed.
			Array<Str *> *columns;

		protected:
			// To string.
			virtual void STORM_FN toS(StrBuf *to) const override;
		};

		// Create a filled schema.
		STORM_CTOR Schema(Str *tableName, Array<Column *> *columns);
		STORM_CTOR Schema(Str *tableName, Array<Column *> *columns, Array<Index *> *indices);

		// Number of columns in this table.
		Nat STORM_FN count() const {
			return columns->count();
		}

		// The name of this table.
		Str *STORM_FN name() const {
			return tableName;
		}

		// Get a column.
		Column *STORM_FN at(Nat id) const {
			return columns->at(id);
		}
		Column *STORM_FN operator[](Nat id) const {
			return columns->at(id);
		}

		// Indices.
		Array<Index *> *STORM_FN indices() const;

	protected:
		// To string.
		virtual void STORM_FN toS(StrBuf *to) const override;

	private:
		// Name of this table.
		Str *tableName;

		// Columns.
		Array<Column *> *columns;

		// Indices.
		Array<Index *> *index;
	};

	BITMASK_OPERATORS(Schema::Attributes);
}
