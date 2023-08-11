#include "stdafx.h"
#include "Debug.h"

#if STORM_GC == STORM_GC_DEBUG
#include "Gc.h"
#include "Format.h"

namespace storm {

	// Header and footer contents and size.
#define HEADER_PATTERN 0xAA
#define FOOTER_PATTERN 0xBB
#define HEADER_SIZE 8
#define FOOTER_SIZE 8

	// Word-align something.
	static inline size_t align(size_t data) {
		return (data + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1);
	}

	static const size_t allocChunk = 5*1024*1024; // 5MB at a time.

#if defined(WINDOWS)

	void GcImpl::newPool() {
		Pool p;
		p.start = (byte *)VirtualAlloc(null, allocChunk, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		p.end = p.start;
		p.limit = p.start + allocChunk;

		pools.push_back(p);
	}

#elif defined(POSIX)

	void GcImpl::newPool() {
		Pool p;
		p.start = (byte *)mmap(null, allocChunk, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		p.end = p.start;
		p.limit = p.start + allocChunk;

		pools.push_back();
	}

#endif

	void *GcImpl::poolAlloc(size_t bytes) {
		util::Lock::L z(allocLock);

		size_t withHeader = bytes + HEADER_SIZE + FOOTER_SIZE + sizeof(size_t);

		if (pools.empty())
			newPool();

		Pool *l = &pools.back();
		if (l->end + withHeader >= l->limit) {
			newPool();
			l = &pools.back();
		}

		byte *start = l->end;
		l->end += withHeader;

		// Allocation size.
		*(size_t *)start = bytes;
		start += sizeof(size_t);
		for (size_t i = 0; i < HEADER_SIZE; i++) {
			*start = HEADER_PATTERN;
			start++;
		}

		for (size_t i = 0; i < FOOTER_SIZE; i++) {
			start[bytes + i] = FOOTER_PATTERN;
		}

		return start;
	}

	static void check(byte *at, byte pattern, size_t count, const wchar_t *msg) {
		bool ok = true;
		for (size_t i = 0; i < count; i++)
			ok &= at[i] == pattern;

		if (!ok) {
			PLN(L"Invalid " << msg << L" at " << (void *)at);
			for (size_t i = 0; i < count; i++)
				PLN(L"  " << toHex(at[i]));
		}
	}

	static size_t verify(void *ptr) {
		byte *at = (byte *)fmt::fromClient(ptr);

		check(at - HEADER_SIZE, HEADER_PATTERN, HEADER_SIZE, L"header");
		size_t size = *(size_t *)(at - HEADER_SIZE - sizeof(size_t));
		check(at + size, FOOTER_PATTERN, FOOTER_SIZE, L"footer");

		if (size != fmt::objSize((fmt::Obj *)at)) {
			PLN(L"Invalid object size at " << ptr);
			PLN(L"  Original: " << size);
			PLN(L"  Current : " << fmt::objSize((fmt::Obj *)at));
		}

		return size;
	}

	struct DbgScan {
		typedef int Result;
		typedef GcImpl Source;

		DbgScan(Source &source) : impl(source) {}

		GcImpl &impl;

		inline ScanOption object(void *start, void *end) { return scanAll; }
		inline bool fix1(void *ptr) { return impl.contains(ptr); }
		inline Result fix2(void **ptr) {
			verify(*ptr);
			return Result();
		}

		inline bool fixHeader1(GcType *header) { return impl.contains(header); }
		inline Result fixHeader2(GcType **header) {
			verify(*header);
			return Result();
		}
	};

	void GcImpl::verify(const Pool &p) {
		byte *at = p.start + HEADER_SIZE + sizeof(size_t);
		while (at < p.end) {
			size_t size = storm::verify(at + fmt::headerSize);

			fmt::Scan<DbgScan>::objects(*this, at + fmt::headerSize, at + fmt::headerSize + size);

			at += size + HEADER_SIZE + FOOTER_SIZE + sizeof(size_t);
		}
	}

	void GcImpl::verify() {
		util::Lock::L z(allocLock);

		for (size_t p = 0; p < pools.size(); p++) {
			verify(pools[p]);
		}
	}

	bool GcImpl::contains(void *ptr) {
		byte *b = (byte *)ptr;
		for (size_t i = 0; i < pools.size(); i++) {
			if (b >= pools[i].start && b < pools[i].end)
				return true;
		}

		return false;
	}

	GcImpl::GcImpl(size_t, nat) {}

	void GcImpl::destroy() {}

	MemorySummary GcImpl::summary() {
		MemorySummary s;
		// We could compute some summary...
		return s;
	}

	void GcImpl::collect() {
		verify();
	}

	Bool GcImpl::collect(Nat time) {
		collect();
		return false;
	}

	GcImpl::ThreadData GcImpl::attachThread() {
		return 0;
	}

	void GcImpl::detachThread(ThreadData &) {}

	void *GcImpl::alloc(const GcType *type) {
		size_t size = fmt::sizeObj(type);
		void *mem = poolAlloc(size);
		return fmt::initObj(mem, type, size);
	}

	void *GcImpl::allocStatic(const GcType *type) {
		return alloc(type);
	}

	GcArray<Byte> *GcImpl::allocBuffer(size_t count) {
		return (GcArray<Byte> *)allocArray(&byteArrayType, count);
	}

	void *GcImpl::allocArray(const GcType *type, size_t count) {
		size_t size = fmt::sizeArray(type, count);
		void *mem = poolAlloc(size);
		return fmt::initArray(mem, type, size, count);
	}

	void *GcImpl::allocArrayRehash(const GcType *type, size_t count) {
		return allocArray(type, count);
	}

	void *GcImpl::allocWeakArray(const GcType *type, size_t count) {
		size_t size = fmt::sizeArray(type, count);
		void *mem = poolAlloc(size);
		return fmt::initWeakArray(mem, type, size, count);
	}

	void *GcImpl::allocWeakArrayRehash(const GcType *type, size_t count) {
		return allocWeakArray(type, count);
	}

	Bool GcImpl::liveObject(RootObject *obj) {
		return true;
	}

	GcType *GcImpl::allocType(GcType::Kind kind, Type *type, size_t stride, size_t entries) {
		size_t s = gcTypeSize(entries) + fmt::headerSize;
		void *mem = poolAlloc(s);
		GcType *t = fmt::initGcType(mem, entries);
		t->kind = kind;
		t->type = type;
		t->stride = stride;
		return t;
	}

	void GcImpl::freeType(GcType *type) {}

	const GcType *GcImpl::typeOf(const void *mem) {
		const fmt::Obj *o = fmt::fromClient(mem);
		if (fmt::objIsCode(o))
			return null;
		else
			return &(fmt::objHeader(o)->obj);
	}

	void GcImpl::switchType(void *mem, const GcType *to) {
		fmt::objReplaceHeader(fmt::fromClient(mem), to);
	}

	void *GcImpl::allocCode(size_t code, size_t refs) {
		size_t size = fmt::sizeCode(code, refs);
		void *mem = poolAlloc(size);
		return fmt::initCode(mem, size, code, refs);
	}

	size_t GcImpl::codeSize(const void *alloc) {
		const fmt::Obj *o = fmt::fromClient(alloc);
		if (fmt::objIsCode(o)) {
			return fmt::objCodeSize(o);
		} else {
			dbg_assert(false, L"Attempting to get the size of a non-code block.");
			return 0;
		}
	}

	GcCode *GcImpl::codeRefs(void *alloc) {
		return fmt::refsCode(fmt::fromClient(alloc));
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

	void GcImpl::checkMemory(const void *, bool) {}

	void GcImpl::checkMemoryCollect() {}

	void GcImpl::dbg_dump() {}

}

#endif
