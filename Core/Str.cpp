#include "stdafx.h"
#include "Str.h"
#include "StrBuf.h"
#include "GcType.h"
#include "Utf.h"
#include "Convert.h"
#include "NumConvert.h"
#include "Io/Serialization.h"

namespace storm {

#ifdef POSIX
	size_t wcslen(const wchar *ch) {
		size_t r = 0;
		while (*ch) {
			ch++; r++;
		}
		return r;
	}

	int wcscmp(const wchar *a, const wchar *b) {
		do {
			if (*a != *b) {
				if (*a < *b)
					return -1;
				else
					return 1;
			}
		} while (*(a++) && *(b++));
		// They are equal!
		return 0;
	}

#define WRAP_STRFN(result, name)								\
	static result name(const wchar *v, wchar **e, int base) {	\
		const nat maxlen = 50;									\
		wchar_t data[maxlen + 1] = { 0 };						\
		for (nat i = 0; i < maxlen && v[i]; i++)				\
			data[i] = v[i];										\
																\
		wchar_t *err = null;									\
		result r = ::name(data, &err, base);					\
		if (e)													\
			*e = (wchar *)(v + (err - data));					\
		return r;												\
	}															\

	WRAP_STRFN(long, wcstol)
	WRAP_STRFN(unsigned long, wcstoul)
	WRAP_STRFN(long long, wcstoll)
	WRAP_STRFN(unsigned long long, wcstoull)

#elif defined(WINDOWS)
#define wcstoll _wcstoi64
#define wcstoull _wcstoui64
#endif

	static GcArray<wchar> empty = {
		1,
	};

	Str::Str() : data(&storm::empty) {}

	Str::Str(const wchar *s) {
		nat count = wcslen(s);
		allocData(count + 1);
		for (nat i = 0; i < count; i++)
			data->v[i] = s[i];
		data->v[count] = 0;
		validate();
	}

#ifdef POSIX
	Str::Str(const wchar_t *s) {
		data = toWChar(engine(), s);
		validate();
	}
#endif

	Str::Str(const wchar *from, const wchar *to) {
		nat count = to - from;
		allocData(count + 1);
		for (nat i = 0; i < count; i++)
			data->v[i] = from[i];
		data->v[count] = 0;
		validate();
	}

	static inline void copy(wchar *&to, const wchar *begin, const wchar *end) {
		for (const wchar *at = begin; at != end; at++)
			*(to++) = *at;
	}

	Str::Str(const wchar *fromA, const wchar *toA, const wchar *fromB, const wchar *toB) {
		nat count = (toA - fromA) + (toB - fromB);
		allocData(count + 1);
		wchar *to = data->v;
		copy(to, fromA, toA);
		copy(to, fromB, toB);
		*to = 0;
		validate();
	}

	Str::Str(const Str *src, const Iter &pos, const Str *insert) {
		nat count = src->charCount() + insert->charCount();
		allocData(count + 1);
		wchar *to = data->v;

		const wchar *first = src->data->v;
		const wchar *cut = src->toPtr(pos);
		const wchar *last = first + src->charCount();
		copy(to, first, cut);
		copy(to, insert->data->v, insert->data->v + insert->charCount());
		copy(to, cut, last);
		*to = 0;
		validate();
	}

	Str::Str(Char ch) {
		wchar lead = ch.leading();
		wchar trail = ch.trailing();

		if (lead) {
			allocData(3);
			data->v[0] = lead;
			data->v[1] = trail;
		} else if (trail) {
			allocData(2);
			data->v[0] = trail;
		} else {
			allocData(1);
		}
		validate();
	}

	Str::Str(Char ch, Nat times) {
		wchar lead = ch.leading();
		wchar trail = ch.trailing();

		if (lead) {
			allocData(2*times + 1);
			for (nat i = 0; i < times; i++) {
				data->v[i*2] = lead;
				data->v[i*2 + 1] = trail;
			}
		} else if (trail) {
			allocData(times + 1);
			for (nat i = 0; i < times; i++) {
				data->v[i] = trail;
			}
		} else {
			allocData(1);
		}
		validate();
	}

	Str::Str(GcArray<wchar> *data) : data(data) {
		validate();
	}

