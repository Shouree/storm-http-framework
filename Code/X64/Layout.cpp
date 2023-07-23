#include "stdafx.h"
#include "Layout.h"
#include "Exception.h"
#include "Asm.h"
#include "Arena.h"
#include "../Binary.h"
#include "../Layout.h"
#include "../PosixEh/FnState.h"

namespace code {
	namespace x64 {

		// Number used for inactive variables.
		static const Nat INACTIVE = 0xFFFFFFFF;

#define TRANSFORM(x) { op::x, &Layout::x ## Tfm }

		const OpEntry<Layout::TransformFn> Layout::transformMap[] = {
			TRANSFORM(prolog),
			TRANSFORM(epilog),
			TRANSFORM(beginBlock),
			TRANSFORM(endBlock),
			TRANSFORM(jmpBlock),
			TRANSFORM(activate),

			TRANSFORM(fnRet),
			TRANSFORM(fnRetRef),
		};


		Layout::Layout(const Arena *arena) : arena(arena) {}

		void Layout::before(Listing *dest, Listing *src) {
			// Initialize some state.
			block = Block();
			usingEH = src->exceptionAware();

			// Compute the layout of all parameters.
			params = arena->createParams();
			params->result(src->result);
			Array<Var> *p = src->allParams();
			for (Nat i = 0; i < p->count(); i++) {
				params->add(i, src->paramDesc(p->at(i)));
			}

			// Figure out which registers we need to spill.
			toPreserve = allUsedRegs(src);
			for (RegSet::Iter i = arena->dirtyRegs->begin(), end = arena->dirtyRegs->end(); i != end; ++i)
				toPreserve->remove(*i);

			// Compute the layout.
			layout = computeLayout(src, params, toPreserve->count());

			// Initialize the 'activated' array.
			Array<Var> *vars = src->allVars();
			activated = new (this) Array<Nat>(vars->count(), 0);
			activationId = 0;

			for (Nat i = 0; i < vars->count(); i++) {
				Var var = vars->at(i);
				if (src->freeOpt(var) & freeInactive)
					activated->at(var.key()) = INACTIVE;
			}

			// The EH table.
			activeBlocks = new (this) Array<ActiveBlock>();
		}

		void Layout::during(Listing *dest, Listing *src, Nat line) {
			static OpTable<TransformFn> t(transformMap, ARRAY_COUNT(transformMap));

			Instr *i = src->at(line);
			TransformFn f = t[i->op()];
			if (f) {
				(this->*f)(dest, src, line);
			} else {
				*dest << i->alter(resolve(src, i->dest()), resolve(src, i->src()));
			}
		}

		void Layout::after(Listing *dest, Listing *src) {
			*dest << alignAs(Size::sPtr);
			*dest << dest->meta();

			// Offset between RBP and RSP.
			*dest << dat(ptrConst(-layout->last()));

			// Output metadata table.
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

			// Output the table containing active blocks. Used by the exception handling mechanism.
			*dest << alignAs(Size::sPtr);
			// Table contents. Each 'row' is 8 bytes.
			for (Nat i = 0; i < activeBlocks->count(); i++) {
				const ActiveBlock &a = activeBlocks->at(i);
				*dest << lblOffset(a.pos);
				*dest << dat(natConst(code::eh::encodeFnState(a.block.key(), a.activated)));
			}

			// Table size.
			*dest << dat(ptrConst(activeBlocks->count()));
		}

		Operand Layout::resolve(Listing *src, const Operand &op) {
			return resolve(src, op, op.size());
		}

		Operand Layout::resolve(Listing *src, const Operand &op, const Size &size) {
			if (op.type() != opVariable)
				return op;

			Var v = op.var();
			if (!src->accessible(v, block))
				throw new (this) VariableUseError(v, block);
			return xRel(size, ptrFrame, layout->at(v.key()) + op.offset());
		}

