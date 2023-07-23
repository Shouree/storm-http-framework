#include "stdafx.h"
#include "PosixParams.h"
#include "Asm.h"

namespace code {
	namespace x64 {

		static Size roundSize(Size sz) {
			if (sz.size64() == 1)
				return sz;
			else if (sz.size64() <= 4)
				return Size::sInt;
			else
				return sz.alignedAs(Size::sWord);
		}

		static primitive::Kind merge(primitive::Kind a, primitive::Kind b) {
			switch (b) {
			case primitive::none:
				return a;
			case primitive::pointer:
				b = primitive::integer;
				break;
			}

			if (a == primitive::none)
				return b;
			if (b == primitive::none)
				return a;
			if (b == primitive::pointer)
				b = primitive::integer;

			switch (a) {
			case primitive::none:
				return b;
			case primitive::pointer:
			case primitive::integer:
				// Regardless of what 'b' is, we should remain in an integer register.
				return primitive::integer;
			case primitive::real:
				// If 'b' is an integer, we shall become an integer as well.
				return b;
			}

			dbg_assert(false, L"Should not be reached.");
			return a;
		}

		static primitive::Kind paramKind(GcArray<Primitive> *layout, Nat from, Nat to) {
			primitive::Kind result = primitive::none;

			for (Nat i = 0; i < layout->count; i++) {
				Primitive p = layout->v[i];
				Nat offset = p.offset().v64();
				if (offset >= from && offset < to) {
					result = merge(result, p.kind());
				}
			}

			return result;
		}




		PosixParams::PosixParams() : code::Params(6, 8, 8, 16) {}

		Reg PosixParams::registerSrc(Nat id) const {
			static Reg v[] = {
				ptrDi, ptrSi, ptrD, ptrC, ptr8, ptr9,
				xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7
			};
			return v[id];
		}


		void PosixParams::resultPrimitive(Primitive p) {
			if (p.kind() == primitive::none)
				resultData = Result::empty();
			else if (p.kind() == primitive::real)
				resultData = Result::inRegister(engine(), asSize(xmm0, p.size()));
			else
				resultData = Result::inRegister(engine(), asSize(ptrA, p.size()));
		}

		void PosixParams::resultComplex(ComplexDesc *desc) {
			resultData = Result::inMemory(ptrDi);
			// Add "dummy" parameter:
			addInt(Param(Param::returnId(), desc->size(), true, 0, true));
		}

		void PosixParams::resultSimple(SimpleDesc *desc) {
			Nat size = desc->size().size64();

			if (size == 0) {
				resultData = Result::empty();
				return;
			}

			// Similar logic to adding parameters.
			if (size > 2*8) {
				// Pass it on the stack:
				resultData = Result::inMemory(ptrDi);
				// Add "dummy" parameter:
				addInt(Param(Param::returnId(), desc->size(), true, 0, true));
				return;
			}

			primitive::Kind part1 = paramKind(desc->v, 0, 8);
			primitive::Kind part2 = paramKind(desc->v, 8, 16);

			Size firstSize(min(size, Nat(8)));
			firstSize = roundSize(firstSize);
			Size secondSize((size > 8) ? (size - 8) : 0);
			secondSize = roundSize(secondSize);

			if (part2 == primitive::none)
				resultData = Result::inRegisters(engine(), 1);
			else
				resultData = Result::inRegisters(engine(), 2);

			Reg intReg = ptrA;
			Reg realReg = xmm0;

			if (part1 == primitive::integer) {
				resultData.putRegister(asSize(intReg, firstSize), 0);
				intReg = ptrD;
			} else if (part1 == primitive::real) {
				resultData.putRegister(asSize(realReg, firstSize), 0);
				realReg = xmm1;
			}

			if (part2 == primitive::integer) {
				resultData.putRegister(asSize(intReg, secondSize), 8);
			} else if (part2 = primitive::real) {
				resultData.putRegister(asSize(realReg, secondSize), 8);
			}
		}


		void PosixParams::addPrimitive(Nat id, Primitive p) {
			switch (p.kind()) {
			case primitive::none:
				break;
			case primitive::pointer:
			case primitive::integer:
				addInt(Param(id, p, true));
				break;
			case primitive::real:
				addReal(Param(id, p, true));
				break;
			}
		}

		void PosixParams::addComplex(Nat id, ComplexDesc *desc) {
			addInt(Param(id, desc->size(), true, 0, true));
		}

		void PosixParams::addSimple(Nat id, SimpleDesc *desc) {
			// Here, we should check 'type' to see if we shall pass parts of it in registers.
			// It seems the algorithm works roughly as follows (from the offical documentation
			// and examining the output of GCC from Experiments/call64.cpp):
			// - If the struct is larger than 2 64-bit words, pass it on the stack.
			// - If the struct does not fit entirely into registers, pass it on the stack.
			// - Examine each 64-bit word of the struct:
			//   - if the word contains only floating point numbers, pass them into a real register.
			//   - otherwise, pass the word in an integer register (eg. int + float).

			Nat size = desc->size().size64();
			if (size > 2*8) {
				// Too large: pass on the stack!
				addStack(Param(id, desc->size(), true, 0, false));
				return;
			}

			primitive::Kind first = paramKind(desc->v, 0, 8);
			primitive::Kind second = paramKind(desc->v, 8, 16);

			Size firstSize(min(size, Nat(8)));
			firstSize = roundSize(firstSize);
			Size secondSize((size > 8) ? (size - 8) : 0);
			secondSize = roundSize(secondSize);

			Nat intCount = (first == primitive::integer) + (second == primitive::integer);
			Nat realCount = (first == primitive::real) + (second == primitive::real);

			if (hasInt(intCount) && hasReal(realCount)) {
				// There is room, add parameters!
				if (first == primitive::integer)
					addInt(Param(id, Size(firstSize), true, 0, false));
				else if (first == primitive::real)
					addReal(Param(id, Size(firstSize), true, 0, false));

				if (second == primitive::integer)
					addInt(Param(id, Size(secondSize), true, 8, false));
				else if (second == primitive::real)
					addReal(Param(id, Size(secondSize), true, 8, false));
			} else {
				// Full, push it on the stack.
				addStack(Param(id, desc->size(), true, 0, false));
			}
		}

	}
}