	void Str::validate() const {
#ifdef SLOW_DEBUG
		for (nat i = 0; i < data->count - 1; i++) {
			if (data->v[i] == 0) {
				assert(false, L"String contains a premature null terminator!");
			}
		}
		assert(data->v[data->count - 1] == 0, L"String is missing a null terminator!");
#endif
	}

	Bool Str::empty() const {
		return data->count == 1;
	}

	Bool Str::any() const {
		return !empty();
	}

	Str *Str::operator +(Str *o) const {
		return new (this) Str(this, o);
	}

	Str *Str::operator +(const wchar *o) const {
		return new (this) Str(this, o);
	}

#ifdef POSIX
	Str *Str::operator +(const wchar_t *o) const {
		return new (this) Str(this, toWChar(engine(), o)->v);
	}
#endif

	Str::Str(const Str *a, const Str *b) {
		size_t aSize = a->data->count - 1;
		size_t bSize = b->data->count - 1;

		allocData(aSize + bSize + 1);
		for (size_t i = 0; i < aSize; i++)
			data->v[i] = a->data->v[i];
		for (size_t i = 0; i < bSize; i++)
			data->v[i + aSize] = b->data->v[i];
		data->v[aSize + bSize] = 0;
	}

	Str::Str(const Str *a, const wchar *b) {
		size_t aSize = a->data->count - 1;
		size_t bSize = wcslen(b);

		allocData(aSize + bSize + 1);
		for (size_t i = 0; i < aSize; i++)
			data->v[i] = a->data->v[i];
		for (size_t i = 0; i < bSize; i++)
			data->v[i + aSize] = b[i];
		data->v[aSize + bSize] = 0;
	}

	Str *Str::operator *(Nat times) const {
		return new (this) Str(this, times);
	}

	Str::Str(const Str *a, Nat times) {
		size_t s = a->data->count - 1;
		allocData(s*times + 1);

		size_t at = 0;
		for (Nat i = 0; i < times; i++) {
			for (size_t j = 0; j < s; j++) {
				data->v[at++] = a->data->v[j];
			}
		}
	}

	Bool Str::operator ==(const Str &o) const {
		if (!sameType(this, &o))
			return false;

		return wcscmp(c_str(), o.c_str()) == 0;
	}

	Bool Str::operator <(const Str &o) const {
		if (!sameType(this, &o))
			return false;

		return wcscmp(c_str(), o.c_str()) < 0;
	}

	Nat Str::hash() const {
		// djb2 hash
		Nat r = 5381;
		size_t to = data->count - 1;
		for (size_t j = 0; j < to; j++)
			r = ((r << 5) + r) + data->v[j];

		return r;
	}

	Bool Str::isInt() const {
		Nat start = 0;
		if (data->v[0] == '-')
			start++;
		for (Nat i = start; i < data->count - 1; i++)
			if (data->v[i] < '0' || data->v[i] > '9')
				return false;
		return true;
	}

	Bool Str::isNat() const {
		for (nat i = 0; i < data->count - 1; i++)
			if (data->v[i] < '0' || data->v[i] > '9')
				return false;
		return true;
	}

	Int Str::toInt() const {
		wchar *end;
		Int r = wcstol(data->v, &end, 10);
		if (end != data->v + data->count - 1)
			throw new (this) StrError(S("Not a number"));
		return r;
	}

	Nat Str::toNat() const {
		wchar *end;
		Nat r = wcstoul(data->v, &end, 10);
		if (end != data->v + data->count - 1)
			throw new (this) StrError(S("Not a number"));
		return r;
	}

	Long Str::toLong() const {
		wchar *end;
		Long r = wcstoll(data->v, &end, 10);
		if (end != data->v + data->count - 1)
			throw new (this) StrError(S("Not a number"));
		return r;
	}

	Word Str::toWord() const {
		wchar *end;
		Word r = wcstoull(data->v, &end, 10);
		if (end != data->v + data->count - 1)
			throw new (this) StrError(S("Not a number"));
		return r;
	}

