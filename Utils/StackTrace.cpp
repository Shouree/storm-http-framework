#include "stdafx.h"
#include "StackTrace.h"
#include "CppInfo.h"
#include "StackInfoSet.h"

#include <iomanip>

// System specific headers.
#include "DbgHelper.h"

StackTrace::StackTrace() : frames(null), size(0), capacity(0) {}

StackTrace::StackTrace(nat n) : frames(new StackFrame[n]), size(n), capacity(n) {
	stackInfo().alloc(frames, size);
}

StackTrace::StackTrace(const StackTrace &o) : frames(null) {
	size = o.size;
	capacity = o.size;
	if (o.frames) {
		frames = new StackFrame[size];
		for (nat i = 0; i < size; i++)
			frames[i] = o.frames[i];
		stackInfo().alloc(frames, size);
	}
}

StackTrace &StackTrace::operator =(const StackTrace &o) {
	StackTrace copy(o);
	swap(frames, copy.frames);
	swap(size, copy.size);
	swap(capacity, copy.capacity);
	return *this;
}

StackTrace::~StackTrace() {
	if (frames)
		stackInfo().free(frames, size);
	delete []frames;
}

void StackTrace::push(const StackFrame &frame) {
	if (size >= capacity) {
		if (capacity == 0)
			capacity = 8;
		else
			capacity *= 2;

		StackFrame *n = new StackFrame[capacity];
		stackInfo().alloc(n, capacity);

		if (frames)
			for (nat i = 0; i < size; i++)
				n[i] = frames[i];

		swap(n, frames);
		if (n)
			stackInfo().free(n, size);
		delete []n;
	}

	frames[size++] = frame;
}

void StackTrace::output(wostream &to) const {
	for (nat i = 0; i < count(); i++) {
		to << endl << std::setw(3) << i;
		to << L": " << (void *)((byte *)frames[i].fnBase + frames[i].offset);
	}
}

String format(const StackTrace &t) {
	std::wostringstream to;
	StdOutput sOut(to);
	StackInfoSet &l = stackInfo();

	for (nat i = 0; i < t.count(); i++) {
		const StackFrame &frame = t[i];

		sOut.nextFrame();
		l.format(sOut, frame.id, frame.fnBase, frame.offset);
	}
	to << endl;

	return to.str();
}

/**
 * System specific code for collecting the stack trace itself.
 */

// Windows stack traces using DbgHelp. Relies on debug information.
#if !defined(STANDALONE_STACKWALK) && defined(X86)

#if defined(X86)
static const DWORD machineType = IMAGE_FILE_MACHINE_I386;

static void initFrame(CONTEXT &context, STACKFRAME64 &frame) {
	frame.AddrPC.Offset = context.Eip;
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = context.Ebp;
	frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrStack.Offset = context.Esp;
	frame.AddrStack.Mode = AddrModeFlat;
}

#elif defined(X64)
static const DWORD machineType = IMAGE_FILE_MACHINE_AMD64;

static void initFrame(CONTEXT &context, STACKFRAME64 &frame) {
	frame.AddrPC.Offset = context.Rip;
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = context.Rsp; // is this correct?
	frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrStack.Offset = context.Rsp;
	frame.AddrStack.Mode = AddrModeFlat;
}
#else
#error "Unknown windows platform!"
#endif

// Warning about not being able to protect from stack-overruns...
#pragma warning ( disable : 4748 )

void createStackTrace(TraceGen &gen, nat skip, void *state) {
	// Initialize the library if it is not already done.
	dbgHelp();

	CONTEXT context;
	if (state) {
		context = *(CONTEXT *)state;
	} else {
#ifdef X64
		RtlCaptureContext(&context);
#else
		// Sometimes RtlCaptureContext crashes for X86, so we do it with inline-assembly instead!
		__asm {
		label:
			mov [context.Ebp], ebp;
			mov [context.Esp], esp;
			mov eax, [label];
			mov [context.Eip], eax;
		}
#endif
	}

	HANDLE process = GetCurrentProcess();
	HANDLE thread = GetCurrentThread();
	STACKFRAME64 frame;
	zeroMem(frame);
	initFrame(context, frame);

	gen.init(0);
	StackInfoSet &s = stackInfo();
	while (StackWalk64(machineType, process, thread, &frame, &context, NULL, NULL, NULL, NULL)) {
		if (skip > 0) {
			skip--;
			continue;
		}

		StackFrame f;
		f.id = s.translate((void *)frame.AddrPC.Offset, f.fnBase, f.offset);

		gen.put(f);
	}
}

#endif

// The stand-alone stack walk for X86 windows. Fails when frame pointer is omitted.
#if defined(STANDALONE_STACKWALK) && defined(X86) && defined(WINDOWS)

static bool onStack(void *stackMin, void *stackMax, void *ptr) {
	return ptr >= stackMin
		&& ptr <= stackMax;
}

static void *prevFrame(void *bp) {
	void **v = (void **)bp;
	return v[0];
}

static void *prevIp(void *bp) {
	void **v = (void **)bp;
	return v[1];
}

static void *prevParam(void *bp, nat id) {
	void **v = (void **)bp;
	return v[2 + id];
}

static NT_TIB *getTIB() {
	NT_TIB *tib;
	__asm {
		// read 'self' from 'fs:0x18'
		mov eax, fs:0x18;
		mov tib, eax;
	}
	assert(tib == tib->Self);
	return tib;
}

