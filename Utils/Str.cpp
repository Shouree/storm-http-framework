#include "stdafx.h"
#include "Str.h"

#include <sstream>
#include <iomanip>
#include <limits.h>

#ifdef WINDOWS

String::String(const char *chars) {
	nat size = strlen(chars);
	// Allocate memory for the worst case.
	nat bufferSize = size * 2 + 1;
	wchar *outBuffer = new wchar[bufferSize];
	outBuffer[0] = 0;

	int count = MultiByteToWideChar(CP_UTF8, 0, chars, size, outBuffer, bufferSize - 1);
	assert(count >= 0);
	outBuffer[count] = 0;

	*this = outBuffer;

	delete []outBuffer;
}

std::string String::toChar() const {
	// Worst case is 4 chars per wchar in UTF-8
	nat bufferSize = size() * 4 + 1;
	char *outBuffer = new char[bufferSize];
	outBuffer[0] = 0;

	int count = WideCharToMultiByte(CP_UTF8, 0, data(), size(), outBuffer, bufferSize - 1, NULL, NULL);
	assert(count >= 0);
	outBuffer[count] = 0;

	std::string result(outBuffer);
	delete []outBuffer;

	return result;
}

#endif

#ifdef POSIX

#include <codecvt>

static std::wstring convert(const char *chars) {
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convert;
	return convert.from_bytes(chars);
}

String::String(const char *chars) : std::wstring(convert(chars)) {}

static std::wstring convert(const wchar *chars) {
	std::wostringstream z;
	z << chars;
	return z.str();
}

String::String(const wchar *chars) : std::wstring(convert(chars)) {}

std::string String::toChar() const {
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convert;
	return convert.to_bytes(*this);
}

namespace std {
	std::wostream &operator <<(std::wostream &to, const wchar *str) {
		wchar prev = 0;
		for (const wchar *at = str; *at; at++) {
			wchar ch = *at;
			if ((ch & 0xFC00) == 0xD800) {
				// Leading pair.
				prev = ch;
			} else if ((ch & 0xFC00) == 0xDC00) {
				// Trailing pair.
				if (prev != 0) {
					nat r = nat(prev & 0x3FF) << nat(10);
					r |= nat(ch & 0x3FF);
					r += 0x10000;
					to << wchar_t(r);
					prev = 0;
				}
			} else {
				// Plain!
				to << wchar_t(ch);
				prev = 0;
			}
		}

		return to;
	}
}
#endif

String String::left(size_t size) const {
	return String(*this, 0, size);
}

String String::right(size_t size) const {
	nat start = 0;
	if (size < this->size()) start = this->size() - size;
	return String(*this, start);
}

#ifdef WINDOWS
#define wcscasecmp _wcsicmp
#define wcsncasecmp _wcsnicmp
#endif

int String::compareNoCase(const String &str) const {
	return wcscasecmp(c_str(), str.c_str());
}

bool String::startsWith(const String &str) const {
	return wcsncmp(c_str(), str.c_str(), str.size()) == 0;
}

bool String::endsWith(const String &str) const {
	if (size() < str.size()) return false;
	nat delta = size() - str.size();
	const wchar_t *ourStart = c_str() + delta;
	return wcscmp(ourStart, str.c_str()) == 0;
}

bool String::startsWithNC(const String &str) const {
	return wcsncasecmp(c_str(), str.c_str(), str.size()) == 0;
}

bool String::endsWithNC(const String &str) const {
	if (size() < str.size()) return false;
	nat delta = size() - str.size();
	const wchar_t *ourStart = c_str() + delta;
	return wcscasecmp(ourStart, str.c_str()) == 0;
}

#ifdef WINDOWS
String String::toUpper() const {
	wchar *buffer = new wchar[size() + 1];
	wcscpy_s(buffer, size() + 1, c_str());
	_wcsupr_s(buffer, size() + 1);
	String result(buffer);
	delete []buffer;
	return result;
}

String String::toLower() const {
	wchar *buffer = new wchar[size() + 1];
	wcscpy_s(buffer, size() + 1, c_str());
	_wcslwr_s(buffer, size() + 1);
	String result(buffer);
	delete []buffer;
	return result;
}
#else
String String::toUpper() const {
	String result(*this);
	for (nat i = 0; i < result.size(); i++) {
		result[i] = towupper(result[i]);
	}
	return result;
}

String String::toLower() const {
	String result(*this);
	for (nat i = 0; i < result.size(); i++) {
		result[i] = towlower(result[i]);
	}
	return result;
}
#endif

String String::trim() const {
	if (size() == 0) return L"";

	nat start = 0, end = size() - 1;
	for (start = 0; start < size(); start++) {
		if (!isspace((*this)[start])) break;
	}

	while (end > start) {
		if (!isspace((*this)[end])) break;
		if (end > 1) --end;
	}

	if (start > end) return L"";
	return substr(start, end - start + 1);
}

vector<String> String::split(const String &delim) const {
	return ::split(*this, delim);
}

vector<String> split(const String &str, const String &delimiter) {
	vector<String> r;

	nat start = 0;
	nat end = str.find(delimiter);
	while (end != String::npos) {
		r.push_back(str.substr(start, end - start));

		start = end + delimiter.size();
		end = str.find(delimiter, start);
	}

	if (start < str.size())
		r.push_back(str.substr(start));

	return r;
}

String toHex(const void *x, bool prefix) {
	if (sizeof(void *) == sizeof(nat)) {
		return toHex(nat(size_t(x)), prefix);
	} else if (sizeof(void *) == sizeof(nat64)) {
		return toHex(nat64(x), prefix);
	}
}

