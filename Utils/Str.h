#pragma once

#include <string>
#include <sstream>
#include <vector>

// String class for use in this project. Adds some neat features to the
// standard std::string.
class String : public std::wstring {
public:
	inline String() {}
	inline String(const std::wstring &str) : std::wstring(str) {}
	inline String(const wchar_t *str) : std::wstring(str) {}
	inline String(nat count, wchar_t ch) : std::wstring(size_t(count), ch) {}

#ifdef POSIX
	String(const wchar *str);
#endif

	// Concat ctor
	inline String(const wchar_t *strA, const wchar_t *strB) : std::wstring(strA) {
		*this += strB;
	}

	// Substr ctors
	inline String(const String &original, size_t start) : std::wstring(original, start) {}
	inline String(const String &original, size_t start, size_t length) : std::wstring(original, start, length) {}

	// Conversion to and from ANSI/UTF-8 strings
	explicit String(const char *str);
	std::string toChar() const;

	// Contains anything?
	inline bool any() const { return size() > 0; }

	// Operators on the string.
	inline String operator +(const String &b) const { return String(this->c_str(), b.c_str()); }

	// Compare without case.
	int compareNoCase(const String &str) const;

	// To uppercase
	String toUpper() const;

	// To lowercase
	String toLower() const;

	// Trim leading and/or trailing spaces
	String trim() const;

	// Starts/ends with?
	bool endsWith(const String &ending) const;
	bool startsWith(const String &beginning) const;

	// Starts/ends with, no case
	bool endsWithNC(const String &ending) const;
	bool startsWithNC(const String &beginning) const;

	// Neat functions inspired from the CString in MFC. All are safe in the sense that they
	// will return a result, even if the parameter indicates something strange.
	String left(size_t size) const;
	String right(size_t size) const;
	inline String mid(size_t start) const { return String(*this, start); }
	inline String mid(size_t start, size_t length) const { return String(*this, start, length); }
	inline String substr(size_t start, size_t length = npos) const { return String(*this, start, length); }

	// Convert a string to other datatypes.
	double toDouble() const;
	int toInt() const;
	nat toNat() const;
	int64 toInt64() const;
	nat64 toNat64() const;
	nat toIntHex() const;

	// Is the string an integer?
	bool isInt() const;
	bool isNat() const;

	// Treat the string as a command line.
	String unescape(bool keepUnknown = false) const;
	String escape() const;
	static String escape(wchar_t ch);
	String firstParam() const;
	String restParams() const;

	// Split string.
	vector<String> split(const String &delim) const;
};

inline String operator +(const String &a, const wchar_t *b) {
	return String(a.c_str(), b);
}

inline String operator +(const wchar_t *a, const String &b) {
	return String(a, b.c_str());
}

// Split 'str' by 'delimiter'. 'str' may start and end with 'delimiter',
// in that case those parts are discarded.
vector<String> split(const String &str, const String &delimiter);

// To string for various datatypes.
String toHex(int i, bool prefix = false);
String toHex(int64 i, bool prefix = false);
String toHex(nat i, bool prefix = false);
String toHex(nat64 i, bool prefix = false);
String toHex(const void *x, bool prefix = true);
String toS(int i);
String toS(nat i);
String toS(int64 i);
String toS(nat64 i);
String toS(double i);
String toS(const wchar_t *w);
const String &toS(const String &s);

// Generic hex-dump.
String toHex(const void *data, size_t count);

template <class A, class B>
inline std::wostream &operator <<(std::wostream &to, const std::pair<A, B*> &pair) {
	return to << pair.first << L" -> " << *pair.second;
}
template <class A, class B>
inline std::wostream &operator <<(std::wostream &to, const std::pair<A, B> &pair) {
	return to << pair.first << L" -> " << pair.second;
}

#ifdef POSIX
inline String toS(const wchar *w) { return String(w); }
namespace std {
	// Needs to be in std to behave properly in other namespaces.
	std::wostream &operator <<(std::wostream &to, const wchar *str);
}
#endif

template <class T>
void join(std::wostream &to, const T &data, const String &between);

// To string based on the output operator.
template <class T>
String toS(T *v) {
	std::wostringstream to;
	if (v)
		to << *v;
	else
		to << L"<null>";
	return to.str();
}
inline String toS(void *v) {
	std::wostringstream to;
	to << v;
	return to.str();
}
template <class T>
String toS(const T &v) {
	std::wostringstream to;
	to << v;
	return to.str();
}

// To string for arrays.
template <class T>
String toS(const vector<T> &data) {
	std::wostringstream to;
	to << L"[";
	join(to, data, L", ");
	to << L"]";
	return to.str();
}

template <class T>
String toS(const set<T> &data) {
	std::wostringstream to;
	to << L"{";
	join(to, data, L", ");
	to << L"}";
	return to.str();
}

// Join a vector or any other sequence type.
template <class T>
void join(std::wostream &to, const T &data, const String &between) {
	typename T::const_iterator i = data.begin();
	typename T::const_iterator end = data.end();
	if (i == end)
		return;

	to << toS(*i);
	for (++i; i != end; ++i) {
		to << between << toS(*i);
	}
}

template <class T, class ToS>
void join(std::wostream &to, const T &data, const String &between, ToS toS) {
	typename T::const_iterator i = data.begin();
	typename T::const_iterator end = data.end();
	if (i == end)
		return;

	to << toS(*i);
	for (++i; i != end; ++i) {
		to << between << toS(*i);
	}
}

template <class T>
String join(const T &data, const String &between) {
	std::wostringstream oss;
	join(oss, data, between);
	return oss.str();
}

template <class T, class ToS>
String join(const T &data, const String &between, ToS toS) {
	std::wostringstream oss;
	join(oss, data, between, toS);
	return oss.str();
}

// Output for arrays.
template <class T>
std::wostream &operator <<(std::wostream &to, const vector<T> &data) {
	join(to, data, L", ");
	return to;
}
