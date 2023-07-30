#include "stdafx.h"
#include "Seh.h"

#ifdef WINDOWS

#include "SafeSeh.h"
#include "Seh64.h"
#include "Code/Binary.h"
#include "Core/Str.h"
#include "Gc/Gc.h"
#include "Gc/CodeTable.h"
#include "Utils/Memory.h"
#include "Utils/StackInfoSet.h"

namespace storm {
	namespace runtime {
		storm::Gc &engineGc(Engine &e);
	}
}

namespace code {
	namespace eh {

		class SehInfo : public StackInfo {
		public:
			virtual bool translate(void *ip, void *&fnBase, int &offset) const {
				storm::CodeTable &table = storm::codeTable();
				void *code = table.find(ip);
				if (!code)
					return false;

				fnBase = code;
				offset = int((byte *)ip - (byte *)code);
				return true;
			}

			virtual void format(GenericOutput &to, void *fnBase, int offset) const {
				Binary *owner = codeBinary(fnBase);
				Str *name = owner->ownerName();
				if (name) {
					to.put(name->c_str());
				} else {
					to.put(S("<unnamed Storm function>"));
				}
			}
		};

		void activateWindowsInfo(Engine &e) {
			static RegisterInfo<SehInfo> info;
#ifdef X64
			storm::runtime::engineGc(e).setEhCallback(&exceptionCallback);
#else
			(void)&e;
#endif
		}



		class Frame : public StackFrame {
		public:
			Frame(const SehFrame &frame)
				: StackFrame(frame.part, frame.activation),
				  framePtr((byte *)frame.framePtr) {}

			virtual void *toPtr(int offset) {
				return framePtr + offset;
			}

		private:
			byte *framePtr;
		};

		void cleanupFrame(SehFrame frame) {
			if (frame.binary) {
				Frame f(frame);
				frame.binary->cleanup(f);
			} else {
				WARNING(L"Using SEH, but no link to the metadata provided!");
			}
		}


		/**
		 * Low-level stuff:
		 */


		// The low-level things here are inspired from the code in OS/Future.cpp, which is in turn
		// inspired by boost::exception_ptr.

		const nat cppExceptionCode = 0xE06D7363;
		const nat cppExceptionMagic = 0x19930520;
#if defined(X86)
		const nat cppExceptionParams = 3;

#if _MSC_VER == 1310
		const nat exceptionInfoOffset = 0x74;
#elif (_MSC_VER == 1400 || _MSC_VER == 1500)
		const nat exceptionInfoOffset = 0x80;
#else
// No special treatment of the re-throw mechanism.
#define MSC_NO_SPECIAL_RETHROW
#endif

#elif defined(X64)
		const nat cppExceptionParams = 4;

#if _MSC_VER==1310
		const nat exceptionInfoOffset = /* probably 0xE0 - 0xC*2, but untested */;
#elif (_MSC_VER == 1400 || _MSC_VER == 1500)
		const nat exceptionInfoOffset = 0xE0;
#else
// No special treatment of the re-throw mechanism.
#define MSC_NO_SPECIAL_RETHROW
#endif

#else
#error "Unsupported architecture!"
#endif

		struct CppTypeInfo {
			unsigned flags;
			DWORD typeInfoOffset;
			int thisOffset;
			int vbaseDescr;
			int vbaseOffset;
			unsigned int size;
			DWORD copyCtorOffset;

			std::type_info *typeInfo(HINSTANCE instance) const {
				return (std::type_info *)(size_t(instance) + size_t(typeInfoOffset));
			}
		};

		struct CppTypeInfoTable {
			unsigned count;
			DWORD infoOffset[1];

			const CppTypeInfo *info(unsigned id, HINSTANCE instance) const {
				return (const CppTypeInfo *)(size_t(instance) + size_t(infoOffset[id]));
			}
		};

