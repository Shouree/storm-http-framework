#pragma once
#include "Driver.h"

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
		void set_int(int64_t value);
		void set_uint(uint64_t value);
		void set_string(const std::string &value);
		void set_string(size_t max_length);
		void set_null();

		// Get the value as a particular type.
		int64_t get_int() const;
		uint64_t get_uint() const;
		std::string get_string() const;

		// Check if the value is null.
		bool is_null() const;

		// Check if truncated. If truncated, return the full size of the data.
		size_t is_truncated() const {
			if (error)
				return data->buffer_length;
			else
				return 0;
		}

	private:
		// Associated data container.
		MYSQL_BIND *data;

		// Is the value null?
		my_bool null_value;

		// Is the value unsigned?
		my_bool unsigned_value;

		// Truncation error?
		my_bool error;

		// Inline data storage large enough to store an integer. This is so that we can avoid
		// some allocations when interoperating with MySQL.
		union Data {
			int64_t signed_v;
			uint64_t unsigned_v;
		} local_buffer;

		// Clear any data in this value, and re-set into the generic null representation.
		void clear();
	};


	/**
	 * Storage of zero or more values in a format that can be used together with MySQL.
	 *
	 * TODO: Make this better suited to Storm.
	 */
	class ValueSet {
	public:
		// Set size.
		ValueSet(size_t count);

		// Destroy.
		~ValueSet();

		// No copying.
		ValueSet(const ValueSet &) = delete;
		ValueSet &operator =(const ValueSet &) = delete;

		// Get a particular value.
		Value &operator[](size_t id) { return values[id]; }
		const Value &operator[](size_t id) const { return values[id]; }

		// Get the size.
		size_t size() const { return values.size(); }

		// Get a pointer to the underlying data. Intended to be passed to the MySQL C-api.
		MYSQL_BIND *data() { return bind_data.data(); }

	private:
		// Storage of MYSQL_BIND structures.
		std::vector<MYSQL_BIND> bind_data;

		// Storage of Value classes. These contain a minimal amount of storage that may be used
		// inside the "bind_data", so this vector's storage must not be reallocated.
		std::vector<Value> values;
	};

}
