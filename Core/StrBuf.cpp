#include "stdafx.h"
#include "StrBuf.h"
#include "Str.h"
#include "Utf.h"
#include "Convert.h"
#include "NumConvert.h"
#include "GcType.h"
#include "GcArray.h"

namespace storm {

	StrFmt::StrFmt() : width(0), flags(defaultFlags), fill(Char(' ')) {}

	StrFmt::StrFmt(Nat width, Byte digits, Byte flags, Char fill) :
		width(width), flags(flags), digits(digits), fill(fill) {}

	void StrFmt::reset() {
		width = 0;
		flags = (flags & ~alignMask) | alignNone;
	}

	void StrFmt::clear() {
		width = 0;
		flags = alignNone | floatNone;
		digits = 0;
		fill = Char(' ');
	}

	void StrFmt::merge(const StrFmt &o) {
		if (o.width)
			width = o.width;
		if (o.digits)
			digits = o.digits;
		if (o.fill != Char('\0'))
			fill = o.fill;

		if ((o.flags & alignMask) != alignNone)
			flags = (flags & ~alignMask) | (o.flags & alignMask);
		if ((o.flags & floatMask) != floatNone)
			flags = (flags & ~floatMask) | (o.flags & floatMask);
	}

	static Bool isLeft(const StrFmt &f) {
		return (f.flags & StrFmt::alignMask) == StrFmt::alignLeft;
	}

	static Bool isRight(const StrFmt &f) {
		// Default is 'right'.
		return (f.flags & StrFmt::alignMask) != StrFmt::alignLeft;
	}

	StrFmt width(Nat w) {
		return StrFmt(w, 0, StrFmt::defaultFlags, Char('\0'));
	}

	StrFmt left() {
		return StrFmt(0, 0, StrFmt::alignLeft, Char('\0'));
	}

	StrFmt left(Nat w) {
		return StrFmt(w, 0, StrFmt::alignLeft, Char('\0'));
	}

	StrFmt right() {
		return StrFmt(0, 0, StrFmt::alignRight, Char('\0'));
	}

	StrFmt right(Nat w) {
		return StrFmt(w, 0, StrFmt::alignRight, Char('\0'));
	}

	StrFmt fill(Char c) {
		return StrFmt(0, 0, StrFmt::defaultFlags, c);
	}

	StrFmt precision(Nat digits) {
		return StrFmt(0, digits, StrFmt::defaultFlags, Char('\0'));
	}

	StrFmt significant(Nat digits) {
		return StrFmt(0, digits, StrFmt::floatSignificant, Char('\0'));
	}

	StrFmt fixed(Nat digits) {
		return StrFmt(0, digits, StrFmt::floatFixed, Char('\0'));
	}

	StrFmt scientific(Nat digits) {
		return StrFmt(0, digits, StrFmt::floatScientific, Char('\0'));
	}

	HexFmt::HexFmt(Word v, Nat d) : value(v), digits(d) {}

	HexFmt STORM_FN hex(Byte v) {
		return HexFmt(v, 2);
	}

	HexFmt STORM_FN hex(Nat v) {
		return HexFmt(v, 8);
	}

	HexFmt STORM_FN hex(Word v) {
		return HexFmt(v, 16);
	}

	HexFmt hex(const void *ptr) {
		return HexFmt(Word(ptr), sizeof(ptr) * 2);
	}

	StrBuf::StrBuf() : buf(null) {
		clear();
	}

	StrBuf::StrBuf(const StrBuf &o) {
		clear();
		buf = copyBuf(o.buf);
		pos = o.pos;
	}

	StrBuf::StrBuf(Str *src) {
		clear();
		buf = copyBuf(src->data);
		pos = Nat(buf->count);
	}

	void StrBuf::deepCopy(CloneEnv *env) {
		buf = copyBuf(buf);
	}

	Str *StrBuf::toS() const {
		// We can not copy any extra data, so we can not use copyBuf()

		GcArray<wchar> *to = runtime::allocArray<wchar>(engine(), &wcharArrayType, pos + 1);
		for (Nat i = 0; i < pos; i++)
			to->v[i] = buf->v[i];

		return new (this) Str(to);
	}

	void StrBuf::toS(StrBuf *to) const {
		to->add(c_str());
	}

	wchar *StrBuf::c_str() const {
		return buf->v;
	}

	bool StrBuf::lastNewline() const {
		if (pos == 0)
			return true;
		return buf->v[pos - 1] == '\n';
	}

	void StrBuf::insertIndent() {
		if (!lastNewline())
			return;

		Nat iLen = Nat(indentStr->charCount());
		Nat iTot = iLen * indentation;
		ensure(pos + iTot);

		for (Nat i = 0; i < indentation; i++) {
			for (Nat j = 0; j < iLen; j++) {
				buf->v[pos++] = indentStr->data->v[j];
			}
		}
	}

