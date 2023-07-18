#pragma once

#if STORM_GC == STORM_GC_MPS
#define STORM_HAS_GC

#include "Lib.h"
#include "Gc/MemorySummary.h"
#include "Gc/License.h"
#include "Gc/Root.h"

namespace storm {

	struct GcThread;


	// For MPS: use a special non-moving pool for IO buffers. The MPS manual states that the LO pool
	// is suitable for this task as it neither protects nor moves its objects. The AMCZ could also be
	// a decent fit, as it does not protect the objects in the pool and does not move the objects
	// when there is an ambiguous reference to the objects.
	// NOTE: The documentation does not tell wether segmens in LO pools can be 'nailed' by ambiguous
	// pointers to other parts than the start of the object. This is neccessary and provided by AMCZ.
	// Possible values: <undefined>, LO = 1, AMCZ = 2
#define MPS_USE_IO_POOL 2

	/**
	 * The integration of MPS to Storm's GC interface.
	 */
	class GcImpl {
	public:
		// Create.
		GcImpl(size_t initialArenaSize, Nat finalizationInterval);

		// Destroy. This function is always called, but may be called twice.
		void destroy();

		// Memory usage summary.
		MemorySummary summary();

		// Do a full GC now.
		void collect();

		// Spend approx. 'time' ms on a GC. Return 'true' if there is more work to be done.
		Bool collect(Nat time);

		// Type we use to store data with a thread.
		typedef GcThread *ThreadData;

		// Register/deregister a thread with us. The Gc interface handles re-registering for us. It
		// even makes sure that these functions are not called in parallel.
		ThreadData attachThread();
		void detachThread(ThreadData data);

		// Allocate an object of a specific type.
		void *alloc(const GcType *type);

		// Allocate an object of a specific type in a non-moving pool.
		void *allocStatic(const GcType *type);

		// Allocate a buffer which is not moving nor protected. The memory allocated from here is
		// also safe to access from threads unknown to the garbage collector.
		GcArray<Byte> *allocBuffer(size_t count);

		// Allocate an array of objects.
		void *allocArray(const GcType *type, size_t count);

		// Allocate an array of objects in response to stale location dependency.
		void *allocArrayRehash(const GcType *type, size_t count);

		// Allocate an array of weak pointers. 'type' is always a GcType instance that is set to
		// WeakArray with one pointer as elements.
		void *allocWeakArray(const GcType *type, size_t count);

		// Allocate a weak array in response to a stale location dependency.
		void *allocWeakArrayRehash(const GcType *type, size_t count);

		// See if an object is live, ie. not finalized.
		static Bool liveObject(RootObject *obj);

		// Allocate a gc type.
		GcType *allocType(GcType::Kind kind, Type *type, size_t stride, size_t entries);

		// Free a gc type (GC implementations may use garbage collection for these as well).
		void freeType(GcType *type);

		// Get the gc type of an allocation.
		static const GcType *typeOf(const void *mem);

		// Change the gc type of an allocation. Can assume that the new type describes a type of the
		// same size as the old type description.
		static void switchType(void *mem, const GcType *to);

		// Allocate a code block with 'code' bytes of machine code and 'refs' entries of reference data.
		void *allocCode(size_t code, size_t refs);

		// Get the size of a code allocation.
		static size_t codeSize(const void *alloc);

		// Get the metadata of a code allocation.
		static GcCode *codeRefs(void *alloc);

		// Start/end of a ramp allocation.
		void startRamp();
		void endRamp();

		// Walk the heap.
		typedef void (*WalkCb)(RootObject *inspect, void *param);
		void walkObjects(WalkCb fn, void *param);

		typedef GcRoot Root;

		// Create a root object.
		Root *createRoot(void *data, size_t count, bool ambiguous);

		// Destroy a root.
		static void destroyRoot(Root *root);

		// Create a watch object (on a GC:d heap, no need to destroy it).
		GcWatch *createWatch();

		// Check memory consistency. Note: Enable checking in 'Gc.cpp' for this to work.
		void checkMemory();
		void checkMemory(const void *object, bool recursive);
		void checkMemoryCollect();

