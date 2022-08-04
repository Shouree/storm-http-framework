#pragma once

#include "Utils/Platform.h"
#include "Core/GcType.h"
#include "OS/UThread.h"

#if !defined(X86) && !defined(X64) && !defined(ARM64)
#error "Stack scanning for machines other than X86, X86-64 and ARM64 is not implemented yet."
#endif

namespace storm {

	/**
	 * Options to control scanning.
	 */
	enum ScanOption {
		// Scan nothing.
		scanNone,

		// Scan only the header (if one exists).
		scanHeader,

		// Scan all pointers.
		scanAll,
	};


	/**
	 * Generic implementation of stack- and object scanning.
	 *
	 * The stack scanning can be used regardless of what object format is used, but the
	 * heap-scanning functions assume that the object format implemented in "Format.h" is used.
	 *
	 * How objects are scanned are described by the template parameter 'Scanner'. See the
	 * implementation of 'NullScanner' below to see what is required by a scanner implementation.
	 *
	 * TODO: We could skip return values and use exceptions exclusively, and use exceptions instead,
	 * as suggested by MPS.
	 */
	template <class Scanner>
	struct Scan {
	private:
		typedef typename Scanner::Result Result;
		typedef typename Scanner::Source Source;

		// Helper functions. The public interface is below.
		static inline Result fix12(Scanner &s, void **ptr) {
			if (s.fix1(*ptr))
				return s.fix2(ptr);
			return Result();
		}

	public:
		// Shorthand for stacks.
		typedef os::InlineSet<os::Stack> StackSet;

		// Scan an array of pointers.
		static Result array(Source &source, void *base, size_t count) {
			void **from = (void **)base;
			Scanner s(source);
			for (size_t i = 0; i < count; i++) {
				Result r = fix12(s, from + i);
				if (r != Result())
					return r;
			}

			return Result();
		}

		// Scan an array of pointers. Also supports scanning a part of a stack where the full extent
		// of the stack is known.
		static Result array(Source &source, void *base, void *limit) {
			void **from = (void **)base;
			void **to = (void **)limit;

			Scanner s(source);
			for (; from != to; from++) {
				Result r = fix12(s, from);
				if (r != Result())
					return r;
			}

			return Result();
		}

