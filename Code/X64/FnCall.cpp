#include "stdafx.h"
#include "FnCall.h"
#include "Params.h"
#include "Asm.h"
#include "../Instr.h"
#include "../Exception.h"
#include "Utils/Bitwise.h"

namespace code {
	namespace x64 {

		ParamInfo::ParamInfo(TypeDesc *desc, const Operand &src, Bool ref)
			: type(desc), src(src), ref(ref), lea(false) {}

		ParamInfo::ParamInfo(TypeDesc *desc, const Operand &src, Bool ref, Bool lea)
			: type(desc), src(src), ref(ref), lea(lea) {}


		/**
		 * Parameters passed on the stack:
		 */

		// Push a value on the stack.
		static Nat pushValue(Listing *dest, const ParamInfo &p) {
			Nat size = p.type->size().size64();
			if (size <= 8) {
				*dest << push(p.src);
				return 8;
			}

			// We need to perform a memcpy-like operation (only for variables).
			if (p.src.type() != opVariable)
				throw new (dest) InvalidValue(S("Can not pass non-variables larger than 8 bytes to functions."));

			Var src = p.src.var();
			Nat pushed = 0;

			// Last part:
			Nat last = size & 0x07;
			size -= last; // Now 'size' is a multiple of 8.
			if (last == 0) {
				// Will be handled by the loop below.
			} else if (last == 1) {
				*dest << push(byteRel(src, Offset(size)));
				pushed += 8;
			} else if (last <= 4) {
				*dest << push(intRel(src, Offset(size)));
				pushed += 8;
			} else /* last < 8 */ {
				*dest << push(longRel(src, Offset(size)));
				pushed += 8;
			}

			while (size >= 8) {
				size -= 8;
				*dest << push(longRel(src, Offset(size)));
				pushed += 8;
			}

			return pushed;
		}

		// Push a pointer to a value onto the stack.
		static Nat pushLea(Listing *dest, const ParamInfo &p) {
			*dest << push(ptrA);
			*dest << lea(ptrA, p.src);
			*dest << swap(ptrA, ptrRel(ptrStack, Offset()));
			return 8;
		}

		// Push a value to the stack, the address is given in 'p.src'.
		static Nat pushRef(Listing *dest, const ParamInfo &p) {
			Nat size = p.type->size().size64();
			Nat bytesPushed = roundUp(size, Nat(8));

			// Save 'ptrA' a bit below the stack (safe as long as 'push + 8 <= 128', which should be OK).
			*dest << mov(longRel(ptrStack, -Offset(bytesPushed + 8)), rax);

			// Load the old 'ptrA'.
			*dest << mov(ptrA, p.src);

			// Last part:
			Nat last = size & 0x07;
			size -= last; // Now 'size' is a multiple of 8.
			if (last == 0) {
				// Will be handled by the loop below.
			} else if (last == 1) {
				*dest << push(byteRel(ptrA, Offset(size)));
			} else if (last <= 4) {
				*dest << push(intRel(ptrA, Offset(size)));
			} else /* last < 8 */ {
				*dest << push(longRel(ptrA, Offset(size)));
			}

			while (size >= 8) {
				size -= 8;
				*dest << push(longRel(ptrA, Offset(size)));
			}

			// Restore the old value of 'rax'.
			*dest << mov(rax, longRel(ptrStack, -Offset(8)));
			return bytesPushed;
		}

		// Push parameters to the stack. Returns the total number of bytes pushed to the stack.
		static Nat pushParams(Listing *dest, Array<ParamInfo> *src, Params *layout) {
			// Note: It might be easier to pre-allocate the size of the stack, and then simply memcpy data.

			Nat pushed = 0;
			Nat size = layout->stackTotalSizeUnaligned();
			if (size & 0x0F) {
				// We need to push an additional word to the stack to keep alignment.
				*dest << push(natConst(0));
				pushed += 8;
				size += 8;
			}

			// Push the parameters.
			for (Nat i = layout->stackCount(); i > 0; i--) {
				const ParamInfo &p = src->at(layout->stackParam(i - 1).id());
				if (p.ref == p.lea) {
					pushed += pushValue(dest, p);
				} else if (p.ref) {
					pushed += pushRef(dest, p);
				} else if (p.lea) {
					pushed += pushLea(dest, p);
				}
			}

			assert(pushed == size, L"Failed to push some parameters to the stack.");
			return pushed;
		}

		/**
		 * Parameters passed in registers.
		 */

		// Parameters passed around while assigning contents to registers.
		struct RegEnv {
			// Output listing.
			Listing *dest;
			// All parameters.
			Array<ParamInfo> *src;
			// The layout we want to produce.
			Params *layout;
			// Currently computing an assignment?
			Bool active[16];
			// Finished assigning a register?
			Bool finished[16];
			// Recursion depth.
			Nat depth;

			ParamInfo &info(Nat id) {
				if (id == Param::returnId())
					return src->last();
				else
					return src->at(id);
			}
		};