		void Layout::spillParams(Listing *dest) {
			Array<Var> *all = dest->allParams();

			for (Nat i = 0; i < params->registerCount(); i++) {
				Param info = params->registerParam(i);
				if (info.size() == Size())
					continue; // Not used.
				if (info.id() == Param::returnId()) {
					*dest << mov(ptrRel(ptrFrame, resultParam()), params->registerSrc(i));
					continue;
				}

				Offset to = layout->at(all->at(info.id()).key());
				to += Offset(info.offset());

				Size size(info.size());
				Reg r = asSize(params->registerSrc(i), size);
				if (r == noReg) {
					// Size not natively supported. Round up!
					size += Size::sInt.alignment();
					r = asSize(params->registerSrc(i), size);
				}
				*dest << mov(xRel(size, ptrFrame, to), r);
			}
		}

		void Layout::prologTfm(Listing *dest, Listing *src, Nat line) {
			// Generate the prolog. Generates push and mov to set up a basic stack frame. Also emits
			// proper unwind data.
			TODO(L"Move stack space allocation into the prolog to emit Win32 EH data?");
			*dest << prolog();

			// Allocate stack space.
			if (layout->last() != Offset())
				*dest << sub(ptrStack, ptrConst(layout->last()));

			// Keep track of offsets.
			Offset offset = -Offset::sPtr;

			// Save registers we need to preserve.
			for (RegSet::Iter i = toPreserve->begin(); i != toPreserve->end(); ++i) {
				*dest << mov(ptrRel(ptrFrame, offset), asSize(i.v(), Size::sPtr));
				*dest << preserve(ptrRel(ptrFrame, offset), asSize(i.v(), Size::sPtr));
				offset -= Offset::sPtr;
			}

			// Spill parameters to the stack.
			spillParams(dest);

			// Initialize the root block.
			initBlock(dest, dest->root(), rax);
		}

		void Layout::epilogTfm(Listing *dest, Listing *src, Nat line) {
			epilog(dest, src, line, true);
		}

		void Layout::epilog(Listing *dest, Listing *src, Nat line, Bool preserveRax) {
			// Destroy blocks. Note: we shall not modify 'block' nor alter the exception table as
			// this may be an early return from the function.
			Block oldBlock = block;
			for (Block now = block; now != Block(); now = src->parent(now)) {
				destroyBlock(dest, now, preserveRax, false);
			}
			block = oldBlock;

			// Restore preserved registers.
			Offset offset = -Offset::sPtr;
			for (RegSet::Iter i = toPreserve->begin(); i != toPreserve->end(); ++i) {
				*dest << mov(asSize(i.v(), Size::sPtr), ptrRel(ptrFrame, offset));
				offset -= Offset::sPtr;
			}

			// The "epilog" pseudo-op generates a LEAVE instruction, which corresponds to these instructions:
			// *dest << mov(ptrStack, ptrFrame);
			// *dest << pop(ptrFrame);

			*dest << code::epilog();
		}

		void Layout::beginBlockTfm(Listing *dest, Listing *src, Nat line) {
			Instr *instr = src->at(line);
			// Note: register is added in the previous pass.
			Reg freeReg = noReg;
			if (instr->dest().type() == opRegister)
				freeReg = instr->dest().reg();

			initBlock(dest, instr->src().block(), freeReg);
		}

		void Layout::endBlockTfm(Listing *dest, Listing *src, Nat line) {
			destroyBlock(dest, src->at(line)->src().block(), false, true);
		}

		void Layout::jmpBlockTfm(Listing *dest, Listing *src, Nat line) {
			// Destroy blocks until we find 'to'.
			Block to = src->at(line)->src().block();

			// We shall not modify the block level after we're done, so we must restore it.
			Block oldBlock = block;
			for (Block now = block; now != to; now = src->parent(now)) {
				if (now == Block()) {
					Str *msg = TO_S(this, S("The block ") << to << S(" is not a parent of ") << oldBlock << S("."));
					throw new (this) BlockEndError(msg);
				}

				destroyBlock(dest, now, false, false);
			}

			*dest << jmp(src->at(line)->dest().label());
			block = oldBlock;
		}

		void Layout::activateTfm(Listing *dest, Listing *src, Nat line) {
			Var var = src->at(line)->src().var();
			Nat &id = activated->at(var.key());

			if (id == 0)
				throw new (this) VariableActivationError(var, S("must be marked with 'freeInactive'."));
			if (id != INACTIVE)
				throw new (this) VariableActivationError(var, S("already activated."));

			id = ++activationId;

			// We only need to update the block id if this impacts exception handling.
			if (src->freeOpt(var) & freeOnException) {
				Label lbl = dest->label();
				*dest << lbl;
				activeBlocks->push(ActiveBlock(block, activationId, lbl));
			}
		}

