#include "stdafx.h"
#include "FnCall.h"
#include "Params.h"
#include "Asm.h"
#include "RemoveInvalid.h"
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

		// Any parameters that need to be copied to memory?
		static Bool hasMemory(Params *params) {
			for (Nat i = 0; i < params->totalCount(); i++)
				if (params->totalAt(i).inMemory())
					return true;
			return false;
		}

		// Basic map of registers preserved in other registers.
		class PreservedRegs {
		public:
			PreservedRegs() {
				for (Nat i = 0; i < ARRAY_COUNT(preservedInt); i++)
					preservedInt[i] = noReg;
				for (Nat i = 0; i < ARRAY_COUNT(preservedFloat); i++)
					preservedFloat[i] = noReg;
			}

			Reg &operator [](Reg r) {
				if (isIntReg(r))
					return preservedInt[intRegNumber(r)];
				else
					return preservedFloat[vectorRegNumber(r)];
			}
		private:
			Reg preservedInt[32];
			Reg preservedFloat[32];
		};

		// Preserve registers for complex parameters.
		static void preserveComplex(Listing *dest, RegSet *used, Block block, Array<ParamInfo> *params, Params *layout) {
			PreservedRegs preserved;
			RegSet *dirtyRegs = code::arm64::dirtyRegs(dest->engine());

			// First: Look for operands that use register-relative addressing and preserve them in
			// registers. This means that we prioritize storing registers that might help us access
			// multiple values over ones that are read directly.
			Bool firstComplex = true;
			for (Nat i = 0; i < layout->totalCount(); i++) {
				Param p = layout->totalAt(i);
				if (p.empty())
					continue;

				ParamInfo &param = params->at(p.id());
				if (as<ComplexDesc>(param.type) != null && firstComplex) {
					// No need to worry about the first complex parameter!
					firstComplex = false;
					continue;
				}

				if (param.src.type() != opRelative)
					continue;
				Reg reg = param.src.reg();
				if (!dirtyRegs->has(reg))
					continue;

				Reg &movedTo = preserved[reg];
				if (movedTo == noReg)
					movedTo = preserveRegInReg(reg, used, dest);

				if (movedTo == noReg) {
					// This means no more registers were available. In this case, revert to loading
					// the value and storing it in a variable.
					Reg tmpReg = ptrr(16); // Should be free.
					Var v = dest->createVar(block, Size::sPtr);
					if (param.ref) {
						*dest << mov(tmpReg, param.src);
						*dest << mov(v, tmpReg);
					} else {
						*dest << lea(tmpReg, param.src);
						*dest << mov(v, tmpReg);
						param.ref = true;
					}
					param.src = Operand(v);
				} else {
					// Success, update the parameter.
					param.src = xRel(param.src.size(), movedTo, param.src.offset());
				}
			}

			// Then: Look for operands that read registers directly. These are not as bad to spill
			// to the stack, which is why we do this later.
			firstComplex = true;
			for (Nat i = 0; i < layout->totalCount(); i++) {
				Param p = layout->totalAt(i);
				if (p.empty())
					continue;

				ParamInfo &param = params->at(p.id());
				if (as<ComplexDesc>(param.type) != null && firstComplex) {
					// No need to worry about the first complex parameter!
					firstComplex = false;
					continue;
				}

				if (param.src.type() != opRegister)
					continue;
				Reg reg = param.src.reg();
				if (!dirtyRegs->has(reg))
					continue;

				Reg &movedTo = preserved[reg];
				if (movedTo == noReg)
					movedTo = preserveRegInReg(reg, used, dest);

				if (movedTo == noReg) {
					// No more registers available. Spill to stack.
					Var v = dest->createVar(block, param.src.size());
					*dest << mov(v, reg);
					param.src = Operand(v);
				} else {
					// Success, update the parameter.
					param.src = asSize(movedTo, param.src.size());
				}
			}
		}

		// Copy parameters to memory as needed.
		static Block copyToMemory(Listing *dest, RegSet *used, Array<ParamInfo> *params, Params *layout, Block currentBlock) {
			if (!hasMemory(layout))
				return Block();

			// Temporary registers we can use:
			Reg reg1 = ptrr(16);
			Reg reg2 = ptrr(17);

			Block block = dest->createBlock(currentBlock);
			*dest << begin(block);

			// First: copy simple parameters to the stack. This potentially frees up registers.
			for (Nat i = 0; i < layout->totalCount(); i++) {
				Param p = layout->totalAt(i);
				// Only worry about parameters that need to be in memory for now.
				if (p.empty() || !p.inMemory())
					continue;

				ParamInfo &info = params->at(p.id());

				// Note: We skip ComplexDesc for later. They require function calls!
				if (as<ComplexDesc>(info.type))
					continue;

				Var v = dest->createVar(block, info.type->size());

				if (info.ref) {
					if (info.src.type() == opRegister) {
						inlineMemcpy(dest, v, xRel(v.size(), info.src.reg(), Offset()), reg1, reg2);
					} else {
						*dest << mov(reg2, info.src);
						// TODO: In many cases we can probably find another register for this part.
						inlineSlowMemcpy(dest, v, xRel(v.size(), reg2, Offset()), reg1);
					}
				} else {
					if (info.src.type() == opRegister) {
						*dest << mov(v, info.src);
					} else {
						inlineMemcpy(dest, v, info.src, reg1, reg2);
					}
				}

				// Modify the parameter so that we know how to handle it later on.
				info.src = v;
				info.ref = false;
				info.lea = true;
			}

			// Now we can continue with the complex parameters! Start with preserving registers so
			// that we can call copy-ctors!
			preserveComplex(dest, used, currentBlock, params, layout);

			// Then we can actually copy parameters.
			for (Nat i = 0; i < layout->totalCount(); i++) {
				Param p = layout->totalAt(i);
				if (p.empty() || !p.inMemory())
					continue;

				ParamInfo &info = params->at(p.id());
				ComplexDesc *desc = as<ComplexDesc>(info.type);
				if (!desc)
					continue;

				if (info.ref)
					*dest << mov(ptrr(1), info.src);
				else
					*dest << lea(ptrr(1), info.src);

				// This is after moving 'src' to x1, that way we never need to worry about
				// preserving anything for the first complex parameter.
				Var v = dest->createVar(block, desc, freeDef | freeInactive);
				*dest << lea(ptrr(0), v);

				*dest << call(desc->ctor, Size());
				*dest << activate(v);

				// Modify the parameter accordingly.
				info.src = v;
				info.ref = false;
				info.lea = true;
			}

			return block;
		}

		static Nat pushParams(RemoveInvalid *tfm, Listing *dest, Array<ParamInfo> *params, Params *layout) {
			if (layout->stackCount() == 0)
				return 0;

			// Reserve space on the stack first.
			*dest << sub(ptrStack, ptrConst(layout->stackTotalSize()));

			// Now we can copy parameters! We need to be careful to not emit memory-memory moves, as
			// we are called inside "RemoveInvalid".

			Reg reg1 = ptrr(16);
			Reg reg2 = ptrr(17);

			for (Nat i = 0; i < layout->stackCount(); i++) {
				ParamInfo &info = params->at(layout->stackParam(i).id());
				Size sz = info.type->size();
				if (info.lea == info.ref) {
					Operand dst = xRel(sz, sp, Offset(layout->stackOffset(i)));
					if (info.src.type() == opRelative || info.src.type() == opVariable) {
						inlineMemcpy(dest, dst, info.src, reg1, reg2);
					} else if (info.src.type() == opConstant) {
						Reg r = asSize(reg1, sz);
						if (info.src.constant() > 0xFFFF) {
							*dest << mov(r, tfm->largeConstant(info.src));
						} else {
							*dest << mov(r, info.src);
						}
						*dest << mov(dst, r);
					} else {
						// We can copy it natively.
						*dest << mov(dst, info.src);
					}
				} else if (info.lea) {
					*dest << lea(reg1, info.src);
					*dest << mov(ptrRel(sp, Offset(layout->stackOffset(i))), reg1);
				} else if (info.ref) {
					Reg tmpReg = noReg;
					if (info.src.type() == opRegister) {
						tmpReg = info.src.reg();
					} else {
						TODO(L"Use a suitable temporary register!");
						// Note: We can probably use x8 since we set the result later. We should check though.
						tmpReg = ptrr(8);
						*dest << mov(tmpReg, info.src);
					}
					Operand dst = xRel(sz, sp, Offset(layout->stackOffset(i)));
					Operand src = xRel(sz, tmpReg, Offset(0));
					inlineMemcpy(dest, dst, src, reg1, reg2);
				}
			}

			return layout->stackTotalSize();
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
			// Recursion depth.
			Nat depth;
		};

		static void setRegister(RegEnv &env, Nat i);

		// Make sure any content inside 'reg' is used now, so that 'reg' can be reused for other purposes.
		static void vacateRegister(RegEnv &env, Reg reg) {
			for (Nat i = 0; i < env.layout->registerCount(); i++) {
				Param p = env.layout->registerAt(i);
				if (p.empty())
					continue;

				const Operand &src = env.src->at(p.id()).src;
				if (src.hasRegister() && same(src.reg(), reg)) {
					// We need to set this register now, otherwise it will be destroyed!
					if (env.active[i]) {
						// Cycle detected. If level is 1, then this just means that the data is
						// already in the right location, so we don't need to do anything.
						if (env.depth > 1) {
							// Cycle detected. Store it in x16 as a temporary and make a note of it.
							*env.dest << mov(asSize(ptrr(16), src.size()), src);
							env.active[i] = false;
						}
					} else {
						setRegister(env, i);
					}
				}
			}
		}

		// Set a register to what it is supposed to be, assuming 'src' is the actual value.
		static void setRegisterVal(RegEnv &env, Reg target, Param param, const Operand &src) {
			if (param.offset() == 0 && src.size().size64() <= 8) {
				if (src.type() == opRegister && same(src.reg(), target)) {
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
			assert(param.size().size64() == 8);
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
				// We need to ensure that the source is in a register. If it already is in a
				// register, use that. Otherwise, use the register we shall fill as a temporary.
				Reg tempReg = asSize(target, Size::sPtr);
				if (src.type() == opRegister)
					tempReg = src.reg();
				else
					*env.dest << mov(tempReg, src);

				Reg to = asSize(target, s);
				if (to == noReg) {
					// Unsupported size, upgrade to the next larger supported one.
					s += Size::sInt.alignment();
					to = asSize(target, s);
				}
				*env.dest << mov(to, xRel(s, tempReg, o));
			}
		}

		// Try to assign the proper value to a single register (other assignments might be performed
		// beforehand to vacate registers).
		static void setRegister(RegEnv &env, Nat i) {
			Param param = env.layout->registerAt(i);
			// Empty?
			if (param.empty())
				return;
			// Already done?
			if (env.finished[i])
				return;

			env.depth++;

			Reg target = env.layout->registerSrc(i);
			ParamInfo &p = env.src->at(param.id());

			// See if 'target' contains something that is used by other parameters.
			env.active[i] = true;
			vacateRegister(env, target);
			if (!env.active[i]) {
				// Stored in tmp register, update our knowledge of its source.
				p.src = asSize(ptrr(16), p.src.size());
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
			env.depth--;
		}

		static void setRegisters(Listing *dest, Array<ParamInfo> *src, Params *layout) {
			RegEnv env = {
				dest,
				src,
				layout,
				{ false },
				{ false },
				0,
			};

			for (Nat i = 0; i < layout->registerCount(); i++) {
				setRegister(env, i);
			}
		}

		static Operand preserveResultReg(Listing *to, Operand resultPos, Reg resultTempReg, RegSet *used) {
			Reg resultReg = resultPos.reg();
			if (isIntReg(resultReg) && intRegNumber(resultReg) >= 19) {
				// No need to preserve. Already safe.
				return resultPos;
			} else {
				used->remove(resultReg);
				used->put(resultTempReg);
				*to << mov(resultTempReg, resultReg);

				if (resultPos.type() == opRegister) {
					return Operand(resultTempReg);
				} else {
					return xRel(resultPos.size(), resultTempReg, resultPos.offset());
				}
			}
		}

		// Actual entry-point.
		void emitFnCall(RemoveInvalid *tfm, Listing *dest, Operand toCall, Operand resultPos, TypeDesc *resultType,
						Bool resultRef, Block currentBlock, RegSet *used, Array<ParamInfo> *params) {
			Engine &e = dest->engine();

			// Don't modify the RegSet, or our params.
			used = new (e) RegSet(*used);
			params = new (e) Array<ParamInfo>(*params);

			// Find a register to use for the result.
			Reg resultTempReg = noReg;
			for (Nat i = 19; i < 29; i++) {
				if (!used->has(ptrr(i))) {
					resultTempReg = ptrr(i);
					break;
				}
			}

			// If we are asked to store an empty result somewhere, we ignore that entirely.
			if (resultType->size() == Size()) {
				resultRef = false;
				resultPos = Operand();
			}

			// If we are asked to store the result in memory as indicated by a register, then we
			// need to preserve the register. We can store it in 'resultTempReg' if that is the
			// case. We must also ensure that the register is preserved when we emit parameter code.
			if (resultPos.type() == opRelative) {
				resultPos = preserveResultReg(dest, resultPos, resultTempReg, used);
			} else if (resultRef && resultPos.type() == opRegister) {
				resultPos = preserveResultReg(dest, resultPos, resultTempReg, used);
			}

			// Figure out parameter layout.
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

			// Start by copying parameters to memory as needed.
			Block block = copyToMemory(dest, used, params, paramLayout, currentBlock);

			// Put parameters onto the stack (if required).
			Nat extraStack = pushParams(tfm, dest, params, paramLayout);

			// Start copying parameters into registers.
			setRegisters(dest, params, paramLayout);

			// Set x8 to a pointer to the result if required.
			if (resultLayout->memory) {
				if (resultRef) {
					*dest << mov(ptrr(8), resultPos);
				} else {
					*dest << lea(ptrr(8), resultPos);
				}
			}

			// Call the function! (We don't need accurate information about registers, so it is OK to pass Size()).
			*dest << call(toCall, Size());

			// Restore stack if needed.
			if (extraStack > 0) {
				*dest << add(ptrStack, ptrConst(extraStack));
			}

			// Handle the result if required.
			if (!resultLayout->memory && resultPos != Operand()) {
				Reg srcReg1 = ptrr(0);
				Reg srcReg2 = ptrr(1);
				if (resultLayout->regType == primitive::real) {
					srcReg1 = dr(0);
					srcReg2 = dr(1);
				}

				Operand store = resultPos;
				if (resultRef) {
					Reg r = ptrr(2);
					if (store.type() == opRegister)
						r = store.reg();
					else
						*dest << mov(r, store);
					store = xRel(resultType->size(), r, Offset());
				}

				// Is it already in the right location?
				if (store.type() == opRegister && same(store.reg(), srcReg1)) {
					// Nothing needs to be done!
				} else {
					// We need to copy the result to memory.
					Nat resultSz = resultType->size().size64();
					if (resultSz <= 1) {
						*dest << mov(store, asSize(srcReg1, Size::sByte));
					} else if (resultSz <= 4) {
						*dest << mov(opOffset(Size::sInt, store, 0), asSize(srcReg1, Size::sInt));
					} else if (resultSz <= 8) {
						*dest << mov(opOffset(Size::sWord, store, 0), asSize(srcReg1, Size::sWord));
					} else {
						*dest << mov(opOffset(Size::sWord, store, 0), asSize(srcReg1, Size::sWord));
						*dest << mov(opOffset(Size::sWord, store, 8), asSize(srcReg2, Size::sWord));
					}
				}
			}

			// Disable the block if we used it.
			if (block != Block()) {
				// If no complex parameters, then we know that the block will only consist of simple
				// types that execute no destructors.
				if (!hasComplex(params)) {
					*dest << end(block);
				} else if (resultPos.type() == opRegister) {
					assert(resultTempReg != noReg, L"Failed to find a free register for storing function result.");
					Reg src = asSize(resultPos.reg(), Size::sPtr);
					*dest << mov(resultTempReg, src);
					*dest << end(block);
					*dest << mov(src, resultTempReg);
				} else {
					*dest << end(block);
				}
			}
		}

	}
}
