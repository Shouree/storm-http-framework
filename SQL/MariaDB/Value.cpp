#include "stdafx.h"
#include "Value.h"
#include "Core/Convert.h"

namespace sql {

	Value::Value(MYSQL_BIND *data)
		: data(data), nullValue(false), error(false), dataLength(0) {

		// Clear the data, as per the manual. There are many fields that are not "public", so
		// this is the "sanctioned" way of doing this...
		memset(data, 0, sizeof(MYSQL_BIND));

		// Set 'length' to 'buffer_length'. This is allowed in the documentation, and allows us
		// to change 'length' without re-submitting the binds to MySQL.
		data->length = &dataLength;

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

	void Value::setInt(Long value) {
		clear();
		data->buffer_type = MYSQL_TYPE_LONGLONG;
		data->buffer = &localBuffer;
		data->buffer_length = sizeof(Long);
		data->is_unsigned = false;
		nullValue = false;
		localBuffer.signedVal = value;
	}

	void Value::setUInt(Word value) {
		clear();
		data->buffer_type = MYSQL_TYPE_LONGLONG;
		data->buffer = &localBuffer;
		data->buffer_length = sizeof(Word);
		data->is_unsigned = true;
		nullValue = false;
		localBuffer.unsignedVal = value;
	}

	void Value::setFloat(Double value) {
		clear();
		data->buffer_type = MYSQL_TYPE_DOUBLE;
		data->buffer_length = sizeof(Double);
		data->buffer = &localBuffer;
		data->is_unsigned = false;
		nullValue = false;
		localBuffer.floatVal = value;
	}

	void Value::setString(Str *value) {
		clear();

		size_t length = convert(value->c_str(), null, 0);

		data->buffer_type = MYSQL_TYPE_STRING;
		data->buffer = malloc(length);
		data->buffer_length = (unsigned long)length;
		convert(value->c_str(), (char *)data->buffer, length);
		dataLength = (unsigned long)(length - 1); // Don't add the null terminator.
		nullValue = false;
	}

	void Value::setString(size_t length) {
		clear();

		data->buffer_type = MYSQL_TYPE_STRING;
		data->buffer_length = (unsigned long)length;
		if (length == 0)
			data->buffer = null;
		else
			data->buffer = malloc(length);
		dataLength = 0;
	}

	void Value::setNull() {
		clear();
	}

	bool Value::isInt() const {
		return data->buffer_type == MYSQL_TYPE_LONGLONG;
	}

	Long Value::getInt() const {
		assert(isInt());

		if (nullValue)
			return 0;

		if (data->is_unsigned)
			return static_cast<Long>(localBuffer.unsignedVal);
		else
			return localBuffer.signedVal;
	}

	bool Value::isUInt() const {
		return data->buffer_type == MYSQL_TYPE_LONGLONG;
	}

	Word Value::getUInt() const {
		assert(isUInt());

		if (nullValue)
			return 0;

		if (data->is_unsigned)
			return localBuffer.unsignedVal;
		else
			return static_cast<Word>(localBuffer.signedVal);
	}

	bool Value::isFloat() const {
		return data->buffer_type == MYSQL_TYPE_DOUBLE;
	}

	Double Value::getFloat() const {
		assert(isFloat());

		if (nullValue)
			return 0;

		return localBuffer.floatVal;
	}

	bool Value::isString() const {
		return data->buffer_type == MYSQL_TYPE_STRING
			|| data->buffer_type == MYSQL_TYPE_VAR_STRING;
	}

	Str *Value::getString(Engine &e) const {
		assert(isString());

		if (nullValue)
			return new (e) Str();

		if (data->buffer_length == 0 || dataLength == 0)
			return new (e) Str();

		const char *buffer = static_cast<const char *>(data->buffer);
		GcArray<wchar> *converted = toWChar(e, buffer, dataLength);
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
		data->buffer = null;
		data->buffer_type = MYSQL_TYPE_NULL;
		nullValue = true;
		data->is_unsigned = false;
		error = false;
		dataLength = 0;
	}

}
