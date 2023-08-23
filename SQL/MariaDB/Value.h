#pragma once
#include "Driver.h"
#include "Core/Str.h"

namespace sql {

	/**
	 * A single value in a column for the MySQL database.
	 *
	 * This class allows converting to and from the MYSQL_BIND structure that represents an
	 * arbitrary datatype. In particular, this class supports storing the data externally so
	 * that we can create an array of MYSQL_BIND structures to pass to the mysql C-api.
	 *
	 * Values are initialized to a generic NULL value, without a particular type.
	 */
	class Value {
	public:
		// Create, and associate with some data. This operation will clear whatever is inside 'data'.
		Value(MYSQL_BIND *data);

		// Destroy. Frees any allocated memory, and clears the associated data.
		~Value();

		// Set the value. This also makes the value into the desired type (e.g. for output). For
		// dynamic data types (e.g. strings), it is possible to set an "empty" representation
		// and populate it later. In these cases, call the version that takes a length parameter
		// as appropriate.
		void setInt(Long value);
		void setUInt(Word value);
		void setFloat(Double value);
		void setString(Str *value);
		void setString(size_t max_length);
		void setNull();

		// Check contained type.
		bool isInt() const;
		bool isUInt() const;
		bool isFloat() const;
		bool isString() const;
		bool isNull() const;

		// Get the value as a particular type.
		Long getInt() const;
		Word getUInt() const;
		Double getFloat() const;
		Str *getString(Engine &e) const;

		// Check if truncated. If truncated, return the full size of the data.
		size_t isTruncated() const {
			if (error)
				return dataLength;
			else
				return 0;
		}

	private:
		// Associated data container.
		MYSQL_BIND *data;

		// Is the value null?
		my_bool nullValue;

		// Truncation error?
		my_bool error;

		// Data length. Only used for strings and other dynamic data structures.
		unsigned long dataLength;

		// Inline data storage large enough to store an integer. This is so that we can avoid
		// some allocations when interoperating with MySQL.
		union Data {
			Long signedVal;
			Word unsignedVal;
			Double floatVal;
		} localBuffer;

		// Clear any data in this value, and re-set into the generic null representation.
		void clear();
	};

}
