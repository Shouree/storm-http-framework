#include "stdafx.h"
#include "Layout.h"
#include "Asm.h"
#include "../Listing.h"
#include "../Binary.h"
#include "../Layout.h"
#include "../Exception.h"
#include "../UsedRegs.h"

namespace code {
	namespace arm64 {

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

			TRANSFORM(mov),
			TRANSFORM(lea),

			TRANSFORM(fnRet),
			TRANSFORM(fnRetRef),
		};

		Layout::Layout(Binary *owner) : owner(owner) {}

		void Layout::before(Listing *dest, Listing *src) {
			// Initialize state.
			currentBlock = Block();
			usingEH = src->exceptionAware();

			// Find registers that need to be preserved.
			preserved = allUsedRegs(src);
			for (Nat i = 0; i < fnDirtyCount; i++)
				preserved->remove(fnDirtyRegs[i]);

			// Figure out result.
			result = code::arm64::result(src->result);

			// Find parameters.
			Array<Var> *p = src->allParams();
			params = new (this) Params();
			for (Nat i = 0; i < p->count(); i++) {
				params->add(i, src->paramDesc(p->at(i)));
			}

			// If the result is stored in registers, we need to "spill" it to non-clobbered
			// registers during cleanup. Make sure we have them.
			// Note: We don't always have to preserve register 19. We only need it if we
			// need to run destructors in the epilog.
			if (result->regType != primitive::none && usingEH) {
				preserved->put(ptrr(19));
				if (result->twoReg)
					preserved->put(ptrr(20));
			}

			Nat preserveCount = preserved->count();
			// If result is passed in memory, we need to spill x8 to the stack as well. We treat it
			// as a regular clobbered register.
			if (result->memory) {
				preserveCount++;
			}
			layout = code::arm64::layout(src, params, preserveCount);

			// Keep track of which parameters are stored indirectly.
			varIndirect = new (this) Array<Bool>(layout->count(), false);
			for (Nat i = 0; i < params->totalCount(); i++) {
				Param par = params->totalAt(i);
				if (par.any()) {
					Var v = p->at(par.id());
					varIndirect->at(v.key()) = par.inMemory();

					// If passed in memory, modify the EH data for the parameter. These parameters
					// are freed by the caller, and should be freed by reference if we ever free
					// them.
					if (par.inMemory()) {
						FreeOpt flags = dest->freeOpt(v);
						flags &= ~freeOnException;
						flags &= ~freeOnBlockExit;
						flags |= freeIndirection;
						dest->freeOpt(v, flags);
					}
				}
			}

			// Initialize our bookkeeping of active variables.
			Array<Var> *vars = src->allVars();
			activated = new (this) Array<Nat>(vars->count(), 0);
			activationId = 0;

			for (Nat i = 0; i < vars->count(); i++) {
				Var var = vars->at(i);
				if (src->freeOpt(var) & freeInactive)
					activated->at(var.key()) = INACTIVE;
			}

			// Keep track of active blocks.
			activeBlocks = new (this) Array<ActiveBlock>();
		}

		void Layout::during(Listing *dest, Listing *src, Nat id) {
			static OpTable<TransformFn> t(transformMap, ARRAY_COUNT(transformMap));

			// Note: mov and lea are the only operands that may have references to memory.
			Instr *i = src->at(id);
			if (TransformFn f = t[i->op()]) {
				(this->*f)(dest, i);
			} else {
#ifdef DEBUG
				// This should not happen, but we keep it for debugging.
				if (i->src().type() == opVariable || i->dest().type() == opVariable) {
					WARNING(L"Unexpected variable reference in: " << i);
				}
#endif

				*dest << i;
			}
		}

		void Layout::after(Listing *dest, Listing *src) {
			*dest << alignAs(Size::sPtr);
			*dest << dest->meta();

			// Total stack size.
			*dest << dat(ptrConst(layout->last()));

			// Output metadata table.
			Array<Var> *vars = src->allVars();
			for (Nat i = 0; i < vars->count(); i++) {
				Var &v = vars->at(i);
				Operand fn = src->freeFn(v);
				if (fn.empty())
					*dest << dat(ptrConst(Offset(0)));
				else
					*dest << dat(src->freeFn(v));
				*dest << dat(intConst(layout->at(v.key())));
				*dest << dat(natConst(activated->at(v.key())));
			}

			// Output active blocks. Used by the exception handling.
			*dest << alignAs(Size::sPtr);
			for (Nat i = 0; i < activeBlocks->count(); i++) {
				const ActiveBlock &a = activeBlocks->at(i);
				*dest << lblOffset(a.pos);
				*dest << dat(natConst(0)); // TODO: Store "a" here!
			}

			// Owner.
			*dest << dat(objPtr(owner));

			// PLN(L"Layout:");
			// for (Nat i = 0; i < layout->count(); i++)
			// 	PLN(i << L": " << layout->at(i));
			// PVAR(dest);
		}