		// Debug output.
		void dbg_dump();

		// License.
		const GcLicense *license();

	private:
		friend class MpsGcWatch;

		// Current arena, pool and format.
		mps_arena_t arena;
		mps_pool_t pool;
		mps_fmt_t format;
		mps_chain_t chain;

		// Separate non-protected pool for GcType objects.
		mps_pool_t gcTypePool;

		// Separate non-moving pool for storing Type-objects. Note: we only have one allocation
		// point for the types since they are rarely allocated. This pool is also used when
		// interfacing with external libraries which require their objects to not move around.
		mps_pool_t typePool;
		mps_ap_t typeAllocPoint;
		util::Lock typeAllocLock;

		// Pool for objects with weak references.
		mps_pool_t weakPool;
		mps_ap_t weakAllocPoint;
		util::Lock weakAllocLock;

		// Special allocation point for 'pool' above. Used when we re-allocate hashtables due to
		// stale location dependencies.
		mps_ap_t hashReallocAp;
		util::Lock hashReallocLock;

#ifdef MPS_USE_IO_POOL
		// Non-moving, non-protected pool for interaction with foreign code (eg. IO operations in
		// the operating system).
		mps_pool_t ioPool;
		mps_ap_t ioAllocPoint;
		util::Lock ioAllocLock;
#endif

		// Pool for runnable code.
		mps_pool_t codePool;
		mps_ap_t codeAllocPoint;
		util::Lock codeAllocLock;

		// Root for exceptions in-flight.
		mps_root_t exRoot;

		// Finalization interval (our copy).
		nat finalizationInterval;

		// Allocate an object in the Type pool.
		void *allocTypeObj(const GcType *type);

		// Attach/detach thread. Sets up/tears down all members in Thread, nothing else.
		void attach(GcThread *thread, const os::Thread &oThread);
		void detach(GcThread *thread);

		// Find the allocation point for the current thread. May run finalizers.
		mps_ap_t &currentAllocPoint();

		// See if there are any finalizers to run at the moment.
		void checkFinalizers();

		// Second stage. Assumes we have exclusive access to the message stream.
		void checkFinalizersLocked();

		// Finalize an object.
		void finalizeObject(void *obj);

		// Are we running finalizers at the moment? Used for synchronization.
		volatile nat runningFinalizers;

		// Struct used for GcTypes inside MPS. As it is not always possible to delete all GcType objects
		// immediatly, we store the freed ones in an InlineSet until we can actually reclaim them.
		struct MpsType : public os::SetMember<MpsType> {
			// Constructor. If we don't provide a constructor, 'type' will be default-initialized to
			// zero (from C++11 onwards), which is wasteful. The actual implementation of the
			// constructor will be inlined anyway.
			MpsType();

			// Reachable?
			bool reachable;

			// The actual GcType. Must be last, as it contains a 'dynamic' array.
			GcType type;
		};

		// Check the return value from an MPS function, throws an exception as appropriate on failure.
		void check(mps_res_t result, const wchar *msg);

		// Throw an error as appropriate.
		void throwError(const wchar *msg);

		// All freed GcType-objects which have not yet been reclaimed.
		os::InlineSet<MpsType> freeTypes;

		// Is this object fully initialized? Used for determining what type of exceptions to throw
		// during startup or teardown.
		bool initialized;

		// During destruction - ignore any freeType() calls?
		bool ignoreFreeType;

		// Size of an MpsType.
		static size_t mpsTypeSize(size_t offsets);

		// Worker function for 'scanning' the MpsType objects.
		static void markType(mps_addr_t addr, mps_fmt_t fmt, mps_pool_t pool, void *p, size_t);

		// Internal helper for 'checkMemory()'.
		friend void checkObject(mps_addr_t addr, mps_fmt_t fmt, mps_pool_t pool, void *p, size_t);

		// Check memory, assuming the object is in a GC pool.
		void checkPoolMemory(const void *object, bool recursive);
	};

}

#endif