		// Zero the memory of a variable. 'initReg' should be true if we need to set <reg> to 0
		// before using it as our zero. 'initReg' will be set to false, so that it is easy to use
		// zeroVar in a loop, causing only the first invocation to emit '<reg> := 0'.
		static void zeroVar(Listing *dest, Offset start, Size size, Reg reg, Bool &initReg) {
			nat s64 = size.size64();
			if (s64 == 0)
				return;

			reg = asSize(reg, Size::sLong);

			if (initReg) {
				*dest << bxor(reg, reg);
				initReg = false;
			}

			// Note: We need to be careful not to overshoot too much. We might have a stack of
			// booleans that fill up 2 or 3 bytes. Using a 4 byte fill then would potentially
			// overwrite some data.
			nat pos = 0;
			while (pos < s64) {
				if (s64 - pos >= 8) {
					*dest << mov(longRel(ptrFrame, start + Offset(pos)), reg);
					pos += 8;
				} else if (s64 - pos >= 4) {
					*dest << mov(intRel(ptrFrame, start + Offset(pos)), asSize(reg, Size::sInt));
					pos += 4;
				} else {
					*dest << mov(byteRel(ptrFrame, start + Offset(pos)), asSize(reg, Size::sByte));
					pos += 1;
				}
			}
		}

		void Layout::initBlock(Listing *dest, Block init, Reg reg) {
			if (block != dest->parent(init)) {
				Str *msg = TO_S(engine(), S("Can not begin ") << init << S(" unless the current is ")
								<< dest->parent(init) << S(". Current is ") << block);
				throw new (this) BlockBeginError(msg);
			}

			block = init;

			Bool initReg = true;
			Bool restoreReg = reg == noReg;
			if (restoreReg) {
				reg = eax;
				*dest << push(eax);
				TODO(L"This is not technically legal on Windows, but as long as no exception occurs it is probably fine.");
			}

			Array<Var> *vars = dest->allVars(init);
			// Go in reverse to make linear accesses in memory when we're using big variables.
			for (Nat i = vars->count(); i > 0; i--) {
				Var v = vars->at(i - 1);

				// Don't initialize parameters or variables that we don't need to initialize.
				if (!dest->isParam(v) && (dest->freeOpt(v) & freeNoInit) == 0)
					zeroVar(dest, layout->at(v.key()), v.size(), reg, initReg);
			}

			if (restoreReg)
				*dest << pop(eax);

			if (usingEH) {
				// Remember where the block started.
				Label lbl = dest->label();
				*dest << lbl;
				activeBlocks->push(ActiveBlock(block, activationId, lbl));
			}
		}

		static void saveResult(Listing *dest, const Result &result) {
			PLN(L"TODO: Not legal on Windows");
			if (result.memoryRegister() != noReg) {
				// We need to keep the stack aligned to 16 bits.
				*dest << push(result.memoryRegister());
				*dest << push(result.memoryRegister());
			} else if (result.registerCount() > 0) {
				Size sz = Size::sPtr * roundUp(result.registerCount(), Nat(2));
				*dest << sub(ptrStack, ptrConst(sz));
				for (Nat i = 0; i < result.registerCount(); i++) {
					*dest << mov(ptrRel(ptrStack, Offset::sPtr * i), asSize(result.registerAt(i), Size::sPtr));
				}
			}
		}

		static void restoreResult(Listing *dest, const Result &result) {
			PLN(L"TODO: Not legal on Windows");
			if (result.memoryRegister() != noReg) {
				*dest << pop(result.memoryRegister());
				*dest << pop(result.memoryRegister());
			} else if (result.registerCount() > 0) {
				Size sz = Size::sPtr * roundUp(result.registerCount(), Nat(2));
				for (Nat i = 0; i < result.registerCount(); i++) {
					*dest << mov(asSize(result.registerAt(i), Size::sPtr), ptrRel(ptrStack, Offset::sPtr * i));
				}
				*dest << add(ptrStack, ptrConst(sz));
			}
		}