		Operand Layout::resolve(Listing *src, const Operand &op) {
			return resolve(src, op, op.size());
		}

		Operand Layout::resolve(Listing *src, const Operand &op, const Size &size) {
			if (op.type() != opVariable)
				return op;

			Var v = op.var();
			if (!src->accessible(v, currentBlock))
				throw new (this) VariableUseError(v, currentBlock);
			return xRel(size, ptrFrame, layout->at(v.key()) + op.offset());
		}

		static void zeroVar(Listing *dest, Offset offset, Size size) {
			// Note: Everything is aligned to 8 bytes, so we can just fill memory with 8-byte store
			// instructions that can be merged by the code generation.
			Nat s = size.size64();
			for (Nat i = 0; i < s; i += 8) {
				*dest << mov(longRel(ptrFrame, offset), xzr);
				offset += Size::sLong;
			}
		}

		void Layout::initBlock(Listing *dest, Block init) {
			if (currentBlock != dest->parent(init)) {
				Str *msg = TO_S(engine(), S("Can not begin ") << init << S(" unless the current is ")
								<< dest->parent(init) << S(". Current is ") << currentBlock);
				throw new (this) BlockBeginError(msg);
			}

			currentBlock = init;

			Array<Var> *vars = dest->allVars(init);
			for (Nat i = 0; i < vars->count(); i++) {
				Var v = vars->at(i);
				if (!dest->isParam(v))
					zeroVar(dest, layout->at(v.key()), v.size());
			}

			if (usingEH) {
				// Remember where the block started.
				Label lbl = dest->label();
				*dest << lbl;
				activeBlocks->push(ActiveBlock(currentBlock, activationId, lbl));
			}
		}

		void Layout::saveResult(Listing *dest, Result *result) {
			if (result->memory)
				return;

			switch (result->regType) {
			case primitive::integer:
				*dest << mov(ptrr(19), ptrr(0));
				if (result->twoReg)
					*dest << mov(ptrr(20), ptrr(1));
				break;
			case primitive::real:
				*dest << mov(xr(19), dr(0));
				if (result->twoReg)
					*dest << mov(xr(20), dr(1));
				break;
			}
		}

		void Layout::restoreResult(Listing *dest, Result *result) {
			if (result->memory || result->regType == primitive::none)
				return;

			switch (result->regType) {
			case primitive::integer:
				*dest << mov(ptrr(0), ptrr(19));
				if (result->twoReg)
					*dest << mov(ptrr(1), ptrr(20));
				break;
			case primitive::real:
				*dest << mov(dr(0), xr(19));
				if (result->twoReg)
					*dest << mov(dr(1), xr(20));
				break;
			}
		}

		void Layout::destroyBlock(Listing *dest, Block destroy, Bool preserveResult, Bool table) {
			if (destroy != currentBlock)
				throw new (this) BlockEndError();

			// Did we save the result?
			Bool savedResult = false;

			// Destroy in reverse order.
			Array<Var> *vars = dest->allVars(destroy);
			for (Nat i = vars->count(); i > 0; i--) {
				Var v = vars->at(i - 1);

				Operand dtor = dest->freeFn(v);
				FreeOpt when = dest->freeOpt(v);

				if (!dtor.empty() && (when & freeOnBlockExit) == freeOnBlockExit) {
					// Should we destroy it right now?
					if (activated->at(v.key()) > activationId)
						continue;

					if (preserveResult && !savedResult) {
						saveResult(dest, result);
						savedResult = true;
					}

					Reg param = ptrr(0);
					if (when & freeIndirection) {
						if (when & freePtr) {
							*dest << mov(param, resolve(dest, v, Size::sPtr));
							*dest << call(dtor, Size());
						} else {
							*dest << mov(param, resolve(dest, v, Size::sPtr));
							*dest << mov(asSize(param, v.size()), xRel(v.size(), param, Offset()));
							*dest << call(dtor, Size());
						}
					} else {
						if (when & freePtr) {
							*dest << lea(param, resolve(dest, v));
							*dest << call(dtor, Size());
						} else {
							*dest << mov(asSize(param, v.size()), resolve(dest, v));
							*dest << call(dtor, Size());
						}
					}
					// TODO: Zero memory to avoid multiple destruction in rare cases?
				}
			}

			if (savedResult) {
				restoreResult(dest, result);
			}

			currentBlock = dest->parent(currentBlock);
			if (usingEH && table) {
				Label lbl = dest->label();
				*dest << lbl;
				activeBlocks->push(ActiveBlock(currentBlock, activationId, lbl));
			}
		}

