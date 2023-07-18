#include "stdafx.h"
#include "Engine.h"
#include "OS/Sync.h"

namespace storm {

	// Shared function pointers.
	static EngineFwdShared fwd = { 0 };

	// Unique function pointers. One for each observed engine-id.
	static EngineFwdUnique *unique = null;
	static Nat uniqueCount = 0;
	static Nat uniqueFilled = 0;
	static os::Lock uniqueLock;

	// Empty (ie. all bytes = 0)?
	bool memempty(const void *src, size_t count) {
		const byte *s = (const byte *)src;
		bool zero = true;
		for (size_t i = 0; i < count; i++)
			zero &= s[i] == 0;
		return zero;
	}

	void *Engine::attach(const EngineFwdShared &shared, const EngineFwdUnique &unique) {
		if (memempty(&fwd, sizeof(EngineFwdShared))) {
			fwd = shared;
		} else {
			assert(memcmp(&fwd, &shared, sizeof(EngineFwdShared)) == 0,
				L"Using two different implementations of Engines with the same instance of a shared library!");
		}

		{
			os::Lock::L z(uniqueLock);
			Nat count = id + 1;
			if (storm::uniqueCount < count) {
				EngineFwdUnique *n = new EngineFwdUnique[count];
				memset(n, 0, count*sizeof(EngineFwdUnique));
				if (storm::uniqueCount > 0)
					memcpy(n, storm::unique, uniqueCount*sizeof(EngineFwdUnique));

				EngineFwdUnique *old = storm::unique;
				atomicCAS(storm::unique, old, n);
				delete []old;
				storm::uniqueCount = count;
			}

			storm::uniqueFilled++;
		}

		EngineFwdUnique &result = storm::unique[id];
		if (!result.identifier) {
			result = unique;
			return null;
		} else {
			return result.identifier;
		}
	}

	void Engine::detach() {
		os::Lock::L z(uniqueLock);

		if (--uniqueFilled == 0) {
			// Last one. Delete the unique array as no one needs it anymore.
			delete []unique;
			uniqueCount = 0;
		}
	}

	void *Engine::data() {
		const EngineFwdUnique &u = unique[identifier()];
		return (*u.getLibData)(*this, u.identifier);
	}


	namespace runtime {

		Type *cppType(Engine &e, Nat id) {
			const EngineFwdUnique &u = unique[e.identifier()];
			return (*u.cppType)(e, u.identifier, id);
		}

		Type *cppTemplateVa(Engine &e, Nat id, Nat count, va_list l) {
			const EngineFwdUnique &u = unique[e.identifier()];
			return (*u.cppTemplateVa)(e, u.identifier, id, count, l);
		}

		const Handle &typeHandle(Type *t) {
			return (*fwd.typeHandle)(t);
		}

		const Handle &voidHandle(Engine &e) {
			return (*fwd.voidHandle)(e);
		}

		const Handle &refObjHandle(Engine &e) {
			return (*fwd.refObjHandle)(e);
		}

		Type *typeOf(const RootObject *o) {
			return (*fwd.typeOf)(o);
		}

		const GcType *typeGc(Type *t) {
			return (*fwd.typeGc)(t);
		}

		Str *typeName(Type *t) {
			return (*fwd.typeName)(t);
		}

		Str *typeIdentifier(Type *t) {
			return (*fwd.typeIdentifier)(t);
		}

		MAYBE(Type *) fromIdentifier(Str *name) {
			return (*fwd.fromIdentifier)(name);
		}

		bool isValue(Type *t) {
			return (*fwd.isValue)(t);
		}

		const GcType *gcTypeOf(const void *alloc) {
			return (*fwd.gcTypeOf)(alloc);
		}

		bool isA(const Type *a, const Type *b) {
			return (*fwd.typeIsA)(a, b);
		}

		bool isA(const RootObject *a, const Type *b) {
			return (*fwd.isA)(a, b);
		}

		Engine &allocEngine(const RootObject *o) {
			return (*fwd.allocEngine)(o);
		}

		void *allocRaw(Engine &e, const GcType *type) {
			return (*fwd.allocRaw)(e, type);
		}

		void *allocStaticRaw(Engine &e, const GcType *type) {
			return (*fwd.allocStaticRaw)(e, type);
		}

		GcArray<Byte> *allocBuffer(Engine &e, size_t count) {
			return (*fwd.allocBuffer)(e, count);
		}

		void *allocObject(size_t size, Type *type) {
			return (*fwd.allocObject)(size, type);
		}

		void *allocArray(Engine &e, const GcType *type, size_t count) {
			return (*fwd.allocArray)(e, type, count);
		}

		void *allocArrayRehash(Engine &e, const GcType *type, size_t count) {
			return (*fwd.allocArrayRehash)(e, type, count);
		}

		void *allocWeakArray(Engine &e, size_t count) {
			return (*fwd.allocWeakArray)(e, count);
		}

		void *allocWeakArrayRehash(Engine &e, size_t count) {
			return (*fwd.allocWeakArrayRehash)(e, count);
		}

		void *allocCode(Engine &e, size_t code, size_t refs) {
			return (*fwd.allocCode)(e, code, refs);
		}

		size_t codeSize(const void *code) {
			return (*fwd.codeSize)(code);
		}

		GcCode *codeRefs(void *code) {
			return (*fwd.codeRefs)(code);
		}

		void codeUpdatePtrs(void *code) {
			(*fwd.codeUpdatePtrs)(code);
		}

		void setVTable(RootObject *object) {
			(*fwd.setVTable)(object);
		}

		bool liveObject(RootObject *object) {
			return (*fwd.liveObject)(object);
		}

		os::ThreadGroup &threadGroup(Engine &e) {
			return (*fwd.threadGroup)(e);
		}

		util::Lock &threadLock(Engine &e) {
			return (*fwd.threadLock)(e);
		}

		GcWatch *createWatch(Engine &e) {
			return (*fwd.createWatch)(e);
		}

		void postStdRequest(Engine &e, StdRequest *request) {
			(*fwd.postStdRequest)(e, request);
		}

		RootObject *cloneObject(RootObject *obj) {
			return (*fwd.cloneObject)(obj);
		}

		RootObject *cloneObjectEnv(RootObject *obj, CloneEnv *env) {
			return (*fwd.cloneObjectEnv)(obj, env);
		}

		Engine *someEngineUnsafe() {
			return (*fwd.someEngineUnsafe)();
		}

	}

	Thread *DeclThread::thread(Engine &e) const {
		const EngineFwdUnique &u = unique[e.identifier()];
		return (*u.getThread)(e, u.identifier, this);
	}

}


namespace os {

	ThreadData *currentThreadData() {
		return (*storm::fwd.getCurrentThreadData)();
	}

	void currentThreadData(ThreadData *data) {
		(*storm::fwd.setCurrentThreadData)(data);
	}

	UThreadState *currentUThreadState() {
		return (*storm::fwd.getCurrentUThreadState)();
	}

	void currentUThreadState(UThreadState *state) {
		(*storm::fwd.setCurrentUThreadState)(state);
	}

	void threadCreated() {
		(*storm::fwd.threadCreated)();
	}

	void threadTerminated() {
		(*storm::fwd.threadTerminated)();
	}

}

StackInfoSet &stackInfo() {
	return (*storm::fwd.stackInfo)();
}
