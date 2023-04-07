#pragma once
#include "Array.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Represents a stack trace from some point in the execution.
	 *
	 * Internally, the trace is stored as a list of pointers to locations inside the functions that
	 * were executed when the trace was captured. This class contains facilities to resolve the
	 * pointers into symbolic names to make the trace more easily readable. This is not done
	 * automatically, however, since many stack traces captured for exceptions do not need to be
	 * resolved if the exception is caught and never shown to the user.
	 */
	class StackTrace {
		STORM_VALUE;
	public:
		// Create an empty stack trace.
		StackTrace(Engine &e);

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *env);

		/**
		 * A single stack frame.
		 *
		 * This is basically a pointer, but due to GC constraints, we need to keep a pointer to the
		 * start of each function and an offset rather than a pointer into the object.
		 */
		class Frame {
			STORM_VALUE;
		public:
			Frame(void *base, Nat offset, Nat id);

			// Pointer to the start of the function.
			UNKNOWN(PTR_GC) void *base;

			// Offset into the function (bytes).
			Nat offset;

			// ID of the backend producing this frame.
			Nat id;

			// Compute the pointer.
			inline void *ptr() const {
				return (byte *)base + offset;
			}
		};

		// Add an element to the trace (to the bottom).
		inline void push(Frame frame) { frames->push(frame); }

		// Reserve space.
		inline void reserve(Nat size) { frames->reserve(size); }

		// Get elements.
		inline Nat STORM_FN count() const { return frames->count(); }
		inline Frame STORM_FN operator[] (Nat i) const { return frames->at(i); }

		// Empty?
		inline Bool STORM_FN empty() const { return frames->empty(); }
		inline Bool STORM_FN any() const { return frames->any(); }

		// Format into a string.
		Str *STORM_FN format() const;
		void STORM_FN format(StrBuf *to) const;

	private:
		// Data.
		Array<Frame> *frames;
	};

	// Output the stack trace.
	wostream &operator <<(wostream &to, const StackTrace &trace);
	StrBuf &STORM_FN operator <<(StrBuf &to, StackTrace trace);

	// Collect a stack trace.
	StackTrace STORM_FN collectStackTrace(EnginePtr e);

	// Collect a stack trace, passing a pointer to previously collected stack state.
	StackTrace collectStackTrace(EnginePtr e, void *state);

}