		void Layout::prologTfm(Listing *dest, Instr *src) {
			// Emit instruction for updating sp, also preserves sp and fp from old frame, and sets fp to sp.
			*dest << instrSrc(engine(), op::prolog, ptrConst(layout->last()));

			// Preserve registers.
			Nat offset = 16; // After sp and fp.

			// Store x8 if we need it.
			if (result->memory) {
				*dest << mov(resultLocation(), ptrr(8));
				offset += 8;
			}

			// Preserve remaining registers.
			for (RegSet::Iter begin = preserved->begin(), end = preserved->end(); begin != end; ++begin) {
				Operand memory = longRel(ptrFrame, Offset(offset));
				Reg reg = asSize(begin.v(), Size::sLong);
				*dest << mov(memory, reg);
				*dest << preserve(memory, reg);
				offset += 8;
			}

			// Preserve parameters.
			TODO(L"Handle simple parameters that are too large to fit in registers!");
			Array<Var> *paramVars = dest->allParams();
			for (Nat i = 0; i < params->registerCount(); i++) {
				Param p = params->registerAt(i);
				if (p == Param())
					continue;

				Offset offset = layout->at(paramVars->at(p.id()).key()) + Offset(p.offset());
				*dest << mov(longRel(ptrFrame, offset), asSize(params->registerSrc(i), Size::sLong));
			}

			// Initialize the root block.
			initBlock(dest, dest->root());
		}

		void Layout::epilogTfm(Listing *dest, Instr *src) {
			// Destroy blocks. Note: We shall not modify 'currentBlock', nor alter the exception
			// table as this may be an early return from the function.
			Block oldBlock = currentBlock;
			for (Block now = currentBlock; now != Block(); now = dest->parent(now)) {
				destroyBlock(dest, now, true, false);
			}
			currentBlock = oldBlock;

			// Restore spilled registers.
			Nat offset = 16;
			if (result->memory)
				offset += 8; // Adjust for preserving x8, but we don't need to restore it.
			for (RegSet::Iter begin = preserved->begin(), end = preserved->end(); begin != end; ++begin) {
				*dest << mov(asSize(begin.v(), Size::sLong), longRel(ptrFrame, Offset(offset)));
				offset += 8;
			}

			// Emit the epilog, and related metadata.
			*dest << instrSrc(engine(), op::epilog, ptrConst(layout->last()));
		}

		void Layout::beginBlockTfm(Listing *dest, Instr *src) {
			initBlock(dest, src->src().block());
		}

		void Layout::endBlockTfm(Listing *dest, Instr *src) {
			destroyBlock(dest, src->src().block(), false, true);
		}

		void Layout::jmpBlockTfm(Listing *dest, Instr *src) {
			// Destroy blocks until we find 'to'.
			Block to = src->src().block();

			// We shall not modify the block level after we're done, so we must restore it.
			Block oldBlock = currentBlock;
			for (Block now = currentBlock; now != to; now = dest->parent(now)) {
				if (now == Block()) {
					Str *msg = TO_S(this, S("The block ") << to << S(" is not a parent of ") << oldBlock << S("."));
					throw new (this) BlockEndError(msg);
				}

				destroyBlock(dest, now, false, false);
			}

			*dest << jmp(src->dest().label());
			currentBlock = oldBlock;
		}

		void Layout::activateTfm(Listing *dest, Instr *src) {
			Var var = src->src().var();
			Nat &id = activated->at(var.key());

			if (id == 0)
				throw new (this) VariableActivationError(var, S("must be marked with 'freeInactive'."));
			if (id != INACTIVE)
				throw new (this) VariableActivationError(var, S("already activated."));

			id = ++activationId;

			// We only need to update the block id if this impacts exception handling.
			if (dest->freeOpt(var) & freeOnException) {
				Label lbl = dest->label();
				*dest << lbl;
				activeBlocks->push(ActiveBlock(currentBlock, activationId, lbl));
			}
		}

