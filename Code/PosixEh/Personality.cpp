#include "stdafx.h"
#include "Personality.h"
#include "FnState.h"
#include "Thunk.h"
#include "Code/Binary.h"
#include "Code/Dwarf/Registers.h"

#ifdef POSIX
#include <cxxabi.h>

#ifndef GCC
#error "This is currently untested on compilers other than GCC"
#endif

#ifdef __USING_SJLJ_EXCEPTIONS__
#error "Storm does not support SJLJ exceptions. Please use DWARF exceptions instead!"
#endif

#ifdef __ARM_EABI_UNWINDER__
#error "ARM eabi unwinder is currently not supported!"
#endif

namespace code {
	namespace eh {

		/**
		 * Description of the data at the end of each function.
		 */
		struct FnData {
			// Number of entries in the block table.
			size_t blockCount;
		};

		/**
		 * Description of a single entry in the block table.
		 */
		struct FnBlock {
			// At which offset do we start?
			Nat offset;

			// Which block?
			Nat block;
		};

		// Get the FnData from a function.
		static const FnData *getData(const void *fn) {
			size_t size = runtime::codeSize(fn);
			const byte *end = (const byte *)fn + size;
			return (const FnData *)(end - sizeof(FnData));
		}

		// Get the block data from a function.
		static const FnBlock *getBlocks(const FnData *data) {
			const FnBlock *end = (const FnBlock *)data;
			return end - data->blockCount;
		}

		// Compare object.
		struct BlockCompare {
			bool operator()(Nat offset, const FnBlock &block) const {
				return offset < block.offset;
			}
		};

		// Our representation of a stack frame.
		class StackFrame : public code::StackFrame {
		public:
			StackFrame(Nat block, Nat activation, void *rbp) : code::StackFrame(block, activation), rbp((byte *)rbp) {}

			virtual void *toPtr(int offset) {
				return rbp + offset;
			}

		private:
			byte *rbp;
		};

		// Find the currently active block and active ID.
		static Nat stormFindBlockActive(const FnData *data, size_t fn, size_t pc) {
			const FnBlock *blocks = getBlocks(data);

			Nat offset = pc - fn;
			Nat invalid = Block().key();

			// The entries are sorted by their 'offset' member. We can perform a binary search!
			// Note: if there are multiple elements that are equal, we need to pick the last one.
			const FnBlock *found = std::upper_bound(blocks, blocks + data->blockCount, offset, BlockCompare());
			if (found == blocks) {
				// Before any block, nothing to do!
				return invalid;
			}
			found--;

			return found->block;
		}

		// Storage from phase 1 to phase 2. Binary compatible with blocks of __cxa_exception in GCC.
		// Names from the GCC implementation for simplicity.

		/**
		 * GCC private exception struct. We need to be able to access various members in here during
		 * the exception handling. More specifically:
		 *
		 * - 'exceptionType' in 'isStormException' to be able to check the type of exception that was thrown.
		 * - 'adjustedPtr' in the personality function to store the resulting exception object in
		 *   phase 1. Otherwise __cxa_begin_catch() does not return the proper value in phase 2 of the
		 *   exception handling.
		 * - 'handlerSwitchValue' in the personality function to store the current block, so we don't
		 *   have to re-compute it in phase 2. This is not important for the implementation to function,
		 *   but GCC does something similar, so why not do the same and gain the small performance boost?
		 * - 'catchTemp' in the personality function to store the location we shall resume from so we
		 *   don't have to re-compute it in phase 2. Not important for the implementation to function,
		 *   but GCC does something similar. It is potentially dangerous to store a GC pointer in this
		 *   structure, as it is not scanned (it is allocated separately using malloc). However, during
		 *   the time the pointer is stored in here, a pointer into the same code block is pinned by
		 *   being on the execution stack anyway, so it is fine in this case.
		 * - 'actionRecord' and 'languageSpecificData' are seemingly free to use. We don't use them.
		 */
		struct ExStore {
			std::type_info *exceptionType;
			void *exceptionDestructor;

			std::unexpected_handler unexpectedHandler;
			std::terminate_handler terminateHandler;