	Float Str::toFloat() const {
		Float r;
		StdIBuf<100> buf(data->v, data->count - 1);
		std::wistream input(&buf);
		input.imbue(std::locale::classic());
		if (!(input >> r))
			throw new (this) StrError(S("Not a floating-point number"));

		wchar_t probe;
		if (input >> probe)
			throw new (this) StrError(S("Not a floating-point number"));

		return r;
	}

	Double Str::toDouble() const {
		Double r;
		StdIBuf<100> buf(data->v, data->count - 1);
		std::wistream input(&buf);
		input.imbue(std::locale::classic());
		if (!(input >> r))
			throw new (this) StrError(S("Not a floating-point number"));

		wchar_t probe;
		if (input >> probe)
			throw new (this) StrError(S("Not a floating-point number"));

		return r;
	}

	static inline int hexDigit(wchar ch) {
		if (ch >= '0' && ch <= '9')
			return ch - '0';
		if (ch >= 'a' && ch <= 'f')
			return ch - 'a' + 10;
		if (ch >= 'A' && ch <= 'F')
			return ch - 'A' + 10;
		return -1;
	}

	Nat Str::hexToNat() const {
		return Nat(hexToWord());
	}

	Word Str::hexToWord() const {
		Word r = 0;
		for (nat i = 0; i < data->count - 1; i++) {
			wchar ch = data->v[i];
			int digit = hexDigit(ch);
			if (digit < 0)
				throw new (this) StrError(S("Not a hexadecimal number"));
			r = (r << 4) | Nat(digit);
		}
		return r;
	}

	template <bool useEscape>
	static bool unescape(const wchar *&src, wchar *&out, Char extra, Char extra2) {
		switch (src[1]) {
		case 'n':
			*out++ = '\n';
			src++;
			return true;
		case 'r':
			*out++ = '\r';
			src++;
			return true;
		case 't':
			*out++ = '\t';
			src++;
			return true;
		case 'v':
			*out++ = '\v';
			src++;
			return true;
		case '0':
			*out++ = '\0';
			src++;
			return true;
		case 'x': {
			int a = hexDigit(src[2]);
			if (a < 0)
				return false;
			int b = hexDigit(src[3]);
			if (b < 0)
				return false;
			src += 3;
			*out++ = wchar((a << 4) | b);
			return true;
		}
		case '\\':
			if (useEscape) {
				*out++ = '\\';
				src++;
				return true;
			}
			// fall thru
		default:
			if (extra.leading() != 0) {
				if (src[1] == extra.leading() && src[2] == extra.trailing()) {
					*out++ = extra.leading();
					*out++ = extra.trailing();
					src += 2;
					return true;
				}
			} else if (extra.trailing() != 0 && src[1] == extra.trailing()) {
				*out++ = extra.trailing();
				src++;
				return true;
			}

			if (extra2.leading() != 0) {
				if (src[1] == extra2.leading() && src[2] == extra2.trailing()) {
					*out++ = extra2.leading();
					*out++ = extra2.trailing();
					src += 2;
					return true;
				}
			} else if (extra2.trailing() != 0 && src[1] == extra2.trailing()) {
				*out++ = extra2.trailing();
				src++;
				return true;
			}

			return false;
		}
	}

	Str *Str::unescape() const {
		return unescape(Char());
	}

	Str *Str::unescape(Char extra) const {
		return unescape(extra, Char());
	}

	Str *Str::unescape(Char extra, Char extra2) const {
		// Note: we never need more space after unescaping a string.
		GcArray<wchar> *buf = runtime::allocArray<wchar>(engine(), &wcharArrayType, data->count);
		wchar *to = buf->v;

		for (const wchar *from = data->v; from < data->v + data->count - 1; from++) {
			wchar ch = *from;
			if (ch == '\\') {
				if (!storm::unescape<true>(from, to, extra, extra2))
					*to++ = '\\';
			} else {
				*to++ = ch;
			}
		}

		return new (this) Str(buf->v);
	}

	Str *Str::unescapeKeepBackslash(Char extra) const {
		GcArray<wchar> *buf = runtime::allocArray<wchar>(engine(), &wcharArrayType, data->count);
		wchar *to = buf->v;

		for (const wchar *from = data->v; from < data->v + data->count - 1; from++) {
			wchar ch = *from;
			if (ch == '\\') {
				if (!storm::unescape<false>(from, to, extra, Char())) {
					*to++ = '\\';
					if (from[1] == '\\') {
						*to++ = '\\';
						from++;
					}
				}
			} else {
				*to++ = ch;
			}
		}

		return new (this) Str(buf->v);
	}

