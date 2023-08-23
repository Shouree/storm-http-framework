#include "stdafx.h"
#include "Value.h"

namespace sql {

	Value::Value(MYSQL_BIND *data)
		: data(data), null_value(false), unsigned_value(false), error(false) {

		// Clear the data, as per the manual. There are many fields that are not "public", so
		// this is the "sanctioned" way of doing this...
		memset(data, 0, sizeof(MYSQL_BIND));

		// Set 'length' to 'buffer_length'. This is allowed in the documentation, and allows us
		// to change 'length' without re-submitting the binds to MySQL.
		data->length = &data->buffer_length;

		// Set is_null so that we can change null values. Documentation says that
		// MYSQL_TYPE_NULL is an alternative.
		data->is_null = &null_value;

		// Unsigned flag and error.
		data->is_unsigned = unsigned_value; // &unsigned_value; gick inte.. data vill inte ha en pekare
		data->error = &error;

		// Set our representation to null.
		clear();
	}

	Value::~Value() {
		clear();

		// Clear the value so that we don't have any lingering pointers to ourselves.
		memset(data, 0, sizeof(MYSQL_BIND));
	}

	void Value::set_int(int64_t value) {
		clear();
		data->buffer_type = MYSQL_TYPE_LONGLONG;
		data->buffer = &local_buffer;
		data->buffer_length = sizeof(int64_t);
		unsigned_value = false;
		null_value = false;
		local_buffer.signed_v = value;
	}

	void Value::set_uint(uint64_t value) {
		clear();
		data->buffer_type = MYSQL_TYPE_LONGLONG;
		data->buffer = &local_buffer;
		data->buffer_length = sizeof(int64_t);
		data->buffer = &local_buffer;
		unsigned_value = true;
		null_value = false;
		local_buffer.unsigned_v = value;
	}

	void Value::set_string(const std::string &value) {
		clear();

		size_t length = value.size() + 1;

		data->buffer_type = MYSQL_TYPE_STRING;
		data->buffer = malloc(length);
		strncpy(static_cast<char *>(data->buffer), value.c_str(), length);
		data->buffer_length = length;
		null_value = false;
	}

	void Value::set_string(size_t length) {
		clear();

		data->buffer_type = MYSQL_TYPE_STRING;
		data->buffer_length = length;
		if (length == 0)
			data->buffer = nullptr;
		else
			data->buffer = malloc(length);
	}

	void Value::set_null() {
		clear();
	}

	int64_t Value::get_int() const {
		if (data->buffer_type != MYSQL_TYPE_LONGLONG)
			////throw Error("Trying to get an integer from a non-integer value."); ///Kasta ett stormerror

			if (null_value)
				return 0;

		if (unsigned_value)
			return static_cast<int64_t>(local_buffer.unsigned_v);
		else
			return local_buffer.signed_v;
	}

	uint64_t Value::get_uint() const {
		if (data->buffer_type != MYSQL_TYPE_LONGLONG)
			//// throw Error("Trying to get an integer from a non-integer value."); //Kasta ett storm errro

			if (null_value)
				return 0;

		if (unsigned_value)
			return local_buffer.unsigned_v;
		else
			return static_cast<uint64_t>(local_buffer.signed_v);
	}

	std::string Value::get_string() const {
		if (data->buffer_type != MYSQL_TYPE_STRING && data->buffer_type != MYSQL_TYPE_VAR_STRING){
			//throw Error("Trying to get a string from a non-string value."); //Kasta ett storm error istället för PLN
			PLN(L"Trying to get a string from a non-string value.");
		}
		if (null_value)
			return "";

		if (data->buffer_length == 0)
			return "";
		const char *buffer = static_cast<const char *>(data->buffer);
		if (buffer[data->buffer_length - 1] == '\0')
			return std::string(buffer, buffer + data->buffer_length - 1);
		else
			return std::string(buffer, buffer + data->buffer_length);
	}

	bool Value::is_null() const {
		return data->buffer_type == MYSQL_TYPE_NULL
			|| null_value;
	}

	void Value::clear() {
		if (data->buffer && data->buffer != &local_buffer) {
			free(data->buffer);
		}
		data->buffer = nullptr;
		data->buffer_type = MYSQL_TYPE_NULL;
		null_value = true;
		unsigned_value = false;
		error = false;
	}


	/**
	 * ValueSet
	 */

	ValueSet::ValueSet(size_t count) : bind_data(count) {
		values.reserve(count);
		for (size_t i = 0; i < count; i++)
			values.emplace_back(&bind_data[i]);
	}

	ValueSet::~ValueSet() {
		// We need to destroy the Value instances first, as they need to look at what is inside
		// bind_data.
		values.clear();
	}

}
