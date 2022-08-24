#include "stdafx.h"
#include "FnCall.h"
#include "Params.h"
#include "Asm.h"
#include "Utils/Bitwise.h"
#include "../Exception.h"

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

		static void pushParams(Listing *dest, Array<ParamInfo> *params, Params *layout) {
			if (layout->stackCount() == 0)
				return;

			// Compute stack layout first, since we need to decrease the stack pointer first.
			Array<Nat> *offsets = new (dest) Array<Nat>(layout->stackCount(), 0);
			Nat size = 0;
			for (Nat i = 0; i < layout->stackCount(); i++) {
				ParamInfo &info = params->at(layout->stackAt(i));
				Size sz;
				if (info.lea)
					sz = Size::sPtr;
				else
					sz = info.type->size().aligned();

				size = roundUp(size, sz.align64()); // Align argument properly.
				offsets->at(i) = size;
				size += roundUp(sz.size64(), Nat(8)); // Minimum alignment is 8.
			}

			// Stack needs to be 16-byte aligned.
			size = roundUp(size, Nat(16));
			*dest << sub(ptrStack, natConst(size));

			// Now we can copy parameters! We need to be careful to not emit memory-memory moves, as
			// we are called inside "RemoveInvalid".

			Reg reg1 = ptrr(16);
			Reg reg2 = ptrr(17);

			for (Nat i = 0; i < layout->stackCount(); i++) {
				ParamInfo &info = params->at(layout->stackAt(i));
				Size sz = info.src.size();
				if (info.lea == info.ref) {
					Operand dst = xRel(sz, sp, Offset(offsets->at(i)));
					if (info.src.type() == OpType::opRegister) {
						*dest << mov(dst, info.src);
					} else if (sz.size64() <= 8) {
						*dest << mov(asSize(reg1, sz), info.src);
						*dest << mov(dst, asSize(reg1, sz));
					} else {
						assert(false, L"Pushing this large operand is not implemented yet!");
					}
				} else if (info.lea) {
					*dest << lea(reg1, info.src);
					*dest << mov(ptrRel(sp, Offset(offsets->at(i))), reg1);
				} else if (info.ref) {
					assert(false, L"Not implemented yet!");
				}
			}
		}

		// Parameters passed around while assigning contents to registers.
		struct RegEnv {
			// Output listing.
			Listing *dest;
			// All parameters.
			Array<ParamInfo> *src;
			// Layout we want to produce.
			Params *layout;
			// Currently computing an assignment of parameter #x?
			Bool active[17];
			// Finished assigning to a register?
			Bool finished[17];
		};

		static void setRegister(RegEnv &env, Nat i);

		// Make sure any content inside 'reg' is used now, so that 'reg' can be reused for other purposes.
		static void vacateRegister(RegEnv &env, Reg reg) {
			for (Nat i = 0; i < env.layout->registerCount(); i++) {
				Param p = env.layout->registerAt(i);
				if (p == Param())
					continue;

				const Operand &src = env.src->at(p.id()).src;
				if (src.hasRegister() && same(src.reg(), reg)) {
					// We need to set this register now, otherwise it will be destroyed!
					if (env.active[i]) {
						// Cycle detected. Store it in x16 as a temporary and make a note of it.
						*env.dest << mov(asSize(ptrr(16), src.size()), src);
						env.active[i] = false;
					} else {
						setRegister(env, i);
					}
				}
			}
		}

		// Set a register to what it is supposed to be, assuming 'src' is the actual value.
		static void setRegisterVal(RegEnv &env, Reg target, Param param, const Operand &src) {
			if (param.offset() == 0 && src.size().size64() <= 8) {
				if (src.type() == opRegister && src.reg() == target) {
					// Already done!
				} else {
					Reg to = asSize(target, src.size());
					if (to == noReg) {
						// Unsupported size, 'src' must be a variable, so we'll simply copy slightly
						// more data than what we actually need.
						Size s = src.size() + Size::sInt.alignment();
						*env.dest << mov(asSize(target, s), xRel(s, src.var(), Offset()));
					} else {
						*env.dest << mov(to, src);
					}
				}
			} else if (src.type() == opVariable) {
				Size s(param.size());
				*env.dest << mov(asSize(target, s), xRel(s, src.var(), Offset(param.offset())));
			} else {
				throw new (env.dest) InvalidValue(S("Can not pass non-variables larger than 8 bytes to functions."));
			}
		}

		// Set a register to what it is supposed to be, assuming the address of 'src' shall be used.
		static void setRegisterLea(RegEnv &env, Reg target, Param param, const Operand &src) {
			assert(param.size() == 8);
			*env.dest << lea(asSize(target, Size::sPtr), src);
		}

		// Set a register to what it is supposed to be, assuming 'src' is a pointer to the actual value.
		static void setRegisterRef(RegEnv &env, Reg target, Param param, const Operand &src) {
			assert(src.size() == Size::sPtr);
			Size s(param.size());
			Offset o(param.offset());

			// If 'target' is a floating-point register, we can't use that as a temporary.
			if (isVectorReg(target)) {
				// However, since they are always assigned last, we know we can use ptr17, as that
				// will be clobbered by the function call anyway.
				*env.dest << mov(ptrr(17), src);
				*env.dest << mov(asSize(target, s), xRel(s, ptrr(17), o));
			} else {
				// Use the register we're supposed to fill as a temporary.
				if (src.type() == opRegister && src.reg() == target)
					; // Already done!
				else
					*env.dest << mov(asSize(target, Size::sPtr), src);
				Reg to = asSize(target, s);
				if (to == noReg) {
					// Unsupported size, upgrade to the next larger supported one.
					s += Size::sInt.alignment();
					to = asSize(target, s);
				}
				*env.dest << mov(to, xRel(s, target, o));
			}
		}

		// Try to assign the proper value to a single register (other assignments might be performed
		// beforehand to vacate registers).
		static void setRegister(RegEnv &env, Nat i) {
			Param param = env.layout->registerAt(i);
			// Empty?
			if (param == Param())
				return;
			// Already done?
			if (env.finished[i])
				return;

			Reg target = env.layout->registerSrc(i);
			ParamInfo &p = env.src->at(param.id());

			// See if 'target' contains something that is used by other parameters.
			env.active[i] = true;
			vacateRegister(env, target);
			if (!env.active[i]) {
				// Stored in tmp register, restore.
				*env.dest << mov(asSize(target, p.src.size()), asSize(ptrr(16), p.src.size()));
			}
			env.active[i] = false;

			// Set the register.
			if (p.ref == p.lea)
				setRegisterVal(env, target, param, p.src);
			else if (p.ref)
				setRegisterRef(env, target, param, p.src);
			else if (p.lea)
				setRegisterLea(env, target, param, p.src);

			// Done!
			env.finished[i] = true;
		}

		static void setRegisters(Listing *dest, Array<ParamInfo> *src, Params *layout) {
			RegEnv env = {
				dest,
				src,
				layout,
				{ false },
				{ false },
			};

			for (Nat i = 0; i < layout->registerCount(); i++) {
				setRegister(env, i);
			}
		}

		// Actual entry-point.
		void emitFnCall(Listing *dest, Operand toCall, Operand resultPos, TypeDesc *resultType,
						Bool resultRef, Block currentBlock, RegSet *used, Array<ParamInfo> *params) {
			Engine &e = dest->engine();

			// Don't modify the RegSet, or our params.
			used = new (e) RegSet(*used);
			params = new (e) Array<ParamInfo>(*params);

			Params *paramLayout = new (e) Params();
			for (Nat i = 0; i < params->count(); i++) {
				paramLayout->add(i, params->at(i).type);

				// Also check so that registers 16 and 17 are not used (we may use them as temporaries).
				const Operand &op = params->at(i).src;
				if (op.hasRegister() && isIntReg(op.reg())) {
					Nat regId = intRegNumber(op.reg());
					if (regId == 16 || regId == 17)
						throw new (dest) InvalidValue(S("Registers x16 and x17 can't be used for function calls."));
				}
			}


			Result *resultLayout = result(resultType);

			// Start by making copies of complex parameters to the stack if necessary.
			Block block = copyComplex(dest, used, params, currentBlock);

			// Put parameters onto the stack (if required).
			pushParams(dest, params, paramLayout);

			// Start copying parameters into registers.
			setRegisters(dest, params, paramLayout);

			// Call the function! (We don't need accurate information about registers, so it is OK to pass Size()).
			*dest << call(toCall, Size());

			// Disable the block if we used it.
			if (block != Block()) {
				TODO(L"Preserve the result while running destructors!");
				*dest << end(block);
			}
		}

	}
}