		void Layout::destroyBlock(Listing *dest, Block destroy, Bool preserveRax, Bool table) {
			if (destroy != block)
				throw new (this) BlockEndError();

			Bool pushedResult = false;
			Array<Var> *vars = dest->allVars(destroy);
			// Destroy in reverse order.
			for (Nat i = vars->count(); i > 0; i--) {
				Var v = vars->at(i - 1);

				Operand dtor = dest->freeFn(v);
				FreeOpt when = dest->freeOpt(v);

				if (!dtor.empty() && (when & freeOnBlockExit) == freeOnBlockExit) {
					// Should we destroy it right now?
					if (activated->at(v.key()) > activationId)
						continue;

					if (preserveRax && !pushedResult) {
						saveResult(dest, params->result());
						pushedResult = true;
					}

					Reg firstParam = params->registerSrc(0);

					TODO(L"Make sure that we have enough shadow space here!");
					if (when & freeIndirection) {
						if (when & freePtr) {
							*dest << mov(firstParam, resolve(dest, v, Size::sPtr));
							*dest << call(dtor, Size());
						} else {
							*dest << mov(firstParam, resolve(dest, v, Size::sPtr));
							*dest << mov(asSize(firstParam, v.size()), xRel(v.size(), firstParam, Offset()));
							*dest << call(dtor, Size());
						}
					} else {
						if (when & freePtr) {
							*dest << lea(firstParam, resolve(dest, v));
							*dest << call(dtor, Size());
						} else {
							*dest << mov(asSize(firstParam, v.size()), resolve(dest, v));
							*dest << call(dtor, Size());
						}
					}
					// TODO: Zero memory to avoid multiple destruction in rare cases?
				}
			}

			if (pushedResult) {
				restoreResult(dest, params->result());
			}

			block = dest->parent(block);
			if (usingEH && table) {
				Label lbl = dest->label();
				*dest << lbl;
				activeBlocks->push(ActiveBlock(block, activationId, lbl));
			}
		}

		// Memcpy using mov instructions. Note: rdx does not need to be preserved,
		// and does never contain a part of the result on any of the supported ABIs.
		static void movMemcpy(Listing *to, Reg dest, Reg src, Size size) {
			Nat total = size.size64();
			Nat offset = 0;

			for (; offset + 8 <= total; offset += 8) {
				*to << mov(rdx, longRel(src, Offset(offset)));
				*to << mov(longRel(dest, Offset(offset)), rdx);
			}

			for (; offset + 4 <= total; offset += 4) {
				*to << mov(edx, intRel(src, Offset(offset)));
				*to << mov(intRel(dest, Offset(offset)), edx);
			}

			for (; offset + 1 <= total; offset += 1) {
				*to << mov(dl, byteRel(src, Offset(offset)));
				*to << mov(byteRel(dest, Offset(offset)), dl);
			}
		}

		// Put the return value into registers. Assumes 'srcPtr' contains a pointer to the struct to
		// be returned. 'srcPtr' can not be 'rdx'.
		static void returnSimple(Listing *dest, const Result &result, SimpleDesc *type,
								Reg srcPtr, Offset resultParam) {

			assert(!same(srcPtr, rdx));

			if (result.memoryRegister() != noReg) {
				*dest << mov(result.memoryRegister(), ptrRel(ptrFrame, resultParam));
				// Inline a memcpy using mov instructions.
				movMemcpy(dest, result.memoryRegister(), srcPtr, type->size());
			} else {
				for (Nat i = 0; i < result.registerCount(); i++) {
					Reg r = result.registerAt(i);
					*dest << mov(r, xRel(size(r), srcPtr, Offset(result.registerOffset(i))));
				}
			}
		}

		static void returnPrimitive(Listing *dest, PrimitiveDesc *p, const Operand &value, Reg target) {
			switch (p->v.kind()) {
			case primitive::none:
				break;
			case primitive::integer:
			case primitive::pointer:
				if (value.type() == opRegister && same(value.reg(), target)) {
					// Already at the correct place!
				} else {
					// A simple 'mov' is enough!
					*dest << mov(asSize(target, value.size()), value);
				}
				break;
			case primitive::real:
				// A simple 'mov' will do!
				*dest << mov(asSize(target, value.size()), value);
				break;
			}
		}

