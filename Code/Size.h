#pragma once

namespace code {
	STORM_PKG(core.asm);

	/**
	 * Machine-independent size description. This class allows composition
	 * of primitive types to composite types. For composite types, the
	 * class keeps track of the needed alignment for the type as well.
	 *
	 * This is currently implemented as two separate sizes, one assuming
	 * 32-bit pointers and the other assuming 64-bit pointers. This will
	 * suffice for a while, and may be extended in the future.
	 */
	class Size {
		STORM_VALUE;

		friend class Offset;
	public:
		// Initialize to zero.
		STORM_CTOR Size();

		// Initialize to a known size (platform independent).
		STORM_CTOR Size(Nat s);

		// Initialize to previously obtained values.
		Size(Nat size32, Nat align32, Nat size64, Nat align64);

		// Get the size for the current platform, the size being properly aligned.
		Nat STORM_FN current() const;

		// Get a Size of zero, only with the align of this Size.
		Size STORM_FN alignment() const;

		// Pointer size.
		static Size sPtr;

		// Integer sizes.
		static Size sChar;
		static Size sByte;
		static Size sInt;
		static Size sNat;
		static Size sLong;
		static Size sWord;

		// Float sizes.
		static Size sFloat;
		static Size sDouble;

		// Addition (note that we do not have subtraction).
		Size &STORM_FN operator +=(const Size &o);
		Size STORM_FN operator +(const Size &o) const;

		// Multiplication with positive. Equal to repeated addition.
		Size &STORM_FN operator *=(Nat o);
		Size STORM_FN operator *(Nat o) const;

		// Equality check.
		Bool STORM_FN operator ==(const Size &o) const;
		Bool STORM_FN operator !=(const Size &o) const;

		// Align the size (as seen by offsets) to the current alignment in here.
		Size STORM_FN aligned() const;

		// Get a Size aligned as another size.
		Size STORM_FN alignedAs(Size other) const;

		// Get an unaligned size.
		Size STORM_FN unaligned() const;

		// Find out the 32- and 64-bit sizes (for storage).
		nat size32() const;
		nat size64() const;
		nat align32() const;
		nat align64() const;

		// Output.
		void STORM_FN toS(StrBuf *to) const;

		// Output.
		friend wostream &operator <<(wostream &to, const Size &s);
	private:
		// Initialize to specific values.
		Size(nat s32, nat s64);

		// 32-bit ptr size and offset.
		Nat s32;

		// 64-bit ptr size and offset.
		Nat s64;
	};

	// Get sizes from Storm.
	inline Size STORM_FN sPtr() { return Size::sPtr; }
	inline Size STORM_FN sChar() { return Size::sChar; }
	inline Size STORM_FN sByte() { return Size::sByte; }
	inline Size STORM_FN sInt() { return Size::sInt; }
	inline Size STORM_FN sNat() { return Size::sNat; }
	inline Size STORM_FN sLong() { return Size::sLong; }
	inline Size STORM_FN sWord() { return Size::sWord; }
	inline Size STORM_FN sFloat() { return Size::sFloat; }
	inline Size STORM_FN sDouble() { return Size::sDouble; }

	/**
	 * Output.
	 */
	wostream &operator <<(wostream &to, const Size &s);


	/**
	 * Offset. Differently from size, an offset does not keep alignment!
	 */
	class Offset {
		STORM_VALUE;

		friend class Size;
	public:
		// Initialize to zero.
		STORM_CTOR Offset();

		// Initialize to a known size (platform independent).
		STORM_CTOR Offset(Int s);

		// Convert from Size.
		STORM_CTOR Offset(Size s);

		// Initialize to specific values.
		Offset(int s32, int s64);

		// Get the size for the current platform.
		Int STORM_FN current() const;

		// Pointer size.
		static Offset sPtr;

		// Integer sizes.
		static Offset sChar;
		static Offset sByte;
		static Offset sInt;
		static Offset sNat;
		static Offset sLong;
		static Offset sWord;

		// Float size.
		static Offset sFloat;
		static Offset sDouble;

		Offset &STORM_FN operator +=(const Offset &o);
		Offset &STORM_FN operator -=(const Offset &o);
		Offset STORM_FN operator +(const Offset &o) const;
		Offset STORM_FN operator -(const Offset &o) const;

		// Addition/subtraction with Size
		Offset &STORM_FN operator +=(const Size &o);
		Offset &STORM_FN operator -=(const Size &o);
		Offset STORM_FN operator +(const Size &o) const;
		Offset STORM_FN operator -(const Size &o) const;

		// Multiplication.
		Offset &STORM_FN operator *=(Int o);
		Offset STORM_FN operator *(Int o) const;

		// Negation. 'add' will still move further from zero.
		Offset STORM_FN operator -() const;

		// Equality check.
		Bool STORM_FN operator ==(const Offset &o) const;
		Bool STORM_FN operator !=(const Offset &o) const;

		// Align this offset to the align present in 'size'.
		Offset STORM_FN alignAs(const Size &s) const;

		// Find out the 32- and 64-bit sizes (for storage).
		inline int v32() const { return o32; }
		inline int v64() const { return o64; }

		// Output.
		void STORM_FN toS(StrBuf *to) const;

		// Output.
		friend wostream &operator <<(wostream &to, const Offset &s);

		// Absolute value.
		Offset STORM_FN abs() const;

	private:
		// 32-bit offset.
		Int o32;

		// 64-bit offset.
		Int o64;
	};

	/**
	 * Output.
	 */
	wostream &operator <<(wostream &to, const Offset &s);

	// Min/max operations.
	using std::min;
	using std::max;
	Size STORM_FN min(Size a, Size b);
	Size STORM_FN max(Size a, Size b);
	Offset STORM_FN min(Offset a, Offset b);
	Offset STORM_FN max(Offset a, Offset b);

}

// Abs.
code::Offset abs(code::Offset s);
