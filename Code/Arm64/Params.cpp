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
			set(id, p.size(), p.offset().v64(), false);
		}

		Param::Param(Nat id, Size size, Nat offset, Bool inMemory) {
			set(id, size, offset, inMemory);
		}

		Bool Param::empty() {
			return data == 0xFFFFFFFE;
		}

		void Param::clear() {
			set(0xFF, Size(), 0xFFFFFFFF, false);
		}

		void Param::set(Nat id, Size size, Nat offset, Bool inMemory) {
			size64 = (size.size64() << 4) | size.align64();
			data = Nat(inMemory);
			data |= (id & 0xFF) << 1;
			data |= (offset & 0x7FFFFF) << 9;
		}

		wostream &operator <<(wostream &to, Param p) {
			if (p.empty()) {
				to << L"empty";
			} else {
				to << L"#" << p.id() << L"+" << p.offset() << L"," << p.size();
			}
			if (p.inMemory())
				to << L"(in memory)";
			return to;
		}

		StrBuf &operator <<(StrBuf &to, Param p) {
			if (p.empty()) {
				to << S("empty");
			} else {
				to << S("#") << p.id() << S("+") << p.offset() << S(",") << p.size();
			}
			if (p.inMemory())
				to << S("(in memory)");
			return to;
		}

		static void clear(GcArray<Param> *array) {
			for (Nat i = array->filled; i < array->count; i++)
				array->v[i].clear();
		}

		static const GcType paramType = {
			GcType::tArray,
			null,
			null,
			sizeof(Param),
			0,
			{},
		};

		const GcType Params::stackParamType = {
			GcType::tArray,
			null,
			null,
			sizeof(StackParam),
			0,
			{}
		};

		Params::Params() {
			integer = runtime::allocArray<Param>(engine(), &paramType, 8);
			real = runtime::allocArray<Param>(engine(), &paramType, 8);
			integer->filled = 0;
			real->filled = 0;
			clear(integer);
			clear(real);
			stackPar = null;
			stackSize = 0;
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
			addParam(integer, Param(id, Size::sPtr, 0, true));
		}

		void Params::addParam(GcArray<Param> *to, Param p) {
			if (to->filled < to->count) {
				to->v[to->filled++] = p;
			} else {
				addStack(p);
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
			// - If the struct is larger than 2 words, pass it on the stack, as if it was "complex".
			// - If the parameter uses 2 registers: "pad" to the next even register.
			// - If registers are full, pass it on the stack.
			// - If all members are floats: pass in fp register. Otherwise, int register.

			// TODO: In some cases, we need to replace the parameter with a pointer to the
			// stack-allocated copy. It is currently not entirely clear *when* to do this.

			Nat size = type->size().size64();
			if (size > 16) {
				// Too large: pass on the stack, replace it with a pointer to the data.
				addParam(integer, Param(id, Size::sPtr, 0, true));
				return;
			}

			// Check which registers to use.
			GcArray<Param> *regType = uniformFp(type) ? real : integer;

			// Two registers?
			if (size > 8) {
				// Round up to the next even register. Note: This is only needed if alignment
				// is 16, which we do not currently support.
				// regType->filled = min(roundUp(regType->filled, size_t(2)), regType->count);

				// Check if enough space.
				if (regType->filled + 2 <= regType->count) {
					regType->v[regType->filled++] = Param(id, Size::sLong, 0, false);
					regType->v[regType->filled++] = Param(id, Size::sLong, 8, false);
					return;
				}
			} else {
				if (regType->filled + 1 <= regType->count) {
					regType->v[regType->filled++] = Param(id, type->size(), 0, false);
					return;
				}
			}

			// Fallback: Push it on the stack.
			addStack(Param(id, type->size(), 0, false));
		}

		void Params::addStack(Param param) {
			// Align argument properly.
			stackSize = roundUp(stackSize, param.size().align64());

			if (!stackPar || stackPar->count == stackPar->filled) {
				Nat newSize = stackPar ? stackPar->filled * 2 : 10;
				GcArray<StackParam> *copy = runtime::allocArray<StackParam>(engine(), &stackParamType, newSize);
				copy->filled = 0;
				if (stackPar) {
					memcpy(copy->v, stackPar->v, sizeof(StackParam) * stackPar->filled);
					copy->filled = stackPar->filled;
				}
				stackPar = copy;
			}

			stackPar->v[stackPar->filled].param = param;
			stackPar->v[stackPar->filled].offset = stackSize;
			stackPar->filled++;

			// Update size. Minimum alignment is 8.
			stackSize += roundUp(param.size().aligned().size64(), Nat(8));
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

			if (stackCount() > 0) {
				*to << S("\nOn stack:");
				for (Nat i = 0; i < stackCount(); i++) {
					*to << S(" ") << stackParam(i) << S("@") << stackOffset(i) << S("\n");
				}
			}
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

		Param Params::totalAt(Nat n) const {
			if (n < integer->count)
				return integer->v[n];
			n -= integer->count;
			if (n < real->count)
				return real->v[n];
			n -= real->count;
			if (stackPar && n < stackPar->filled)
				return stackPar->v[n].param;
			assert(false, L"Out of bounds.");
			return Param();
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
