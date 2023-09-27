#pragma once

#include "Gc/Gc.h"
#include "BootStatus.h"
#include "World.h"
#include "Scope.h"
#include "BuiltIn.h"
#include "VTableCall.h"
#include "SharedLibs.h"
#include "Code/Arena.h"
#include "Code/RefSource.h"
#include "Code/Reference.h"
#include "Utils/Lock.h"
#include "Utils/StackInfo.h"

// TODO: Do not depend on path!
#include "Utils/Path.h"

namespace storm {

	class Thread;
	class Package;
	class NameSet;
	class ScopeLookup;
	class StdIo;
	class TextInput;
	class TextOutput;
	class Visibility;

	/**
	 * Defines the root object of the compiler. This object contains everything needed by the
	 * compiler itself, and shall be kept alive as long as anything from the compiler is used. Two
	 * separate instances of this class can be used as two completely separate runtime environments.
	 *
	 * TODO? Break this into two parts; one that contains stuff needed from external libraries, and
	 * one that is central to the compiler.
	 */
	class Engine : NoCopy {
	private:
		// The ID of this engine. Used in shared libraries. This must be the first member of the
		// Engine class.
		Nat id;

	public:
		// Threading mode for the engine.
		enum ThreadMode {
			// Create a new main thread for the compiler (this is the storm::Compiler thread) to use
			// for compiling code. This means that the compiler will manage itself, but care must be
			// taken not to manipulate anything below the engine in another thread.
			newMain,

			// Reuses the caller of the constructor as the compiler thread (storm::Compiler). This
			// makes it easier to interface with the compiler itself, since all calls can be made
			// directly into the compiler, but care must be taken to allow the compiler to process
			// any messages sent to the compiler by calling UThread::leave(), or make the thread
			// wait in a os::Lock or os::Sema for events.
			reuseMain,
		};

		// Create the engine.
		// 'root' is the location of the root package directory on disk. The package 'core' is
		// assumed to be found as a subdirectory of the given root path.
		// 'stackBase' is the base of the current thread's stack. Eg. the address of argc and/or
		// argv or some other variable allocated on the stack near 'main'.
		// TODO: Do not depend on Util/Path!
		Engine(const Path &root, ThreadMode mode, void *stackBase);

		// Destroy. This will wait until all threads have terminated properly.
		~Engine();

		// Interface to the GC we're using.
		Gc gc;

		// Get a C++ type by its id.
		Type *cppType(Nat id) const;

		// Get a C++ template by its id.
		TemplateList *cppTemplate(Nat id) const;

		// Get a C++ named thread by its id.
		Thread *cppThread(Nat id) const;

		// Current boot status.
		inline BootStatus boot() const { return bootStatus; }
		inline bool has(BootStatus s) { return boot() >= s; }

		// Advance boot status.
		void advance(BootStatus to);

		// Run the function for all named objects in the cache.
		typedef void (*NamedFn)(Named *);
		void forNamed(NamedFn fn);

		// Get the one and only pointer handle for TObject.
		const Handle &tObjHandle();

		// Get a handle usable to treat regular objects as references in hash containers.
		const Handle &refObjHandle();

		// The threadgroup which all threads spawned from here shall belong to.
		os::ThreadGroup threadGroup;

		// The lock that is used to synchronize thread creation in storm::Thread::thread().
		// A regular util::Lock is used since it needs to be recursive when threads are reused.
		util::Lock threadLock;


		/**
		 * Packages.
		 *
		 * All package lookup performed from here will completely disregard visibility; they are
		 * indended for 'supervisor' usage.
		 */

		// Get the root package.
		Package *package();

		// Get the core package. Stored specially since it is often needed.
		Package *corePackage();

		// Get a package relative to the root. If 'create', the package will be created. No
		// parameters are supported in the name.
		Package *package(SimpleName *name, bool create = false);

		// Find a NameSet relative to the root. If 'create', creates packages along the way.
		NameSet *nameSet(SimpleName *name, bool create = false);
		NameSet *nameSet(SimpleName *name, NameSet *root, bool create = false);

		// Parse a string into a simple name and get the corresponding package.
		Package *package(const wchar *name);

		// Get the package corresponding to a certain path. Will try to load any packages not
		// already loaded. Returns null if none is found.
		MAYBE(Package *) package(Url *path);

		// Access the global map of packages.
		Map<Url *, Package *> *pkgMap();

		/**
		 * Scopes.
		 */

		// Get the root scope.
		Scope scope();

