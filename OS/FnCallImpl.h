#pragma once
#include "Utils/Platform.h"

namespace storm {
	class Place;
}

namespace os {

	/**
	 * Implementation of the FnCall logic in a generic fashion. Reuqires variadic templates. Included from 'FnCall.h'.
	 */

	namespace impl {


#ifndef USE_VA_TEMPLATE
#error "The generic FnCall implementation requires varidic templates."
#endif

		// Helper macro, enabling the function (with void return value) if 'U' is a pointer.
#define IF_POINTER(U) typename std::enable_if<std::is_pointer<U>::value>::type
#define IF_NOT_POINTER(U) typename std::enable_if<!std::is_pointer<U>::value>::type

		/**
		 * Forward declarations of platform specific alterations to the generic behavior.
		 */

		// Copy a 'void *' to a member function pointer of the appropriate type.
		template <class R>
		void toMember(R &to, const void *from);

		// Decide if we shall perform a member or nonmember call depending on what the function pointer looks like.
		inline bool memberCall(bool member, const void *fn);


		/**
		 * Generic implementation:
		 */


		/**
		 * Recurse through the parameter list until we reach the end.
		 */
		template <class Result, class Par, bool member>
		struct ParamHelp {
			template <class ... T>
			static void call(const void *fn, void **params, void *out, nat pos, T& ... args) {
				pos--;
				ParamHelp<Result, typename Par::PrevType, member>
					::call(fn, params, out, pos, *(typename Par::HereType *)params[pos], args...);
			}

			template <class ...T>
			static void callFirst(const void *fn, void **params, void *out, void *first, nat pos, T& ... args) {
				pos--;
				ParamHelp<Result, typename Par::PrevType, member>
					::callFirst(fn, params, out, first, pos, *(typename Par::HereType *)params[pos], args...);
			}
		};


		/**
		 * At the end. Perform a nonmember call with a result.
		 */
		template <class Result>
		struct ParamHelp<Result, Param<void, void>, false> {
			template <class ... T>
			static void call(const void *fn, void **, void *out, nat pos, T& ... args) {
				typedef Result CODECALL (*Fn)(T...);
				Fn p = (Fn)fn;
				new (storm::Place(out)) Result((*p)(args...));
			}

			template <class ...T>
			static void callFirst(const void *fn, void **params, void *out, void *first, nat pos, T& ... args) {
				call(fn, params, out, pos, first, args...);
			}
		};

		/**
		 * At the end. Perform a member call with a result.
		 */
		template <class Result>
		struct ParamHelp<Result, Param<void, void>, true> {
			class Dummy : NoCopy {};

			template <class U, class ... T>
			static IF_POINTER(U) call(const void *fn, void **, void *out, nat pos, U first, T& ... args) {
				typedef Result CODECALL (Dummy::*Fn)(T...);
				Fn p = null;
				toMember(p, fn);
				Dummy *f = (Dummy *)first;
				new (storm::Place(out)) Result((f->*p)(args...));
			}

			template <class U, class ... T>
			static IF_NOT_POINTER(U) call(const void *fn, void **, void *out, nat pos, U first, T& ... args) {
				typedef Result CODECALL (*Fn)(U, T...);
				Fn p = (Fn)fn;
				new (storm::Place(out)) Result((*p)(first, args...));
			}

			static void call(const void *fn, void **, void *out, nat pos) {
				typedef Result CODECALL (*Fn)();
				Fn p = (Fn)fn;
				new (storm::Place(out)) Result((*p)());
			}

			template <class ...T>
			static void callFirst(const void *fn, void **params, void *out, void *first, nat pos, T& ... args) {
				call(fn, params, out, pos, first, args...);
			}
		};


		/**
		 * At the end. Perform a nonmember call without a result.
		 */
		template <>
		struct ParamHelp<void, Param<void, void>, false> {
			template <class ... T>
			static void call(const void *fn, void **, void *out, nat pos, T& ... args) {
				typedef void CODECALL (*Fn)(T...);
				Fn p = (Fn)fn;
				(*p)(args...);
			}


			template <class ...T>
			static void callFirst(const void *fn, void **params, void *out, void *first, nat pos, T& ... args) {
				call(fn, params, out, pos, first, args...);
			}
		};


		/**
		 * At the end. Perform a member call without a result.
		 */
		template <>
		struct ParamHelp<void, Param<void, void>, true> {
			class Dummy : NoCopy {};

			template <class U, class ... T>
			static IF_POINTER(U) call(const void *fn, void **, void *out, nat pos, U first, T& ... args) {
				typedef void CODECALL (Dummy::*Fn)(T...);
				Fn p = null;
				toMember(p, fn);
				Dummy *f = (Dummy *)first;
				(f->*p)(args...);
			}

			template <class U, class ... T>
			static IF_NOT_POINTER(U) call(const void *fn, void **, void *out, nat pos, U first, T& ... args) {
				typedef void CODECALL (*Fn)(U, T...);
				Fn p = (Fn)fn;
				(*p)(first, args...);
			}

			static void call(const void *fn, void **, void *out, nat pos) {
				typedef void CODECALL (*Fn)();
				Fn p = (Fn)fn;
				(*p)();
			}

			template <class ...T>
			static void callFirst(const void *fn, void **params, void *out, void *first, nat pos, T& ... args) {
				call(fn, params, out, pos, first, args...);
			}
		};

		template <class Result, class Par>
		void call(const void *fn, bool member, void **params, void *first, void *result) {
			if (memberCall(member, fn)) {
				if (first) {
					ParamHelp<Result, Par, true>::callFirst(fn, params, result, first, Par::count);
				} else {
					ParamHelp<Result, Par, true>::call(fn, params, result, Par::count);
				}
			} else {
				if (first) {
					ParamHelp<Result, Par, false>::callFirst(fn, params, result, first, Par::count);
				} else {
					ParamHelp<Result, Par, false>::call(fn, params, result, Par::count);
				}
			}
		}

		/**
		 * Platform specific implementations:
		 */

#if defined(GCC) && defined(POSIX) && defined(X64)

		template <class R>
		void toMember(R &to, const void *from) {
			to = asMemberPtr<R>(from);
		}

		inline bool memberCall(bool member, const void *fn) {
			// Do not attempt to perform a member call if the function pointer is not a vtable
			// reference. In some cases, especially when using optimizations, GCC makes full use of
			// its assumptions that 'this' pointers are not null and may thus crash 'too early' in
			// some cases.
			return member && (size_t(fn) & 0x1);
		}

#elif defined(GCC) && defined(POSIX) && defined(ARM64)

		template <class R>
		void toMember(R &to, const void *from) {
			to = asMemberPtr<R>(from);
		}

		inline bool memberCall(bool member, const void *fn) {
			// Do not attempt to perform a member call if the function pointer is not a vtable
			// reference. In some cases, especially when using optimizations, GCC makes full use of
			// its assumptions that 'this' pointers are not null and may thus crash 'too early' in
			// some cases.
			return member && (size_t(fn) & 0x1);
		}

#else
#error "Please provide a function pointer specifics for your platform!"
#endif

	}
}
