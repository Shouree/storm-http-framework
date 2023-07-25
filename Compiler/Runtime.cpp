#include "stdafx.h"
#include "Runtime.h"
#include "Gc/Gc.h"
#include "Type.h"
#include "Engine.h"
#include "Core/Str.h"
#include "Core/Io/StdStream.h"
#include "Gc/Code.h"
#include "StdIoThread.h"

namespace storm {
	namespace runtime {

		/**
		 * Implements the functions declared in 'Core/Runtime.h' for the compiler.
		 */

		Type *cppType(Engine &e, Nat id) {
			return e.cppType(id);
		}

		Type *cppTemplateVa(Engine &e, Nat id, Nat count, va_list l) {
			const nat maxCount = 16;
			assert(count < maxCount, L"Too many template parameters used: " + ::toS(count) + L" max " + ::toS(maxCount));

			TemplateList *tList = e.cppTemplate(id);
			if (!tList)
				return null;

			Nat params[maxCount];
			for (nat i = 0; i < count; i++)
				params[i] = va_arg(l, Nat);

			return tList->find(params, count);
		}

		const Handle &typeHandle(Type *t) {
			return t->handle();
		}

		const Handle &voidHandle(Engine &e) {
			return e.voidHandle();
		}

		const Handle &refObjHandle(Engine &e) {
			return e.refObjHandle();
		}

		Type *typeOf(const RootObject *o) {
			return Gc::typeOf(o)->type;
		}

		const GcType *typeGc(Type *t) {
			return t->gcType();
		}

		Str *typeName(Type *t) {
			return t->shortIdentifier();
		}

		Str *typeIdentifier(Type *t) {
			return mangleName(t->path());
		}

		static Type *CODECALL fromIdentifierI(Str *name) {
			Engine &e = name->engine();
			return lookupMangledName(e.scope(), name);
		}

		MAYBE(Type *) fromIdentifier(Str *name) {
			// Note: This needs to be thread-safe!
			// It is called during deserialization (for example) to find types. This may be
			// done on other threads than the compiler thread.
			const os::Thread &t = Compiler::thread(name->engine())->thread();
			if (t != os::Thread::current()) {
				os::Future<Type *> f;
				os::FnCall<Type *> p = os::fnCall().add(name);
				os::UThread::spawn(address(&fromIdentifierI), false, p, f, &t);
				return f.result();
			} else {
				return fromIdentifierI(name);
			}
		}

		bool isValue(Type *t) {
			return (t->typeFlags & typeValue) != 0;
		}

		const GcType *gcTypeOf(const void *alloc) {
			return Gc::typeOf(alloc);
		}

		bool isA(const Type *a, const Type *b) {
			return a->chain->isA(b);
		}

		bool isA(const RootObject *a, const Type *t) {
			return typeOf(a)->chain->isA(t);
		}

		Engine &allocEngine(const RootObject *o) {
			return Gc::typeOf(o)->type->engine;
		}

		void *allocRaw(Engine &e, const GcType *type) {
			return e.gc.alloc(type);
		}

		void *allocStaticRaw(Engine &e, const GcType *type) {
			return e.gc.allocStatic(type);
		}

		GcArray<Byte> *allocBuffer(Engine &e, size_t count) {
			return e.gc.allocBuffer(count);
		}

		static NOINLINE void allocFailSize(const GcType *t, size_t size) {
			PLN(L"Invalid type description found! " << size << L" vs " << t->stride);
			debugAssertFailed();
		}

		static NOINLINE void allocFailType(const GcType *t, Type *type) {
			PLN(L"Invalid type reference found! GcType: " << (void *)t << L", actual: " << type);
			debugAssertFailed();
		}

		void *allocObject(size_t size, Type *type) {
			const GcType *t = type->gcType();
#ifdef DEBUG
			// Try to keep the failure case out of the hot code, even in debug mode.
			if (size > t->stride)
				allocFailSize(t, size);
			if (t->type != type)
				allocFailType(t, type);
#endif
			return type->engine.gc.alloc(t);
		}

		void *allocArray(Engine &e, const GcType *type, size_t count) {
			return e.gc.allocArray(type, count);
		}

		void *allocArrayRehash(Engine &e, const GcType *type, size_t count) {
			return e.gc.allocArrayRehash(type, count);
		}

		void *allocWeakArray(Engine &e, size_t count) {
			return e.gc.allocWeakArray(count);
		}

		void *allocWeakArrayRehash(Engine &e, size_t count) {
			return e.gc.allocWeakArrayRehash(count);
		}

		GcWatch *createWatch(Engine &e) {
			return e.gc.createWatch();
		}

		void *allocCode(Engine &e, size_t code, size_t refs) {
			return e.gc.allocCode(code, refs);
		}

		size_t codeSize(const void *code) {
			return Gc::codeSize(code);
		}

		GcCode *codeRefs(void *code) {
			return Gc::codeRefs(code);
		}

		void codeUpdatePtrs(void *code) {
			gccode::updatePtrs(code, Gc::codeRefs(code));
		}

		void setVTable(RootObject *object) {
			typeOf(object)->vtable()->insert(object);
		}

		bool liveObject(RootObject *object) {
			return Gc::liveObject(object);
		}

		os::ThreadGroup &threadGroup(Engine &e) {
			return e.threadGroup;
		}

		util::Lock &threadLock(Engine &e) {
			return e.threadLock;
		}

		void attachThread(Engine &e) {
			e.gc.attachThread();
		}

		void detachThread(Engine &e, const os::Thread &thread) {
			e.gc.detachThread(thread);
		}

		void reattachThread(Engine &e, const os::Thread &thread) {
			e.gc.reattachThread(thread);
		}

		void postStdRequest(Engine &e, StdRequest *request) {
			e.stdIo()->post(request);
		}

		RootObject *cloneObject(RootObject *obj) {
			if (obj == null)
				return null;

			// Nothing needs to be done for TObjects.
			if (TObject *t = as<TObject>(obj))
				return t;

			return cloneObjectEnv(obj, new (obj) CloneEnv());
		}

		RootObject *cloneObjectEnv(RootObject *obj, CloneEnv *env) {
			if (obj == null)
				return null;

			// Nothing needs to be done for TObjects.
			if (TObject *t = as<TObject>(obj))
				return t;

			Object *src = (Object *)obj;

			if (Object *prev = env->cloned(src))
				return prev;

			Type *t = typeOf(src);
			const GcType *gcType = t->gcType();
			void *mem = t->engine.gc.alloc(gcType);

			Type::CopyCtorFn ctor = t->rawCopyConstructor();
			if (ctor) {
				(*ctor)(mem, src);
			} else {
				// No copy constructor... Well, then we do it the hard way!
				memcpy(mem, src, gcType->stride);
			}

			Object *result = (Object *)mem;
			result->deepCopy(env);

			env->cloned(src, result);
			return result;
		}

		void checkObject(Engine &e, const void *obj) {
			e.gc.checkMemory(obj, false);
		}

		// Only used from Code/, not a part of the public API.
		Gc &engineGc(Engine &e) {
			return e.gc;
		}

	}
}
