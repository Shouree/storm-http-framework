#pragma once

/**
 * Note: This file is designed to be included from other projects (namely, Compiler) without needing
 * to have full knowledge of which GC is the active one at the moment. Thus, care needs to be taken
 * when editing this file not to depend on such knowledge.
 *
 * More specifically, even though GcImpl is known, we sometimes use a dummy interface (in
 * SampleImpl.h) with an incorrect size. Thus, we may not depend on knowledge of sizeof(GcImpl) in
 * this file (in the .cpp-file we can do this, however).
 *
 * This also means that it is dangerous to make things inside the various GC implementations inline.
 */

#include "Utils/Exception.h"
#include "Utils/Lock.h"

#include "Core/GcType.h"
#include "Core/GcArray.h"
#include "Core/GcWatch.h"
#include "Core/GcCode.h"

// In case we're included from somewhere other than the Gc module.
#include "License.h"
#include "Format.h" // for fmt::wordAlign
#include "SampleImpl.h"
#include "Root.h"

#ifdef STORM_GC

/**
 * We were included from a file in the Gc tree:
 * Include all possible GC implementations. Only one will be selected.
 */
#include "Zero.h"
#include "MPS/Impl.h"
#include "SMM/Impl.h"
#include "Debug.h"

#ifndef STORM_HAS_GC
// If this happens, an unselected GC has been selected in Core/Storm.h.
#error "No GC selected!"
#endif

#endif


namespace storm {

	class Type;

	/**
	 * This is the interface to the garbage collector. To reliably run a GC, we need to be able to
	 * describe where each type stores its pointers, and possibly what finalizers to execute.
	 *
	 * Note: Any types describing data layout should be allocated from the corresponding Gc class,
	 * as special care might need to be taken not to interfere with the garbage collector.
	 *
	 * Storm supports multiple garbage collectors. See 'Core/Storm.h' for details on how to select
	 * between them. This class mainly performs sanity checking and forwards calls to the selected
	 * back-end. The design does not support switching garbage collectors during run-time. See
	 * "Malloc.h" for a simple implementation of the GC interface using malloc. This can be used as
	 * a template for new GC implementations.
	 */
	class Gc : NoCopy {
	public:
		// Create.
		// 'initialArenaSize' - an initial estimate of the arena size. May be disregarded by the gc if needed.
		// 'finalizationInterval' - how seldom the gc should check for finalizations. An interval of 500 means
		//                          every 500 allocations.
		Gc(size_t initialArenaSize, nat finalizationInterval);

		// Destroy.
		~Gc();

		// Destroy the Gc before the destructor is executed.
		void destroy();

		// Memory information.
		MemorySummary summary();


		/**
		 * Manual garbage collection hints.
		 */

		// Do a full garbage collection now.
		void collect();

		// Spend approx 'time' ms on an incremental collection if possible. Returns true if there is more to do.
		bool collect(Nat time);

		// TODO: Add interface for managing pause times and getting information about allocations.


		/**
		 * Thread management.
		 */

		// Register the current thread for use with the gc.
		void attachThread();

		// Re-register a thread for use with the gc. Can not be used for new threads.
		void reattachThread(const os::Thread &thread);

		// Unregister a thread from the gc. This has to be done before the thread is destroyed.
		void detachThread(const os::Thread &thread);

		// Get the thread data for a thread, given a pointer to the implementation. Returns
		// "default" if the thread is not registered with the GC.
		static GcImpl::ThreadData threadData(GcImpl *from, const os::Thread &thread, const GcImpl::ThreadData &def);


		/**
		 * Memory allocation.
		 */

		// Allocate an object of a specific type. Assumes type->type != t(Weak)Array.
		inline void *alloc(const GcType *type) {
			assert(type->kind == GcType::tType
				|| type->kind == GcType::tFixed
				|| type->kind == GcType::tFixedObj,
				L"Wrong type for calling alloc().");

			return impl->alloc(type);
		}

		// Allocate an object of a specific type in a non-moving pool. We assume that non-moving
		// objects are rare, and they may thus be implemented less efficient than movable objects.
		inline void *allocStatic(const GcType *type) {
			assert(type->kind == GcType::tType
				|| type->kind == GcType::tFixed
				|| type->kind == GcType::tFixedObj,
				L"Wrong type for calling allocStatic().");

			return impl->allocStatic(type);
		}

		// Allocate a buffer which is not moving nor protected. The memory allocated from here is
		// also safe to access from threads unknown to the garbage collector.
		inline GcArray<Byte> *allocBuffer(size_t count) {
			return impl->allocBuffer(count);
		}

		// Allocate an array of objects. Assumes type->type == tArray.
		inline void *allocArray(const GcType *type, size_t count) {
			assert(type->kind == GcType::tArray, L"Wrong type for calling allocArray().");

			return impl->allocArray(type, count);
		}

		// Allocate an array of weak pointers.
		inline void *allocWeakArray(size_t count) {
			return impl->allocWeakArray(&weakArrayType, count);
		}

		// See if the object is live. An object is considered live until it has been
		// finalized. Finalized objects may not be collected immediately after they have been
		// finalized, and therefore they may still appear inside weak sets etc. after that. The GC
		// implementation marks such objects, so that it is possible to see if they are finalized.
		static inline Bool liveObject(RootObject *obj) {
			return GcImpl::liveObject(obj);
		}


		/**
		 * Management of Gc types.
		 *
		 * These objects are semi-managed by the GC. They are not collected automatically, but they
		 * are destroyed together with the GC object. That means, small leaks of GcType objects are
		 * not a problem as long as they don't grow over time.
		 *
		 * GC backends may collect these automatically if they desire, but are not required to do so.
		 */

