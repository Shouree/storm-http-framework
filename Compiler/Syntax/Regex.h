#pragma once
#include "Core/Array.h"
#include "Core/StrBuf.h"
#include "Core/Str.h"
#include "Core/Exception.h"
#include "Core/Io/Buffer.h"
#include "Core/Maybe.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		/**
		 * Regex matching. Stores pre-parsed patterns.
		 *
		 * Note: Surrogate pairs are not properly supported.
		 *
		 * Note: Care must be taken when serializing this class. Linux and Windows uses signed and
		 * unsigned wchar, respectively.
		 */
		class Regex {
			STORM_VALUE;
		public:
			// Create a matcher for the pattern.
			STORM_CTOR Regex(Str *pattern);

			// Deep copy.
			void STORM_FN deepCopy(CloneEnv *env);

			// Does this regex match the empty string?
			Bool STORM_FN matchesEmpty() const;

			// Match the string 'str' starting from 'start' (if present). Returns an iterator if a
			// match was possible, or null if the regex did not match. Note the difference between
			// null (no match) and the start iterator (a match of the empty string).
			Maybe<Str::Iter> STORM_FN match(Str *str);
			Maybe<Str::Iter> STORM_FN match(Str *str, Str::Iter start);

			// Match the entire string 'str' starting from 'start'. Does not return an iterator as
			// that iterator would always be the end iterator or nothing.
			Bool STORM_FN matchAll(Str *str);
			Bool STORM_FN matchAll(Str *str, Str::Iter start);

			// Match the buffer 'b' starting from 'start' (if present). Returns an integer
			// indicating if a match was possible. When matching a binary, we treat each byte in the
			// binary as a codepoint and match it against the pattern. This way it is possible to
			// use "\x??" to match arbitrary bytes.
			Maybe<Nat> STORM_FN match(Buffer b);
			Maybe<Nat> STORM_FN match(Buffer b, Nat start);

			// Match an entire buffer.
			Bool STORM_FN matchAll(Buffer b);
			Bool STORM_FN matchAll(Buffer b, Nat start);

			// Is this a 'simple' regex? Ie. a regex that is basically a string literal?
			Bool STORM_FN simple() const;

			// Get the contents of the simple literal.
			MAYBE(Str *) STORM_FN simpleStr() const;

			/**
			 * C++ api used by the parser.
			 */

			static const nat NO_MATCH;

			// Corresponding to above, but with indices. Returns NO_MATCH if no match is found.
			Nat matchRaw(Str *str) const;
			Nat matchRaw(Str *str, Nat start) const;

			// Same regex as another one?
			Bool STORM_FN operator ==(Regex o) const;
			Bool STORM_FN operator !=(Regex o) const;

			// Hash this regex.
			Nat STORM_FN hash() const;

			// Output.
			void STORM_FN toS(StrBuf *to) const;

		private:
			// Character set.
			class Set {
				STORM_VALUE;
			public:
				// Create.
				Set(GcArray<wchar> *chars, Int first, Bool inverted);

				// Value of 'first' when it is empty (large enough that it does not fit in a 16-bit
				// codepoint).
				static Int EMPTY;

				// Create an empty set.
				static Set empty();

				// Create a set containing a single character.
				static Set single(wchar ch);

				// Create a set matching any character.
				static Set all();

				// Parse a regex.
				static Set parse(Engine &e, const wchar *str, nat len, nat &pos);

				// Parse a group.
				static Set parseGroup(Engine &e, const wchar *str, nat len, nat &pos);

				// Characters in this set.
				GcArray<wchar> *chars;

				// Single character to match (if any).
				Int first;

				// Inverted set?
				Bool inverted;

				// Does this set contain a specific character?
				Bool contains(wchar ch) const;

				// Number of chars in here.
				nat count() const;

				// Output.
				void output(StrBuf *to) const;

				// Compare.
				Bool operator ==(const Set &o) const;
				Bool operator !=(const Set &o) const;

				// Hash.
				Nat hash() const;
			};

			// A state in the NFA.
			class State {
				STORM_VALUE;
			public:
				// Matching characters.
				Set match;

				// Allow skipping this state?
				// Used for * and ?
				Bool skippable;

				// Allow repeating this state?
				// Used for the * and + repetitions.
				Bool repeatable;

				// Parse the next state from a regex.
				static State parse(Engine &e, const wchar *str, nat len, nat &pos);

				// Output.
				void output(StrBuf *to) const;

				// Simple?
				Bool simple() const;

				// Compare.
				Bool operator ==(const State &o) const;
				Bool operator !=(const State &o) const;

				// Hash.
				Nat hash() const;
			};

			// Flags.
			enum {
				// Nothing special, use the generic matcher.
				fComplex,

				// No repeating structures.
				fNoRepeat,

				// No sets, no repeating structures, basically a simple string.
				fSimple,
			};

			// All states.
			// We use 'filled' in here to store flags regarding the complexity of the regex.
			GcArray<State> *states;

			// Parse a string.
			void parse(Str *parse);

			// Generic matching. Works for byte and wchar. Note: we don't handle "char" correctly,
			// as that type might be signed.
			template <class T>
			Nat matchRaw(const T *str, Nat len, Nat start) const;

			// Match a simple string (i.e. no uncertanties).
			template <class T>
			Nat matchSimple(const T *str, Nat len, Nat start) const;

			// Match a string without repeats.
			template <class T>
			Nat matchNoRepeat(const T *str, Nat len, Nat start) const;

			// Match a complex string.
			template <class T>
			Nat matchComplex(const T *str, Nat len, Nat start) const;

			// Output friends.
			friend wostream &operator <<(wostream &to, Regex r);
		};

		// Output.
		wostream &operator <<(wostream &to, Regex r);

		/**
		 * Regex exception.
		 */
		class EXCEPTION_EXPORT RegexError : public Exception {
			STORM_EXCEPTION;
		public:
			RegexError(const wchar *regex, const wchar *message);
			STORM_CTOR RegexError(Str *regex, Str *message);
			virtual void STORM_FN message(StrBuf *to) const;
		private:
			Str *regex;
			Str *msg;
		};

	}
}
