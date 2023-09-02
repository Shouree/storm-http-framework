#pragma once
#include "Core/Array.h"

namespace sql {

	/**
	 * A class that represents a generalized query string.
	 *
	 * This is so that we can describe parts of the query in a general manner, bridging the
	 * differences between different databases. Using this approach the query builder in the SQL
	 * library can output a generic representation of what it wishes to achieve, and then each SQL
	 * driver can make adjustments to fit the peculiarities of that driver.
	 *
	 * Some examples of differences:
	 * - Quoting of names: MySQL uses backtick, which is non-standard. Others use single quote.
	 * - Placeholders in prepared statements: Most accept ?, but Oracle uses a special syntax.
	 *
	 * The actual QueryStr class is immutable. Use the QueryBuilder to build a string.
	 *
	 * The representation in the class consists of a sequence of integers that dictate what to add,
	 * accompanied by a sequence of strings that are added as appropriate. The integers act like
	 * "op-codes" that determine how to interpret the strings, and if anything need to be added in
	 * between.
	 */
	class QueryStr : public Object {
		STORM_CLASS;
	public:
		// Enum of generic types representable here.
		// The representation of QueryStr allows annotating all types
		// with a size parameter, even though that does not necessarily
		// make sense for all supported types.
		enum Type {
			// TEXT - generic string. If parameterized, specifies maximum size.
			text,
			// INTEGER - generic integer.
			integer,
			// REAL - generic float.
			real,
			// ...
		};


		/**
		 * Class to generate a visitor.
		 */
		class Visitor : public Object {
			STORM_ABSTRACT_CLASS;
		public:
			STORM_CTOR Visitor();

			// Output regular text.
			virtual void STORM_FN put(StrBuf *to, Str *text);

			// Output a name.
			virtual void STORM_FN name(StrBuf *to, Str *name);

			// Output a placeholder.
			virtual void STORM_FN placeholder(StrBuf *to);

			// Output autoincrement.
			virtual void STORM_FN autoincrement(StrBuf *to);

			// Output a type. The size is valid unless it is Nat(-1).
			virtual void STORM_FN type(StrBuf *to, Type type, Nat size);
		};


		// Create.
		QueryStr(GcArray<Nat> *ops, GcArray<Str *> *text);

		// Generate a query string using the specified method.
		Str *generate(Visitor *visitor) const;

	protected:
		// To string.
		void STORM_FN toS(StrBuf *to) const override;

	private:
		// Operations.
		GcArray<Nat> *opData;

		// Text.
		GcArray<Str *> *textData;
	};


	/**
	 * A builder class to create a QueryStr.
	 */
	class QueryStrBuilder : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR QueryStrBuilder();

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *env);

		// Add a piece of a regular string.
		void STORM_FN put(Str *str);

		// Add a quoted name.
		void STORM_FN name(Str *str);

		// Add a placeholder for prepared statements.
		void STORM_FN placeholder();

		// Add "autoincrement" keyword.
		void STORM_FN autoincrement();

		// Add a generic type.
		void STORM_FN type(QueryStr::Type type);
		void STORM_FN type(QueryStr::Type type, Nat size);

		// Create the query string.
		QueryStr *STORM_FN build();

	private:
		// String buffer for the string data being built.
		StrBuf *currentStr;

		// Op-code array being constructed.
		Array<Nat> *ops;

		// String array being constructed.
		Array<Str *> *strings;

		// Flush the current string.
		void flush();
	};

}