		// Allocate a gc type.
		GcType *allocType(GcType::Kind kind, Type *type, size_t stride, size_t entries);
		GcType *allocType(const GcType *original);

		// Deallocate a gc type.
		void freeType(GcType *type);

		// Get the gc type of an allocation.
		static inline const GcType *typeOf(const void *mem) {
			return GcImpl::typeOf(mem);
		}

		// Change the gc type of an allocation. Used during startup to change header from a fake one
		// (to allow basic scanning) to a real one (with a proper Type) when startup has progressed
		// further.
		static void switchType(void *mem, const GcType *to);


		/**
		 * Allocation of code storage. Code allocations are slightly more complex than regular data
		 * allocations. First and foremost, this memory needs to be executable, but it also contains
		 * references to other objects in rather weird formats (for example, relative
		 * offsets). Aside from that, there is also the requirement that we may not have a header on
		 * these objects, even though they are generally different size (so we can not use GcArray,
		 * which stores its size at offset 0).
		 *
		 * To meet these requirements, a code allocation consist of two parts: one code part and one
		 * metadata part. The code part is first, and at some later point (the gc keeps track of
		 * this), the metadata block begins. Here, information about all references in the current
		 * block are stored.
		 */

		// Allocate a code block with 'code' bytes of machine code storage and 'refs' entries of reference data.
		inline void *allocCode(size_t code, size_t refs) {
			return impl->allocCode(fmt::wordAlign(code), refs);
		}

		// Get the size of a code allocation.
		static inline size_t codeSize(const void *alloc) {
			return GcImpl::codeSize(alloc);
		}

		// Access the metadata of a code allocation. Note: all references must be scannable at *any* time.
		static inline GcCode *codeRefs(void *alloc) {
			return GcImpl::codeRefs(alloc);
		}


		/**
		 * Notify the GC that we're trying to allocate a fairly large data structure from this
		 * thread in the near future, most of which will be dead right after the work is
		 * complete. This hint could be ignored by the underlying implementation.
		 */
		class RampAlloc {
		public:
			RampAlloc(Gc &owner);
			~RampAlloc();

		private:
			RampAlloc(const RampAlloc &);
			RampAlloc &operator =(const RampAlloc &);

			Gc &owner;
		};


		/**
		 * Iterate through all objects on the heap.
		 *
		 * Note: inside the callback it is only possible to access the object which is passed to the
		 * function and any memory on the stack (this means, for example, it is not possible to use
		 * as<> or call any virtual members in the object). Do not take locks nor attempt to perform
		 * a thread switch inside the callback.
		 */

		// Callback function for a heap walk.
		typedef void (*WalkCb)(RootObject *inspect, void *param);

		// Walk the heap. This usually incurs a full Gc, so it is not a cheap operation.
		void walkObjects(WalkCb fn, void *param);


		/**
		 * Roots.
		 */

		typedef GcRoot Root;

		// Allocate a root that scans an array of pointers. 'count' is the number of pointers
		// contained in the array, not the size of the allocation.
		inline Root *createRoot(void *data, size_t count) {
			return createRoot(data, count, false);
		}

		inline Root *createRoot(void *data, size_t count, bool ambiguous) {
			Root *r = impl->createRoot(data, count, ambiguous);
			if (r) {
				util::Lock::L z(rootLock);
				roots.insert(r);
				r->owner = this;
			}
			return r;
		}

		// Destroy a root.
		static inline void destroyRoot(Root *root) {
			if (!root)
				return;

			if (root->owner) {
				GcImpl::destroyRoot(root);

				util::Lock::L z(root->owner->rootLock);
				root->owner->roots.erase(root);
				root->owner = null;
			}

			delete root;
		}


		/**
		 * Watch object.
		 */

		inline GcWatch *createWatch() {
			return impl->createWatch();
		}


		/**
		 * Debugging/testing.
		 */

		// Stress-test the gc by allocating a large number of objects.
		bool test(nat times = 100);

		// Check memory consistency. Note: Enable checking in 'Gc.cpp' for this to work.
		void checkMemory();

		// Check consistency of a single object. Note: Enable checking in 'Gc.cpp' for this to work.
		void checkMemory(const void *object, bool recursive);

		// Do a gc and check memory consistency (sometimes forces memory issues to appear better than
		// just calling 'checkMemory').
		void checkMemoryCollect();

		// Dump memory information to stdout.
		void dbg_dump();

		// License.
		const GcLicense *license();

	private:
		// GcType for weak arrays.
		static const GcType weakArrayType;

		// GC-specific things. Defined in the header file corresponding to the GC:s entry point.
		// Called with a reference to this object, so that it can be initialized properly.
		GcImpl *impl;

		// Data for a thread.
		struct ThreadData {
			size_t refs;
			GcImpl::ThreadData data;
		};

		// Remember all threads.
		typedef map<uintptr_t, ThreadData> ThreadMap;
		ThreadMap threads;

		// Lock for manipulating the attached threads.
		util::Lock threadLock;

		// Set of all roots, so that we can destroy them when the GC is shut down.
		os::InlineSet<Root> roots;

		// Lock for the root set.
		util::Lock rootLock;

		// Destroyed already?
		Bool destroyed;
	};


	/**
	 * Error thrown when interacting with the GC.
	 *
	 * This error is thrown in contexts where it does not make sense to allocate a Storm
	 * exception. For example, when it is not possible to initialize the GC for some reason.
	 *
	 * For the GC error known by Storm, look in Core/Exception.h.
	 */
	class BasicGcError : public Exception {
	public:
		BasicGcError(const String &msg) : msg(msg) {}
		virtual String what() const { return msg; }
	private:
		String msg;
	};


}
