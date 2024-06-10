#include "stdafx.h"
#include "Size.h"
#include "Utils/Bitwise.h"
#include "Core/StrBuf.h"

namespace code {

	// Helpers for the bitmasks. Stores size+align as: 0xasssssss
	static const Nat sizeMask = 0x0FFFFFFF;
	static const Nat alignMask = 0xF0000000;
	static const Nat alignShift = 28;
	static const Nat defAlign = 1 << alignShift;

	// Maximum useful alignment for 32- and 64-bit.
	static const Nat maxAlign32 = 8; // suprisingly, this is not 4.
	static const Nat maxAlign64 = 8;

	static inline Nat size(Nat in) {
		return in & sizeMask;
	}

	static inline void size(Nat &in, Nat size) {
		in = (in & ~sizeMask) | (size & sizeMask);
	}

	static inline Nat align(Nat in) {
		// Always return at least 1
		return max(Nat(1), (in & alignMask) >> alignShift);
	}

	static inline void align(Nat &in, Nat align, Nat maxAlign) {
		align = min(align, maxAlign);
		in = (in & ~alignMask) | ((align << alignShift) & alignMask);
	}

	static inline void set(Nat &in, Nat size, Nat maxAlign) {
		code::size(in, size);
		align(in, size, maxAlign);
	}

	static inline void add(Nat &in, Nat other, Nat maxAlign) {
		// Update alignment requirement.
		nat a = max(align(in), align(other));
		align(in, a, maxAlign);

		// Alignd and add.
		// Note: our alignment is not always important; consider the following example:
		// int, bool, bool, where the booleans do not have to be aligned to 4 bytes like the int.
		// If we had used 'align' instead of 'align(other)', we would have aligned them too strict.
		nat s = roundUp(size(in), align(other)) + size(other);
		size(in, s);
	}

	static inline void mul(Nat &in, Nat other, Nat maxAlign) {
		if (other == 0) {
			// Nothing.
			size(in, 0);
		} else if (other == 1) {
			// Identity.
		} else {
			nat extra = 0;
			size(extra, (other - 1) * roundUp(size(in), align(in)));
			align(extra, align(in), maxAlign);
			add(in, extra, maxAlign);
		}
	}

	static void output(wostream &to, Int v) {
		to << toHex(size(v)) << L"(" << toHex(align(v)) << L")";
	}

	Size::Size() : s32(defAlign), s64(defAlign) {}

	Size::Size(Nat s) : s32(0), s64(0) {
		set(s32, s, maxAlign32);
		set(s64, s, maxAlign64);
	}

	Size::Size(nat s32, nat s64) : s32(0), s64(0) {
		set(this->s32, s32, maxAlign32);
		set(this->s64, s64, maxAlign64);
	}

	Size::Size(nat size32, nat align32, nat size64, nat align64) : s32(0), s64(0) {
		code::size(s32, size32);
		code::align(s32, align32, maxAlign32);
		code::size(s64, size64);
		code::align(s64, align64, maxAlign64);
	}

	Size Size::sPtr = Size(4, 8);
	Size Size::sChar = Size(1);
	Size Size::sByte = Size(1);
	Size Size::sInt = Size(4);
	Size Size::sNat = Size(4);
	Size Size::sLong = Size(8);
	Size Size::sWord = Size(8);

	Size Size::sFloat = Size(4);
	Size Size::sDouble = Size(8);

	nat Size::current() const {
		switch (sizeof(void *)) {
		case 4:
			return roundUp(code::size(s32), code::align(s32));
		case 8:
			return roundUp(code::size(s64), code::align(s64));
		default:
			assert(false, "Only 32 and 64-bit platforms are supported now.");
			return 0;
		}
	}

	Size Size::alignment() const {
		return Size(0, code::align(s32), 0, code::align(s64));
	}

	Size &Size::operator +=(const Size &o) {
		add(s32, o.s32, maxAlign32);
		add(s64, o.s64, maxAlign64);
		return *this;
	}

	Size Size::operator +(const Size &o) const {
		Size t = *this;
		t += o;
		return t;
	}

	Size &Size::operator *=(nat o) {
		mul(s32, o, maxAlign32);
		mul(s64, o, maxAlign64);
		return *this;
	}

	Size Size::operator *(nat o) const {
		Size t = *this;
		t *= o;
		return t;
	}

	bool Size::operator ==(const Size &o) const {
		return code::size(s32) == code::size(o.s32) && code::size(s64) == code::size(o.s64);
	}

	bool Size::operator !=(const Size &o) const {
		return !(*this == o);
	}

	wostream &operator <<(wostream &to, const Size &s) {
		if (code::size(s.s32) == code::size(s.s64) && code::align(s.s32) == code::align(s.s64)) {
			output(to, s.s32);
		} else {
			output(to, s.s32);
			to << L"/";
			output(to, s.s64);
		}
		return to;
	}

	void Size::toS(StrBuf *to) const {
		*to << ::toS(*this).c_str();
	}

