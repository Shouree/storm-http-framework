#pragma once
#include "Utils/Platform.h"
#include "Utils/Templates.h"

namespace storm {
	class Place;
}

namespace os {

	/**
	 * Implementation of the FnCall logic in a generic fashion. Reuqires variadic templates. Included from 'FnCall.h'.
	 */

	namespace impl {


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


#ifdef USE_VA_TEMPLATE

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

#else

		/**
		 * Implementation that does not rely on variadic arguments. Limits number of parameters.
		 *
		 * Made to be used on the VS2008 compiler in 64-bit mode. Here, it is OK to return a void
		 * expression, so we don't have to special case the return type at least.
		 */

		// To make vtable calls work.
		class DummyClass : NoCopy {};

		template <class P1, class P2, class P3, class P4, class P5, class P6>
		struct Params6 {
			typedef P1 First;

			template <class Result>
			static Result call(const void *fn, void *first, void **rest) {
				typedef Result CODECALL(*Fn)(P1, P2, P3, P4, P5, P6);
				Fn p = (Fn)fn;
				return (*p)(*(P1 *)first, *(P2 *)rest[0], *(P3 *)rest[1], *(P4 *)rest[2], *(P5 *)rest[3], *(P6 *)rest[4]);
			}

			template <class Result>
			static Result callMember(const void *fn, void *first, void **rest) {
				typedef Result CODECALL (DummyClass::*Fn)(P2, P3, P4, P5, P6) const;
				DummyClass *f = *(DummyClass **)first;
				Fn p = null;
				toMember(p, fn);
				return (f->*p)(*(P2 *)rest[0], *(P3 *)rest[1], *(P4 *)rest[2], *(P5 *)rest[3], *(P6 *)rest[4]);
			}

			// template <class T>
			// struct Append {
			// 	typedef Params7<P1, P2, P3, P4, P5, P6, T> T;
			// };
			// typedef Params7<void *, P1, P2, P3, P4, P5, P6>;
		};

		template <class P1, class P2, class P3, class P4, class P5>
		struct Params5 {
			typedef P1 First;

			template <class Result>
			static Result call(const void *fn, void *first, void **rest) {
				typedef Result CODECALL(*Fn)(P1, P2, P3, P4, P5);
				Fn p = (Fn)fn;
				return (*p)(*(P1 *)first, *(P2 *)rest[0], *(P3 *)rest[1], *(P4 *)rest[2], *(P5 *)rest[3]);
			}

			template <class Result>
			static Result callMember(const void *fn, void *first, void **rest) {
				typedef Result CODECALL (DummyClass::*Fn)(P2, P3, P4, P5) const;
				DummyClass *f = *(DummyClass **)first;
				Fn p = null;
				toMember(p, fn);
				return (f->*p)(*(P2 *)rest[0], *(P3 *)rest[1], *(P4 *)rest[2], *(P5 *)rest[3]);
			}

			template <class T>
			struct Append {
				typedef Params6<P1, P2, P3, P4, P5, T> T;
			};
			typedef Params6<void *, P1, P2, P3, P4, P5> WithThis;
		};

		template <class P1, class P2, class P3, class P4>
		struct Params4 {
			typedef P1 First;

			template <class Result>
			static Result call(const void *fn, void *first, void **rest) {
				typedef Result CODECALL(*Fn)(P1, P2, P3, P4);
				Fn p = (Fn)fn;
				return (*p)(*(P1 *)first, *(P2 *)rest[0], *(P3 *)rest[1], *(P4 *)rest[2]);
			}

			template <class Result>
			static Result callMember(const void *fn, void *first, void **rest) {
				typedef Result CODECALL (DummyClass::*Fn)(P2, P3, P4) const;
				DummyClass *f = *(DummyClass **)first;
				Fn p = null;
				toMember(p, fn);
				return (f->*p)(*(P2 *)rest[0], *(P3 *)rest[1], *(P4 *)rest[2]);
			}

			template <class T>
			struct Append {
				typedef Params5<P1, P2, P3, P4, T> T;
			};
			typedef Params5<void *, P1, P2, P3, P4> WithThis;
		};

		template <class P1, class P2, class P3>
		struct Params3 {
			typedef P1 First;

			template <class Result>
			static Result call(const void *fn, void *first, void **rest) {
				typedef Result CODECALL(*Fn)(P1, P2, P3);
				Fn p = (Fn)fn;
				return (*p)(*(P1 *)first, *(P2 *)rest[0], *(P3 *)rest[1]);
			}

