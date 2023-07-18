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

		template <class P1, class P2, class P3, class P4, class P5, class P6>
		struct Params6 {
			template <class Result>
			static Result call(const void *fn, void *first, void **rest) {
				typedef Result CODECALL(*Fn)(P1, P2, P3, P4, P5, P6);
				Fn p = (Fn)fn;
				return (*p)(*(P1 *)first, *(P2 *)rest[0], *(P3 *)rest[1], *(P4 *)rest[2], *(P5 *)rest[3], *(P6 *)rest[4]);
			}

			template <class Result, class Type>
			static Result callMember(const void *fn, Type *first, void **rest) {
				typedef Result CODECALL (Type::*Fn)(P1, P2, P3, P4, P5) const;
				Fn p = null;
				toMember(p, fn);
				return (first->*p)(*(P1 *)rest[0], *(P2 *)rest[1], *(P3 *)rest[2], *(P4 *)rest[3], *(P5 *)rest[4], *(P6 *)rest[5]);
			}

			template <class T>
			struct Prepend {
				typedef Params6<T, P1, P2, P3, P4, P5> T;
			};
		};

		template <class P1, class P2, class P3, class P4, class P5>
		struct Params5 {
			template <class Result>
			static Result call(const void *fn, void *first, void **rest) {
				typedef Result CODECALL(*Fn)(P1, P2, P3, P4, P5);
				Fn p = (Fn)fn;
				return (*p)(*(P1 *)first, *(P2 *)rest[0], *(P3 *)rest[1], *(P4 *)rest[2], *(P5 *)rest[3]);
			}

			template <class Result, class Type>
			static Result callMember(const void *fn, Type *first, void **rest) {
				typedef Result CODECALL (Type::*Fn)(P1, P2, P3, P4, P5) const;
				Fn p = null;
				toMember(p, fn);
				return (first->*p)(*(P1 *)rest[0], *(P2 *)rest[1], *(P3 *)rest[2], *(P4 *)rest[3], *(P5 *)rest[4]);
			}

			template <class T>
			struct Prepend {
				typedef Params6<T, P1, P2, P3, P4, P5> T;
			};
		};

		template <class P1, class P2, class P3, class P4>
		struct Params4 {
			template <class Result>
			static Result call(const void *fn, void *first, void **rest) {
				typedef Result CODECALL(*Fn)(P1, P2, P3, P4);
				Fn p = (Fn)fn;
				return (*p)(*(P1 *)first, *(P2 *)rest[0], *(P3 *)rest[1], *(P4 *)rest[2]);
			}

			template <class Result, class Type>
			static Result callMember(const void *fn, Type *first, void **rest) {
				typedef Result CODECALL (Type::*Fn)(P1, P2, P3, P4) const;
				Fn p = null;
				toMember(p, fn);
				return (first->*p)(*(P1 *)rest[0], *(P2 *)rest[1], *(P3 *)rest[2], *(P4 *)rest[3]);
			}

			template <class T>
			struct Prepend {
				typedef Params5<T, P1, P2, P3, P4> T;
			};
		};

		template <class P1, class P2, class P3>
		struct Params3 {
			template <class Result>
			static Result call(const void *fn, void *first, void **rest) {
				typedef Result CODECALL(*Fn)(P1, P2, P3);
				Fn p = (Fn)fn;
				return (*p)(*(P1 *)first, *(P2 *)rest[0], *(P3 *)rest[1]);
			}

			template <class Result, class Type>
			static Result callMember(const void *fn, Type *first, void **rest) {
				typedef Result CODECALL (Type::*Fn)(P1, P2, P3) const;
				Fn p = null;
				toMember(p, fn);
				return (first->*p)(*(P1 *)rest[0], *(P2 *)rest[1], *(P3 *)rest[2]);
			}

			template <class T>
			struct Prepend {
				typedef Params4<T, P1, P2, P3> T;
			};
		};

		template <class P1, class P2>
		struct Params2 {
			template <class Result>
			static Result call(const void *fn, void *first, void **rest) {
				typedef Result CODECALL(*Fn)(P1, P2);
				Fn p = (Fn)fn;
				return (*p)(*(P1 *)first, *(P2 *)rest[0]);
			}

			template <class Result, class Type>
			static Result callMember(const void *fn, Type *first, void **rest) {
				typedef Result CODECALL (Type::*Fn)(P1, P2) const;
				Fn p = null;
				toMember(p, fn);
				return (first->*p)(*(P1 *)rest[0], *(P2 *)rest[1]);
			}

			template <class T>
			struct Prepend {
				typedef Params3<T, P1, P2> T;
			};
		};

		template <class P1>
		struct Params1 {
			template <class Result>
			static Result call(const void *fn, void *first, void **rest) {
				typedef Result CODECALL (*Fn)(P1);
				Fn p = (Fn)fn;
				return (*p)(*(P1 *)first);
			}

			template <class Result, class Type>
			static Result callMember(const void *fn, Type *first, void **rest) {
				typedef Result CODECALL (Type::*Fn)(P1) const;
				Fn p = null;
				toMember(p, fn);
				return (first->*p)(*(P1 *)rest[0]);
			}

			template <class T>
			struct Prepend {
				typedef Params2<T, P1> T;
			};
		};

		struct Params0 {
			template <class Result>
			static Result call(const void *fn, void *first, void **) {
				typedef Result CODECALL (*Fn)();
				Fn p = (Fn)fn;
				return (*p)();
			}

			template <class Result, class Type>
			static Result callMember(const void *fn, Type *first, void **) {
				typedef Result CODECALL (Type::*Fn)() const;
				Fn p = null;
				toMember(p, fn);
				return (first->*p)();
			}

			template <class T>
			struct Prepend {
				typedef Params1<T> T;
			};
		};


		template <class ParPack>
		struct ExpandPack {
			typedef typename ExpandPack<typename ParPack::PrevType>::T::Prepend<typename ParPack::HereType>::T T;
		};

		template <>
		struct ExpandPack<Param<void, void>> {
			typedef Params0 T;
		};

		template <class Result, class First, class All>
		struct CallHelpI {
			static Result call(const void *fn, bool member, void *first, void **params) {
				return ExpandPack<All>::T::call<Result>(fn, first, params);
			}
		};

		template <class Result, class First, class All>
		struct CallHelpI<Result, First *, All> {
			static Result call(const void *fn, bool member, void *first, void **params) {
				if (member) {
					return ExpandPack<typename All::PrevType>::T
						::callMember<Result, First>(fn, *(First **)first, params);
				}

				return ExpandPack<All>::T::call<Result>(fn, first, params);
			}
		};

		template <class Result, class Par>
		struct CallHelp {
			static void call(const void *fn, bool member, void *first, void **params, void *result) {
				new (storm::Place(result)) Result(
					CallHelpI<Result, typename Par::HereType, Par>::call(fn, member, first, params));
			}
		};

		template <class Result>
		struct CallHelp<Result, Param<void, void>> {
			static void call(const void *fn, bool member, void *first, void **params, void *result) {
				new (storm::Place(result)) Result(
					CallHelpI<Result, void, Param<void, void>>::call(fn, member, first, params));
			}
		};

		template <class Par>
		struct CallHelp<void, Par> {
			static void call(const void *fn, bool member, void *first, void **params, void *result) {
				CallHelpI<void, typename Par::HereType, Par>::call(fn, member, first, params);
			}
		};

		template <>
		struct CallHelp<void, Param<void, void>> {
			static void call(const void *fn, bool member, void *first, void **params, void *result) {
				CallHelpI<void, void, Param<void, void>>::call(fn, member, first, params);
			}
		};

		template <class Result, class Par>
		void call(const void *fn, bool member, void **params, void *first, void *result) {
			member = memberCall(member, fn);

			void *firstParam;
			if (first) {
				firstParam = &first;
			} else {
				firstParam = params[0];
				params = params + 1;
			}

			CallHelp<Result, Par>::call(fn, member, firstParam, params, result);
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