		static void returnSimple(Listing *dest, Result *result, Size size, Reg src) {
			if (result->memory) {
				// Memcpy using the mov instruction.
				Reg destReg = ptrB;
				if (src == ptrB)
					destReg = ptrC;

				inlineMemcpy(dest, destReg, src, ptrr(16), ptrr(17));

			} else if (result->regType != primitive::none) {
				// Just populate the relevant registers:
				if (result->regType == primitive::real) {
					if (size.size64() <= 1) {
						*dest << mov(br(0), xRel(Size::sByte, src, Offset()));
					} else if (size.size64() <= 4) {
						*dest << mov(sr(0), xRel(Size::sInt, src, Offset()));
					} else if (size.size64() <= 8) {
						*dest << mov(dr(0), xRel(Size::sLong, src, Offset()));
					} else {
						*dest << mov(dr(0), xRel(Size::sLong, src, Offset()));
						*dest << mov(dr(1), xRel(Size::sLong, src, Offset(Size::sLong)));
					}
				} else {
					if (size.size64() <= 1) {
						*dest << mov(al, xRel(Size::sByte, src, Offset()));
					} else if (size.size64() <= 4) {
						*dest << mov(eax, xRel(Size::sInt, src, Offset()));
					} else if (size.size64() <= 8) {
						*dest << mov(rax, xRel(Size::sLong, src, Offset()));
					} else {
						*dest << mov(rax, xRel(Size::sLong, src, Offset()));
						*dest << mov(rbx, xRel(Size::sLong, src, Offset(Size::sLong)));
					}
				}
			}
		}

		void Layout::fnRetTfm(Listing *dest, Instr *src) {
			Operand value = resolve(dest, src->src());
			assert(value.size() == dest->result->size(), L"Wrong size passed to fnRet!");

			// Handle the return value.
			if (as<PrimitiveDesc>(dest->result)) {
				switch (result->regType) {
				case primitive::none:
					break;
				case primitive::integer:
				case primitive::pointer:
					if (value.type() == opRegister && same(value.reg(), ptrA)) {
						// Already where it is supposed to be!
					} else {
						// Just use a mov!
						*dest << mov(asSize(ptrA, value.size()), value);
					}
					break;
				case primitive::real:
					*dest << mov(asSize(dr(0), value.size()), value);
					break;
				}
			} else if (ComplexDesc *c = as<ComplexDesc>(dest->result)) {
				// Call the copy constructor.
				*dest << lea(ptrB, value);
				*dest << lea(ptrA, resultLocation());
				*dest << call(c->ctor, Size());
			} else if (SimpleDesc *s = as<SimpleDesc>(dest->result)) {
				*dest << lea(ptrA, value);
				returnSimple(dest, result, s->size(), ptrA);
			} else {
				assert(false);
			}

			epilogTfm(dest, src);
			*dest << ret(Size()); // We won't do register usage analysis, so this is OK.
		}

		void Layout::fnRetRefTfm(Listing *dest, Instr *src) {
			Operand value = resolve(dest, src->src());
			assert(value.size() == Size::sPtr, L"Wrong size passed to fnRetRef!");

			// Handle the return value.
			if (PrimitiveDesc *p = as<PrimitiveDesc>(dest->result)) {
				if (result->regType != primitive::none) {
					Reg target = result->regType == primitive::real ? dr(0) : ptrA;

					if (value.type() == opRegister) {
						*dest << mov(asSize(target, p->v.size()), xRel(p->v.size(), value.reg(), Offset()));
					} else {
						*dest << mov(ptrA, value);
						*dest << mov(asSize(target, p->v.size()), xRel(p->v.size(), ptrA, Offset()));
					}
				}
			} else if (ComplexDesc *c = as<ComplexDesc>(dest->result)) {
				// Call the copy constructor.
				*dest << lea(ptrB, value);
				*dest << lea(ptrA, resultLocation());
				*dest << call(c->ctor, Size());
			} else if (SimpleDesc *s = as<SimpleDesc>(dest->result)) {
				Reg reg = ptrA;
				if (value.type() == opRegister) {
					reg = value.reg();
				} else {
					*dest << mov(reg, value);
				}
				returnSimple(dest, result, s->size(), reg);
			} else {
				assert(false);
			}

			epilogTfm(dest, src);
			*dest << ret(Size()); // We won't do register usage analysis, so this is OK.
		}

