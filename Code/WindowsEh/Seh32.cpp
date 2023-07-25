#include "stdafx.h"
#include "Seh.h"

#ifdef WINDOWS

#include "SafeSeh.h"
#include "Code/Binary.h"
#include "Code/FnState.h"
#include "Utils/Memory.h"

namespace code {
	namespace eh {

		struct SEHFrame;

		// Stack frame we're implementing.
		class Frame : public StackFrame {
		public:
			Frame(SEHFrame *frame);

			virtual void *toPtr(int offset);

		private:
			SEHFrame *frame;
		};

		// The SEH frame on the stack.
		struct SEHFrame {
			// SEH chain.
			SEHFrame *prev;
			const void *sehHandler;

			// Pointer to the running code. This is so that we are able to extract the location of
			// the Binary object during unwinding.
			void *self;

			// The topmost active block and active variables.
			Nat activePartVars;

			// Current EBP points to this.
			void *lastEbp;

			// Get EBP.
			inline void *ebp() {
				return &lastEbp;
			}

			// Create from EBP.
			static SEHFrame *fromEbp(void *ebp) {
				byte *p = (byte *)ebp;
				return (SEHFrame *)(p - OFFSET_OF(SEHFrame, lastEbp));
			}

			// Create from SEH frame.
			static SEHFrame *fromSEH(void *seh) {
				byte *p = (byte *)seh;
				return (SEHFrame *)(p - OFFSET_OF(SEHFrame, prev));
			}

			// Cleanup this frame.
			void cleanup() {
				Binary *owner = codeBinary(self);
				if (owner) {
					Frame f(this);
					owner->cleanup(f);
				} else {
					WARNING(L"Using SEH, but no link to the metadata provided!");
				}
			}

			// Partial cleanup of this frame. Will set 'activePart' accordingly.
			void cleanup(Nat until) {
				Binary *owner = codeBinary(self);
				if (owner) {
					Frame f(this);
					Nat part = owner->cleanup(f, until);
					activePartVars = encodeFnState(part, f.activation);
				} else {
					WARNING(L"Using SEH, but no link to the metadata provided!");
				}
			}

			// Check if we want to catch any object at all in this frame.
			bool hasCatch() {
				Binary *owner = codeBinary(self);
				if (!owner)
					return false;
				return owner->hasCatch();
			}

			// Check if we want to catch this exception.
			bool hasCatch(storm::RootObject *exception, Binary::Resume &resume) {
				Binary *owner = codeBinary(self);
				if (!owner)
					return false;
				Nat part, activation;
				decodeFnState(activePartVars, part, activation);
				return owner->hasCatch(part, exception, resume);
			}

		};


		Frame::Frame(SEHFrame *f) : StackFrame(0, 0), frame(f) {
			decodeFnState(f->activePartVars, block, activation);
		}

		void *Frame::toPtr(int offset) {
			byte *ebp = (byte *)frame->ebp();
			return ebp + offset;
		}


	}
}


#ifndef EXCEPTION_UNWINDING
// No proper definition found...
#define EXCEPTION_UNWINDING 2
#endif
#ifndef EXCEPTION_NONCONTINUABLE
#define EXCEPTION_NONCONTINUABLE 1
#endif

using namespace code::eh;


// The low-level stuff here is inspired from the code in OS/Future.cpp, which is in turn inspired by
// boost::exception_ptr.

static const nat cppExceptionCode = 0xE06D7363;
static const nat cppExceptionMagic = 0x19930520;
static const nat cppExceptionParams = 3;
#if _MSC_VER==1310
static const nat exceptionInfoOffset = 0x74;
#elif (_MSC_VER == 1400 || _MSC_VER == 1500)
static const nat exceptionInfoOffset = 0x80;
#else
// No special treatment of the re-throw mechanism.
#define MSC_NO_SPECIAL_RETHROW
#endif

static bool isCppException(_EXCEPTION_RECORD *record) {
	return record->ExceptionCode == cppExceptionCode
		&& record->NumberParameters >= cppExceptionParams
		&& record->ExceptionInformation[0] == cppExceptionMagic;
}

enum CppTypeFlags {
	simpleType = 1,
	virtualBaseType = 4,
};

struct CppTypeInfo {
	unsigned flags;
	std::type_info * typeInfo;
	int thisOffset;
	int vbaseDescr;
	int vbaseOffset;
	unsigned long size;
};

struct CppTypeInfoTable {
	unsigned count;
	const CppTypeInfo *info[1];
};

struct CppExceptionType {
	unsigned flags;
	void *dtor;
	void(*handler)();
	const CppTypeInfoTable *table;
};