		static void setRegister(RegEnv &env, Nat i);

		// Make sure any content inside 'reg' is used now, so that 'reg' can be reused for other purposes.
		static void vacateRegister(RegEnv &env, Reg reg) {
			for (Nat i = 0; i < env.layout->registerCount(); i++) {
				Param p = env.layout->registerParam(i);
				if (p == Param())
					continue;

				const Operand &src = env.info(p.id()).src;
				if (src.hasRegister() && same(src.reg(), reg)) {
					// We need to set this register now, otherwise it will be destroyed!
					if (env.active[i]) {
						// Cycle detected. If level is 1, then this just means that the data is
						// already in the right location, so we don't need to do anything.
						if (env.depth > 1) {
							// Cycle detected. Push the register we should vacate onto the stack and
							// keep a note about that for later.
							*env.dest << push(src);
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
			assert(param.size().size64() == 8);
			*env.dest << lea(asSize(target, Size::sPtr), src);
		}

		// Set a register to what it is supposed to be, assuming 'src' is a pointer to the actual value.
		static void setRegisterRef(RegEnv &env, Reg target, Param param, const Operand &src) {
			assert(src.size() == Size::sPtr);
			Size s(param.size());
			Offset o(param.offset());

			// If 'target' is a floating-point register, we can't use that as a temporary.
			if (fpRegister(target)) {
				// However, since they are always assigned last, we know we can use ptr10, as that
				// will be clobbered by the function call anyway.
				*env.dest << mov(ptr10, src);
				*env.dest << mov(asSize(target, s), xRel(s, ptr10, o));
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
			Param param = env.layout->registerParam(i);
			// Empty?
			if (param == Param())
				return;
			// Already done?
			if (env.finished[i])
				return;

			env.depth++;

			Reg target = env.layout->registerSrc(i);
			ParamInfo &p = env.info(param.id());

			// See if 'target' contains something that is used by other parameters.
			env.active[i] = true;
			vacateRegister(env, target);
			if (!env.active[i]) {
				// This register is stored on the stack for now. Restore it before we continue!
				p.src = Operand(asSize(target, p.src.size()));
				*env.dest << pop(p.src);
			}
			env.active[i] = false;

			// Set the register.
			if (p.ref == p.lea)
				setRegisterVal(env, target, param, p.src);
			else if (p.ref)
				setRegisterRef(env, target, param, p.src);
			else if (p.lea)
				setRegisterLea(env, target, param, p.src);

			// Note that we're done.
			env.finished[i] = true;
			env.depth--;
		}

		// Set all registers to their proper values for a function call.
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


		/**
		 * Complex parameters.
		 */

		// Find registers we need to preserve while calling constructors. We assume that 'used' contains
		// all registers in the parameters. Modifies 'used'.
		static void preserveComplex(Listing *dest, RegSet *used, Block block, Array<ParamInfo> *params) {
			// Remove 'used' for the first parameter.
			for (Nat i = 0; i < params->count(); i++) {
				ParamInfo &param = params->at(i);

				if (as<ComplexDesc>(param.type) != null) {
					// We do not need to preserve anything required by the first complex
					// parameter. It will manage anyway!
					if (param.src.hasRegister())
						used->remove(param.src.reg());
					break;
				}
			}

			// Add the dirty registers to the mix.
			RegSet *dirty = new (dest) RegSet();
			for (size_t i = 0; i < fnDirtyCount; i++) {
				dirty->put(fnDirtyRegs[i]);
				used->put(fnDirtyRegs[i]);
			}

			// Move things around!
			Bool firstComplex = true;
			for (Nat i = 0; i < params->count(); i++) {
				ParamInfo &param = params->at(i);

				if (as<ComplexDesc>(param.type) != null && firstComplex) {
					// We do not need to preserve anything required by the first complex
					// parameter. It will manage anyway!
					firstComplex = false;
					continue;
				}

				if (!param.src.hasRegister())
					continue;

				Reg srcReg = param.src.reg();
				if (!dirty->has(srcReg))
					// No need to preserve.
					continue;

				Reg into = unusedRegUnsafe(used);
				if (into == noReg) {
					// No more registers. Create a variable!
					Var v = dest->createVar(block, param.src.size());
					*dest << mov(v, param.src);
					param.src = v;
				} else {
					// Put it in 'into' instead!
					into = asSize(into, param.src.size());
					*dest << mov(into, param.src);
					param.src = into;
					used->put(into);
				}
			}
		}

		static Block copyComplex(Listing *dest, RegSet *used, Array<ParamInfo> *params, Block currentBlock) {
			Block block = dest->createBlock(currentBlock);

			// Add registers from parameters to 'used'.
			used = new (used) RegSet(*used);
			for (Nat i = 0; i < params->count(); i++) {
				ParamInfo &param = params->at(i);
				if (param.src.hasRegister())
					used->put(param.src.reg());
			}

			// We need to tell the next stage what register is free to use!
			*dest << begin(dest->engine(), block)->alterDest(asSize(unusedReg(used), Size::sLong));

			// Find registers we need to preserve while calling constructors.
			preserveComplex(dest, used, block, params);

			for (Nat i = 0; i < params->count(); i++) {
				ParamInfo &param = params->at(i);

				if (ComplexDesc *c = as<ComplexDesc>(param.type)) {
					Var v = dest->createVar(block, c, freeDef | freeInactive);

					// Call the copy constructor.
					*dest << lea(ptrDi, v);
					if (param.ref == param.lea) {
						*dest << lea(ptrSi, param.src);
					} else if (param.ref) {
						*dest << mov(ptrSi, param.src);
					} else {
						assert(false, L"Can not use the 'lea'-mode for complex parameters.");
					}
					*dest << call(c->ctor, Size());
					*dest << activate(v);

					// Modify the parameter so that we use the newly created parameter.
					param.src = v;
					param.ref = false;
					param.lea = true;
				}
			}

			return block;
		}

		/**
		 * Misc. helpers.
		 */

		// Do 'param' contain any complex parameters?
		static bool hasComplex(Array<ParamInfo> *params) {
			for (Nat i = 0; i < params->count(); i++)
				if (as<ComplexDesc>(params->at(i).type))
					return true;
			return false;
		}

		/**
		 * The actual entry-point.
		 */

		void emitFnCall(Listing *dest, Operand toCall, Operand resultPos, TypeDesc *resultType,
						Bool resultRef, Block currentBlock, RegSet *used, Array<ParamInfo> *params) {

			Block block;
			bool complex = hasComplex(params);
			Params *layout = new (dest) Params();
			layout->result(resultType);
			for (Nat i = 0; i < params->count(); i++)
				layout->add(i, params->at(i).type);

			Result result = layout->result();

			// Copy 'used' so we do not alter our callers version.
			used = new (used) RegSet(*used);

			// Is the result parameter in a register that needs to be preserved?
			if (resultRef && resultPos.type() == opRegister) {
				Bool resultDirty = false;
				for (size_t i = 0; i < fnDirtyCount; i++) {
					if (same(fnDirtyRegs[i], resultPos.reg())) {
						resultDirty = true;
						break;
					}
				}

				if (resultDirty) {
					// We need to preserve it somewhere. Pick any of the registers that need to be
					// preserved separately.
					static const Reg alternatives[] = { ptrB, ptr12, ptr14, ptr15 };
					Reg to = noReg;
					for (Nat i = 0; i < ARRAY_COUNT(alternatives); i++) {
						if (!used->has(alternatives[i])) {
							to = asSize(alternatives[i], Size::sPtr);
							break;
						}
					}

					used->put(to);
					*dest << mov(to, resultPos);
					resultPos = to;
				}
			}

			if (result.memoryRegister() != noReg) {
				if (resultRef) {
					params->push(ParamInfo(ptrDesc(dest->engine()), resultPos, false, false));
				} else {
					params->push(ParamInfo(ptrDesc(dest->engine()), resultPos, false, true));
				}
			}

			// Create copies of complex parameters (inside a block) if needed.
			if (complex)
				block = copyComplex(dest, used, params, currentBlock);

			// Push parameters on the stack. This is a 'safe' operation since it does not destroy any registers.
			Nat pushed = pushParams(dest, params, layout);

			// Assign parameters to registers.
			setRegisters(dest, params, layout);

			// Call the function (we do not need accurate knowledge of dirty registers from here).
			*dest << call(toCall, Size());

			// Handle the return value if required.
			if (result.memoryRegister() != noReg) {
				// No need to do anything, the callee wrote the result in the right place for us.
				// Also: anything that fits in a register is passed in registers, so we never have to
				// read the register from memory.
			} else if (result.registerCount() > 0) {
				if (resultRef) {
					*dest << mov(ptrSi, resultPos);
					resultPos = xRel(resultType->size(), ptrSi, Offset());
				}

				if (result.registerCount() == 1 && resultPos.type() == opRegister) {
					if (!same(result.registerAt(0), resultPos.reg())) {
						*dest << mov(resultPos, result.registerAt(0));
					}
				} else {
					for (Nat i = 0; i < result.registerCount(); i++) {
						Reg r = result.registerAt(i);
						*dest << mov(opOffset(size(r), resultPos, result.registerOffset(i).v64()), r);
					}
				}
			}

			// Pop the stack.
			if (pushed > 0)
				*dest << add(ptrStack, ptrConst(pushed));

			if (complex) {
				const Operand &target = resultPos;
				if (target.type() == opRegister) {
					// 'r15' should be free now. It is not exposed outside of the backend.
					*dest << mov(asSize(r15, target.size()), target);
					*dest << end(block);
					*dest << mov(target, asSize(r15, target.size()));
				} else {
					*dest << end(block);
				}
			}
		}

	}
}