	void StrBuf::fill(Nat toOutput) {
		Nat w = fmt.width;
		Char fill = fmt.fill;

		// Nothing to do?
		if (toOutput >= w)
			return;

		wchar first = fill.leading();
		wchar last = fill.trailing();
		Nat toFill = w - toOutput;
		if (first)
			ensure(pos + toFill*2);
		else
			ensure(pos + toFill);

		for (Nat i = 0; i < toFill; i++) {
			if (first)
				buf->v[pos++] = first;
			buf->v[pos++] = last;
		}
	}

	void StrBuf::fillReverse(Nat toOutput) {
		Nat w = fmt.width;
		Char fill = fmt.fill;

		// Nothing to do?
		if (toOutput >= w)
			return;

		wchar first = fill.leading();
		wchar last = fill.trailing();
		Nat toFill = w - toOutput;
		if (first)
			ensure(pos + toFill*2);
		else
			ensure(pos + toFill);

		for (Nat i = 0; i < toFill; i++) {
			buf->v[pos++] = last;
			if (first)
				buf->v[pos++] = first;
		}
	}

	StrBuf *StrBuf::add(const wchar *data) {
		Nat len = 0;
		Nat points = 0;
		Nat iLen = Nat(indentStr->charCount());
		Nat iTot = iLen * indentation;

		// Compute the length.
		if (lastNewline())
			len += iTot;

		for (Nat at = 0; data[at]; at++) {
			if (data[at] == '\n' && data[at + 1] != '\0')
				len += iTot;
			len++;
			if (!utf16::leading(data[at]))
				points++;
		}

		// Add fill.
		insertIndent();
		if (isRight(fmt))
			fill(points);

		// Reserve space (sometimes a bit too much, but that is fine).
		ensure(pos + len);

		// Copy.
		for (Nat at = 0; data[at]; at++) {
			insertIndent();
			buf->v[pos++] = data[at];
		}

		if (isLeft(fmt))
			fill(points);
		fmt.reset();

		return this;
	}

#ifdef POSIX
	StrBuf *StrBuf::add(const wchar_t *data) {
		return add(toWChar(engine(), data)->v);
	}
#endif

	StrBuf *StrBuf::addRaw(wchar ch) {
		ensure(pos + 1);
		buf->v[pos++] = ch;
		return this;
	}

	StrBuf *StrBuf::add(const Str *str) {
		return add(str->c_str());
	}

	StrBuf *StrBuf::add(const Object *obj) {
		// We're doing 'toS' to make the formatting predictable.
		if (obj) {
			return add(obj->toS());
		} else {
			return add(L"null");
		}
	}

	StrBuf *StrBuf::add(const TObject *obj) {
		// We're doing 'toS' to make the formatting predictable.
		if (obj) {
			Thread *thread = obj->associatedThread();
			if (thread && thread->thread() != os::Thread::current()) {
				os::Thread t = thread->thread();
				os::Future<Str *> f;
				os::FnCall<Str *, 1> p = os::fnCall().add(obj);
				os::UThread::spawn(address<Str * (CODECALL TObject::*)() const>(&TObject::toS), true, p, f, &t);
				add(f.result());
			} else {
				add(obj->toS());
			}
			return this;
		} else {
			return add(L"null");
		}
	}

	StrBuf *StrBuf::add(Bool b) {
		return add(b ? L"true" : L"false");
	}

	StrBuf *StrBuf::add(Byte i) {
		return add(Word(i));
	}

	StrBuf *StrBuf::add(Int i) {
		return add(Long(i));
	}

	StrBuf *StrBuf::add(Nat i) {
		return add(Word(i));
	}

	// 64-bit numbers never need more than 21 digits, including a - sign.
	static const Nat maxDigits = 21;

	static void reverse(wchar *begin, wchar *end) {
		while ((begin != end) && (begin != --end)) {
			swap(*(begin++), *end);
		}
	}

	StrBuf *StrBuf::add(Long i) {
		insertIndent();

		Nat worst = max(maxDigits, fmt.width * fmt.fill.size());

		ensure(pos + worst);

		bool negative = i < 0;

		Nat start = pos;
		while (pos < start + worst) {
			Long last = i % 10;
			i /= 10;

			if (last > 0 && negative)
				last -= 10;
			buf->v[pos++] = wchar(abs(last) + '0');
			if (i == 0)
				break;
		}

		if (negative)
			buf->v[pos++] = '-';

		// Fill if needed.
		if (isRight(fmt)) {
			fillReverse(pos - start);
			reverse(buf->v + start, buf->v + pos);
		} else if (isLeft(fmt)) {
			reverse(buf->v + start, buf->v + pos);
			fill(pos - start);
		}
		fmt.reset();

		return this;
	}