		void Layout::movTfm(Listing *out, Instr *instr) {
			Operand src = instr->src();
			Operand dst = instr->dest();

			// Note: We can assume that only one parameter is a variable (other has to be a register).

			if (src.type() == opVariable) {
				Nat varId = src.var().key();
				Bool indirect = varIndirect->at(varId);
				Offset stackOffset = layout->at(varId);
				Offset varOffset = src.offset();

				if (indirect) {
					// We have: mov <reg>, <var>
					// Can transform into:
					// mov <reg>, <var>
					// mov <reg>, [<reg>+<offset>]
					Reg r = asSize(dst.reg(), Size::sPtr);
					*out << mov(r, ptrRel(ptrFrame, stackOffset));
					*out << mov(dst, xRel(src.size(), r, varOffset));
				} else {
					*out << instr->alterSrc(xRel(src.size(), ptrFrame, stackOffset + varOffset));
				}
			} else if (dst.type() == opVariable) {
				Nat varId = dst.var().key();
				Bool indirect = varIndirect->at(varId);
				Offset stackOffset = layout->at(varId);
				Offset varOffset = dst.offset();

				if (indirect) {
					// We have: mov <var>, <reg>
					// Can transform into:
					// mov x16, <var>
					// mov [x16+<offset>], <reg>
					// Note: It is a bit risky to use x16 or x17 without checking if we can trash it.
					// However, direct writes to the variable like this are rare, and Storm almost
					// never uses x16 and x17, so we should be fine.
					Reg r = ptrr(16);
					if (same(r, src.reg()))
						r = ptrr(17);
					*out << mov(r, ptrRel(ptrFrame, stackOffset));
					*out << mov(xRel(dst.size(), r, varOffset), src);
				} else {
					*out << instr->alterDest(xRel(dst.size(), ptrFrame, stackOffset + varOffset));
				}
			} else {
				// No changes needed.
				*out << instr;
			}
		}

		void Layout::leaTfm(Listing *dest, Instr *instr) {
			// Note: We only need to consider 'src' here!
			Operand src = instr->src();
			if (src.type() != opVariable) {
				*dest << instr;
				return;
			}

			Nat varId = src.var().key();
			Bool indirect = varIndirect->at(varId);
			Offset stackOffset = layout->at(varId);
			Offset varOffset = src.offset();

			// Handle indirection if needed.
			if (indirect) {
				*dest << mov(instr->dest(), ptrRel(ptrFrame, stackOffset));
				if (varOffset != Offset())
					*dest << add(instr->dest(), ptrConst(varOffset));
			} else {
				*dest << instr->alterSrc(xRel(src.size(), ptrFrame, stackOffset + varOffset));
			}
		}

		Array<Offset> *layout(Listing *src, Params *params, Nat spilled) {
			Array<Offset> *result = code::layout(src, Size::sPtr); // Specify minimum alignment.
			Array<Var> *paramVars = src->allParams();

			// A stack frame on Arm64 is as follows. Offsets are relative sp and x29 (frame
			// ptr). The prolog will make sure that we can use x29 as the frame pointer in the
			// generated code. This is important as the function-call code adjusts sp to make room
			// for parameters, and we still need the ability to access local variables at that
			// point.
			//
			// ...
			// param1
			// param0    <- old sp
			// localN
			// ...
			// local0
			// spilled-paramN
			// ...
			// spilled-param0
			// spilledN
			// ...
			// spilled0
			// old-x30
			// old-x29   <- sp, x29

			// Account for parameters that are in registers and need to be spilled to the stack.
			// TODO: It would be nice to avoid this, but we need more advanced register allocation
			// for that to work properly.
			Nat spilledParams = 0;
			for (Nat i = 0; i < params->registerCount(); i++)
				if (params->registerAt(i) != Param())
					spilledParams++;

			// TODO: We might need to align spilled registers to a 16-byte boundary to make stp and
			// ldp work properly.

			// Account for the "stack frame" (x30 and x29):
			spilled += 2;

			// Adjust "result" to account for spilled registers.
			Nat adjust = (spilled + spilledParams) * 8;
			for (Nat i = 0; i < result->count(); i++) {
				result->at(i) += Offset(adjust);
			}

			// Fill in the parameters that need to be spilled to the stack.
			Nat spillCount = spilled;
			for (Nat i = 0; i < params->registerCount(); i++) {
				Param p = params->registerAt(i);
				// Some params are "padding".
				if (p.empty())
					continue;

				// If multiple parts of the same parameter, then don't update the variable again.
				if (p.offset() == 0)
					result->at(paramVars->at(p.id()).key()) = Offset(spillCount * 8);

				spillCount += 1;
			}

			// Ensure that the size of the stack is rounded up to a multiple of 16 bytes.
			if (result->last().v64() & 0xF)
				result->last() += Offset::sPtr;

			// Now we can fill in the offset of parameters passed on the stack.
			for (Nat i = 0; i < params->stackCount(); i++) {
				Offset off(params->stackOffset(i));
				off += result->last(); // Increase w. size of entire stack frame.
				result->at(paramVars->at(params->stackParam(i).id()).key()) = off;
			}

			return result;
		}
	}
}
