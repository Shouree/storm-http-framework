#pragma once
#include "Object.h"
#include "TObject.h"
#include "Char.h"
#include "GcArray.h"

namespace storm {
	STORM_PKG(core);

	class Str;

	/**
	 * Format description for a StrBuf.
	 *
	 * Use the helper functions to create instances of this object.
	 *
	 * Interesting idea: Make it possible to mark a region inside a StrBuf (possiby consisting of
	 * multiple put-operations) and apply a format to that region as a whole.
	 */
	class StrFmt {
		STORM_VALUE;
	public:
		StrFmt();
		StrFmt(Nat width, Byte digits, Byte flags, Char fill);

		// Min width of the next output.
		Nat width;

		// Fill character.
		Char fill;

		// Flags. Text alignment and float mode.
		Byte flags;

		// Number of digits in float numbers.
		Byte digits;

		// Reset after outputting something. May contain values that affects the formatting.
		void reset();

		// Clear. Restores all properties to values that do not affect another StrFmt.
		void clear();

		// Merge with another StrFmt.
		void merge(const StrFmt &o);

		// Contants for flags.
		enum {
			alignNone = 0x00,
			alignLeft = 0x01,
			alignRight = 0x02,

			alignMask = 0x03, // 0000 0011

			floatNone = 0x00,
			floatSignificant = 0x04,
			floatFixed = 0x08,
			floatScientific = 0x0C,

			floatMask = 0x0C, // 0000 1100

			defaultFlags = alignNone | floatNone,
		};
	};

	// Set the width of the next item that is outputted. It is filled with the character set by
	// `fill` to match the size if it is not already wide enough.
	StrFmt STORM_FN width(Nat width);

	// Left align the next output.
	StrFmt STORM_FN left();

	// Left align the next output and specify a width.
	StrFmt STORM_FN left(Nat width);

	// Right align the next output.
	StrFmt STORM_FN right();

	// Right align the next output and specify a width.
	StrFmt STORM_FN right(Nat width);

	// Set the fill character used to pad output.
	StrFmt STORM_FN fill(Char fill);

	// Set precision of the floating point output without modifying the mode.
	StrFmt STORM_FN precision(Nat digits);

	// Output floating point numbers in decimal form with the specified number of significant
	// digits, at maximum. This is the default.
	StrFmt STORM_FN significant(Nat digits);

	// Output floating point numbers in decimal form with the specified number of decimal places.
	StrFmt STORM_FN fixed(Nat decimals);

	// Output floating point numbers in scientific notation with the specified number of decimal
	// digits.
	StrFmt STORM_FN scientific(Nat digits);

	/**
	 * Hex format for numbers. Use the helper functions below to create instances of this object.
	 */
	class HexFmt {
		STORM_VALUE;
	public:
		HexFmt(Word value, Nat digits);

		// Value.
		Word value;

		// # of digits.
		Nat digits;
	};

	// Create hex formats.
	HexFmt STORM_FN hex(Byte v);
	HexFmt STORM_FN hex(Nat v);
	HexFmt STORM_FN hex(Word v);
	HexFmt hex(const void *ptr);

	/**
	 * Mutable string buffer for constructing strings quickly and easily. Approximates the ostream
	 * of std to some extent wrt formatting options.
	 */
	class StrBuf : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR StrBuf();
		StrBuf(const StrBuf &o);
		STORM_CTOR StrBuf(Str *from);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Get the string we built.
		virtual Str *STORM_FN toS() const;
		virtual void STORM_FN toS(StrBuf *buf) const;

		// C version.
		wchar *c_str() const;

		// Append stuff (these are useful in situations where we can't use the name "<<", in grammars for example)
		// TODO: Rename to "push" to be consistent with Array etc.
		StrBuf *add(const wchar *str);
		StrBuf *addRaw(wchar str);
		StrBuf *STORM_FN add(const Str *str);
		StrBuf *STORM_FN add(const Object *obj);
		StrBuf *STORM_FN add(const TObject *obj);
		StrBuf *STORM_FN add(Bool b);
		StrBuf *STORM_FN add(Byte i);
		StrBuf *STORM_FN add(Int i);
		StrBuf *STORM_FN add(Nat i);
		StrBuf *STORM_FN add(Long i);
		StrBuf *STORM_FN add(Word i);
		StrBuf *STORM_FN add(Float f);
		StrBuf *STORM_FN add(Double d);
		StrBuf *STORM_FN add(Char c);
		StrBuf *STORM_FN add(HexFmt h);

		// Append stuff with the << operator.
		StrBuf &operator <<(const void *ptr);
		StrBuf &operator <<(const wchar *str) { return *add(str); }
		StrBuf &STORM_FN operator <<(const Str *str) { return *add(str); }
		StrBuf &STORM_FN operator <<(const Object *obj) { return *add(obj); }
		StrBuf &STORM_FN operator <<(const TObject *obj) { return *add(obj); }
		StrBuf &STORM_FN operator <<(Bool b) { return *add(b); }
		StrBuf &STORM_FN operator <<(Byte i) { return *add(i); }
		StrBuf &STORM_FN operator <<(Int i) { return *add(i); }
		StrBuf &STORM_FN operator <<(Nat i) { return *add(i); }
		StrBuf &STORM_FN operator <<(Long i) { return *add(i); }
		StrBuf &STORM_FN operator <<(Word i) { return *add(i); }
		StrBuf &STORM_FN operator <<(Float f) { return *add(f); }
		StrBuf &STORM_FN operator <<(Double d) { return *add(d); }
		StrBuf &STORM_FN operator <<(Char c) { return *add(c); }
		StrBuf &STORM_FN operator <<(HexFmt f) { return *add(f); }

#ifdef POSIX
		StrBuf &operator <<(const wchar_t *str) { return *add(str); }
		StrBuf *add(const wchar_t *str);
#endif

		// Formatting options.
		StrBuf &STORM_FN operator <<(StrFmt fmt);

		// Clear.
		void STORM_FN clear();

		// Empty?
		Bool STORM_FN empty() const;
		Bool STORM_FN any() const;

		// Control indentation.
		void STORM_FN indent();
		void STORM_FN dedent();

		// Indentation string.
		void STORM_FN indentBy(Str *str);

		// Get/set the current format.
		inline StrFmt STORM_FN format() { return fmt; }
		inline void STORM_ASSIGN format(StrFmt f) { fmt = f; }

	private:
		// Buffer. Always zero-terminated as memory is filled with zero from the start.
		GcArray<wchar> *buf;

		// Current position in 'buf'.
		Nat pos;

		// Indentation string.
		Str *indentStr;

		// Current indentation.
		Nat indentation;

		// Current format.
		StrFmt fmt;

		// Ensure capacity (excluding the null-terminator).
		void ensure(nat capacity);

		// Was the last character inserted a newline?
		bool lastNewline() const;

		// Insert indentation here if needed.
		void insertIndent();

		// Insert fill character if needed.
		void fill(Nat toOutput);

		// Insert fill character if needed, assuming we will reverse the string afterwards.
		void fillReverse(Nat toOutput);

		// Copy a buffer.
		GcArray<wchar> *copyBuf(GcArray<wchar> *buf) const;
	};


	/**
	 * Indent the StrBuf. Restores the indentation when it goes out of scope.
	 *
	 * Note: Maybe we do not need to expose this to Storm, as we can have other ways of doing the
	 * same thing there.
	 */
	class Indent {
		STORM_VALUE;
	public:
		STORM_CTOR Indent(StrBuf *buf);
		~Indent();

	private:
		StrBuf *buf;
	};
}