	static bool escape(Char ch, StrBuf *to, Char extra, Char extra2) {
		if (ch == Char('\n')) {
			*to << L"\\n";
			return true;
		} else if (ch == Char('\r')) {
			*to << L"\\r";
			return true;
		} else if (ch == Char('\t')) {
			*to << L"\\t";
			return true;
		} else if (ch == Char('\v')) {
			*to << L"\\v";
			return true;
		} else if (ch == Char('\\')) {
			*to << L"\\\\";
			return true;
		} else if (ch == Char('\0')) {
			*to << L"\\0";
			return true;
		} else if (ch == extra && extra != Char()) {
			*to << L"\\" << extra;
			return true;
		} else if (ch == extra2 && extra2 != Char()) {
			*to << L"\\" << extra2;
			return true;
		} else if (ch.codepoint() < 32) {
			*to << L"\\x" << hex(Byte(ch.codepoint()));
			return true;
		} else {
			return false;
		}
	}

	Str *Str::escape() const {
		return escape(Char());
	}

	Str *Str::escape(Char extra) const {
		return escape(extra, Char());
	}

	Str *Str::escape(Char extra, Char extra2) const {
		// We do not know how much buffer we will need...
		StrBuf *to = new (this) StrBuf();

		for (Iter i = begin(), e = end(); i != e; ++i) {
			Char ch = i.v();

			if (!storm::escape(ch, to, extra, extra2))
				*to << ch;
		}

		return to->toS();
	}

	Bool Str::startsWith(const Str *s) const {
		return startsWith(s->c_str());
	}

	Bool Str::endsWith(const Str *s) const {
		return endsWith(s->c_str());
	}

	Bool Str::startsWith(const wchar *s) const {
		for (nat i = 0; s[i] != 0; i++) {
			if (data->v[i] != s[i])
				return false;
		}

		return true;
	}

	Bool Str::endsWith(const wchar *s) const {
		nat sLen = wcslen(s);
		if (sLen > charCount())
			return false;
		nat offset = charCount() - sLen;

		for (nat i = 0; i < sLen; i++) {
			if (data->v[offset + i] != s[i])
				return false;
		}

		return true;
	}

	static Nat toCrLfHelp(GcArray<wchar> *src, GcArray<wchar> *dest) {
		Nat pos = 0;
		for (Nat i = 0; i + 1 < src->count; i++) {
			if (src->v[i] == '\n') {
				if (i > 0 && src->v[i-1] != '\r') {
					if (dest)
						dest->v[pos] = '\r';
					pos++;
				}
			}
			if (dest)
				dest->v[pos] = src->v[i];
			pos++;
		}
		return pos;
	}

	Str *Str::toCrLf() const {
		Nat len = toCrLfHelp(data, null);
		if (len == charCount())
			return (Str *)this;

		GcArray<wchar> *to = runtime::allocArray<wchar>(engine(), &wcharArrayType, len + 1);
		toCrLfHelp(data, to);
		return new (this) Str(to);
	}

	static Nat fromCrLfHelp(GcArray<wchar> *src, GcArray<wchar> *dest) {
		Nat pos = 0;
		for (Nat i = 0; i + 1 < src->count; i++) {
			if (src->v[i] == '\r') {
				// Note: we do not need to check for out of bounds here, as we know strings are
				// always null terminated, and as such this element always exists (but might contain
				// null).
				if (src->v[i+1] == '\n') {
					// Ignore this one.
				} else {
					// Replace with \n
					if (dest)
						dest->v[pos] = '\n';
					pos++;
				}
			} else {
				if (dest)
					dest->v[pos] = src->v[i];
				pos++;
			}
		}
		return pos;
	}

	Str *Str::fromCrLf() const {
		Nat len = fromCrLfHelp(data, null);
		if (len == charCount())
			return (Str *)this;

		GcArray<wchar> *to = runtime::allocArray<wchar>(engine(), &wcharArrayType, len + 1);
		fromCrLfHelp(data, to);
		return new (this) Str(to);
	}