	StrBuf *StrBuf::add(Word i) {
		insertIndent();

		Nat worst = max(maxDigits, fmt.width * fmt.fill.size());

		ensure(pos + worst);

		Nat start = pos;
		while (pos < start + worst) {
			buf->v[pos++] = wchar((i % 10) + '0');
			i /= 10;
			if (i == 0)
				break;
		}

		// Fill if needed.
		if (isRight(fmt)) {
			fillReverse(pos - start);
			reverse(buf->v + start, buf->v + pos);
		} else if (isLeft(fmt)) {
			reverse(buf->v + start, buf->v + pos);
			fill(pos - start);
		}
		fmt.reset();

		return this;
	}

	StrBuf *StrBuf::add(Float f) {
		return add(Double(f));
	}

	StrBuf *StrBuf::add(Double f) {
		StdOBuf<100> buf;
		std::wostream stream(&buf);
		stream.imbue(std::locale::classic());

		stream << std::setprecision(fmt.digits);

		switch (fmt.flags & StrFmt::floatMask) {
		case StrFmt::floatNone:
		case StrFmt::floatSignificant:
			// Nothing needs to be done. This is the default!
			break;
		case StrFmt::floatFixed:
			stream << std::fixed;
			break;
		case StrFmt::floatScientific:
			stream << std::scientific;
			break;
		}

		stream << f;

		wchar b[100];
		buf.copy(b, 100);
		return add(b);
	}

	StrBuf *StrBuf::add(Char c) {
		const wchar str[3] = {
			c.leading(),
			c.trailing(),
			0
		};

		if (str[0])
			return add(str);
		else
			return add(str + 1);
	}

	StrBuf *StrBuf::add(HexFmt f) {
		// Enough for a 128 bit integer.
		const Nat bufSize = 32;
		wchar buf[bufSize + 1];

		const char digits[] = "0123456789ABCDEF";

		wchar *to = buf + bufSize;
		*to = 0;
		for (Nat i = 0; i < min(bufSize, f.digits); i++) {
			Nat part = f.value & 0x0F;
			f.value = f.value >> 4;
			*--to = digits[part];
		}

		return add(to);
	}

	void StrBuf::clear() {
		buf = null;
		pos = 0;
		indentStr = new (this) Str(L"    ");
		indentation = 0;
		// Note: clear and reset have slightly different semantics.
		fmt.clear();
		fmt.reset();

		// Default precision is 6 digits.
		fmt.digits = 6;
	}

	void StrBuf::indent() {
		indentation++;
	}

	void StrBuf::dedent() {
		indentation--;
	}

	void StrBuf::indentBy(Str *str) {
		indentStr = str;
	}

	StrBuf &StrBuf::operator <<(StrFmt f) {
		fmt.merge(f);
		return *this;
	}

	StrBuf &StrBuf::operator <<(const void *ptr) {
		size_t v = (size_t)ptr;
		const Nat digits = sizeof(size_t) * 2;
		wchar buf[digits + 3];
		buf[0] = '0';
		buf[1] = 'x';
		buf[digits + 2] = 0;

		for (Nat i = 0; i < digits; i++) {
			Nat digit = (v >> ((digits - i - 1) * 4)) & 0xF;
			if (digit < 0xA)
				buf[i + 2] = '0' + digit;
			else
				buf[i + 2] = 'A' + digit - 0xA;
		}

		return *add(buf);
	}

	Bool StrBuf::empty() const {
		return pos == 0 || buf == null;
	}

	Bool StrBuf::any() const {
		return !empty();
	}

	void StrBuf::ensure(Nat capacity) {
		Nat curr = 0;
		if (buf)
			curr = Nat(buf->count) - 1;

		if (curr >= capacity)
			return;

		// Grow!
		Nat newCap = max(capacity, curr * 2);
		newCap = max(newCap, Nat(16));

		GcArray<wchar> *to = runtime::allocArray<wchar>(engine(), &wcharArrayType, newCap + 1);
		for (Nat i = 0; i < pos; i++)
			to->v[i] = buf->v[i];

		buf = to;
	}

	GcArray<wchar> *StrBuf::copyBuf(GcArray<wchar> *buf) const {
		if (!buf)
			return null;
		GcArray<wchar> *to = runtime::allocArray<wchar>(engine(), &wcharArrayType, buf->count);
		for (Nat i = 0; i < buf->count; i++)
			to->v[i] = buf->v[i];
		return to;
	}

	/**
	 * Indent.
	 */

	Indent::Indent(StrBuf *buf) : buf(buf) {
		buf->indent();
	}

	Indent::~Indent() {
		buf->dedent();
	}
}