		// Get the default scope lookup.
		ScopeLookup *scopeLookup();

		/**
		 * Code generation.
		 */

		// The arena used for code generation for this platform.
		code::Arena *arena();

		// VTable call stubs.
		VTableCalls *vtableCalls();

		// Get the one and only Handle object for void.
		const Handle &voidHandle();

		// Get a reference to a function in the runtime.
		code::Ref ref(builtin::BuiltIn ref);

		// Default visibility objects.
		enum VisType {
			vPublic,
			vTypePrivate,
			vTypeProtected,
			vPackagePrivate,
			vFilePrivate,

			// Should be last.
			visCount,
		};

		// Get a visibility object.
		Visibility *visibility(VisType type);

		// Get the StdIo object.
		StdIo *stdIo();

		// Get stdin, stdout and stderr. Note: get/set from Storm using interface in Lib/Io.h.
		TextInput *stdIn();
		TextOutput *stdOut();
		TextOutput *stdError();

		// Set std streams.
		void stdIn(TextInput *to);
		void stdOut(TextOutput *to);
		void stdError(TextOutput *to);

		// Get/set argv array. Interface for accessing it is in Lib/Argv.h
		// These functions are atomic. The getter might return null.
		Array<Str *> *argv();
		void argv(Array<Str *> *val);

		// Get 'TypeDesc' objects that are frequently used in the system.
		code::TypeDesc *ptrDesc();
		code::TypeDesc *voidDesc();

		// Get a single shared instance of a function that reads objects from references.
		Function *readObjFn();
		Function *readTObjFn();

		// Load a shared library.
		SharedLib *loadShared(Url *file);

		// Functions called on thread attach/detach.
		void attachThread();
		void detachThread();

		// Print a summary of all threads in the system. Accessible from Storm as debug.threadSummary().
		void threadSummary();

	private:
		// The compiler C++ world.
		World world;

		// How far along in the boot process?
		BootStatus bootStatus;

		/**
		 * GC:d objects.
		 */
		struct GcRoot {
			// Handle for TObject.
			Handle *tObjHandle;

			// Handle for reference hashes to Objects.
			Handle *refObjHandle;

			// Void handle.
			Handle *voidHandle;

			// Root package.
			Package *root;

			// Core package (populated on demand).
			Package *corePkg;

			// Quick lookup from path to package.
			Map<Url *, Package *> *pkgMap;

			// Root scope lookup.
			ScopeLookup *rootLookup;

			// Arena.
			code::Arena *arena;

			// VTableCalls.
			code::VTableCalls *vtableCalls;

			// References.
			code::RefSource *refs[builtin::count];

			// TypeDesc objects that are used a lot throughout the system.
			code::TypeDesc *ptrDesc;
			code::TypeDesc *voidDesc;

			// Read Objects or TObjects from references.
			Function *readObj;
			Function *readTObj;

			// Default visibility objects.
			Visibility *visibility[visCount];

			// Handles to readers for stdin, stdout and stderror.
			TextInput *stdIn;
			TextOutput *stdOut;
			TextOutput *stdError;

			// Argv visible to the system.
			Array<Str *> *argv;
		};

		GcRoot o;

		// Root for GcRoot.
		Gc::Root *objRoot;

		// Loaded libraries.
		SharedLibs libs;

		// Standard IO thread.
		StdIo *ioThread;

		// Lock used for syncronizing object creation.
		util::Lock createLock;

		// Create references.
		code::RefSource *createRef(builtin::BuiltIn which);

		// Create visibility objects.
		Visibility *createVisibility(VisType t);

		// Plug into the stack traces in order to properly scan the stack traces.
		class StormInfo : public StackInfo {
		public:
			StormInfo(Gc &gc);
			~StormInfo();

			// Clear all allocations from the MPS.
			void clear();

			virtual void alloc(StackFrame *frames, nat count) const;
			virtual void free(StackFrame *frames, nat count) const;

			virtual bool translate(void *ip, void *&fnBase, int &offset) const;
			virtual void format(GenericOutput &to, void *fnBase, int offset) const;

		private:
			Gc &gc;

			// Current allocations.
			typedef map<size_t, Gc::Root *> RootMap;
			mutable RootMap roots;
		};

		// StormInfo instance and ID.
		StormInfo stackInfo;
		int stackId;

		// Cleanup code.
		void destroy();
	};


	STORM_PKG(core);
	// Force garbage collection from Storm.
	void STORM_FN gc(EnginePtr e);

}