			template <class Result>
			static Result callMember(const void *fn, void *first, void **rest) {
				typedef Result CODECALL (DummyClass::*Fn)(P2, P3) const;
				DummyClass *f = *(DummyClass **)first;
				Fn p = null;
				toMember(p, fn);
				return (f->*p)(*(P2 *)rest[0], *(P3 *)rest[1]);
			}

			template <class T>
			struct Append {
				typedef Params4<P1, P2, P3, T> T;
			};
			typedef Params4<void *, P1, P2, P3> WithThis;
		};

		template <class P1, class P2>
		struct Params2 {
			typedef P1 First;

			template <class Result>
			static Result call(const void *fn, void *first, void **rest) {
				typedef Result CODECALL(*Fn)(P1, P2);
				Fn p = (Fn)fn;
				return (*p)(*(P1 *)first, *(P2 *)rest[0]);
			}

			template <class Result>
			static Result callMember(const void *fn, void *first, void **rest) {
				typedef Result CODECALL (DummyClass::*Fn)(P2) const;
				DummyClass *f = *(DummyClass **)first;
				Fn p = null;
				toMember(p, fn);
				return (f->*p)(*(P2 *)rest[0]);
			}

			template <class T>
			struct Append {
				typedef Params3<P1, P2, T> T;
			};
			typedef Params3<void *, P1, P2> WithThis;
		};

		template <class P1>
		struct Params1 {
			typedef P1 First;

			template <class Result>
			static Result call(const void *fn, void *first, void **rest) {
				typedef Result CODECALL (*Fn)(P1);
				Fn p = (Fn)fn;
				return (*p)(*(P1 *)first);
			}

			template <class Result>
			static Result callMember(const void *fn, void *first, void **rest) {
				typedef Result CODECALL (DummyClass::*Fn)() const;
				DummyClass *f = *(DummyClass **)first;
				Fn p = null;
				toMember(p, fn);
				return (f->*p)();
			}

			template <class T>
			struct Append {
				typedef Params2<P1, T> T;
			};
			typedef Params2<void *, P1> WithThis;
		};

		struct Params0 {
			typedef void First;

			template <class Result>
			static Result call(const void *fn, void *first, void **) {
				typedef Result CODECALL (*Fn)();
				Fn p = (Fn)fn;
				return (*p)();
			}

			template <class T>
			struct Append {
				typedef Params1<T> T;
			};
			typedef Params1<void *> WithThis;
		};


		template <class ParPack>
		struct ExpandPack {
			typedef typename ExpandPack<typename ParPack::PrevType>::T::Append<typename ParPack::HereType>::T T;
		};

		template <>
		struct ExpandPack<Param<void, void>> {
			typedef Params0 T;
		};

		template <class Result, class First, class Expanded>
		struct CallHelpI {
			static Result call(const void *fn, bool member, void *first, void **params) {
				return Expanded::call<Result>(fn, first, params);
			}
		};

		template <class Result, class First, class Expanded>
		struct CallHelpI<Result, First *, Expanded> {
			static Result call(const void *fn, bool member, void *first, void **params) {
				if (member) {
					return Expanded::callMember<Result>(fn, first, params);
				}

				return Expanded::call<Result>(fn, first, params);
			}
		};

		template <class Result, class Expanded>
		struct CallHelp {
			static void call(const void *fn, bool member, void *first, void **params, void *result) {
				new (storm::Place(result)) Result(
					CallHelpI<Result, typename Expanded::First, Expanded>::call(fn, member, first, params));
			}
		};

		template <class Expanded>
		struct CallHelp<void, Expanded> {
			static void call(const void *fn, bool member, void *first, void **params, void *result) {
				CallHelpI<void, typename Expanded::First, Expanded>::call(fn, member, first, params);
			}
		};

		template <class Result, class Par>
		void call(const void *fn, bool member, void **params, void *first, void *result) {
			member = memberCall(member, fn);

			typedef typename ExpandPack<Par>::T Expanded;

			void *firstParam;
			if (first) {
				firstParam = &first;

				CallHelp<Result, typename Expanded::WithThis>::call(fn, member, firstParam, params, result);
			} else {
				firstParam = params[0];
				params = params + 1;

				CallHelp<Result, Expanded>::call(fn, member, firstParam, params, result);
			}
		}

#endif

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

#elif defined(VISUAL_STUDIO) && defined(WINDOWS) && defined(X64)

		template <class R>
		void toMember(R &to, const void *from) {
			to = asMemberPtr<R>(from);
		}

		inline bool memberCall(bool member, const void *fn) {
			return member;
		}

#else
#error "Please provide a function pointer specifics for your platform!"
#endif

	}
}