			void *nextException;
			int handlerCount;
#ifdef __ARM_EABI_UNWINDER__
			// For some ARM architectures:
			ExStore* nextPropagatingException;
			int propagationCount;
#else
			int handlerSwitchValue;
			const byte *actionRecord;
			const byte *languageSpecificData;
			void *catchTemp;
			void *adjustedPtr;
#endif

			struct _Unwind_Exception exception;
		};

		static RootObject *isStormException(_Unwind_Exception_Class type, struct _Unwind_Exception *data) {
			// Find the type info, and a pointer to the exception we're catching.
			const std::type_info *info = null;
			void *object = null;

			// For GCC:
			// This is 'GNUCC++\0' in hex.
			if (type == 0x474e5543432b2b00LL) {
				ExStore *store = BASE_PTR(ExStore, data, exception);
				info = store->exceptionType;
				object = store + 1;
			}

			// TODO: Handle LLVM exceptions as well. They have another type signature, but I believe
			// the layout of the 'data' is the same.

			// Unable to find type-info. The exception probably originated from somewhere we don't
			// know about.
			if (!info)
				return null;

			// This should actually be platform independent as long as the C++ Itanium ABI is used!
			// However, the Itanium ABI does not specify how to check if one typeid is a subclass
			// of another. That is GCC specific, but Clang most likely follows the same convention.

#if 0
			// This is likely slower and less appropriate than calling "do_catch" directly. It does,
			// however, seem to work, and shows what we're trying to accomplish.

			// We only support pointers.
			const abi::__pointer_type_info *ptr = dynamic_cast<const abi::__pointer_type_info *>(info);
			if (!ptr)
				return null;

			// Must point to a class.
			const abi::__class_type_info *srcClass = dynamic_cast<const abi::__class_type_info *>(ptr->__pointee);
			if (!srcClass)
				return null;

			object = *(void **)object;

#if 0
			// This is a bit clumsy and unreliable as we don't have the definition of the type __upcast_result.
			const abi::__class_type_info *target = (const abi::__class_type_info *)&typeid(storm::RootObject);

			// We don't have the type declaration for __class_type_info::__upcast_result.
			struct upcast_result {
				size_t data[10];
			} result;
			if (!srcClass->__do_upcast(target, object, (abi::__class_type_info::__upcast_result &)result))
				return null;
#else
			// This seems more appropriate. It calls __do_upcast eventually, but has access to the
			// upcast_result type, so it is likely more robust. Tell the implementation that the
			// pointer is const.
			if (!typeid(RootObject).__do_catch(srcClass, &object, abi::__pbase_type_info::__const_mask))
				return null;

			return (RootObject *)object;
#endif

#else
			// This is likely the best option. We avoid doing many dynamic_casts ourselves, and rely
			// on vtables instead. This is not ABI-compliant, as __do_catch is not mandated by the ABI.

			// Try to catch a storm::RootObject *.
			if (!typeid(RootObject const * const).__do_catch(info, &object, abi::__pbase_type_info::__const_mask))
				return null;

			return *(RootObject **)object;
#endif

		}

		// The personality function called by the C++ runtime.
		_Unwind_Reason_Code stormPersonality(int version, _Unwind_Action actions, _Unwind_Exception_Class type,
											struct _Unwind_Exception *data, struct _Unwind_Context *context) {

			// We assume 'version == 1'.
			// 'type' is a 8 byte identifier. Equal to 0x47 4e 55 43 43 2b 2b 00 or 'GNUCC++\0' for exceptions from G++.
			// 'data' is compiler specific information about the exception. See 'unwind.h' for details.
			// 'context' is the unwinder state.

			// Note: using _Unwind_GetCFA seems to return the CFA of the previous frame: it is not
			// rewinded to the start of the function to be examined, rather only to the point where
			// the next function was called.
			size_t fn = _Unwind_GetRegionStart(context);
			size_t pc = _Unwind_GetIP(context);
			size_t fp = _Unwind_GetGR(context, DWARF_EH_FRAME_POINTER);

			const FnData *fnData = getData((const void *)fn);
			Binary *binary = codeBinary((const void *)fn);

