#include "stdafx.h"
#include "Params.h"
#include "Asm.h"
#include "Utils/Bitwise.h"

namespace code {
	namespace arm64 {

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
				addInt(Param(id, type->size(), true, 0, true));
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
					addInt(Param(id, type->size(), true, 0, false));
					return;
				}
			}

			// Fallback: Push it on the stack.
			addStack(Param(id, type->size(), true, 0, false));
		}

		Reg Params::registerSrc(Nat n) const {
			const Nat intRegs = 8;
			if (n < intRegs)
				return ptrr(n);
			else
				return dr(n - intRegs);
		}

		Params *layoutParams(Array<TypeDesc *> *types) {
			Params *p = new (types) Params();
			for (Nat i = 0; i < types->count(); i++)
				p->add(i, types->at(i));
			return p;
		}


		/**
		 * Result.
		 */

		Result::Result(TypeDesc *result) {
			regType = primitive::none;
			twoReg = false;
			memory = false;

			if (PrimitiveDesc *p = as<PrimitiveDesc>(result)) {
				add(p);
			} else if (ComplexDesc *c = as<ComplexDesc>(result)) {
				add(c);
			} else if (SimpleDesc *s = as<SimpleDesc>(result)) {
				add(s);
			} else {
				assert(false, L"Unknown type description found.");
			}
		}

		void Result::toS(StrBuf *to) const {
			if (memory) {
				*to << S("(memory)");
			} else if (regType == primitive::none) {
				*to << S("(none)");
			} else {
				*to << primitive::name(regType);
			}
		}

		void Result::add(PrimitiveDesc *desc) {
			regType = desc->v.kind();
			if (regType == primitive::pointer)
				regType = primitive::integer;
		}

		void Result::add(ComplexDesc *desc) {
			// Complex types are easy. They are always passed in memory.
			memory = true;
		}

		void Result::add(SimpleDesc *desc) {
			// The logic here is very similar to 'Params::addDesc'.
			Nat sz = desc->size().size64();
			if (sz > 16) {
				// Too large. Pass it on the stack!
				memory = true;
				return;
			}

			if (sz > 8)
				twoReg = true;

			if (uniformFp(desc))
				regType = primitive::real;
			else
				regType = primitive::integer;
		}

		Result *result(TypeDesc *result) {
			return new (result) Result(result);
		}

	}
}
