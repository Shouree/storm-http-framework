#pragma once
#include "Code/TypeDesc.h"

/**
 * Shared data structures between "Call.cpp" and "Callee.cpp"
 */

struct TinyIntParam {
	Int a;
	Int b;

	TinyIntParam(Int a, Int b) : a(a), b(b) {}
};

code::SimpleDesc *tinyIntDesc(Engine &e);

struct SmallIntParam {
	size_t a;
	size_t b;

	// Make sure it is not a POD.
	SmallIntParam(size_t a, size_t b) : a(a), b(b) {}
};

code::SimpleDesc *smallIntDesc(Engine &e);


struct LargeIntParam {
	size_t a;
	size_t b;
	size_t c;

	// Make sure it is not a POD.
	LargeIntParam(size_t a, size_t b, size_t c) : a(a), b(b), c(c) {}
};

code::SimpleDesc *largeIntDesc(Engine &e);

struct MixedParam {
	size_t a;
	Float b;
	Float c;

	// Make sure it is not a POD.
	MixedParam(size_t a, Float b, Float c) : a(a), b(b), c(c) {}
};

code::SimpleDesc *mixedDesc(Engine &e);

struct ByteStruct {
	Byte a;
	Byte b;

	ByteStruct(Byte a, Byte b) : a(a), b(b) {}
};

code::SimpleDesc *bytesDesc(Engine &e);