		struct CppExceptionType {
			unsigned flags;
			DWORD dtorOffset;
			DWORD handlerOffset; // void (*handler)()
			DWORD tableOffset;

			const CppTypeInfoTable *table(HINSTANCE instance) const {
				return (const CppTypeInfoTable *)(size_t(instance) + size_t(tableOffset));
			}
		};

		static bool isObjPtr(const CppTypeInfoTable *table, HINSTANCE instance) {
			for (unsigned i = 0; i < table->count; i++) {
				const CppTypeInfo *info = table->info(i, instance);

				if (*info->typeInfo(instance) == typeid(storm::RootObject *))
					return true;
			}

			return false;
		}


		static bool isCppException(EXCEPTION_RECORD *record) {
			return record
				&& record->ExceptionCode == cppExceptionCode
				&& record->NumberParameters == cppExceptionParams
				&& record->ExceptionInformation[0] == cppExceptionMagic;
		}

		extern "C"
		EXCEPTION_DISPOSITION windowsHandler(_EXCEPTION_RECORD *er, void *frame, _CONTEXT *ctx, void *dispatch) {
			PLN(L"FRAME " << frame);
			SehFrame f = extractFrame(er, frame, ctx, dispatch);
			PLN(L"Checking " << frame);
			PVAR(toHex(Nat(er->ExceptionFlags)));
			if ((er->ExceptionFlags & EXCEPTION_UNWINDING) && (er->ExceptionFlags & EXCEPTION_TARGET_UNWIND)) {
				PLN(L"Target frame!");
			}

			if (er->ExceptionFlags & EXCEPTION_UNWINDING) {
				PLN(L"Cleanup!");
				// Only need to do cleanup!
				cleanupFrame(f);
				return ExceptionContinueSearch;
			}

			// Early out if we have no catch clauses:
			if (!f.binary || !f.binary->hasCatch())
				return ExceptionContinueSearch;

			// Only catch C++ exceptions.
			if (!isCppException(er))
				return ExceptionContinueSearch;

#ifdef MSC_NO_SPECIAL_RETHROW
			// It seems we don't have to handle re-throws as a special case on newer MSC versions.
			// Check if this is true...
			if (!er->ExceptionInformation[2]) {
				WARNING(L"It seems like we need to handle re-throws like in older MSC versions!");
				return ExceptionContinueSearch;
			}
#else
			if (!er->ExceptionInformation[2]) {
				byte *t = (byte *)_errno();
				er = *(_EXCEPTION_RECORD **)(t + exceptionInfoOffset);

				if (!isCppException(er))
					return ExceptionContinueSearch;
			}
#endif

			HINSTANCE instance = cppExceptionParams >= 4
				? HINSTANCE(er->ExceptionInformation[3])
				: HINSTANCE(NULL);

			const CppExceptionType *type = (const CppExceptionType *)er->ExceptionInformation[2];

			// Apparently, the 'simple type' flag is set when we're working with pointers to const objects.
			// If it's not a simple type, don't bother looking for a void *.
			// if ((type->flags & simpleType) != 0)
			// 	return ExceptionContinueSearch;

			// The table seems to be a table of possible types that can catch the exception. We look for
			// 'RootObject *' in that, then we know if it's a pointer or not!
			if (!isObjPtr(type->table(instance), instance))
				return ExceptionContinueSearch;

			// It seems like we don't have to worry about extra padding etc for simple types at least.
			storm::RootObject **object = (storm::RootObject **)er->ExceptionInformation[1];
			if (!object)
				return ExceptionContinueSearch;

			PVAR(*object);

			Binary::Resume resume;
			if (f.binary->hasCatch(f.part, *object, resume)) {
				// We found a matching catch block! Unwind the stack and resume!
				resumeFrame(f, resume, *object, ctx, er, dispatch);

				// Note: Might not return to here, but if we do we should continue execution.
				return ExceptionContinueExecution;
			}

			return ExceptionContinueSearch;
		}


	}
}

#endif
