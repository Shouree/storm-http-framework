#include "stdafx.h"
#include "Params.h"
#include "Asm.h"

namespace code {
	namespace x64 {

		Param::Param() {
			clear();
		}

		Param::Param(Nat id, Primitive p) {
			set(id, p.size().size64(), p.offset().v64());
		}

		Param::Param(Nat id, Nat size, Nat offset) {
			set(id, size, offset);
		}

		void Param::clear() {
			set(0xFF, 0, 0xFFFFFFFF);
		}

		void Param::set(Nat id, Nat size, Nat offset) {
			data = size & 0xF;
			data |= (id & 0xFF) << 4;
			data |= (offset & 0xFFFFFF) << 12;
		}

		wostream &operator <<(wostream &to, Param p) {
			if (p.id() == 0xFF) {
				to << L"empty";
			} else {
				to << L"#" << p.id() << L"+" << p.offset() << L"," << p.size();
			}
			return to;
		}

		StrBuf &operator <<(StrBuf &to, Param p) {
			if (p.id() == 0xFF) {
				to << S("empty");
			} else {
				to << S("#") << p.id() << S("+") << p.offset() << S(",") << p.size();
			}
			return to;
		}

		static void clear(GcArray<Param> *array) {
			for (Nat i = array->filled; i < array->count; i++)
				array->v[i].clear();
		}

		Params::Params() {
			integer = runtime::allocArray<Param>(engine(), &natArrayType, 6);
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
				tryAdd(integer, Param(id, p));
				break;
			case primitive::real:
				tryAdd(real, Param(id, p));
				break;
			default:
				dbg_assert(false, L"Unknown primitive type!");
			}
		}

		void Params::addDesc(Nat id, ComplexDesc *type) {
			// The complex types are actually simple. We just add a pointer to our parameter list!
			tryAdd(integer, Param(id, 8, 0));
		}

		void Params::tryAdd(GcArray<Param> *to, Param p) {
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

		void Params::addDesc(Nat id, SimpleDesc *type) {
			// Here, we should check 'type' to see if we shall pass parts of it in registers.
			// It seems the algorithm works roughly as follows (from the offical documentation
			// and examining the output of GCC from Experiments/call64.cpp):
			// - If the struct is larger than 2 64-bit words, pass it on the stack.
			// - If the struct does not fit entirely into registers, pass it on the stack.
			// - Examine each 64-bit word of the struct:
			//   - if the word contains only floating point numbers, pass them into a real register.
			//   - otherwise, pass the word in an integer register (eg. int + float).

			Nat size = type->size().size64();
			if (size > 2*8) {
				// Too large: pass on the stack!
				stack->push(id);
				return;
			}

			primitive::Kind first = paramKind(type->v, 0, 8);
			primitive::Kind second = paramKind(type->v, 8, 16);

			Nat firstSize = min(size, Nat(8));
			Nat secondSize = (size > 8) ? (size - 8) : 0;

			size_t iCount = integer->filled;
			size_t rCount = real->filled;

			if (tryAdd(first, Param(id, firstSize, 0)) &&
				tryAdd(second, Param(id, secondSize, 8))) {
				// It worked!
			} else {
				// Not enough room. Roll back and pass on the stack instead.
				integer->filled = iCount;
				clear(integer);
				real->filled = rCount;
				clear(real);

				stack->push(id);
			}
		}

		bool Params::tryAdd(primitive::Kind kind, Param p) {
			GcArray<Param> *use = null;
			switch (kind) {
			case primitive::none:
				return true;
			case primitive::pointer:
			case primitive::integer:
				use = integer;
				break;
			case primitive::real:
				use = real;
				break;
			}

			if (use->filled < use->count) {
				use->v[use->filled++] = p;
				return true;
			}

			return false;
		}

		static void put(StrBuf *to, GcArray<Param> *p, const wchar **names) {
			for (Nat i = 0; i < p->filled; i++) {
				*to << S("\n") << names[i] << S(":") << p->v[i];
			}
		}

		void Params::toS(StrBuf *to) const {
			*to << S("Parameters:");
			const wchar *i[] = { S("RDI"), S("RSI"), S("RDX"), S("RCX"), S("R8"), S("R9") };
			const wchar *e[] = { S("XMM0"), S("XMM1"), S("XMM2"), S("XMM3"), S("XMM4"), S("XMM5"), S("XMM6"), S("XMM7") };
			put(to, integer, i);
			put(to, real, e);

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
			static Reg v[] = {
				ptrDi, ptrSi, ptrD, ptrC, ptr8, ptr9,
				xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7
			};
			return v[n];
		}

		Params *params(Array<TypeDesc *> *types) {
			Params *p = new (types) Params();
			for (Nat i = 0; i < types->count(); i++)
				p->add(i, types->at(i));
			return p;
		}

		/**
		 * Result.
		 */

		Result::Result(TypeDesc *result) {
			part1 = primitive::none;
			part2 = primitive::none;
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

			if (part1 == primitive::pointer)
				part1 = primitive::integer;
			if (part2 == primitive::pointer)
				part2 = primitive::integer;
		}

		void Result::toS(StrBuf *to) const {
			*to << primitive::name(part1) << S(":") << primitive::name(part2);
		}

		void Result::add(PrimitiveDesc *desc) {
			// Just add the primitive to the relevant register.
			// We do not support >64 bit numbers, so this is fine!
			part1 = desc->v.kind();
		}

		void Result::add(ComplexDesc *desc) {
			// Complex types are easy. They are always passed in memory.
			memory = true;
		}

		void Result::add(SimpleDesc *desc) {
			// The logic here is very similar to 'Params::addDesc'.
			if (desc->size().size64() > 2*8) {
				// Too large. Pass it on the stack!
				memory = true;
				return;
			}

			part1 = paramKind(desc->v, 0, 8);
			part2 = paramKind(desc->v, 8, 16);
		}

		Result *result(TypeDesc *result) {
			return new (result) Result(result);
		}

		Params *layoutParams(TypeDesc *result, Array<TypeDesc *> *params) {
			TODO(L"Use the generic Params interface here as well!");
			Params *layout = new (params) Params();
			if (code::x64::result(result)->memory)
				layout->add(Param::returnId, ptrPrimitive());
			for (Nat i = 0; i < params->count(); i++)
				layout->add(i, params->at(i));
			return layout;
		}

	}
}