	Bool Str::operator ==(const wchar *s) const {
		return wcscmp(c_str(), s) == 0;
	}

	Bool Str::operator !=(const wchar *s) const {
		return wcscmp(c_str(), s) != 0;
	}

	void Str::deepCopy(CloneEnv *env) {
		// We don't have any mutable data we need to clone.
	}

	Str *Str::toS() const {
		// We're not mutable anyway...
		return (Str *)this;
	}

	void Str::toS(StrBuf *buf) const {
		buf->add(this);
	}

	const wchar *Str::c_str() const {
		return data->v;
	}

	const char *Str::utf8_str() const {
		return toChar(engine(), data->v)->v;
	}

	Nat Str::peekLength() const {
		return data->count - 1;
	}

	void Str::allocData(nat count) {
		data = runtime::allocArray<wchar>(engine(), &wcharArrayType, count);
	}

	Str::Iter Str::begin() const {
		return Iter(this, 0);
	}

	Str::Iter Str::end() const {
		return Iter();
	}

	Str::Iter Str::posIter(Nat pos) const {
		return Iter(this, pos);
	}

	Str *Str::substr(Iter start) const {
		return substr(start, end());
	}

	Str *Str::substr(Iter start, Iter end) const {
		const wchar *s = toPtr(start);
		const wchar *e = toPtr(end);

		// Make sure the iterators are in the right order.
		if (s > e)
			return new (this) Str(S(""));

		return new (this) Str(s, e);
	}

	Str *Str::cut(Iter start) const {
		return substr(start);
	}

	Str *Str::cut(Iter start, Iter end) const {
		return substr(start, end);
	}

	Str *Str::remove(Iter start, Iter end) const {
		return new (this) Str(data->v, toPtr(start), toPtr(end), data->v + charCount());
	}

	Str *Str::insert(Iter pos, Str *s) const {
		return new (this) Str(this, pos, s);
	}

	Str::Iter Str::find(Char ch) const {
		return find(ch, begin());
	}

	Str::Iter Str::find(Char ch, Iter start) const {
		Iter pos = start, last = end();
		for (; pos != last; ++pos)
			if (pos.v() == ch)
				return pos;
		return pos;
	}

	Str::Iter Str::findLast(Char ch) const {
		return findLast(ch, end());
	}

	Str::Iter Str::findLast(Char ch, Iter last) const {
		// TODO: We should search backwards...
		Iter pos = begin();
		Iter result = end();
		for (; pos != last; ++pos) {
			if (pos.v() == ch)
				result = pos;
		}

		return result;
	}

	void Str::write(OStream *to) const {
		GcArray<char> *buf = toChar(engine(), data->v);
		Buffer b = fullBuffer((GcArray<Byte> *)buf);
		b.filled(b.count() - 1);

		to->writeNat(b.filled());
		to->write(b);
	}

	Str::Str(IStream *from) {
		Nat count = from->readNat();
		Buffer b = from->readAll(count);
		if (!b.full())
			throw new (this) SerializationFormatError(S("Not enough data."));

		size_t sz = convert((char *)b.dataPtr(), count, NULL, 0);
		data = runtime::allocArray<wchar>(from->engine(), &wcharArrayType, sz);
		convert((char *)b.dataPtr(), count, data->v, sz);

		validate();
	}

	Str *Str::read(IStream *from) {
		return new (from) Str(from);
	}

	Str::Str(IStream *from, Nat limitBytes) {
		Nat count = from->readNat();
		if (count >= limitBytes / sizeof(wchar))
			throw new (this) SizeLimitReached(S("a string"), count * sizeof(wchar), limitBytes);
		Buffer b = from->readAll(count);
		if (!b.full())
			throw new (this) SerializationFormatError(S("Not enough data."));

		size_t sz = convert((char *)b.dataPtr(), count, NULL, 0);
		data = runtime::allocArray<wchar>(from->engine(), &wcharArrayType, sz);
		convert((char *)b.dataPtr(), count, data->v, sz);

		validate();
	}

	Str *Str::read(IStream *from, Nat limit) {
		return new (from) Str(from, limit);
	}

