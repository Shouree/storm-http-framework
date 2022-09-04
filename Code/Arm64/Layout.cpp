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

		Layout::Layout(Binary *owner) : owner(owner) {}

		void Layout::before(Listing *dest, Listing *src) {
			// Initialize state.
			currentBlock = Block();

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

			Nat preserveCount = preserved->count();
			// If result is passed in memory, we need to spill it to the stack as well. We treat it
			// as a regular clobbered register.
			if (result->memory)
				preserveCount++;
			layout = code::arm64::layout(src, params, preserveCount);
		}

		void Layout::during(Listing *dest, Listing *src, Nat id) {
			static OpTable<TransformFn> t(transformMap, ARRAY_COUNT(transformMap));

			Instr *i = src->at(id);
			if (TransformFn f = t[i->op()]) {
				(this->*f)(dest, i);
			} else {
				*dest << i->alter(resolve(src, i->dest()), resolve(src, i->src()));
			}
		}

		void Layout::after(Listing *dest, Listing *src) {
			*dest << alignAs(Size::sPtr);
			*dest << dest->meta();

			TODO(L"Add metadata!");

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
			Array<Var> *paramVars = dest->allParams();
			for (Nat i = 0; i < params->registerCount(); i++) {
				Param p = params->registerAt(i);
				if (p == Param())
					continue;

				Offset offset = layout->at(paramVars->at(p.id()).key());
				*dest << mov(longRel(ptrFrame, offset), asSize(params->registerSrc(i), Size::sLong));
			}

			// TODO: Initialize the block!
			currentBlock = dest->root();
		}

		void Layout::epilogTfm(Listing *dest, Instr *src) {
			// Destroy blocks. Note: We shall not modify 'currentBlock', nor alter the exception
			// table as this may be an early return from the function.
			Block oldBlock = currentBlock;
			for (Block now = currentBlock; now != Block(); now = dest->parent(now)) {
				// destroy block
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
			currentBlock = src->src().block();
			// TODO: Initialize the block.
		}

		void Layout::endBlockTfm(Listing *dest, Instr *src) {
			currentBlock = dest->parent(src->src().block());
			// TODO: Destroy the block.
		}

		void Layout::jmpBlockTfm(Listing *dest, Instr *src) {
			// TODO: Implement!
		}

		void Layout::activateTfm(Listing *dest, Instr *src) {
			// TODO: Implement!
		}

		void Layout::fnRetTfm(Listing *dest, Instr *src) {
			// TODO: Finish.
			epilogTfm(dest, src);
			*dest << ret(Size()); // We won't do register usage analysis, so this is OK.
		}

		void Layout::fnRetRefTfm(Listing *dest, Instr *src) {
			// TODO: Finish.
			epilogTfm(dest, src);
			*dest << ret(Size()); // We won't do register usage analysis, so this is OK.
		}

		Array<Offset> *layout(Listing *src, Params *params, Nat spilled) {
			Array<Offset> *result = code::layout(src);
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
				if (p == Param())
					continue;

				result->at(paramVars->at(p.id()).key()) = Offset(spillCount * 8);
				spillCount += 1;
			}

			// Fill in the offset of parameters passed on the stack.
			for (Nat i = 0; i < params->stackCount(); i++) {
				Offset off(params->stackOffset(i));
				off += result->last(); // Increase w. size of entire stack frame.
				result->at(paramVars->at(params->stackParam(i)).key()) = off;
			}

			// Finally: ensure that the size of the stack is rounded up to a multiple of 16 bytes.
			if (result->last().v64() & 0xF)
				result->last() += Offset::sPtr;

			return result;
		}
	}
}