		void Layout::fnRetTfm(Listing *dest, Listing *src, Nat line) {
			Operand value = resolve(src, src->at(line)->src());
			if (value.size() != dest->result->size()) {
				StrBuf *msg = new (this) StrBuf();
				*msg << S("Wrong size passed to fnRet. Got: ");
				*msg << value.size();
				*msg << S(" but expected ");
				*msg << dest->result->size() << S(".");
				throw new (this) InvalidValue(msg->toS());
			}

			// Handle the return value.
			if (PrimitiveDesc *p = as<PrimitiveDesc>(src->result)) {
				if (params->result().registerCount() > 0)
					returnPrimitive(dest, p, value, params->result().registerAt(0));
			} else if (ComplexDesc *c = as<ComplexDesc>(src->result)) {
				TODO(L"Ensure that we have enough shadow space!");
				// Call the copy-ctor.
				*dest << lea(params->registerSrc(0), value);
				*dest << mov(params->registerSrc(1), ptrRel(ptrFrame, resultParam()));
				*dest << call(c->ctor, Size());
				// Set 'rax' to the address of the return value.
				*dest << mov(ptrA, ptrRel(ptrFrame, resultParam()));
			} else if (SimpleDesc *s = as<SimpleDesc>(src->result)) {
				if (params->result().memoryRegister() != noReg) {
					// Just copy using memcpy. We also need to set 'ptrA' accordingly.
					// Note: ptrC is always safe to use in this context, both on Windows and Posix.
					*dest << lea(ptrC, value);
					*dest << mov(ptrA, ptrRel(ptrFrame, resultParam()));
					movMemcpy(dest, ptrA, ptrC, s->size());
				} else {
					Reg r = params->registerSrc(0);
					*dest << lea(r, value);
					returnSimple(dest, params->result(), s, r, resultParam());
				}
			} else {
				assert(false);
			}

			epilogTfm(dest, src, line);
			*dest << ret(Size()); // We will not analyze registers anymore, Size() is fine.
		}

		static void returnPrimitiveRef(Listing *dest, PrimitiveDesc *p, const Operand &value, Reg target) {
			Size s(p->v.size());
			switch (p->v.kind()) {
			case primitive::none:
				break;
			case primitive::integer:
			case primitive::pointer:
				// Always two 'mov'.
				*dest << mov(target, value);
				*dest << mov(asSize(target, s), xRel(s, target, Offset()));
				break;
			case primitive::real:
				// Note: ptrA is safe as temporary here.
				*dest << mov(ptrA, value);
				*dest << mov(asSize(target, s), xRel(s, ptrA, Offset()));
				break;
			}
		}

		void Layout::fnRetRefTfm(Listing *dest, Listing *src, Nat line) {
			Operand value = resolve(src, src->at(line)->src());

			// Handle the return value.
			if (PrimitiveDesc *p = as<PrimitiveDesc>(src->result)) {
				if (params->result().registerCount() > 0)
					returnPrimitiveRef(dest, p, value, params->result().registerAt(0));
			} else if (ComplexDesc *c = as<ComplexDesc>(src->result)) {
				// Call the copy-ctor.
				*dest << mov(params->registerSrc(0), value);
				*dest << mov(params->registerSrc(1), ptrRel(ptrFrame, resultParam()));
				*dest << call(c->ctor, Size());
				// Set 'rax' to the address of the return value.
				*dest << mov(ptrA, ptrRel(ptrFrame, resultParam()));
			} else if (SimpleDesc *s = as<SimpleDesc>(src->result)) {
				if (params->result().memoryRegister() != noReg) {
					// Just copy using memcpy. We also need to set 'ptrA' accordingly.
					// Note: ptrC is always safe to use here!
					*dest << mov(ptrC, value);
					*dest << mov(ptrA, ptrRel(ptrFrame, resultParam()));
					movMemcpy(dest, ptrA, ptrC, s->size());
				} else {
					Reg r = params->registerSrc(0);
					*dest << mov(r, value);
					returnSimple(dest, params->result(), s, r, resultParam());
				}
			} else {
				assert(false);
			}

			epilogTfm(dest, src, line);
			*dest << ret(Size()); // We will not analyze registers anymore, Size() is fine.
		}

	}
}
