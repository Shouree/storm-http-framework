#include "stdafx.h"
#include "LayoutVars.h"
#include "Arena.h"
#include "Asm.h"
#include "Binary.h"
#include "Exception.h"
#include "SafeSeh.h"
#include "Seh.h"
#include "../Layout.h"

namespace code {
	namespace x86 {

		// Number used for inactive variables.
		static const Nat INACTIVE = 0xFFFFFFFF;

		// Number of words used for an EH frame.
		static const Nat EH_WORDS = 4;

#define TRANSFORM(x) { op::x, &LayoutVars::x ## Tfm }

		const OpEntry<LayoutVars::TransformFn> LayoutVars::transformMap[] = {
			TRANSFORM(prolog),
			TRANSFORM(epilog),
			TRANSFORM(beginBlock),
			TRANSFORM(endBlock),
			TRANSFORM(jmpBlock),
			TRANSFORM(activate),
			TRANSFORM(jmp),

			TRANSFORM(fnRet),
			TRANSFORM(fnRetRef),
		};

		LayoutVars::LayoutVars() {}

		Operand LayoutVars::resultLoc() {
			if (memberFn) {
				return ptrRel(ptrFrame, Offset::sPtr * 3);
			} else {
				return ptrRel(ptrFrame, Offset::sPtr * 2);
			}
		}

		static void updateLabels(Array<Nat> *update, Array<Label> *labels, Nat current) {
			if (!labels)
				return;

			for (Nat i = 0; i < labels->count(); i++) {
				Label l = labels->at(i);
				while (update->count() <= l.key())
					update->push(0);

				update->at(l.key()) = current;
			}
		}

		static Array<Nat> *computeActivations(Listing *src) {
			Nat current = 0;
			Array<Nat> *result = new (src) Array<Nat>();
			for (Nat i = 0; i < src->count(); i++) {
				Instr *instr = src->at(i);
				Array<Label> *labels = src->labels(i);
				switch (instr->op()) {
				case op::activate:
					current++;
					// Fall through.
				case op::beginBlock:
				case op::endBlock:
				case op::prolog:
				case op::epilog:
					updateLabels(result, labels, INACTIVE);
					break;
				default:
					updateLabels(result, labels, current);
				}
			}

			// Last labels. Doesn't matter too much, but we want the elements in the array so we
			// don't crash.
			updateLabels(result, src->labels(src->count()), current);

			return result;
		}

		void LayoutVars::before(Listing *dest, Listing *src) {
			usingEH = src->exceptionAware();
			resultParam = code::x86::resultParam(src->result);
			memberFn = src->member;

			RegSet *used;

			// At least on Windows, registers are not preserved if an exception is caught
			// here. Therefore, if we contain any exception handlers, we just assume that all
			// registers are dirty.
			if (src->exceptionCaught()) {
				// TODO: If we ever support 32-bit Linux, this is probably not needed.
				used = allRegs(engine());
			} else {
				used = allUsedRegs(src);
				add64(used);
			}

			preserved = new (this) RegSet();
			RegSet *notPreserved = fnDirtyRegs(engine());

			// Any registers in 'all' which are not in 'notPreserved' needs to be properly stored
			// through this function call.

			for (RegSet::Iter i = used->begin(); i != used->end(); ++i) {
				if (!notPreserved->has(i.v()))
					preserved->put(i.v());
			}

			layout = code::x86::layout(src, preserved->count(), usingEH, resultParam, memberFn);

			// Initialize the 'activated' array.
			Array<Var> *vars = src->allVars();
			activated = new (this) Array<Nat>(vars->count(), 0);
			activationId = 0;

			for (Nat i = 0; i < vars->count(); i++) {
				Var var = vars->at(i);
				if (src->freeOpt(var) & freeInactive)
					activated->at(var.key()) = INACTIVE;
			}

			selfLbl = dest->label();
			*dest << selfLbl;

			lblActivation = computeActivations(src);
		}

		void LayoutVars::during(Listing *dest, Listing *src, Nat line) {
			static OpTable<TransformFn> t(transformMap, ARRAY_COUNT(transformMap));

			Instr *i = src->at(line);
			TransformFn f = t[i->op()];
			if (f) {
				(this->*f)(dest, src, line);
			} else {
				*dest << i->alter(resolve(src, i->dest()), resolve(src, i->src()));
			}
		}