			if (actions & _UA_SEARCH_PHASE) {
				// Phase 1: Search for handlers.

				// See if this function has any handlers at all.
				if (!binary->hasCatch())
					return _URC_CONTINUE_UNWIND;

				// See if this is an exception Storm can catch (somewhat expensive).
				RootObject *object = isStormException(type, data);
				if (!object)
					return _URC_CONTINUE_UNWIND;

				// Find if it is desirable to actually catch it.
				Nat encoded = stormFindBlockActive(fnData, fn, pc);
				Nat block, active;
				decodeFnState(encoded, block, active);

				if (block == Block().key())
					return _URC_CONTINUE_UNWIND;

				Binary::Resume resume;
				if (!binary->hasCatch(block, object, resume))
					return _URC_CONTINUE_UNWIND;

				// Store things to phase 2, right above the _Unwind_Exception, just like GCC does:
				ExStore *store = BASE_PTR(ExStore, data, exception);

				// Adjusted pointer (otherwise __cxa_begin_catch won't work):
				store->adjustedPtr = object;

				// The following two are not too important, but we keep them at reasonable places as
				// to not confuse the GCC implementation. We recover them later, but we could compute
				// them again.
				store->catchTemp = resume.ip;
				store->languageSpecificData = (byte *)fp + resume.stackOffset;
				store->handlerSwitchValue = encoded;

				// Tell the system we have a handler! It will call us with _UA_HANDLER_FRAME later.
				return _URC_HANDLER_FOUND;
			} else if ((actions & _UA_CLEANUP_PHASE) && (actions & _UA_HANDLER_FRAME)) {
				// Phase 2: Cleanup, but resume here!

				// Read data from the exception object, like GCC does. We saved this data earlier.
				Nat block, active;
				ExStore *store = BASE_PTR(ExStore, data, exception);
				decodeFnState(Nat(store->handlerSwitchValue), block, active);
				pc = (size_t)store->catchTemp;

				// Cleanup our frame.
				StackFrame frame(block, active, (void *)fp);
				block = binary->cleanup(frame, block);

				// Note: It seems we can only set "RAX" and "RDX" here (that's what GCC uses,
				// so those are likely the only ones that will work reliably).
				// Set up the context as desired. We need to restore the stack pointer to what
				// we would expect. For this reason, we first call a 'thunk' that does this
				// for us. We also let the thunk call '__cxa_begin_catch' and '__cxa_end_catch'
				// for us (we could call them here, but doing it later matches what C++ does
				// and saves us a register). We set RDX to the PC we wish to jump to, and
				// RAX contains the new SP we should install.
				_Unwind_SetGR(context, DWARF_EH_RETURN_0, (_Unwind_Word)data);
				_Unwind_SetGR(context, DWARF_EH_RETURN_1, (_Unwind_Word)pc);
				_Unwind_SetIP(context, (_Unwind_Word)&posixResumeExceptionThunk);

				return _URC_INSTALL_CONTEXT;
			} else if (actions & _UA_CLEANUP_PHASE) {
				// Phase 2: Cleanup!

				// Clean up what we can!
				Nat block, active;
				Nat encoded = stormFindBlockActive(fnData, fn, pc);
				decodeFnState(encoded, block, active);
				if (block != Block().key()) {
					StackFrame frame(block, active, (void *)fp);
					binary->cleanup(frame);
				}
				return _URC_CONTINUE_UNWIND;
			} else {
				// Just pretend we did something useful...
				printf("WARNING: Personality function called with an unknown action!\n");
				return _URC_CONTINUE_UNWIND;
			}
		}

		extern "C" void *posixResumeException(_Unwind_Exception *data, const void **spOut) {
			// Get the exception storage so that we can extract the new stack pointer.
			ExStore *store = BASE_PTR(ExStore, data, exception);
			if (spOut)
				*spOut = store->languageSpecificData;

			// Get the exception. Since it is a pointer, we can deallocate directly.
			// Note that we store the dereferenced pointer inside the EH table, so we don't need
			// to dereference it again here!
			void *exception = __cxa_begin_catch(data);

			// We don't need the data to be alive anymore. We can deallocate it now! This
			// technically means that we are lying to the runtime slightly, (i.e. rethrows do not
			// look like rethrows), but we don't use anything that matters for this anyway.
			__cxa_end_catch();

			return exception;
		}

	}
}

#else

namespace code {
	namespace eh {

		_Unwind_Reason_Code stormPersonality(int, _Unwind_Action, _Unwind_Exception_Class,
											_Unwind_Exception *, _Unwind_Context *) {
			return 0;
		}

	}
}

#endif