void createStackTrace(TraceGen &gen, nat skip, void *state) {
	NT_TIB *tib = getTIB();
	void *stackMax = tib->StackBase;
	void *stackMin = tib->StackLimit;

	void *base = null;
	if (state) {
		base = ((CONTEXT *)state)->Ebp;
	} else {
		__asm {
			mov base, ebp;
		}
	}

	// Count frames.
	nat frames = 0;
	for (void *now = base; onStack(stackMin, stackMax, prevFrame(now)); now = prevFrame(now))
		frames++;

	if (frames > skip)
		frames -= skip;
	else
		skip = 0;

	// Collect the trace itself.
	gen.init(frames);
	StackInfoSet &s = stackInfo();
	void *now = base;
	for (nat i = 0; i < skip; i++)
		now = prevFrame(now);

	for (nat i = 0; i < frames; i++) {
		StackFrame f;
		f.id = s.translate(prevIp(now), f.fnBase, f.offset);
		now = prevFrame(now);

		gen.put(f);
	}
}

#endif

#if defined(WINDOWS) && defined(X64)

namespace internal {

	// Copied from winternl.h to make it possible to reserve enough stack space for the history table.
	// Note: We have some leeway in the history size just in case.
#define UNWIND_HISTORY_SIZE (12 + 4)

	struct UNWIND_HISTORY_ENTRY {
		ULONG64 base;
		RUNTIME_FUNCTION *entry;
	};

	struct UNWIND_HISTORY_TABLE {
		ULONG count;
		BYTE localHint;
		BYTE globalHint;
		BYTE search;
		BYTE once;
		ULONG64 lowAddr;
		ULONG64 highAddr;
		UNWIND_HISTORY_ENTRY entries[UNWIND_HISTORY_SIZE];
	};

	struct KNONVOLATILE_CONTEXT_POINTERS;

	extern "C"
	NTSYSAPI PRUNTIME_FUNCTION RtlLookupFunctionEntry(DWORD64 pc, PDWORD64 base, UNWIND_HISTORY_TABLE *history);

	extern "C"
	NTSYSAPI void *RtlVirtualUnwind(DWORD handlerType, DWORD64 base, DWORD64 pc,
									PRUNTIME_FUNCTION entry, PCONTEXT context,
									PVOID *handlerData, PDWORD64 establisher,
									KNONVOLATILE_CONTEXT_POINTERS *nv);

}

void createStackTrace(TraceGen &gen, nat skip, void *state) {
	using namespace internal;

	StackInfoSet &s = stackInfo();
	gen.init(0);

	CONTEXT context;
	RtlCaptureContext(&context);

	// Note: It is fine if we don't support leaf functions. We know that this is not a leaf function
	// (we call other functions), so we will have metadata, and all preceeding functions need
	// metadata.
	DWORD64 base = null;
	UNWIND_HISTORY_TABLE history;
	zeroMem(history);
	RUNTIME_FUNCTION *entry;
	while (entry = RtlLookupFunctionEntry(context.Rip, &base, &history)) {
		if (skip > 0) {
			skip--;
		} else {
			StackFrame frame;
			frame.id = s.translate((void *)context.Rip, frame.fnBase, frame.offset);
			gen.put(frame);
		}

		DWORD64 infoAddress = base + entry->UnwindData;
		DWORD type = (*(byte *)infoAddress) >> 3; // According to specification of the info struct.

		// If bit 3 is set, then this is a nested information, and we might want to "fold" the call
		// into the previous entry?

		DWORD64 oldIp = context.Rip;

		void *handler = 0;
		DWORD64 establisher = 0;
		RtlVirtualUnwind(type, base, oldIp, entry, &context, &handler, &establisher, NULL);
		// Note: There is not really a good way to see if RtlVirtualUnwind succeeded or not.

		if (context.Rip == 0)
			break;
	}
}

#endif

#ifdef POSIX
#include <execinfo.h>

#if defined(ARM64)

// Note: Calling "backtrace()" on Arm64 sometimes crashes internally if there are functions without
// unwind info on the stack. However, the calling convention requires storing stack- and base
// pointers on the stack. That makes it very easy to traverse the stack anyway!
void createStackTrace(TraceGen &gen, nat skip, void *state) {
	(void)state; // TODO: We could extract x29 from state here.
	void *framePointer = null;
	__asm__ volatile ("mov %0, x29\n"
					: "=r" (framePointer)
					: : );

	nat depth = 0;
	for (void *fp = framePointer; fp; fp = *(void **)fp)
		depth++;

	if (depth >= skip)
		depth -= skip;
	else
		skip = 0;

	gen.init(depth);

	StackInfoSet &s = stackInfo();
	depth = 0;
	while (framePointer) {
		void *returnIp = ((void **)framePointer)[1];
		framePointer = ((void **)framePointer)[0];

		if (depth >= skip) {
			StackFrame frame;
			frame.id = s.translate(returnIp, frame.fnBase, frame.offset);
			gen.put(frame);
		}
		depth++;
	}
}

#else

void createStackTrace(TraceGen &gen, nat skip, void *state) {
	(void) state;
	// Note: We could call _Unwind_Backtrace directly to avoid any size limitations.
	const int MAX_DEPTH = 100;
	void *buffer[MAX_DEPTH];
	int depth = backtrace(buffer, MAX_DEPTH);
	if (depth < 0)
		return;

	nat levels = nat(depth);
	if (levels <= skip)
		return;

	levels -= skip;

	gen.init(levels);
	StackInfoSet &s = stackInfo();
	for (nat i = 0; i < levels; i++) {
		StackFrame frame;
		frame.id = s.translate(buffer[i + skip], frame.fnBase, frame.offset);

		gen.put(frame);
	}
}

#endif

#endif

class STGen : public TraceGen {
public:
	StackTrace trace;

	void init(size_t count) {}

	void put(const StackFrame &frame) {
		trace.push(frame);
	}
};

StackTrace stackTrace(nat skip) {
	STGen gen;
	createStackTrace(gen, skip);
	return gen.trace;
}