		void LayoutVars::after(Listing *dest, Listing *src) {
			*dest << alignAs(Size::sPtr);
			*dest << dest->meta();

			// Total stack size.
			*dest << dat(ptrConst(layout->last()));

			// All variables. Create VarCleanup instances.
			Array<Var> *vars = src->allVars();

			for (nat i = 0; i < vars->count(); i++) {
				Var &v = vars->at(i);
				Operand fn = src->freeFn(v);
				if (fn.empty())
					*dest << dat(ptrConst(Offset(0)));
				else
					*dest << dat(src->freeFn(v));
				*dest << dat(intConst(layout->at(v.key())));
				*dest << dat(natConst(activated->at(v.key())));

				// This happens sometimes in code generation, for example when a variable definition is never
				// reached. As such, we should not complain too much about it. It was useful for debugging
				// the initial migration, however.
				// if (activated->at(v.key()) == INACTIVE)
				// 	// Dont be too worried about zero-sized variables.
				// 	if (v.size() != Size())
				// 		throw new (this) VariableActivationError(v, S("Never activated."));
			}
		}

		Operand LayoutVars::resolve(Listing *listing, const Operand &src) {
			if (src.type() != opVariable)
				return src;

			Var v = src.var();
			if (!listing->accessible(v, block))
				throw new (this) VariableUseError(v, block);
			return xRel(src.size(), ptrFrame, layout->at(v.key()) + src.offset());
		}

		// Zero the memory of a variable. 'initEax' should be true if we need to set eax to 0 before
		// using it as our zero. 'initEax' will be set to false, so that it is easy to use zeroVar
		// in a loop, causing only the first invocation to emit 'eax := 0'.
		static void zeroVar(Listing *dest, Offset start, Size size, bool &initEax) {
			nat s32 = size.size32();
			if (s32 == 0)
				return;

			if (initEax) {
				*dest << bxor(eax, eax);
				initEax = false;
			}

			nat pos = 0;
			while (pos < s32) {
				if (s32 - pos >= 4) {
					*dest << mov(intRel(ptrFrame, start + Offset(pos)), eax);
					pos += 4;
				} else {
					*dest << mov(byteRel(ptrFrame, start + Offset(pos)), al);
					pos += 1;
				}
			}
		}

		void LayoutVars::initBlock(Listing *dest, Block init) {
			if (block != dest->parent(init)) {
				Str *msg = TO_S(engine(), S("Can not begin ") << init << S(" unless the current is ")
								<< dest->parent(init) << S(". Current is ") << block);
				throw new (this) BlockBeginError(msg);
			}

			block = init;

			TODO(L"We can't assume that eax is free at this point.");
			bool initEax = true;

			Array<Var> *vars = dest->allVars(init);
			Array<Listing::CatchInfo> *catchInfo = dest->catchInfo(init);
			// Go in reverse to make linear accesses in memory when we're using big variables.
			for (nat i = vars->count(); i > 0; i--) {
				Var v = vars->at(i - 1);

				if (!dest->isParam(v))
					zeroVar(dest, layout->at(v.key()), v.size(), initEax);
			}

			// If the block was empty, we don't need to update the info.
			if (vars->any())
				updateBlockId(dest);
			else if (catchInfo && catchInfo->any())
				updateBlockId(dest);
		}

		static void saveResult(Listing *dest) {
			if (PrimitiveDesc *p = as<PrimitiveDesc>(dest->result)) {
				Size s = p->v.size();
				switch (p->v.kind()) {
				case primitive::none:
					break;
				case primitive::integer:
				case primitive::pointer:
					if (s == Size::sLong)
						*dest << push(ptrD);
					*dest << push(ptrA);
					break;
				case primitive::real:
					*dest << sub(ptrStack, ptrConst(s));
					*dest << fstp(xRel(s, ptrStack, Offset()));
					break;
				}
			} else {
				// In both cases we need to the address to the value on the stack.
				*dest << push(ptrA);
			}
		}

		static void restoreResult(Listing *dest) {
			if (PrimitiveDesc *p = as<PrimitiveDesc>(dest->result)) {
				Size s = p->v.size();
				switch (p->v.kind()) {
				case primitive::none:
					break;
				case primitive::integer:
				case primitive::pointer:
					*dest << pop(ptrA);
					if (s == Size::sLong)
						*dest << pop(ptrD);
					break;
				case primitive::real:
					*dest << fld(xRel(s, ptrStack, Offset()));
					*dest << add(ptrStack, ptrConst(s));
					break;
				}
			} else {
				// In both cases we need to the address to the value on the stack.
				*dest << pop(ptrA);
			}
		}

