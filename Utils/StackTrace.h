#pragma once

/**
 * Collection of a stack trace, and tools for showing it to the user using the information
 * available in the Arena for symbolic lookup, alongside any debug information for the C++ code.
 * This will be extended to allow for inspection later on, so that the one who generated code
 * can inspect variables and similar things.
 */

/**
 * Fixed-size data entry for stack traces.
 */
struct StackFrame {
	StackFrame() : fnBase(null), offset(0), id(0) {}

	// Base and offset for the function.
	void *fnBase;
	int offset;

	// Which backend produced the data above.
	int id;
};


/**
 * Representation of the stack trace itself. It will collect a copy of the stack with minimal processing
 * so that we can later inspect the data when and if we want to. However, this means that any data stored
 * here should not be considered live objects, since they are nothing more than a bitwise copy of the
 * original value.
 * This is since we probably want to include a Trace in exceptions, and we want to do as little as possible
 * when an exception is thrown in order to minimize code that should not fail. Also, we do not want to spend
 * time generating things that will never be shown anyway.
 */
class StackTrace : public Printable {
public:

	// Create an empty stack trace.
	StackTrace();

	// Create a trace with 'n' uninitialized frames in it.
	StackTrace(nat n);

	// Dtor.
	~StackTrace();

	// Copy.
	StackTrace(const StackTrace &o);
	StackTrace &operator =(const StackTrace &o);

	// Element access.
	inline StackFrame &operator [] (nat i) { return frames[i]; }
	inline const StackFrame &operator [] (nat i) const { return frames[i]; }

	// Size.
	inline nat count() const { return size; }

	// Add an element (grows).
	void push(const StackFrame &frame);

protected:
	// Output. Not formatted in any way.
	virtual void output(wostream &to) const;

private:
	// Storage of the data.
	StackFrame *frames;

	// Number of frames.
	nat size;

	// Capacity.
	nat capacity;
};

// Generate a stack trace from the calling point in the code.
StackTrace stackTrace(nat skip = 0);

// Fromat an entire trace.
String format(const StackTrace &trace);


// Generate a stack trace using a custom container.
class TraceGen {
public:
	// Initialize the data structure with 'count' elements.
	virtual void init(size_t count) = 0;

	// Add a frame.
	virtual void put(const StackFrame &frame) = 0;
};

// State is a pointer to a platform-specific collected stack state.
void createStackTrace(TraceGen &gen, nat skip = 0, void *state = null);