String toHex(int x, bool prefix) {
	std::wostringstream oss;

	if (prefix) oss << L"0x";
	oss << std::hex << std::setw(CHAR_BIT / 4) << std::setfill(L'0') << x;

	return oss.str();
}

String toHex(int64 x, bool prefix) {
	std::wostringstream oss;

	if (prefix) oss << L"0x";
	oss << std::hex << std::setw(CHAR_BIT / 4) << std::setfill(L'0') << x;

	return oss.str();
}

String toHex(nat x, bool prefix) {
	std::wostringstream oss;

	if (prefix) oss << L"0x";
	oss << std::hex << std::setw(CHAR_BIT / 4) << std::setfill(L'0') << x;

	return oss.str();
}

String toHex(nat64 x, bool prefix) {
	std::wostringstream oss;

	if (prefix) oss << L"0x";
	oss << std::hex << std::setw(CHAR_BIT / 4) << std::setfill(L'0') << x;

	return oss.str();
}

String toHex(const void *data, size_t size) {
	std::wostringstream to;
	to << std::setfill(L'0');
	to << std::hex;
	const nat width = 16;
	const byte *d = (const byte *)data;
	for (size_t i = 0; i < size; i++) {
		if (i % width == 0) {
			if (i > 0)
				to << L"\n";
			to << std::setw(4) << i << L" -";
		}

		to << L" " << std::setw(2) << int(d[i]);
	}

	return to.str();
}


template <class T>
inline String genericToS(T t) {
	std::wostringstream oss;
	oss << t;
	return oss.str();
}

String toS(int i) { return genericToS(i); }
String toS(nat i) { return genericToS(i); }
String toS(int64 i) { return genericToS(i); }
String toS(nat64 i) { return genericToS(i); }
String toS(double i) { return genericToS(i); }
String toS(const wchar_t *s) { return String(s); }
const String &toS(const String &s) { return s; }

double String::toDouble() const {
	wchar_t *end;
	return wcstod(c_str(), &end);
}

int String::toInt() const {
	wchar_t *end;
	return wcstol(c_str(), &end, 10);
}

nat String::toNat() const {
	wchar_t *end;
	return wcstoul(c_str(), &end, 10);
}

int64 String::toInt64() const {
	wchar_t *end;
#ifdef WINDOWS
	return _wcstoi64(c_str(), &end, 10);
#else
	return wcstoll(c_str(), &end, 10);
#endif
}

nat64 String::toNat64() const {
	wchar_t *end;
#ifdef WINDOWS
	return _wcstoui64(c_str(), &end, 10);
#else
	return wcstoull(c_str(), &end, 10);
#endif
}

nat String::toIntHex() const {
	wchar_t *end;
	if (left(2) == L"0x") {
		return wcstol(mid(2).c_str(), &end, 16);
	} else {
		return wcstol(c_str(), &end, 16);
	}
}

bool String::isInt() const {
	if (size() == 0)
		return false;

	nat i = 0;
	if ((*this)[i] == '-')
		i = 1;

	for (; i < size(); i++) {
		wchar_t c = (*this)[i];
		if (c < '0' || c > '9')
			return false;
	}
	return true;
}

bool String::isNat() const {
	for (nat i = 0; i < size(); i++) {
		wchar_t c = (*this)[i];
		if (c < '0' || c > '9')
			return false;
	}
	return true;
}

String String::unescape(bool keepUnknown) const {
	std::wostringstream ret;

	const String &str = *this;

	for (nat i = 0; i < str.size(); i++) {
		wchar_t ch = str[i];
		if (ch == '\\') {
			switch (str[++i]) {
				case 'n':
					ret << L'\n';
					break;
				case 'r':
					ret << '\r';
					break;
				case 't':
					ret << '\t';
					break;
				case 'v':
					ret << '\v';
					break;
				case 'x':
					//till hexadecimal...
					ret << wchar_t(str.mid(i + 1, 4).toIntHex());
					i += 4;
					break;
				default:
					if (keepUnknown)
						ret << '\\';
					ret << str[i];
					break;
			}
		} else {
			ret << ch;
		}
	}

	return ret.str().c_str();
}

String String::escape() const {
	std::wostringstream to;
	for (nat i = 0; i < size(); i++)
		to << escape((*this)[i]);
	return to.str();
}

String String::escape(wchar_t c) {
	switch (c) {
	case '\n':
		return L"\\n";
	case '\r':
		return L"\\r";
	case '\t':
		return L"\\t";
	default:
		return String(1, c);
	}
}

static nat firstParameterEnd(const String &str) {
	bool content = false;
	bool inString = false;
	bool lastQuoted = false;
	for (nat i = 0; i < str.size(); i++) {
		bool quote = false;
		switch (str[i]) {
			case '"':
				if (content) {
					if (!lastQuoted) return i + (inString ? 1 : 0);
				} else {
					if (!lastQuoted) inString = !inString;
				}
				break;
			case ' ':
				if (!inString) if (content) return i;
				break;
			case '\\':
				content = true;
				quote = true;
				break;
			default:
				content = true;
				break;
		}
		lastQuoted = quote;
	}

	return str.size();
}

String String::firstParam() const {
	nat end = firstParameterEnd(*this);

	String toReturn = left(end).trim();

	if ((toReturn[0] == '"') && (toReturn[toReturn.size() - 1] == '"')) {
		return toReturn.mid(1, toReturn.size() - 2);
	}
	return toReturn;
}

String String::restParams() const {
	nat end = firstParameterEnd(*this);

	if (end == size())
		return L"";

	String toReturn = mid(end).trim();
	return toReturn;
}