		void LayoutVars::destroyBlock(Listing *dest, Block destroy, bool preserveEax) {
			if (destroy != block)
				throw new (this) BlockEndError();

			bool pushedEax = false;

			Array<Var> *vars = dest->allVars(destroy);
			Array<Listing::CatchInfo> *catchInfo = dest->catchInfo(destroy);
			// Destroy in reverse order.
			for (Nat i = vars->count(); i > 0; i--) {
				Var v = vars->at(i - 1);

				Operand dtor = dest->freeFn(v);
				FreeOpt when = dest->freeOpt(v);

				if (!dtor.empty() && (when & freeOnBlockExit) == freeOnBlockExit) {
					// Should we destroy it right now?
					if (activated->at(v.key()) > activationId)
						continue;

					if (preserveEax && !pushedEax) {
						saveResult(dest);
						pushedEax = true;
					}

					if (when & freePtr) {
						*dest << lea(ptrA, resolve(dest, v));
						*dest << push(ptrA);
						*dest << call(dtor, Size());
						*dest << add(ptrStack, ptrConst(Offset::sPtr));
					} else if (v.size().size32() <= Size::sInt.size32()) {
						*dest << push(resolve(dest, v));
						*dest << call(dtor, Size());
						*dest << add(ptrStack, ptrConst(v.size()));
					} else {
						*dest << push(high32(resolve(dest, v)));
						*dest << push(low32(resolve(dest, v)));
						*dest << call(dtor, Size());
						*dest << add(ptrStack, ptrConst(v.size()));
					}

					// TODO? Zero memory to avoid multiple destruction in rare cases?
				}
			}

			if (pushedEax)
				restoreResult(dest);

			block = dest->parent(block);
			// If empty, we don't need to update.
			if (vars->any())
				updateBlockId(dest);
			else if (catchInfo && catchInfo->any())
				updateBlockId(dest);
		}

		void LayoutVars::prologTfm(Listing *dest, Listing *src, Nat line) {
			// Set up stack frame.
			*dest << push(ptrFrame);
			*dest << mov(ptrFrame, ptrStack);

			// Allocate stack space.
			*dest << sub(ptrStack, ptrConst(layout->last()));

			// Keep track of offsets...
			Offset offset = -Offset::sPtr;

			// Extra data needed for exception handling.
			if (usingEH) {
				// Current block id.
				*dest << mov(intRel(ptrFrame, offset), natConst(0));
				blockId = offset;
				offset -= Offset::sInt;

				// Self pointer.
				*dest << mov(ptrRel(ptrFrame, offset), selfLbl);
				offset -= Offset::sPtr;

				// Standard SEH frame.
				*dest << mov(ptrRel(ptrFrame, offset), xConst(Size::sPtr, Word(&::x86SafeSEH)));
				offset -= Offset::sPtr;

				// Previous SEH frame
				*dest << threadLocal() << mov(ptrA, ptrRel(noReg, Offset()));
				*dest << mov(ptrRel(ptrFrame, offset), ptrA);

				// Set ourselves as the current frame.
				*dest << lea(ptrA, ptrRel(ptrFrame, offset));
				*dest << threadLocal() << mov(ptrRel(noReg, Offset()), ptrA);

				offset -= Offset::sPtr;
			}

			// Save any registers we need to preserve.
			for (RegSet::Iter i = preserved->begin(); i != preserved->end(); ++i) {
				*dest << mov(ptrRel(ptrFrame, offset), asSize(*i, Size::sPtr));
				offset -= Offset::sPtr;
			}

			// Initialize the root block.
			initBlock(dest, dest->root());
		}

		void LayoutVars::epilogTfm(Listing *dest, Listing *src, Nat line) {
			// Destroy blocks. Note: we shall not modify 'block' as this may be an early return from
			// the function.
			Block oldBlock = block;
			for (Block now = block; now != Block(); now = src->parent(now)) {
				destroyBlock(dest, now, true);
			}
			block = oldBlock;

			// Restore preserved registers.
			{
				Offset offset = -Offset::sPtr;
				if (usingEH)
					offset -= Offset::sPtr * EH_WORDS;
				for (RegSet::Iter i = preserved->begin(); i != preserved->end(); ++i) {
					*dest << mov(asSize(*i, Size::sPtr), ptrRel(ptrFrame, offset));
					offset -= Offset::sPtr;
				}
			}

			if (usingEH) {
				// Remove the SEH. Note: ptrC is not preserved across function calls, so it is OK to use it here!
				// We can not use ptrA nor ptrD as rax == eax:edx
				*dest << mov(ptrC, ptrRel(ptrFrame, -Offset::sPtr * EH_WORDS));
				*dest << threadLocal() << mov(ptrRel(noReg, Offset()), ptrC);
			}

			// TODO: We could emit a "LEAVE" instr instead, that is supposedly faster.
			*dest << mov(ptrStack, ptrFrame);
			*dest << pop(ptrFrame);
		}