	Size Size::aligned() const {
		return Size(size32(), align32(), size64(), align64());
	}

	Size Size::alignedAs(Size other) const {
		return Size(roundUp(code::size(s32), other.align32()),
					other.align32(),
					roundUp(code::size(s64), other.align64()),
					other.align64());
	}

	Size Size::unaligned() const {
		return Size(code::size(s32), 1, code::size(s64), 1);
	}

	Nat Size::size32() const {
		return roundUp(code::size(s32), code::align(s32));
	}

	Nat Size::size64() const {
		return roundUp(code::size(s64), code::align(s64));
	}

	Nat Size::align32() const {
		return code::align(s32);
	}

	Nat Size::align64() const {
		return code::align(s64);
	}


	Offset::Offset() : o32(0), o64(0) {}

	Offset::Offset(Int s) : o32(s), o64(s) {}

	Offset::Offset(Size s) : o32(Int(size(s.s32))), o64(Int(size(s.s64))) {}

	Offset::Offset(Int o32, Int o64) : o32(o32), o64(o64) {}

	Offset Offset::sPtr = Offset(4, 8);
	Offset Offset::sChar = Offset(1);
	Offset Offset::sByte = Offset(1);
	Offset Offset::sInt = Offset(4);
	Offset Offset::sNat = Offset(4);
	Offset Offset::sLong = Offset(8);
	Offset Offset::sWord = Offset(8);
	Offset Offset::sFloat = Offset(4);
	Offset Offset::sDouble = Offset(8);

	Int Offset::current() const {
		switch (sizeof(void *)) {
		case 4:
			return o32;
		case 8:
			return o64;
		default:
			assert(false, "Only 32 and 64-bit platforms are supported now.");
			return 0;
		}
	}

	Offset &Offset::operator +=(const Offset &o) {
		o32 += o.o32;
		o64 += o.o64;
		return *this;
	}

	Offset &Offset::operator -=(const Offset &o) {
		o32 -= o.o32;
		o64 -= o.o64;
		return *this;
	}

	Offset Offset::operator +(const Offset &o) const {
		Offset t = *this;
		t += o;
		return t;
	}

	Offset Offset::operator -(const Offset &o) const {
		Offset t = *this;
		t -= o;
		return t;
	}

	Offset &Offset::operator +=(const Size &o) {
		o32 += size(o.s32);
		o64 += size(o.s64);
		return *this;
	}

	Offset &Offset::operator -=(const Size &o) {
		o32 -= size(o.s32);
		o64 -= size(o.s64);
		return *this;
	}

	Offset Offset::operator +(const Size &o) const {
		Offset t = *this;
		t += o;
		return t;
	}

	Offset Offset::operator -(const Size &o) const {
		Offset t = *this;
		t -= o;
		return t;
	}

	Offset &Offset::operator *=(Int o) {
		o32 *= o;
		o64 *= o;
		return *this;
	}

	Offset Offset::operator *(Int o) const {
		Offset z = *this;
		z *= o;
		return z;
	}

	Offset Offset::operator -() const {
		return Offset(-o32, -o64);
	}

	bool Offset::operator ==(const Offset &o) const {
		return o32 == o.o32 && o64 == o.o64;
	}

	bool Offset::operator !=(const Offset &o) const {
		return !(*this == o);
	}

	wostream &operator <<(wostream &to, const Offset &s) {
		if (s.o32 >= 0)
			to << L"+";
		else
			to << L"-";
		if (s.o32 == s.o64)
			to << toHex(abs(s.o32), true);
		else
			to << toHex(abs(s.o32), true) << "/" << toHex(abs(s.o64), true);
		return to;
	}

	void Offset::toS(StrBuf *to) const {
		*to << ::toS(*this).c_str();
	}

	Offset Offset::alignAs(const Size &s) const {
		bool neg = o32 < 0;
		Int n32 = roundUp(std::abs(o32), int(s.align32()));
		Int n64 = roundUp(std::abs(o64), int(s.align64()));
		return Offset(neg ? -n32 : n32,
					neg ? -n64 : n64);
	}

	Offset Offset::abs() const {
		return Offset(::abs(o32), ::abs(o64));
	}

	Size min(Size a, Size b) {
		return Size(min(a.size32(), b.size32()),
					min(a.align32(), b.align32()),
					min(a.size64(), b.size64()),
					min(a.align64(), b.align64()));
	}

	Size max(Size a, Size b) {
		return Size(max(a.size32(), b.size32()),
					max(a.align32(), b.align32()),
					max(a.size64(), b.size64()),
					max(a.align64(), b.align64()));
	}

	Offset min(Offset a, Offset b) {
		return Offset(min(a.v32(), b.v32()), min(a.v64(), b.v64()));
	}

	Offset max(Offset a, Offset b) {
		return Offset(max(a.v32(), b.v32()), max(a.v64(), b.v64()));
	}

}

code::Offset abs(code::Offset s) {
	return s.abs();
}

