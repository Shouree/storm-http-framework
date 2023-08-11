#include "stdafx.h"
#include "Zero.h"

#if STORM_GC == STORM_GC_ZERO
#include "Gc.h"

#ifdef POSIX
#include <sys/mman.h>
#endif

namespace storm {

	// Word-align something.
	static inline size_t align(size_t data) {
		return (data + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1);
	}

	static const size_t headerSizeWords = 4;
	static const size_t allocChunk = 1024*1024; // 1MB at a time.

#if defined(WINDOWS)

	void GcImpl::newPool() {
		allocStart = (byte *)VirtualAlloc(null, allocChunk, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		allocEnd = allocStart + allocChunk;
		totalOsAlloc += allocChunk;
	}

#elif defined(POSIX)

	void GcImpl::newPool() {
		allocStart = (byte *)mmap(null, allocChunk, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		allocEnd = allocStart + allocChunk;
		totalOsAlloc += allocChunk;
	}

#endif

	void *GcImpl::poolAlloc(size_t bytes) {
		util::Lock::L z(allocLock);

		if (size_t(allocEnd - allocStart) < bytes)
			newPool();

		void *result = allocStart;
		allocStart += bytes;
		return result;
	}

	GcImpl::GcImpl(size_t, nat) : allocStart(null), allocEnd(null) {}

	void GcImpl::destroy() {}

	MemorySummary GcImpl::summary() {
		MemorySummary s;
		s.objects = totalAlloc;
		s.allocated = s.reserved = totalOsAlloc;
		return s;
	}

	void GcImpl::collect() {}

	Bool GcImpl::collect(Nat time) {
		return false;
	}

	GcImpl::ThreadData GcImpl::attachThread() {
		return 0;
	}

	void GcImpl::detachThread(ThreadData &) {}

	void *GcImpl::alloc(const GcType *type) {
		size_t size = align(type->stride) + headerSizeWords*sizeof(size_t);
		void *mem = poolAlloc(size);
		memset(mem, 0, size);
		memset(mem, 0xFF, headerSizeWords*sizeof(size_t));

		*(const GcType **)mem = type;
		void *start = (size_t *)mem + headerSizeWords;
		assert(typeOf(start) == type);
		return start;
	}

	void *GcImpl::allocStatic(const GcType *type) {
		return alloc(type);
	}

	GcArray<Byte> *GcImpl::allocBuffer(size_t count) {
		return (GcArray<Byte> *)allocArray(&byteArrayType, count);
	}

	void *GcImpl::allocArray(const GcType *type, size_t count) {
		size_t size = align(type->stride)*count + 2*sizeof(size_t) + headerSizeWords*sizeof(size_t);
		void *mem = poolAlloc(size);
		memset(mem, 0, size);
		memset(mem, 0xFF, headerSizeWords*sizeof(size_t));

		*(const GcType **)mem = type;

		void *start = (size_t *)mem + headerSizeWords;
		*(size_t *)start = count;
		return start;
	}

	void *GcImpl::allocArrayRehash(const GcType *type, size_t count) {
		return allocArray(type, count);
	}

	void *GcImpl::allocWeakArray(const GcType *type, size_t count) {
		size_t size = sizeof(type->stride)*count + 2*sizeof(size_t) + headerSizeWords*sizeof(size_t);
		void *mem = poolAlloc(size);
		memset(mem, 0, size);
		memset(mem, 0xFF, headerSizeWords*sizeof(size_t));

		*(const GcType **)mem = type;

		void *start = (size_t *)mem + headerSizeWords;
		*(size_t *)start = (count << 1) | 1;
		return start;
	}

	void *GcImpl::allocWeakArrayRehash(const GcType *type, size_t count) {
		return allocWeakArray(type, count);
	}

	Bool GcImpl::liveObject(RootObject *obj) {
		return true;
	}

	GcType *GcImpl::allocType(GcType::Kind kind, Type *type, size_t stride, size_t entries) {
		size_t s = gcTypeSize(entries);
		GcType *t = (GcType *)malloc(s);
		memset(t, 0, s);
		t->kind = kind;
		t->type = type;
		t->stride = stride;
		t->count = entries;
		return t;
	}

	void GcImpl::freeType(GcType *type) {
		free(type);
	}

	const GcType *GcImpl::typeOf(const void *mem) {
		const GcType **data = (const GcType **)mem;

		// Crude memory validation...
		// for (nat i = 0; i < (headerSizeWords-1)*sizeof(size_t); i++) {
		// 	byte *addr = (byte *)data - i - 1;
		// 	if (*addr != 0xFF) {
		// 		PLN(L"Wrote before the object: " << mem << L" offset: -" << (i+1));
		// 	}
		// }

		return *(data - headerSizeWords);
	}

	void GcImpl::switchType(void *mem, const GcType *to) {
		const GcType **data = (const GcType **)mem;
		*(data - headerSizeWords) = to;
	}

	void *GcImpl::allocCode(size_t code, size_t refs) {
		code = align(code);
		size_t size = code + sizeof(GcCode) + refs*sizeof(GcCodeRef) - sizeof(GcCodeRef) + headerSizeWords*sizeof(size_t);
		void *mem = poolAlloc(size);
		memset(mem, 0, size);
		memset((size_t *)mem + 1, 0xFF, (headerSizeWords - 1)*sizeof(size_t));

		*(size_t *)mem = code;

		void *start = (void **)mem + headerSizeWords;
		void *refPtr = (byte *)start + code;
		*(size_t *)refPtr = refs;

		return start;
	}

	size_t GcImpl::codeSize(const void *alloc) {
		const size_t *d = (const size_t *)alloc;
		return *(d - headerSizeWords);
	}

	GcCode *GcImpl::codeRefs(void *alloc) {
		void *p = ((byte *)alloc) + align(codeSize(alloc));
		return (GcCode *)p;
	}

	void GcImpl::startRamp() {}

	void GcImpl::endRamp() {}

	void GcImpl::walkObjects(WalkCb fn, void *param) {
		// Nothing to do...
	}

	GcImpl::Root *GcImpl::createRoot(void *data, size_t count, bool ambiguous) {
		// No roots here!
		return null;
	}

	void GcImpl::destroyRoot(Root *root) {}

	class MallocWatch : public GcWatch {
	public:
		virtual void add(const void *addr) {}
		virtual void remove(const void *addr) {}
		virtual void clear() {}
		virtual bool moved() { return false; }
		virtual bool moved(const void *addr) { return false; }
		virtual GcWatch *clone() const {
			return new MallocWatch();
		}
	};

	GcWatch *GcImpl::createWatch() {
		return new MallocWatch();
	}

	void GcImpl::checkMemory() {}

	void GcImpl::checkMemory(const void *object, bool recursive) {
		const GcType **data = (const GcType **)object;

		for (nat i = 0; i < (headerSizeWords-1)*sizeof(size_t); i++) {
			byte *addr = (byte *)data - i - 1;
			if (*addr != 0xFF) {
				PLN(L"Wrote before the object: " << object << L" offset: -" << (i+1));
			}
		}
	}

	void GcImpl::checkMemoryCollect() {}

	void GcImpl::dbg_dump() {}

}

#endif