		void LayoutVars::beginBlockTfm(Listing *dest, Listing *src, Nat line) {
			initBlock(dest, src->at(line)->src().block());
		}

		void LayoutVars::endBlockTfm(Listing *dest, Listing *src, Nat line) {
			destroyBlock(dest, src->at(line)->src().block(), false);
		}

		void LayoutVars::jmpBlockTfm(Listing *dest, Listing *src, Nat line) {
			// Destroy blocks until we find 'to'.
			Block to = src->at(line)->src().block();

			// We shall not modify the block level after we're done, so we must restore it.
			Block oldBlock = block;
			for (Block now = block; now != to; now = src->parent(now)) {
				if (now == Block()) {
					Str *msg = TO_S(this, S("The block ") << to << S(" is not a parent of ") << oldBlock << S("."));
					throw new (this) BlockEndError(msg);
				}

				destroyBlock(dest, now, false);
			}

			// Update the activation ID if needed.
			Label jmpTo = src->at(line)->dest().label();
			Nat activation = lblActivation->at(jmpTo.key());
			if (activation != INACTIVE && activation != activationId)
				updateBlockId(dest, activation);

			*dest << jmp(jmpTo);
			block = oldBlock;
		}

		void LayoutVars::activateTfm(Listing *dest, Listing *src, Nat line) {
			Var var = src->at(line)->src().var();
			Nat &id = activated->at(var.key());

			if (id == 0)
				throw new (this) VariableActivationError(var, S("must be marked with 'freeInactive'."));
			if (id != INACTIVE)
				throw new (this) VariableActivationError(var, S("already activated."));

			id = ++activationId;

			// We only need to update the block id if this impacts exception handling.
			if (src->freeOpt(var) & freeOnException)
				updateBlockId(dest);
		}

		void LayoutVars::jmpTfm(Listing *dest, Listing *src, Nat line) {
			Instr *i = src->at(line);

			if (i->dest().type() == opLabel) {
				Label to = i->dest().label();

				// Set the activation ID first, unless we don't need to (since we jump to a location
				// that does that anyway) or it is the same as currently.
				Nat activation = lblActivation->at(to.key());
				if (activation != INACTIVE && activation != activationId)
					updateBlockId(dest, activation);
			}

			*dest << i;
		}

		// Memcpy using mov instructions.
		static void movMemcpy(Listing *to, Reg dest, Reg src, Size size) {
			Nat total = size.size32();
			Nat offset = 0;

			for (; offset + 4 <= total; offset += 4) {
				*to << mov(edx, intRel(src, Offset(offset)));
				*to << mov(intRel(dest, Offset(offset)), edx);
			}

			for (; offset + 1 <= total; offset += 1) {
				*to << mov(dl, byteRel(src, Offset(offset)));
				*to << mov(byteRel(dest, Offset(offset)), dl);
			}
		}

		static void returnPrimitive(Listing *dest, PrimitiveDesc *p, const Operand &value) {
			switch (p->v.kind()) {
			case primitive::none:
				break;
			case primitive::integer:
			case primitive::pointer:
				if (value.type() == opRegister && same(value.reg(), ptrA)) {
					// Already at the proper place!
				} else if (value.size() == Size::sLong) {
					*dest << mov(high32(rax), high32(value));
					*dest << mov(low32(rax), low32(value));
				} else {
					// A simple 'mov' is enough!
					*dest << mov(asSize(ptrA, value.size()), value);
				}
				break;
			case primitive::real:
				// We need to load it on the FP stack.
				if (value.size() == Size::sLong) {
					*dest << push(high32(value));
					*dest << push(low32(value));
				} else {
					*dest << push(value);
				}
				*dest << fld(xRel(value.size(), ptrStack, Offset()));
				*dest << add(ptrStack, ptrConst(value.size()));
				break;
			}
		}