	void Str::write(ObjOStream *to) const {
		to->startPrimitive(strId);
		write(to->to);
		to->end();
	}

	Str::Str(ObjIStream *from) {
		Nat count = from->from->readNat();
		from->checkArrayAlloc(sizeof(wchar), count);
		Buffer b = from->from->readAll(count);
		if (!b.full())
			throw new (this) SerializationFormatError(S("Not enough data."));

		size_t sz = convert((char *)b.dataPtr(), count, NULL, 0);
		data = runtime::allocArray<wchar>(from->engine(), &wcharArrayType, sz);
		convert((char *)b.dataPtr(), count, data->v, sz);

		from->end();

		validate();
	}

	Str *Str::read(ObjIStream *from) {
		return (Str *)from->readPrimitiveObject(strId);
	}

	const wchar *Str::toPtr(const Iter &i) const {
		if (i.atEnd())
			return data->v + data->count - 1;
		else if (i.owner == this)
			return data->v + i.pos;
		else
			// Fallback if it is referring to the wrong object.
			return data->v;
	}

	Str::Iter::Iter() : owner(null), pos(0) {}

	Str::Iter::Iter(const Str *owner, Nat pos) : owner(owner), pos(pos) {}

	void Str::Iter::deepCopy(CloneEnv *env) {
		// What we need to do here is to make sure that the string inside of us is changed if the
		// string was copied. Otherwise we might end up with a string + iterator pair that no longer
		// match in threaded calls.
		if (Object *clone = env->cloned(const_cast<Str *>(owner)))
			owner = (Str *)clone;
	}

	Str::Iter &Str::Iter::operator ++() {
		if (atEnd())
			return *this;

		if (utf16::leading(owner->data->v[pos]))
			pos += 2;
		else
			pos++;

		return *this;
	}

	Str::Iter Str::Iter::operator ++(int dummy) {
		Iter t = *this;
		++*this;
		return t;
	}

	Str::Iter Str::Iter::operator +(Nat steps) const {
		Iter tmp = *this;
		for (Nat i = 0; i < steps; i++)
			++tmp;
		return tmp;
	}

	Nat Str::Iter::operator -(const Iter &o) const {
		const Str *owner = this->owner;
		Nat thisPos = this->pos;
		Nat otherPos = o.pos;

		// If one of the iterators is the at-end operator, make it into a "proper" end for the
		// string in the other iterator.
		if (!owner)
			owner = o.owner;
		if (!owner)
			return 0;

		// Now, the other owner must either be null or the same as 'owner' now. Otherwise, we are
		// comparing different strings.
		if (o.owner != owner && o.owner != null)
			return 0;

		if (!this->owner)
			thisPos = owner->data->count - 1;
		if (!o.owner)
			otherPos = owner->data->count - 1;

		Nat diff = 0;
		for (Nat at = otherPos; at < thisPos; diff++) {
			if (utf16::leading(owner->data->v[at]))
				at += 2;
			else
				at += 1;
		}

		return diff;
	}

	Bool Str::Iter::operator ==(const Iter &o) const {
		if (atEnd() || o.atEnd())
			return atEnd() == o.atEnd();

		return owner == o.owner && pos == o.pos;
	}

	Bool Str::Iter::operator !=(const Iter &o) const {
		return !(*this == o);
	}

	Bool Str::Iter::operator >(const Iter &o) const {
		return o < *this;
	}

	Bool Str::Iter::operator <(const Iter &o) const {
		if (o.atEnd() && !atEnd())
			return false;

		if (owner != o.owner)
			return false;

		return pos < o.pos;
	}

	Bool Str::Iter::operator >=(const Iter &o) const {
		return (*this > o) || (*this == o);
	}

	Bool Str::Iter::operator <=(const Iter &o) const {
		return (*this < o) || (*this == o);
	}

	// Get the value.
	Char Str::Iter::operator *() const {
		return v();
	}

	Char Str::Iter::v() const {
		if (atEnd())
			return Char(Nat(0));

		wchar p = owner->data->v[pos];
		if (utf16::leading(p)) {
			return Char(utf16::assemble(p, owner->data->v[pos + 1]));
		} else {
			return Char(p);
		}
	}

