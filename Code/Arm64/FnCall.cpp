#include "stdafx.h"
#include "FnCall.h"
#include "Params.h"
#include "Asm.h"
#include "Utils/Bitwise.h"

namespace code {
	namespace arm64 {

		ParamInfo::ParamInfo(TypeDesc *desc, const Operand &src, Bool ref)
			: type(desc), src(src), ref(ref), lea(false) {}

		// Create set of registers used for function parameters.
		static RegSet *dirtyRegs(Engine &e) {
			RegSet *r = new (e) RegSet();
			for (size_t i = 0; i < fnDirtyCount; i++)
				r->put(fnDirtyRegs[i]);
			return r;
		}

		// Any complex parameters?
		static Bool hasComplex(Array<ParamInfo> *params) {
			for (Nat i = 0; i < params->count(); i++)
				if (as<ComplexDesc>(params->at(i).type))
					return true;
			return false;
		}

		// Copy complex parameters to the stack. Return a new block if we copied parameters to the stack.
		static Block copyComplex(Listing *dest, RegSet *used, Array<ParamInfo> *params, Block currentBlock) {
			if (!hasComplex(params))
				return Block();

			TODO(L"Save dirty registers!");

			Block block = dest->createBlock(currentBlock);
			*dest << begin(block);

			// Spill registers that get clobbered by function calls:
			RegSet *dirtyRegs = code::arm64::dirtyRegs(dest->engine());
			for (Nat i = 0; i < params->count(); i++) {
				Operand &op = params->at(i).src;
				if (!op.hasRegister())
					continue;

				Reg reg = op.reg();

				// Need to preserve?
				if (!dirtyRegs->has(reg))
					continue;

				// Note: Updates the list of params and 'used' as well.
				op = preserveReg(reg, used, dest, block);
			}

			// Make copies of objects. Note: We know that x0..x8 are not used now!
			for (Nat i = 0; i < params->count(); i++) {
				ParamInfo &param = params->at(i);
				ComplexDesc *c = as<ComplexDesc>(param.type);
				if (!c)
					continue;

				Var v = dest->createVar(block, c, freeDef | freeInactive);
				*dest << lea(ptrr(0), v);
				if (param.ref)
					*dest << mov(ptrr(1), param.src);
				else
					*dest << lea(ptrr(1), param.src);

				*dest << call(c->ctor, Size());
				*dest << activate(v);

				// Modify the parameter accordingly.
				param.src = v;
				param.ref = false;
				param.lea = true;
			}

			return block;
		}

		// Actual entry-point.
		void emitFnCall(Listing *dest, Operand toCall, Operand resultPos, TypeDesc *resultType,
						Bool resultRef, Block currentBlock, RegSet *used, Array<ParamInfo> *params) {
			Engine &e = dest->engine();

			// Don't modify the RegSet, or our params.
			used = new (e) RegSet(*used);
			params = new (e) Array<ParamInfo>(*params);

			Params *paramLayout = new (e) Params();
			for (Nat i = 0; i < params->count(); i++)
				paramLayout->add(i, params->at(i).type);

			Result *resultLayout = result(resultType);

			// Start by making copies of complex parameters to the stack if necessary.
			Block block = copyComplex(dest, used, params, currentBlock);

			PVAR(paramLayout);

			TODO(L"FIXME");

			// Disable the block if we used it.
			if (block != Block()) {
				TODO(L"Preserve the result while running destructors!");
				*dest << end(block);
			}
		}

	}
}