		void LayoutVars::fnRetTfm(Listing *dest, Listing *src, Nat line) {
			Operand value = resolve(src, src->at(line)->src());
			assert(value.size() == src->result->size(), L"Wrong size passed to fnRet!");

			if (PrimitiveDesc *p = as<PrimitiveDesc>(src->result)) {
				returnPrimitive(dest, p, value);
			} else if (ComplexDesc *c = as<ComplexDesc>(src->result)) {
				// Call the copy-constructor.
				*dest << lea(ptrA, value);
				*dest << push(ptrA);
				*dest << push(resultLoc());
				*dest << call(c->ctor, Size());
				*dest << add(ptrStack, ptrConst(Size::sPtr * 2));
				*dest << lea(ptrA, value);
			} else if (SimpleDesc *s = as<SimpleDesc>(src->result)) {
				// Note: We're assuming that the type is not a POD since they are sometimes returned in registers!
				*dest << lea(ptrC, value);
				*dest << mov(ptrA, resultLoc());
				movMemcpy(dest, ptrA, ptrC, s->size());
			} else {
				assert(false);
			}

			epilogTfm(dest, src, line);
			*dest << ret(Size()); // We will not analyze registers anymore, Size() is fine.
		}

		static void returnPrimitiveRef(Listing *dest, PrimitiveDesc *p, const Operand &value) {
			Size s(p->v.size());
			switch (p->v.kind()) {
			case primitive::none:
				break;
			case primitive::integer:
			case primitive::pointer:
				// Always two 'mov'. If 64-bit, could be 3.
				*dest << mov(ptrA, value);
				if (s == Size::sLong) {
					*dest << mov(high32(rax), intRel(ptrA, Offset::sInt));
					*dest << mov(low32(rax), intRel(ptrA, Offset()));
				} else {
					*dest << mov(asSize(ptrA, s), xRel(s, ptrA, Offset()));
				}
				break;
			case primitive::real:
				// Load to the FP stack.
				*dest << mov(ptrA, value);
				*dest << fld(xRel(s, ptrA, Offset()));
				break;
			}
		}

		void LayoutVars::fnRetRefTfm(Listing *dest, Listing *src, Nat line) {
			Operand value = resolve(src, src->at(line)->src());

			if (PrimitiveDesc *p = as<PrimitiveDesc>(src->result)) {
				returnPrimitiveRef(dest, p, value);
			} else if (ComplexDesc *c = as<ComplexDesc>(src->result)) {
				// Call the copy-constructor.
				*dest << push(value);
				*dest << push(resultLoc());
				*dest << call(c->ctor, Size());
				*dest << add(ptrStack, ptrConst(Size::sPtr));
				*dest << pop(ptrA);
			} else if (SimpleDesc *s = as<SimpleDesc>(src->result)) {
				// Note: We're assuming that the type is not a POD since they are sometimes returned in registers!
				*dest << mov(ptrC, value);
				*dest << mov(ptrA, resultLoc());
				movMemcpy(dest, ptrA, ptrC, s->size());
			} else {
				assert(false);
			}

			epilogTfm(dest, src, line);
			*dest << ret(Size()); // We will not analyze registers anymore, Size() is fine.
		}

		void LayoutVars::updateBlockId(Listing *dest) {
			updateBlockId(dest, activationId);
		}

		void LayoutVars::updateBlockId(Listing *dest, Nat activation) {
			if (usingEH) {
				Nat id = encodeFnState(block.key(), activation);
				*dest << mov(intRel(ptrFrame, blockId), natConst(id));
			}
		}

		static void layoutParams(Array<Offset> *result, Listing *src, Bool resultParam, Bool member) {
			Offset offset = Offset::sPtr * 2; // old ebp and return address
			Array<Var> *params = src->allParams();
			for (Nat i = 0; i < params->count(); i++) {
				if (resultParam) {
					// Add space for the result parameter
					if (i == 0 && !member)
						offset += Size::sPtr;
					else if (i == 1 && member)
						offset += Size::sPtr;
				}

				Var var = params->at(i);
				Nat id = var.key();
				result->at(id) = offset;
				offset = (offset + var.size().aligned()).alignAs(Size::sPtr);
			}
		}

		Array<Offset> *layout(Listing *src, Nat savedRegs, Bool usingEH, Bool resultParam, Bool member) {
			Array<Offset> *result = code::layout(src);
			Array<Var> *all = src->allVars();

			Offset varOffset;
			// Exception handler frame.
			if (usingEH)
				varOffset += Size::sPtr * EH_WORDS;
			// Saved registers.
			varOffset += Size::sPtr * savedRegs;

			// Update all variables.
			for (nat i = 0; i < all->count(); i++) {
				Var var = all->at(i);
				Nat id = var.key();

				if (src->isParam(var)) {
					// Handled later.
				} else {
					result->at(id) = -(result->at(id) + var.size().aligned() + varOffset);
				}
			}

			// Update all parameters.
			layoutParams(result, src, resultParam, member);

			result->last() = result->last() + varOffset;
			return result;
		}

	}
}