static bool isObjPtr(const CppTypeInfoTable *table) {
	for (unsigned i = 0; i < table->count; i++) {
		// if (*table->info[i]->typeInfo == typeid(void *))
		// 	return true;

		// Only catch Storm objects.
		if (*table->info[i]->typeInfo == typeid(storm::RootObject *))
			return true;
	}
	return false;
}

// Called when we catch an exception. Called from a shim in assembler located in SafeSeh.asm
extern "C"
void *x86SEHCleanup(SEHFrame *frame, storm::Nat cleanUntil, void *exception) {
	frame->cleanup(cleanUntil);
	return exception;
}

extern "C"
EXCEPTION_DISPOSITION __cdecl x86SEH(_EXCEPTION_RECORD *er, void *frame, _CONTEXT *ctx, void *dispatchCtx) {
	SEHFrame *f = SEHFrame::fromSEH(frame);
	if (er->ExceptionFlags & EXCEPTION_UNWINDING) {
		// We just need to do cleanup.
		f->cleanup();
		return ExceptionContinueSearch;
	}

	// Early out if this frame doesn't have any catch clauses at all.
	if (!f->hasCatch())
		return ExceptionContinueSearch;

	// Note: If we need to do this on X64, treat parameter 3 as a HINSTANCE and treat remaining
	// pointers as relative to that.
	if (!isCppException(er))
		return ExceptionContinueSearch;

#ifdef MSC_NO_SPECIAL_RETHROW
	// It seems we don't have to handle re-throws as a special case on newer MSC versions.
	// Include a check so that we get to know if this assumption is wrong.
	if (!er->ExceptionInformation[2]) {
		WARNING(L"It seems like we need to handle re-throws like in older MSC versions!");
		return ExceptionContinueSearch;
	}
#else
	if (!er->ExceptionInformation[2]) {
		// Re-throw, we need to get the info from TLS.
		byte *t = (byte *)_errno();
		er = *(_EXCEPTION_RECORD **)(t + exceptionInfoOffset);

		if (!isCppException(er))
			return ExceptionContinueSearch;
	}
#endif

	const CppExceptionType *type = (const CppExceptionType *)er->ExceptionInformation[2];

	// Apparently, the 'simple type' flag is set when we're working with pointers to const objects.
	// If it's not a simple type, don't bother looking for a void *.
	// if ((type->flags & simpleType) != 0)
	// 	return ExceptionContinueSearch;

	// The table seems to be a table of possible types that can catch the exception. We look for
	// 'RootObject *' in that, then we know if it's a pointer or not!
	if (!isObjPtr(type->table))
		return ExceptionContinueSearch;

	// It seems like we don't have to worry about extra padding etc for simple types at least.
	storm::RootObject **object = (storm::RootObject **)er->ExceptionInformation[1];
	if (!object)
		return ExceptionContinueSearch;

#ifdef X64
	PLN(L"TODO!");
	abort();
#else
	code::Binary::Resume resume;
	if (f->hasCatch(*object, resume)) {
		// It seems we need to initiate stack unwinding for cleanup ourselves.
		er->ExceptionFlags |= EXCEPTION_UNWINDING;
		// This seems to fail on newer MSC.
		x86Unwind(er, frame);
		er->ExceptionFlags &= ~DWORD(EXCEPTION_UNWINDING);

		// No, we can continue from the exception... Clear the noncontinuable flag.
		er->ExceptionFlags &= ~DWORD(EXCEPTION_NONCONTINUABLE);

		// Build a stack "frame" that executes 'x86EhEntry' and returns to the resume point with Eax
		// set as intended. This approach places all data in registers, so that data on the stack is
		// not clobbered if we need it. It is also nice, as we don't have to think too carefully about
		// calling conventions and stack manipulations.
		ctx->Ebp = (UINT_PTR)f->ebp();
		ctx->Esp = ctx->Ebp + resume.stackOffset;

		// Run.
		ctx->Eip = (UINT_PTR)&x86EhEntry;
		// Return to.
		ctx->Edx = (UINT_PTR)resume.ip;
		// Exception.
		ctx->Eax = (UINT_PTR)*object;
		// Current part.
		ctx->Ebx = (UINT_PTR)resume.cleanUntil;
		// Current frame.
		ctx->Ecx = (UINT_PTR)f;

		// Note: We also need to restore the EH chain (fs:[0]). The ASM shim does this for us.

		return ExceptionContinueExecution;
	}
#endif

	return ExceptionContinueSearch;
}


#endif
