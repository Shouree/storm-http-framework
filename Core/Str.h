#pragma once
#include "Object.h"
#include "Char.h"
#include "GcArray.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * The string type used in Storm.
	 *
	 * Strings are immutable sequences of unicode codepoints. The implementation stores strings in
	 * UTF-16, but hides this fact by disallowing low-level access to the underlying representation.
	 *
	 * Note: We may want to enforce proper normalization of strings to avoid weird results.
	 */
	class Str : public Object {
		STORM_CLASS;
	public:
		// Create an empty string.
		STORM_CTOR Str();

		// Create from a string literal.
		explicit Str(const wchar *s);

#ifdef POSIX
		// If wchar_t is a different size, allow creation from literals with wchar_t as well.
		explicit Str(const wchar_t *s);
#endif

		// Create from a substring of a c-string.
		Str(const wchar *from, const wchar *to);

		// Create a string from a buffer.
		Str(GcArray<wchar> *data);

		// Create from a single char or series of chars.
		STORM_CTOR Str(Char ch);
		STORM_CTOR Str(Char ch, Nat count);

		// Empty?
		Bool STORM_FN empty() const;
		Bool STORM_FN any() const;

		// Concatenation.
		Str *STORM_FN operator +(Str *o) const;
		Str *operator +(const wchar *o) const;
#ifdef POSIX
		Str *operator +(const wchar_t *o) const;
#endif

		// Multiplication.
		Str *STORM_FN operator *(Nat times) const;

		// Compare.
		virtual Bool STORM_FN operator ==(const Str &o) const;
		virtual Bool STORM_FN operator <(const Str &o) const;

		// Hash.
		virtual Nat STORM_FN hash() const;

		// Is this an integer?
		Bool STORM_FN isInt() const;
		Bool STORM_FN isNat() const;

		// Interpret as numbers.
		Int STORM_FN toInt() const;
		Nat STORM_FN toNat() const;
		Long STORM_FN toLong() const;
		Word STORM_FN toWord() const;
		Float STORM_FN toFloat() const;
		Double STORM_FN toDouble() const;

		// Interpret as hexadecimal numbers.
		Nat STORM_FN hexToNat() const;
		Word STORM_FN hexToWord() const;

		// Escape/unescape characters. Any unknown escape sequences are kept as they are.
		Str *STORM_FN unescape() const;
		Str *STORM_FN unescape(Char extra) const;
		Str *STORM_FN unescape(Char extra, Char extra2) const;
		Str *STORM_FN escape() const;
		Str *STORM_FN escape(Char extra) const;
		Str *STORM_FN escape(Char extra, Char extra2) const;

		// Starts/ends with?
		Bool STORM_FN startsWith(const Str *s) const;
		Bool STORM_FN endsWith(const Str *s) const;
		Bool startsWith(const wchar *s) const;
		Bool endsWith(const wchar *s) const;

		// Deep copy (nothing needs to be done really).
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// To string.
		virtual Str *STORM_FN toS() const;
		virtual void STORM_FN toS(StrBuf *buf) const;

		// Get a c-string.
		const wchar *c_str() const;

		// Get an UTF-8 encoded c-string allocated on the GC heap.
		const char *utf8_str() const;

		// Convert to/from cr-lf line endings. Returns the same string if possible.
		Str *STORM_FN toCrLf() const;
		Str *STORM_FN fromCrLf() const;

		// Compare to c-string.
		Bool operator ==(const wchar *s) const;
		Bool operator !=(const wchar *s) const;

		// Peek at the length of the underlying representation.
		Nat peekLength() const;

		/**
		 * Iterator.
		 */
		class Iter {
			STORM_VALUE;
		public:
			// Create an iterator to end.
			STORM_CTOR Iter();

			// Deep copy.
			void STORM_FN deepCopy(CloneEnv *env);

			// Advance.
			Iter &STORM_FN operator ++();
			Iter STORM_FN operator ++(int dummy);
			Iter STORM_FN operator +(Nat steps) const;

			// Compute difference.
			Nat STORM_FN operator -(const Iter &o) const;

			// Compare.
			Bool STORM_FN operator ==(const Iter &o) const;
			Bool STORM_FN operator !=(const Iter &o) const;
			Bool STORM_FN operator >(const Iter &o) const;
			Bool STORM_FN operator <(const Iter &o) const;
			Bool STORM_FN operator >=(const Iter &o) const;
			Bool STORM_FN operator <=(const Iter &o) const;

			// Get the value.
			Char operator *() const;
			Char STORM_FN v() const;

			// Peek at the raw offset.
			inline Nat offset() const { return pos; }

			// Peek at the string.
			inline const Str *data() const { return owner; }

		private:
			friend class Str;

			// Create iterator to start.
			Iter(const Str *str, Nat pos);

			// String we're referring to.
			const Str *owner;
			Nat pos;

			// At the end?
			bool atEnd() const;
		};

		// Begin and end.
		Iter STORM_FN begin() const;
		Iter STORM_FN end() const;

		// Get an iterator to a specific position.
		Iter posIter(Nat pos) const;

		// Substring. TODO: Rename?
		Str *STORM_FN substr(Iter from) const;
		Str *STORM_FN substr(Iter from, Iter to) const;

		// Alternative name for substr.
		Str *STORM_FN cut(Iter from) const;
		Str *STORM_FN cut(Iter from, Iter to) const;

		// Remove characters from the middle of the string.
		Str *STORM_FN remove(Iter from, Iter to) const;

		// Insert an entire string at a given position.
		Str *STORM_FN insert(Iter pos, Str *str) const;

		// Find a character in the string. Returns the first appearance of the character.
		Iter STORM_FN find(Char ch) const;
		Iter STORM_FN find(Char ch, Iter start) const;

		// Find the last occurrence of 'ch' in the string. Note that 'last' is *not* examined.
		Iter STORM_FN findLast(Char ch) const;
		Iter STORM_FN findLast(Char ch, Iter last) const;

		// Read/write (raw).
		void STORM_FN write(OStream *to) const;
		static Str *STORM_FN read(IStream *from);

		// Create a string from the stream. Use 'read' from Storm.
		explicit Str(IStream *from);

		// Serialization.
		void STORM_FN write(ObjOStream *to) const;
		static Str *STORM_FN read(ObjIStream *from);

	private:
		friend class Iter;
		friend class StrBuf;

		// Data we're storing. Always null-terminated or null.
		GcArray<wchar> *data;

		// Number of characters in 'data'.
		inline size_t charCount() const { return data->count - 1; }

		// Concatenation constructor.
		Str(const Str *a, const Str *b);
		Str(const Str *a, const wchar *b);

		// Repetition constructor.
		Str(const Str *a, Nat times);

		// Create from two substrings of a c-string.
		Str(const wchar *fromA, const wchar *toA, const wchar *fromB, const wchar *toB);

		// Create by inserting a string at a specific position.
		Str(const Str *into, const Iter &pos, const Str *insert);

		// Allocate 'data'.
		void allocData(nat count);

		// Convert an iterator to a pointer.
		const wchar *toPtr(const Iter &i) const;

		// Validate this string.
		void validate() const;
	};

	// Remove the indentation from a string.
	Str *STORM_FN removeIndentation(Str *str);

	// Remove leading and trailing empty lines from a string.
	Str *STORM_FN trimBlankLines(Str *src);

	// Strip whitespace from a string.
	Str *STORM_FN trimWhitespace(Str *src);

	// Output for iterators for convenience.
	StrBuf *STORM_FN operator <<(StrBuf *to, Str::Iter iter);

#ifdef POSIX
	// Low-level string operations for UTF-16.
	size_t wcslen(const wchar *str);
	int wcscmp(const wchar *a, const wchar *b);
#endif
}
