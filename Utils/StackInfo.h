#pragma once
#include "StackTrace.h"
#include "Lock.h"

/**
 * Pluggable output stream for the stack info.
 */
class GenericOutput : NoCopy {
public:
	virtual void put(const wchar *str) = 0;
	virtual void put(const char *str) = 0;
	virtual void put(size_t i) = 0;
	virtual void putHex(size_t i) = 0;

	// Called if a single frame consists of multiple frames due to inlining.
	virtual void nextFrame() = 0;
};

/**
 * wostream implementation.
 */
class StdOutput : public GenericOutput {
public:
	StdOutput(wostream &to) : to(to), frameNumber(0) {}

	virtual void put(const wchar *str) { to << str; }
	virtual void put(const char *str) { to << str; }
	virtual void put(size_t i) { to << i; }
	virtual void putHex(size_t i) { to << toHex(i); }
	virtual void nextFrame();
private:
	nat frameNumber;
	wostream &to;
};


/**
 * Pluggable interface for collecting and showing information on the stack.
 */
class StackInfo : NoCopy {
public:
	// Called when the system allocates an array of StackFrame structures so that a backend can keep
	// any data referred by the trace alive until the trace is not needed anymore. The function is
	// called after the stack trace is populated.
	virtual void alloc(StackFrame *frames, nat count) const;
	virtual void free(StackFrame *frames, nat count) const;

	// Called during stack trace collection to resolve a possibly garbage collected pointer into a
	// base pointer for the function and an offset. Returns true if we know of this function.
	virtual bool translate(void *ip, void *&fnBase, int &offset) const = 0;

	// Format a function call. Only called for the StackInfo instance whose 'translate' returned true.
	virtual void format(GenericOutput &to, void *fnBase, int offset) const = 0;
};