		// Scan all UThreads running on a specific thread. 'stacks' is the set of all stacks that
		// are tied to this particular thread. Out of those threads, one may be scheduled
		// currently. The extent of this thread is specified by 'current' (the lower end, on X86).
		// Additionally, returns the number of bytes scanned, which may be interesting for some GC
		// implementations.
		static Result stacks(Source &source, const StackSet &stacks, void *current, size_t *scanned) {
			size_t bytesScanned = 0;

			// We scan all UThreads on this thread, if one of them is the currently running thread
			// their 'desc' is null. In that case we delay scanning that thread until after the
			// other threads. If we do not find an 'active' thread, we're in the middle of a thread
			// switch, which means that we can scan all threads as if they were sleeping.

			// Aside from that, we need to be aware that UThreads may be executed by another thread
			// during detours. The UThreads will always be located inside the stacks set on the
			// thread where they are intended to run. They can not be moved around while they
			// contain anything useful since it is not possible to move the UThreads between sets
			// atomically. Instead, the Stack objects participating in a thread switch are
			// marked with a 'detourActive' != 0. Any such UThread shall be ignored during stack
			// scanning since they are considered to belong to another thread. The UThread is
			// instead associated with the proper thread by following the pointer inside the
			// 'detourTo' member.

			// Note: This code currently contains bits that are specific to X86 CPUs, e.g. that the
			// stack grows towards lower addresses.

			// Remember we did not find a running stack.
			void **to = null;

			Scanner s(source);
			StackSet::iterator end = stacks.end();
			for (StackSet::iterator i = stacks.begin(); i != end; ++i) {
				// If this thread is used as a detour thread, do not scan it at all.
				const os::Stack *first = *i;
				if (first->detourActive)
					continue;

				// Examine the main stack and all detours for this thread.
				for (const os::Stack *stack = first; stack; stack = stack->detourTo) {
					// Is this the currently scheduled UThread for this thread?
					if (!stack->desc) {
						// We should not find two of these for any given thread.
						dbg_assert(to == null, L"We found two potential main stacks.");

						// This is the main stack. Scan it later!
						to = (void **)stack->limit();

						// Do a quick sanity check. If the stack is allocated, then we don't want to
						// scan it if "current" is outside the range. This can happen when an
						// exception is thrown though a "stackCall" mechanism.
						if (stack->low() != stack->high()) {
							// If it is outside, then we don't scan it now. We're in the process of
							// destroying the stack.
							if (size_t(current) <= size_t(stack->low()) ||
								size_t(current) > size_t(stack->high()))
								to = null;
						}
						continue;
					}

					// All is well. Commence scanning!
					void **low = (void **)stack->desc->low;
					void **high = (void **)stack->limit();
					bytesScanned += (char *)high - (char *)low;
					for (; low < high; low++) {
						Result r = fix12(s, low);
						if (r != Result())
							return r;
					}
				}
			}

			// If we are right in the middle of a thread switch, we will fail to find a main
			// stack. This means we have already scanned all stacks, and thus we do not need to do
			// anything more.
			if (to == null) {
#ifdef DEBUG
				// To see if this ever happens, and if it is handled correctly. This is *really*
				// rare, as we have to hit a window of ~6 machine instructions when pausing another
				// thread to see this.
				PLN(L"------------ We found an UThread in the process of switching! ------------");
#endif
			} else {
				bytesScanned += (char *)to - (char *)current;
				for (void **at = (void **)current; at < to; at++) {
					Result r = fix12(s, at);
					if (r != Result())
						return r;
				}
			}

			if (scanned)
				*scanned = bytesScanned;

			return Result();
		}

	};


	/**
	 * Simple implementation of a scanner for illustration purposes. Contains all members required
	 * by a full scanner.
	 *
	 * This class is instantiated on the stack inside the scanning function, in order to provide a
	 * place where implementations may place variables that benefit from being inlined by the
	 * compiler, and hopefully placed in registers.
	 */
	struct NullScanner {
		// Describes the type of results returned from the 'fix' functions, and subsequently also by
		// the scanning functions. The value Result() is considered 'success'.
		typedef int Result;

		// Where we get the scanning information from initially. This class/struct need to be
		// constructible from an instance of this type.
		typedef void *Source;

		// Constructor from 'Source'.
		NullScanner(Source &source) {}

		// Called before each object is scanned when scanning formatted objects (not called when
		// scanning a bunch of raw pointers, such as when scanning the stack). Determines if a
		// particular object should be scanned, and to what degree it should be scanned (eg. not at
		// all, only the header, or everything).
		inline ScanOption object(void *start, void *end) { return scanAll; }

		// Called once for each reference. Assumed to be a quick "early-out" check that the GC can
		// use to quickly discard references that are not interesting at the moment. We also assume
		// that this check can be called for internal pointers to objects without harm. Returns
		// 'true' if 'fix2' need to be called.
		inline bool fix1(void *ptr) { return false; }

		// Called whenever a reference passes the check in 'fix1'. If required by the GC, only base
		// pointers to objects may be passed to this function. Note that the pointer may be altered
		// by this function in case the object moved.
		inline Result fix2(void **ptr) { return Result(); }

		// Called by the object scanning in "Format.h" for each object header, in case those need
		// special treatment. Works like 'fix1' and 'fix2', except that both steps require proper
		// base pointers, as indicated by the type.
		inline bool fixHeader1(GcType *header) { return false; }
		inline Result fixHeader2(GcType **header) { return Result(); }
	};

}
