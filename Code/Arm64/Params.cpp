#include "stdafx.h"
#include "Params.h"
#include "Asm.h"
#include "Utils/Bitwise.h"

namespace code {
	namespace arm64 {

		Param::Param() {
			clear();
		}

		Param::Param(Nat id, Primitive p) {
			set(id, p.size().size64(), p.offset().v64(), false);
		}

		Param::Param(Nat id, Nat size, Nat offset, Bool stack) {
			set(id, size, offset, stack);
		}

		void Param::clear() {
			set(0xFF, 0, 0xFFFFFFFF, false);
		}

		void Param::set(Nat id, Nat size, Nat offset, Bool stack) {
			data = size & 0xF;
			data |= Nat(stack) << 4;
			data |= (id & 0xFF) << 5;
			data |= (offset & 0xFFFFFF) << 13;
		}

		wostream &operator <<(wostream &to, Param p) {
			if (p.id() == 0xFF) {
				to << L"empty";
			} else {
				to << L"#" << p.id() << L"+" << p.offset() << L"," << p.size();
			}
			if (p.stack())
				to << L"(stack)";
			return to;
		}

		StrBuf &operator <<(StrBuf &to, Param p) {
			if (p.id() == 0xFF) {
				to << S("empty");
			} else {
				to << S("#") << p.id() << S("+") << p.offset() << S(",") << p.size();
			}
			if (p.stack())
				to << S("(stack)");
			return to;
		}

		static void clear(GcArray<Param> *array) {
			for (Nat i = array->filled; i < array->count; i++)
				array->v[i].clear();
		}


		Params::Params() {
			integer = runtime::allocArray<Param>(engine(), &natArrayType, 8);
			real = runtime::allocArray<Param>(engine(), &natArrayType, 8);
			integer->filled = 0;
			real->filled = 0;
			clear(integer);
			clear(real);
			stack = new (this) Array<Nat>();
		}

		void Params::add(Nat id, TypeDesc *type) {
			if (PrimitiveDesc *p = as<PrimitiveDesc>(type)) {
				add(id, p->v);
			} else if (ComplexDesc *c = as<ComplexDesc>(type)) {
				addDesc(id, c);
			} else if (SimpleDesc *s = as<SimpleDesc>(type)) {
				addDesc(id, s);
			} else {
				assert(false, L"Unknown type description found.");
			}
		}

		void Params::add(Nat id, Primitive p) {
			switch (p.kind()) {
			case primitive::none:
				// Nothing to do!
				break;
			case primitive::pointer:
			case primitive::integer:
				addParam(integer, Param(id, p));
				break;
			case primitive::real:
				addParam(real, Param(id, p));
				break;
			default:
				dbg_assert(false, L"Unknown primitive type!");
			}
		}

		void Params::addDesc(Nat id, ComplexDesc *type) {
			// The complex types are actually simple. We just add a pointer to our parameter list!
			addParam(integer, Param(id, 8, 0, true));
		}

		void Params::addParam(GcArray<Param> *to, Param p) {
			if (to->filled < to->count) {
				to->v[to->filled++] = p;
			} else {
				stack->push(p.id());
			}
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
			for (Nat i = 0; i < layout->count; i++) {
				if (layout->v[i].kind() != primitive::real)
					return false;
				if (layout->v[i].size().size64() != size)
					return false;
				if (Nat(layout->v[i].offset().v64()) != i * size)
					return false;
			}

			return true;
		}

		void Params::addDesc(Nat id, SimpleDesc *type) {
			// Here, we need to check the type to see if we shall pass parts of it in registers.
			// The rules are roughly:
			// - If the struct is larger than 2 words, pass it on the stack.
			// - If the parameter uses 2 registers: "pad" to the next even register.
			// - If registers are full, pass it on the stack.
			// - If all members are floats: pass in fp register. Otherwise, int register.

			// TODO: In some cases, we need to replace the parameter with a pointer to the
			// stack-allocated copy. It is currently not entirely clear *when* to do this.

			Nat size = type->size().size64();
			if (size > 16) {
				// Too large: pass on the stack!
				stack->push(id);
				return;
			}

			// Check which registers to use.
			GcArray<Param> *regType = uniformFp(type) ? real : integer;

			// Two registers?
			if (size > 8) {
				// Round up to the next even register.
				regType->filled = min(roundUp(regType->filled, size_t(2)), regType->count);

				// Check if enough space.
				if (regType->filled + 2 <= regType->count) {
					regType->v[regType->filled++] = Param(id, 8, 0, false);
					regType->v[regType->filled++] = Param(id, 8, 8, false);
					return;
				}
			} else {
				if (regType->filled + 1 <= regType->count) {
					regType->v[regType->filled++] = Param(id, size, 0, false);
					return;
				}
			}

			// Fallback: Push it on the stack.
			stack->push(id);
		}

		static void put(StrBuf *to, GcArray<Param> *p, const wchar **names) {
			for (Nat i = 0; i < p->filled; i++) {
				*to << S("\n") << names[i] << S(":") << p->v[i];
			}
		}

		void Params::toS(StrBuf *to) const {
			*to << S("Parameters:");
			for (Nat i = 0; i < integer->filled; i++)
				*to << S("\nx") << i << S(":") << integer->v[i];
			for (Nat i = 0; i < real->filled; i++)
				*to << S("\nd") << i << S(":") << real->v[i];

			if (stack->count() > 0) {
				*to << S("\nOn stack: ") << stack->at(0);
				for (Nat i = 1; i < stack->count(); i++) {
					*to << S(", ") << stack->at(i);
				}
			}
		}

		Nat Params::registerCount() const {
			return integer->count + real->count;
		}

		Param Params::registerAt(Nat n) const {
			if (n < integer->count)
				return integer->v[n];
			if (n - integer->count < real->count)
				return real->v[n - integer->count];
			assert(false, L"Out of bounds.");
			return Param();
		}

		Reg Params::registerSrc(Nat n) const {
			if (n < integer->count)
				return ptrr(n);
			else
				return dr(n - integer->count);
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
			if (desc->size().size64() > 16) {
				// Too large. Pass it on the stack!
				memory = true;
				return;
			}

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
