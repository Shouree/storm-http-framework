#include "stdafx.h"
#include "Params.h"
#include "Asm.h"
#include "Utils/Bitwise.h"

namespace code {
	namespace arm64 {

		static Size roundSize(Size sz) {
			if (sz.size64() == 1)
				return sz;
			else if (sz.size64() <= 4)
				return Size::sInt;
			else
				return sz.alignedAs(Size::sWord);
		}

		Params::Params() : code::Params(8, 8, 8, 16) {}

		void Params::addPrimitive(Nat id, Primitive p) {
			switch (p.kind()) {
			case primitive::none:
				// Nothing to do!
				break;
			case primitive::pointer:
			case primitive::integer:
				addInt(Param(id, p, true));
				break;
			case primitive::real:
				addReal(Param(id, p, true));
				break;
			default:
				dbg_assert(false, L"Unknown primitive type!");
			}
		}

		void Params::addComplex(Nat id, ComplexDesc *desc) {
			// The complex types are actually simple. We just add a pointer to our parameter list!
			addInt(Param(id, desc->size(), true, 0, true));
		}

		// Check if the parameter is a uniform float struct.
		static Bool uniformFp(SimpleDesc *type) {
			GcArray<Primitive> *layout = type->v;
			if (layout->count <= 0)
				return false;
			if (layout->v[0].kind() != primitive::real)
				return false;
			if (layout->v[0].offset().v64() != 0)
				return false;

			// Check for this size.
			Nat size = layout->v[0].size().size64();

			// Others should be evenly distributed.
			for (Nat i = 1; i < layout->count; i++) {
				if (layout->v[i].kind() != primitive::real)
					return false;
				if (layout->v[i].size().size64() != size)
					return false;
				if (Nat(layout->v[i].offset().v64()) != i * size)
					return false;
			}

			return true;
		}

		void Params::addSimple(Nat id, SimpleDesc *type) {
			// Here, we need to check the type to see if we shall pass parts of it in registers.
			// The rules are roughly:
			// - If the struct is a uniform FP struct that fits in 4 or fewer registers, pass it in the
			//   corresponding fp registers.
			// - If the struct is larger than 2 words, pass it on the stack, as if it was "complex".
			// - If the parameter has a natural alignment of >= 16 bytes, "pad" to the next even register.
			//   (we support no such types at the moment)
			// - If registers are full, pass it on the stack.
			// - If all members are floats: pass in fp register. Otherwise, int register.

			// Uniform FP struct (HFA)?
			if (uniformFp(type)) {
				Nat regs = type->v->count;
				if (regs <= 4 && hasReal(regs)) {
					// Pass it in the next fp registers.
					for (Nat i = 0; i < regs; i++) {
						const Primitive &p = type->v->v[i];
						addReal(Param(id, p.size(), true, p.offset().v64(), false));
					}

				} else {
					// Pass it on the stack.
					addStack(Param(id, type->size(), true, 0, false));
				}

				return;
			}

			// Now, we know that we are dealing with a type that should be passed in integer registers.

			Nat size = type->size().size64();
			if (size > 16) {
				// Too large: pass on the stack, replace it with a pointer to the data.
				addInt(Param(id, roundSize(type->size()), true, 0, true));
				return;
			}

			// Two registers?
			if (size > 8) {
				// Check if enough space.
				if (hasInt(2)) {
					addInt(Param(id, Size::sLong, true, 0, false));
					addInt(Param(id, Size::sLong, true, 8, false));
					return;
				}
			} else {
				if (hasInt(1)) {
					addInt(Param(id, roundSize(type->size()), true, 0, false));
					return;
				}
			}

			// Fallback: Push it on the stack.
			addStack(Param(id, type->size(), true, 0, false));
		}

		void Params::resultPrimitive(Primitive p) {
			switch (p.kind()) {
			case primitive::none:
				// Nothing to do!
				break;
			case primitive::pointer:
			case primitive::integer:
				resultData = Result::inRegister(engine(), asSize(ptrr(0), p.size()));
				break;
			case primitive::real:
				resultData = Result::inRegister(engine(), asSize(dr(0), p.size()));
				break;
			default:
				dbg_assert(false, L"Unknown primitive type!");
			}
		}

		void Params::resultComplex(ComplexDesc *) {
			// Always returned in memory. Always in x8.
			resultData = Result::inMemory(ptrr(8));
		}

		void Params::resultSimple(SimpleDesc *type) {
			// Similar to 'addSimple' above, but don't have to handle cases with not enough register space.

			// Uniform FP struct (HFA)?
			if (uniformFp(type)) {
				Nat regs = type->v->count;
				if (regs <= 4) {
					resultData = Result::inRegisters(engine(), regs);
					for (Nat i = 0; i < regs; i++) {
						const Primitive &p = type->v->v[i];
						resultData.putRegister(asSize(dr(i), p.size()), p.offset().v64());
					}
					return;
				}
			} else {
				// Integer!
				Nat size = type->size().size64();
				if (size <= 8) {
					// One register:
					Reg r = asSize(ptrr(0), roundSize(type->size()));
					resultData = Result::inRegister(engine(), r);
					return;
				} else if (size <= 16) {
					// Two registers:
					resultData = Result::inRegisters(engine(), 2);
					resultData.putRegister(xr(0), 0);
					resultData.putRegister(xr(1), 8);
					return;
				}
			}

			// Pass it in memory:
			resultData = Result::inMemory(ptrr(8));
		}

		Reg Params::registerSrc(Nat n) const {
			const Nat intRegs = 8;
			if (n < intRegs)
				return ptrr(n);
			else
				return dr(n - intRegs);
		}

		Params *layoutParams(TypeDesc *result, Array<TypeDesc *> *types) {
			Params *p = new (types) Params();
			p->result(result);
			for (Nat i = 0; i < types->count(); i++)
				p->add(i, types->at(i));
			return p;
		}

	}
}