	Bool Str::Iter::atEnd() const {
		return owner ? pos + 1 == owner->data->count : true;
	}


	/**
	 * Utility functions.
	 */

	// Indentation...
	struct Indentation {
		wchar ch;
		nat count;

		static const nat invalid = -1;
	};

	static Indentation indentOf(const wchar *str, nat start) {
		Indentation r = { 0, 0 };

		if (str[start] != ' ' && str[start] != '\t')
			return r;

		r.ch = str[start];
		for (r.count = 0; str[start + r.count] == r.ch; r.count++)
			;

		return r;
	}

	static nat nextLine(const wchar *str, nat start) {
		for (; str[start] != 0 && str[start] != '\n'; start++)
			;

		if (str[start] != 0)
			start++;

		if (str[start] != 0 && str[start] == '\r')
			start++;

		return start;
	}

	static bool whitespace(wchar ch) {
		switch (ch) {
		case ' ':
		case '\t':
		case '\r':
		case '\n':
			return true;
		default:
			return false;
		}
	}

	static bool emptyLine(const wchar *str, nat start) {
		nat end = nextLine(str, start);
		for (nat i = start; i < end; i++) {
			if (!whitespace(str[i]))
				return false;
		}

		return true;
	}

	static Indentation min(const Indentation &a, const Indentation &b) {
		nat count = Indentation::invalid;

		if (a.count == Indentation::invalid)
			count = b.count;
		else if (b.count == Indentation::invalid)
			count = a.count;
		else
			count = ::min(a.count, b.count);

		wchar ch = 0;
		if (a.ch == 0)
			ch = b.ch;
		else if (b.ch == 0)
			ch = a.ch;
		else if (a.ch == b.ch)
			ch = a.ch;

		Indentation r = { ch, count };
		return r;
	}

	Str *removeIndentation(Str *str) {
		const wchar *src = str->c_str();

		// Examine the indentation of all lines...
		Indentation remove = { 0, Indentation::invalid };
		for (nat at = 0; src[at] != 0; at = nextLine(src, at)) {
			if (!emptyLine(src, at))
				remove = min(remove, indentOf(src, at));
		}

		if (remove.count == Indentation::invalid)
			return str;

		// Now we have some kind of indentation.
		StrBuf *to = new (str) StrBuf();

		nat at = 0;
		nat end = 0;
		while (src[at] != 0) {
			end = nextLine(src, at);

			if (emptyLine(src, at)) {
				for (nat i = at; i < end; i++)
					if (src[i] == '\n' || src[i] == '\r')
						to->addRaw(src[i]);
			} else {
				at += remove.count;
				for (nat i = at; i < end; i++)
					to->addRaw(src[i]);
			}

			at = end;
		}

		return to->toS();
	}

	Str *trimBlankLines(Str *str) {
		const wchar *src = str->c_str();

		nat start = 0;
		nat end = 0;

		for (nat at = 0; src[at] != 0; at = nextLine(src, at)) {
			if (!emptyLine(src, at)) {
				end = start = at;
				break;
			}
		}

		for (nat at = start; src[at] != 0; at = nextLine(src, at)) {
			if (!emptyLine(src, at)) {
				end = at;
			}
		}

		end = nextLine(src, end);
		while (end > 0) {
			wchar ch = src[end-1];
			if (ch == '\n' || ch == '\r')
				end--;
			else
				break;
		}

		return str->substr(str->posIter(start), str->posIter(end));
	}

	Str *trimWhitespace(Str *str) {
		const wchar *begin = str->c_str();
		const wchar *end = str->c_str();

		for (const wchar *at = begin; *at; at++) {
			if (whitespace(*at))
				begin = at + 1;
			else
				break;
		}

		for (const wchar *at = begin; *at; at++) {
			if (!whitespace(*at))
				end = at + 1;
		}

		return new (str) Str(begin, end);
	}

	StrBuf *operator <<(StrBuf *to, Str::Iter iter) {
		*to << S("Iterator: ");
		*to << iter.data()->substr(iter.data()->begin(), iter);
		*to << S("|>");
		*to << iter.data()->substr(iter);
		return to;
	}

}
