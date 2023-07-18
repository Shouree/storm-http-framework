#pragma once
#include "GcArray.h"
#include <cstdarg>

namespace storm {
	struct GcType;
	struct GcCode;
	class GcWatch;
	class RootObject;
	class CloneEnv;
	class StdRequest;

	namespace runtime {

		/**
		 * This file declares the interface to the runtime that code in Core/ needs, but can not be
		 * implemented here, as the implementation differs depending on if Core/ is linked to the
		 * Compiler/ or a shared library. For shared libraries, include SharedLib.h from *one*
		 * source file, to get the implementation suitable for shared libraries (which just
		 * delegates calls to the compiler linked at runtime).
		 */

		// Get the Storm type description for a C++ type given its ID (used by the generated type information code).
		Type *cppType(Engine &e, Nat id);

		// Variadic argument variant of 'cppTemplate'.
		Type *cppTemplateVa(Engine &e, Nat id, Nat count, va_list params);

		// Get the Storm type description for a C++ template, given its ID and template type ids (we
		// do not support eg. Array<Array<X>> from C++.
		inline Type *cppTemplate(Engine &e, Nat id, Nat count, ...) {
			Type *r;
			va_list l;
			va_start(l, count);
			r = cppTemplateVa(e, id, count, l);
			va_end(l);
			return r;
		}

		// Get a type handle for 'type'.
		const Handle &typeHandle(Type *t);

		// Get the handle for 'void'.
		const Handle &voidHandle(Engine &e);

		// Get the handle for treating regular objects as reference types in hash maps.
		const Handle &refObjHandle(Engine &e);

		// Get the type of an allocation.
		Type *typeOf(const RootObject *o);

		// Get the GcType for an allocation.
		const GcType *gcTypeOf(const void *alloc);

		// Get the GcType for a particular Storm type.
		const GcType *typeGc(Type *t);

		// Get the name of a type (expected to be used for pretty-printing).
		Str *typeName(Type *t);

		// Get a (mangled) identifier of a type that is suitable for serialization.
		Str *typeIdentifier(Type *t);

		// Get the type with the mangled name 'name'.
		MAYBE(Type *) fromIdentifier(Str *name);

		// Is 't' a value type?
		bool isValue(Type *t);

		// Is type A an instance of type B?
		bool isA(const Type *a, const Type *b);

		// Is the object A an instance of type B?
		bool isA(const RootObject *a, const Type *b);

		// Get the engine object for an allocation.
		Engine &allocEngine(const RootObject *o);

		// Allocate some raw memory for some data.
		void *allocRaw(Engine &e, const GcType *type);

		// Allocate some non-moving raw memory for some data.
		void *allocStaticRaw(Engine &e, const GcType *type);

		// Allocate a buffer which is safe to use from any thread in the system (even threads not
		// registered with Storm).
		GcArray<Byte> *allocBuffer(Engine &e, size_t count);

		// Allocate an object of the given type (size used for sanity checking).
		void *allocObject(size_t size, Type *type);

		// Allocate an array of objects.
		void *allocArray(Engine &e, const GcType *type, size_t count);

		template <class T>
		inline GcArray<T> *allocArray(Engine &e, const GcType *type, size_t count) {
			return (GcArray<T> *)allocArray(e, type, count);
		}

		// Allocate an array of objects in response to a re-hash. Affects the behavior of the GC in
		// respect to scheduling collections.
		void *allocArrayRehash(Engine &e, const GcType *type, size_t count);

		template <class T>
		inline GcArray<T> *allocArrayRehash(Engine &e, const GcType *type, size_t count) {
			return (GcArray<T> *)allocArrayRehash(e, type, count);
		}

		// Allocate a weak array of pointers.
		void *allocWeakArray(Engine &e, size_t count);

		template <class T>
		inline GcWeakArray<T> *allocWeakArray(Engine &e, size_t count) {
			return (GcWeakArray<T> *)allocWeakArray(e, count);
		}

		// Allocate a weak array of pointers in response to a re-hash. Affects the behavior of the
		// GC in respect to scheduling collections.
		void *allocWeakArrayRehash(Engine &e, size_t count);

		template <class T>
		inline GcWeakArray<T> *allocWeakArrayRehash(Engine &e, size_t count) {
			return (GcWeakArray<T> *)allocWeakArrayRehash(e, count);
		}

		// Allocate a code segment with at least 'code' bytes of memory for the code, and 'refs'
		// entries for references.
		void *allocCode(Engine &e, size_t code, size_t refs);

		// Get the code size of a previous code allocation (references are not counted).
		size_t codeSize(const void *code);

		// Get the references for a block of code.
		GcCode *codeRefs(void *code);

		// Update all pointers in a code allocation. This copies all pointer values from the
		// 'codeRef' section into the data section.
		void codeUpdatePtrs(void *code);

		// Re-set the vtable of an object to what it should be.
		void setVTable(RootObject *object);

		// Check if the current TObject is a live object. Objects may be finalized by the GC before
		// they are collected, so we shall consider them as "nonexistent" during that time. This is
		// relevant in the implementation of containers like WeakSet.
		bool liveObject(RootObject *object);

		// Get the thread group to use for all threads.
		os::ThreadGroup &threadGroup(Engine &e);

		// Get the lock used to synchronize thread startup in the Thread class.
		util::Lock &threadLock(Engine &e);

		// Create a GcWatch object.
		GcWatch *createWatch(Engine &e);

		// Post an IO-request for standard in/out/error.
		void postStdRequest(Engine &e, StdRequest *request);

		// Clone any heap-allocated object from C++.
		RootObject *CODECALL cloneObject(RootObject *obj);
		RootObject *CODECALL cloneObjectEnv(RootObject *obj, CloneEnv *env);

		// Check consistency of an object. Only available when GC debugging is active. Not available
		// in shared libraries.
		void checkObject(Engine &e, const void *obj);

		// Get some engine. If a thread was created from a particular engine, that engine is
		// returned. This is intended to be used in cases where exceptions (rarely) need to be
		// thrown, not for general use.
		Engine &someEngine();

		// Get some engine, but may return null. The version above asserts on error.
		Engine *someEngineUnsafe();


	}
}
