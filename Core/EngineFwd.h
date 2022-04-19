#pragma once
#include "OS/UThread.h"

class StackInfoSet;

namespace storm {
	struct GcType;
	struct GcCode;
	class GcWatch;
	class RootObject;
	class CloneEnv;
	class StdRequest;

	/**
	 * Functions forwarded to the Engine that instantiated a shared library. This is split into two
	 * parts, one part with functions that have the same implementation for all shared libraries,
	 * and one part that differs between different instances.
	 */

	/**
	 * Shared part.
	 */
	struct EngineFwdShared {
		// Everything present in storm::runtime:
		const Handle &(*typeHandle)(Type *t);
		const Handle &(*voidHandle)(Engine &e);
		const Handle &(*refObjHandle)(Engine &e);
		Type *(*typeOf)(const RootObject *o);
		const GcType *(*gcTypeOf)(const void *alloc);
		const GcType *(*typeGc)(Type *t);
		Str *(*typeName)(Type *t);
		Str *(*typeIdentifier)(Type *t);
		MAYBE(Type *) (*fromIdentifier)(Str *name);
		bool (*isValue)(Type *t);
		bool (*typeIsA)(const Type *a, const Type *b);
		bool (*isA)(const RootObject *a, const Type *b);
		Engine &(*allocEngine)(const RootObject *o);
		void *(*allocRaw)(Engine &e, const GcType *type);
		void *(*allocStaticRaw)(Engine &e, const GcType *type);
		GcArray<Byte> *(*allocBuffer)(Engine &e, size_t size);
		void *(*allocObject)(size_t size, Type *type);
		void *(*allocArray)(Engine &e, const GcType *type, size_t count);
		void *(*allocWeakArray)(Engine &e, size_t count);
		void *(*allocCode)(Engine &e, size_t code, size_t refs);
		size_t (*codeSize)(const void *code);
		GcCode *(*codeRefs)(void *code);
		void (*codeUpdatePtrs)(void *code);
		void (*setVTable)(RootObject *object);
		bool (*liveObject)(RootObject *object);
		os::ThreadGroup &(*threadGroup)(Engine &e);
		util::Lock &(*threadLock)(Engine &e);
		GcWatch *(*createWatch)(Engine &e);
		void (*postStdRequest)(Engine &e, StdRequest *request);
		RootObject *(*cloneObject)(RootObject *obj);
		RootObject *(*cloneObjectEnv)(RootObject *obj, CloneEnv *env);
		Engine &(*someEngine)();
		Engine *(*someEngineUnsafe)();

		// Additional functions required.
		os::ThreadData *(*getCurrentThreadData)();
		void (*setCurrentThreadData)(os::ThreadData *data);
		os::UThreadState *(*getCurrentUThreadState)();
		void (*setCurrentUThreadState)(os::UThreadState *state);
		void (*threadCreated)();
		void (*threadTerminated)();

		// Get the global instace for stack traces.
		StackInfoSet &(*stackInfo)();

		// Completely different type to catch errors when using initializer lists.
		Float dummy;
	};

	/**
	 * Unique part.
	 *
	 * This is so that each shared library can have its own "namespace" of unique type identifiers.
	 */
	struct EngineFwdUnique {
		Type *(*cppType)(Engine &e, void *identifier, Nat id);
		Type *(*cppTemplateVa)(Engine &e, void *identifier, Nat id, Nat count, va_list params);
		Thread *(*getThread)(Engine &e, void *identifier, const DeclThread *decl);
		void *(*getLibData)(Engine &e, void *identifier);

		// Identifier. Passed to all functions in here.
		void *identifier;

		// Completely different type to catch errors when using initializer lists.
		Float dummy;
	};

	// Create the shared part.
	const EngineFwdShared &engineFwd();

}
