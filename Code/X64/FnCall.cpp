#include "stdafx.h"
#include "FnCall.h"
#include "Params.h"
#include "Asm.h"
#include "Arena.h"
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
		 * Common context for the function call mechanisms.
		 */
		class FnCallState {
		public:
			// Create.
			FnCallState(const Arena *arena, Listing *dest, Array<ParamInfo> *params, RegSet *used, Block parent, Bool member)
				: arena(arena), dest(dest), params(params), parent(parent), created() {
				this->used = new (used) RegSet(*used);
				this->layout = arena->createParams(member);
			}

			// Arena.
			const Arena *arena;

			// Destination listing.
			Listing *dest;

			// Parameters (note: we don't copy it, so that we get an accurate version when initializing)
			Array<ParamInfo> *params;

			// Parent block.
			Block parent;

			// Used registers.
			RegSet *used;

			// Layout.
			Params *layout;


			// Get the block created for this call. Initializes it if necessary.
			Block block() {
				if (created != Block())
					return created;

				created = dest->createBlock(parent);

				// We need to tell the next stage which register is free to use.
				*dest << begin(dest->engine(), created)->alterDest(asSize(findFreeReg(), Size::sLong));

				return created;
			}

			// Find a temporary register to use:
			Reg findFreeReg() {
				RegSet *tmp = new (dest) RegSet(*used);
				for (Nat i = 0; i < params->count(); i++) {
					ParamInfo &param = params->at(i);
					if (param.src.hasRegister())
						tmp->put(param.src.reg());
				}

				return unusedReg(tmp);
			}

			// Check if we have created a block.
			Bool blockCreated() const {
				return created != Block();
			}

		private:
			// Created block, if any.
			Block created;

			FnCallState(const FnCallState &);
			FnCallState &operator =(const FnCallState &);
		};


		/**
		 * Parameters passed on the stack. Old implementation relying on manipulating rsp.
		 */

		// // Push a value on the stack.
		// static Nat pushValue(Listing *dest, const ParamInfo &p) {
		// 	Nat size = p.type->size().size64();
		// 	if (size <= 8) {
		// 		*dest << push(p.src);
		// 		return 8;
		// 	}

		// 	// We need to perform a memcpy-like operation (only for variables).
		// 	if (p.src.type() != opVariable)
		// 		throw new (dest) InvalidValue(S("Can not pass non-variables larger than 8 bytes to functions."));

		// 	Var src = p.src.var();
		// 	Nat pushed = 0;

		// 	// Last part:
		// 	Nat last = size & 0x07;
		// 	size -= last; // Now 'size' is a multiple of 8.
		// 	if (last == 0) {
		// 		// Will be handled by the loop below.
		// 	} else if (last == 1) {
		// 		*dest << push(byteRel(src, Offset(size)));
		// 		pushed += 8;
		// 	} else if (last <= 4) {
		// 		*dest << push(intRel(src, Offset(size)));
		// 		pushed += 8;
		// 	} else /* last < 8 */ {
		// 		*dest << push(longRel(src, Offset(size)));
		// 		pushed += 8;
		// 	}

		// 	while (size >= 8) {
		// 		size -= 8;
		// 		*dest << push(longRel(src, Offset(size)));
		// 		pushed += 8;
		// 	}

		// 	return pushed;
		// }

		// // Push a pointer to a value onto the stack.
		// static Nat pushLea(Listing *dest, const ParamInfo &p) {
		// 	*dest << push(ptrA);
		// 	*dest << lea(ptrA, p.src);
		// 	*dest << swap(ptrA, ptrRel(ptrStack, Offset()));
		// 	return 8;
		// }

		// // Push a value to the stack, the address is given in 'p.src'.
		// static Nat pushRef(Listing *dest, const ParamInfo &p) {
		// 	Nat size = p.type->size().size64();
		// 	Nat bytesPushed = roundUp(size, Nat(8));

		// 	// Save 'ptrA' a bit below the stack (safe as long as 'push + 8 <= 128', which should be OK).
		// 	*dest << mov(longRel(ptrStack, -Offset(bytesPushed + 8)), rax);

		// 	// Load the old 'ptrA'.
		// 	*dest << mov(ptrA, p.src);

		// 	// Last part:
		// 	Nat last = size & 0x07;
		// 	size -= last; // Now 'size' is a multiple of 8.
		// 	if (last == 0) {
		// 		// Will be handled by the loop below.
		// 	} else if (last == 1) {
		// 		*dest << push(byteRel(ptrA, Offset(size)));
		// 	} else if (last <= 4) {
		// 		*dest << push(intRel(ptrA, Offset(size)));
		// 	} else /* last < 8 */ {
		// 		*dest << push(longRel(ptrA, Offset(size)));
		// 	}

		// 	while (size >= 8) {
		// 		size -= 8;
		// 		*dest << push(longRel(ptrA, Offset(size)));
		// 	}

		// 	// Restore the old value of 'rax'.
		// 	*dest << mov(rax, longRel(ptrStack, -Offset(8)));
		// 	return bytesPushed;
		// }

		// // Push parameters to the stack. Returns the total number of bytes pushed to the stack.
		// static Nat pushParams(Listing *dest, Array<ParamInfo> *src, Params *layout) {
		// 	// Note: It might be easier to pre-allocate the size of the stack, and then simply memcpy data.

		// 	Nat pushed = 0;
		// 	Nat size = layout->stackTotalSizeUnaligned();
		// 	if (size & 0x0F) {
		// 		// We need to push an additional word to the stack to keep alignment.
		// 		*dest << push(natConst(0));
		// 		pushed += 8;
		// 		size += 8;
		// 	}

		// 	// Push the parameters.
		// 	for (Nat i = layout->stackCount(); i > 0; i--) {
		// 		const ParamInfo &p = src->at(layout->stackParam(i - 1).id());
		// 		if (p.ref == p.lea) {
		// 			pushed += pushValue(dest, p);
		// 		} else if (p.ref) {
		// 			pushed += pushRef(dest, p);
		// 		} else if (p.lea) {
		// 			pushed += pushLea(dest, p);
		// 		}
		// 	}

		// 	assert(pushed == size, L"Failed to push some parameters to the stack.");
		// 	return pushed;
		// }


		/**
		 * Parameters passed on the stack. New implementation allocating stack space upfront.
		 */

		// Store a value on the stack.
		static void storeStackValue(Listing *dest, Reg tmpReg, Nat offset, const ParamInfo &p) {
			Size size = p.type->size();
			Nat nSize = size.size64();
			if (nSize <= 8) {
				Reg r = asSize(tmpReg, size);
				if (p.src.type() == opRegister)
					r = p.src.reg();
				else
					*dest << mov(r, p.src);

				*dest << mov(xRel(size, ptrStack, Offset(offset)), r);
				return;
			}

			if (p.src.type() != opVariable)
				throw new (dest) InvalidValue(S("Can not pass non-variables larger than 8 bytes to functions."));

			Var src = p.src.var();

			Reg large = asSize(tmpReg, Size::sLong);
			Nat pos = 0;
			while (pos + 8 <= nSize) {
				*dest << mov(large, longRel(src, Offset(pos)));
				*dest << mov(longRel(ptrStack, Offset(offset + pos)), large);
				pos += 8;
			}

			if (pos + 1 >= nSize) {
				Reg r = asSize(tmpReg, Size::sByte);
				*dest << mov(r, byteRel(src, Offset(pos)));
				*dest << mov(byteRel(ptrStack, Offset(offset + pos)), r);
				pos += 1;
			} else if (pos + 4 >= nSize) {
				Reg r = asSize(tmpReg, Size::sInt);
				*dest << mov(r, intRel(src, Offset(pos)));
				*dest << mov(intRel(ptrStack, Offset(offset + pos)), r);
				pos += 4;
			}
		}

		static Reg loadAddr(Listing *dest, Reg tmpReg, const Operand &src) {
			if (src.type() == opRegister)
				return src.reg();

			tmpReg = asSize(tmpReg, Size::sPtr);
			*dest << mov(tmpReg, src);
			return tmpReg;
		}

		// Store a value on the stack, from a reference to the value.
		static void storeStackRef(Listing *dest, Reg tmpReg, Nat offset, const ParamInfo &p) {
			Size size = p.type->size();
			Nat nSize = size.size64();
			if (nSize <= 8) {
				Reg src = loadAddr(dest, tmpReg, p.src);
				*dest << mov(asSize(tmpReg, size), xRel(size, src, Offset()));
				*dest << mov(xRel(size, ptrStack, Offset(offset)), asSize(tmpReg, size));
				return;
			}

			// TODO: In cases where we have additional temorary registers, we can eliminate extra
			// load operations.

			Reg large = asSize(tmpReg, Size::sLong);
			Nat pos = 0;
			while (pos + 8 <= nSize) {
				Reg addr = loadAddr(dest, tmpReg, p.src);
				*dest << mov(large, longRel(addr, Offset(pos)));
				*dest << mov(longRel(ptrStack, Offset(offset + pos)), large);
				pos += 8;
			}

			if (pos < nSize && nSize - pos <= 1) {
				Reg addr = loadAddr(dest, tmpReg, p.src);
				Reg r = asSize(tmpReg, Size::sByte);
				*dest << mov(r, byteRel(addr, Offset(pos)));
				*dest << mov(byteRel(ptrStack, Offset(offset + pos)), r);
				pos += 1;
			} else if (pos < nSize && nSize - pos <= 4) {
				Reg addr = loadAddr(dest, tmpReg, p.src);
				Reg r = asSize(tmpReg, Size::sInt);
				*dest << mov(r, intRel(addr, Offset(pos)));
				*dest << mov(intRel(ptrStack, Offset(offset + pos)), r);
				pos += 1;
			}
		}

		// Store a reference to a parameter on the stack.
		static void storeStackLea(Listing *dest, Reg tmpReg, Nat offset, const ParamInfo &p) {
			tmpReg = asSize(tmpReg, Size::sPtr);
			*dest << lea(tmpReg, p.src);
			*dest << mov(ptrRel(ptrStack, Offset(offset)), tmpReg);
		}

		// Store stack parameters. As we are not allowed to modify the stack pointer on Windows, we
		// instead allocate a variable at the end of the current block. This means we can be sure
		// that we have enough space reserved on the stack to simply write the parameters in memory
		// where they need to be without worrying.
		// This function must be careful not to trash registers which contain parameters.
		static void storeStackParams(FnCallState &state) {
			Nat totalSize = state.layout->stackTotalSize();
			if (totalSize == 0)
				return;

			Block block = state.block();

			// Create a variable with the appropriate size (no need to initialize it, we will never
			// use it per se, it is only there to ensure that we have enough free space at the end
			// of the stack).
			state.dest->createVar(block, Size(totalSize), Operand(), freeNoInit);

			Reg tmpReg = state.findFreeReg();
			// TODO: Currently, the 'findFreeReg' above throws if no register is available.
			// We could instead either:
			// - store a parameter that should be on the stack in memory, and use that register
			//   (possible since we can store parameters in any order)
			// - use the shadow space if we are on Windows to spill (or allocate more memory
			//   above to allow spilling).

			// Fill the stack right to left for consistency with 'push'-based approaches.
			for (Nat i = state.layout->stackCount(); i > 0; i--) {
				Nat offset = state.layout->stackOffset(i - 1);
				const ParamInfo &p = state.params->at(state.layout->stackParam(i - 1).id());
				if (p.ref == p.lea) {
					storeStackValue(state.dest, tmpReg, offset, p);
				} else if (p.ref) {
					storeStackRef(state.dest, tmpReg, offset, p);
				} else if (p.lea) {
					storeStackLea(state.dest, tmpReg, offset, p);
				}
			}
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
		// all registers in the parameters.
		static void preserveComplex(FnCallState &state, Block block) {
			RegSet *used = new (state.used) RegSet(*state.used);

			// Compute "used" for all but the first complex parameter.
			for (Nat i = 0; i < state.params->count(); i++) {
				ParamInfo &param = state.params->at(i);

				if (as<ComplexDesc>(param.type) != null)
					continue;

				if (param.src.hasRegister())
					used->put(param.src.reg());
			}

			// Add the dirty registers to the mix.
			const RegSet *dirty = state.arena->dirtyRegs;
			used->put(dirty);

			// Move things around!
			Bool firstComplex = true;
			for (Nat i = 0; i < state.params->count(); i++) {
				ParamInfo &param = state.params->at(i);

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
					Var v = state.dest->createVar(block, param.src.size());
					*state.dest << mov(v, param.src);
					param.src = v;
				} else {
					// Put it in 'into' instead!
					into = asSize(into, param.src.size());
					*state.dest << mov(into, param.src);
					param.src = into;
					used->put(into);
				}
			}
		}

		static bool hasComplex(Array<ParamInfo> *params) {
			for (Nat i = 0; i < params->count(); i++)
				if (as<ComplexDesc>(params->at(i).type))
					return true;
			return false;
		}

		static void copyComplex(FnCallState &state) {
			if (!hasComplex(state.params))
				return;

			Block currentBlock = state.block();

			// Find registers we need to preserve while calling constructors.
			preserveComplex(state, currentBlock);

			for (Nat i = 0; i < state.params->count(); i++) {
				ParamInfo &param = state.params->at(i);

				if (ComplexDesc *c = as<ComplexDesc>(param.type)) {
					FreeOpt freeOpt = freeInactive | freeOnException;
					// Consider if we need to destroy parameters:
					if (!state.layout->calleeDestroyParams())
						freeOpt |= freeOnBlockExit;

					Var v = state.dest->createVar(currentBlock, c, freeOpt);
					TODO(L"On 64-bit Windows, these parameters might need to be 16-byte aligned.");

					// Call the copy constructor.
					*state.dest << lea(state.layout->registerSrc(0), v);
					if (param.ref == param.lea) {
						*state.dest << lea(state.layout->registerSrc(1), param.src);
					} else if (param.ref) {
						*state.dest << mov(state.layout->registerSrc(1), param.src);
					} else {
						assert(false, L"Can not use the 'lea'-mode for complex parameters.");
					}
					*state.dest << call(c->ctor, Size());
					*state.dest << activate(v);

					// Modify the parameter so that we use the newly created parameter.
					param.src = v;
					param.ref = false;
					param.lea = true;
				}
			}
		}

		static void loadOffset(Listing *dest, Reg tmpReg, Nat offset, const ParamInfo &param, Size size) {
			Reg out = asSize(tmpReg, size);

			if (param.ref == param.lea) {
				switch (param.src.type()) {
				case opRegister:
					assert(offset == 0);
					*dest << mov(out, asSize(param.src.reg(), size));
					break;
				case opRelative:
					*dest << mov(out, xRel(size, param.src.reg(), param.src.offset() + Offset(offset)));
					break;
				case opVariable:
					*dest << mov(out, xRel(size, param.src.var(), param.src.offset() + Offset(offset)));
					break;
				default:
					assert(false);
				}
			} else if (param.ref) {
				Reg addr;
				if (param.src.type() == opRegister) {
					addr = param.src.reg();
				} else {
					addr = asSize(tmpReg, Size::sPtr);
					*dest << mov(addr, param.src);
				}

				*dest << mov(out, xRel(size, addr, Offset(offset)));
			} else {
				assert(false, L"Can not use the 'lea'-mode for parameters passed in memory.");
			}
		}

		static void copySimple(FnCallState &state, Param p) {
			if (p.empty())
				return;

			if (!p.inMemory())
				return;

			if (p.id() == Param::returnId())
				return;

			ParamInfo &info = state.params->at(p.id());

			// Complex parameters are already handled!
			if (as<ComplexDesc>(info.type))
				return;

			// OK, we found a parameter to copy! Create a variable and copy it! It is simple, so we
			// don't need to worry about copy-ctors and dtors!
			Var v = state.dest->createVar(state.block(), info.type, freeOnNone);
			TODO(L"On 64-bit windows, these parameters might need to be 16-byte aligned!");

			Reg tmpReg = asSize(state.findFreeReg(), Size::sLong);
			Nat size = info.type->size().size64();

			Nat offset = 0;
			while (offset + 8 <= size) {
				loadOffset(state.dest, tmpReg, offset, info, Size::sLong);
				*state.dest << mov(longRel(v, Offset(offset)), tmpReg);
				offset += 8;
			}

			if (offset < size && size - offset <= 1) {
				loadOffset(state.dest, tmpReg, offset, info, Size::sByte);
				*state.dest << mov(byteRel(v, Offset(offset)), asSize(tmpReg, Size::sByte));
				offset += 1;
			} else if (offset < size && size - offset <= 4) {
				loadOffset(state.dest, tmpReg, offset, info, Size::sInt);
				*state.dest << mov(intRel(v, Offset(offset)), asSize(tmpReg, Size::sInt));
				offset += 4;
			}

			info.src = v;
			info.ref = false;
			info.lea = true;
		}

		static void copySimple(FnCallState &state) {
			for (Nat i = 0; i < state.layout->totalCount(); i++)
				copySimple(state, state.layout->totalParam(i));
		}

		/**
		 * The actual entry-point.
		 */

		void emitFnCall(const Arena *arena, Listing *dest, Operand toCall, Operand resultPos, TypeDesc *resultType,
						Bool member, Bool resultRef, Block currentBlock, RegSet *used, Array<ParamInfo> *params) {

			// Shared state for other functions:
			FnCallState state(arena, dest, params, used, currentBlock, member);
			bool complex = hasComplex(params);

			state.layout->result(resultType);
			for (Nat i = 0; i < params->count(); i++)
				state.layout->add(i, params->at(i).type);

			Result result = state.layout->result();

			// Is the result parameter in a register that needs to be preserved?
			if (resultRef && resultPos.type() == opRegister) {
				Bool resultDirty = arena->dirtyRegs->has(resultPos.reg());

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
			copyComplex(state);

			// Create copies for simple parameters that are to be passed by pointer.
			copySimple(state);

			// Push parameters on the stack. Needs to preserve registers.
			storeStackParams(state);

			// Assign parameters to registers.
			setRegisters(dest, params, state.layout);

			// If it is the callee's responsibility to destroy parameters, then we know that no
			// destructors will be executed by the 'end(block)' operation. As the X64 backend uses
			// table-based exceptions, it will not even destroy any registers (it just emits a
			// label). As such, it is fine to just emit it and be happy.
			if (state.blockCreated() && state.layout->calleeDestroyParams()) {
				*dest << end(state.block());
			}

			// Call the function (we do not need accurate knowledge of dirty registers from here).
			*dest << call(toCall, Size());

			// Handle the return value if required.
			if (result.memoryRegister() != noReg) {
				// No need to do anything, the callee wrote the result in the right place for us.
				// Also: anything that fits in a register is passed in registers, so we never have to
				// read the register from memory.
			} else if (result.registerCount() > 0) {
				if (resultRef) {
					Reg reg = state.layout->registerSrc(1);
					*dest << mov(reg, resultPos);
					resultPos = xRel(resultType->size(), reg, Offset());
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

			// If it is our responsibility to destroy parameters, then we need to be a bit more
			// careful. If the result is in a register, we need to preserve it since destructors
			// might be executed.
			if (state.blockCreated() && !state.layout->calleeDestroyParams()) {
				const Operand &target = resultPos;

				bool needProtection = target.type() == opRegister;
				if (needProtection) {
					// See if any destructors will be executed. If not, then we are safe anyway.
					needProtection = false;
					Array<Var> *vars = dest->allVars(state.block());
					for (Nat i = 0; i < vars->count(); i++) {
						if (dest->freeOpt(vars->at(i)) & freeOnBlockExit) {
							needProtection = true;
							break;
						}
					}
				}

				if (needProtection) {
					// 'r15' should be free now. It is not exposed outside of the backend.
					*dest << mov(asSize(r15, target.size()), target);
					*dest << end(state.block());
					*dest << mov(target, asSize(r15, target.size()));
				} else {
					*dest << end(state.block());
				}
			}
		}

	}
}
