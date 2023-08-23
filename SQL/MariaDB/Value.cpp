#include "stdafx.h"
#include "Value.h"
#include "Core/Convert.h"

namespace sql {

	Value::Value(MYSQL_BIND *data)
		: data(data), nullValue(false), error(false) {

		// Clear the data, as per the manual. There are many fields that are not "public", so
		// this is the "sanctioned" way of doing this...
		memset(data, 0, sizeof(MYSQL_BIND));

		// Set 'length' to 'buffer_length'. This is allowed in the documentation, and allows us
		// to change 'length' without re-submitting the binds to MySQL.
		data->length = &data->buffer_length;

		// Set is_null so that we can change null values. Documentation says that
		// MYSQL_TYPE_NULL is an alternative.
		data->is_null = &nullValue;

		// Unsigned error.
		data->error = &error;

		// Set our representation to null.
		clear();
	}

	Value::~Value() {
		clear();

		// Clear the value so that we don't have any lingering pointers to ourselves.
		memset(data, 0, sizeof(MYSQL_BIND));
	}

	void Value::setInt(int64_t value) {
		clear();
		data->buffer_type = MYSQL_TYPE_LONGLONG;
		data->buffer = &localBuffer;
		data->buffer_length = sizeof(int64_t);
		data->is_unsigned = false;
		nullValue = false;
		localBuffer.signedVal = value;
	}

	void Value::setUInt(uint64_t value) {
		clear();
		data->buffer_type = MYSQL_TYPE_LONGLONG;
		data->buffer = &localBuffer;
		data->buffer_length = sizeof(int64_t);
		data->buffer = &localBuffer;
		data->is_unsigned = true;
		nullValue = false;
		localBuffer.unsignedVal = value;
	}

	void Value::setString(const std::string &value) {
		clear();

		size_t length = value.size() + 1;

		data->buffer_type = MYSQL_TYPE_STRING;
		data->buffer = malloc(length);
		strncpy(static_cast<char *>(data->buffer), value.c_str(), length);
		data->buffer_length = length;
		nullValue = false;
	}

	void Value::setString(size_t length) {
		clear();

		data->buffer_type = MYSQL_TYPE_STRING;
		data->buffer_length = length;
		if (length == 0)
			data->buffer = nullptr;
		else
			data->buffer = malloc(length);
	}

	void Value::setNull() {
		clear();
	}

	bool Value::isInt() const {
		return data->buffer_type == MYSQL_TYPE_LONGLONG;
	}

	int64_t Value::getInt() const {
		assert(isInt());

		if (nullValue)
			return 0;

		if (data->is_unsigned)
			return static_cast<int64_t>(localBuffer.unsignedVal);
		else
			return localBuffer.signedVal;
	}

	bool Value::isUInt() const {
		return data->buffer_type == MYSQL_TYPE_LONGLONG;
	}

	uint64_t Value::getUInt() const {
		assert(isUInt());

		if (nullValue)
			return 0;

		if (data->is_unsigned)
			return localBuffer.unsignedVal;
		else
			return static_cast<uint64_t>(localBuffer.signedVal);
	}

	bool Value::isString() const {
		return data->buffer_type == MYSQL_TYPE_STRING
			|| data->buffer_type == MYSQL_TYPE_VAR_STRING;
	}

	Str *Value::getString(Engine &e) const {
		assert(isString());

		if (nullValue)
			return new (e) Str();

		if (data->buffer_length == 0)
			return new (e) Str();

		const char *buffer = static_cast<const char *>(data->buffer);
		GcArray<wchar> *converted = toWChar(e, buffer, data->buffer_length);
		return new (e) Str(converted);
	}

	bool Value::isNull() const {
		return data->buffer_type == MYSQL_TYPE_NULL
			|| nullValue;
	}

	void Value::clear() {
		// Safeguard if we were not initialized.
		if (!data)
			return;

		if (data->buffer && data->buffer != &localBuffer) {
			free(data->buffer);
		}
		data->buffer = nullptr;
		data->buffer_type = MYSQL_TYPE_NULL;
		nullValue = true;
		data->is_unsigned = false;
		error = false;
	}

}
