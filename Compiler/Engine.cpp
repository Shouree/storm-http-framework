#include "stdafx.h"
#include "Engine.h"
#include "Init.h"
#include "Type.h"
#include "Package.h"
#include "Code.h"
#include "Function.h"
#include "Core/Hash.h"
#include "Core/Thread.h"
#include "Core/Str.h"
#include "Core/Map.h"
#include "Core/Set.h"
#include "Core/StrBuf.h"
#include "Core/Handle.h"
#include "Core/Io/Utf8Text.h"
#include "Core/Convert.h"
#include "Core/Variant.h"
#include "Lib/Enum.h"
#include "Lib/Fn.h"
#include "Lib/Maybe.h"
#include "Lib/Join.h"
#include "Syntax/Node.h"
#include "Utils/Memory.h"
#include "Utils/StackInfoSet.h"
#include "StdIoThread.h"
#include "Visibility.h"
#include "Exception.h"
#include "SystemException.h"
#include "OS/StackTrace.h"

// Only included from here:
#include "OS/SharedMaster.h"

namespace storm {

	static THREAD Engine *currentEngine;
	namespace runtime {
		Engine *someEngineUnsafe() {
			return currentEngine;
		}
	}

	// Default arena size. 256 MB should be enough for a while at least.
	static const size_t defaultArena = 256 * 1024 * 1024;

	// Default finalizer interval.
	static const nat defaultFinalizer = 1000;

	// Get the compiler thread from flags.
	static os::Thread mainThread(Engine::ThreadMode mode, Engine &e) {
		switch (mode) {
		case Engine::newMain:
			return os::Thread::spawn(util::Fn<void>(), e.threadGroup);
		case Engine::reuseMain:
			os::Thread::initThread(); // TODO: Call cleanThread as well..
			e.attachThread();
			currentEngine = &e;
			return os::Thread::current();
		default:
			assert(false, L"Unknown thread mode.");
			WARNING(L"Unknown thread mode, defaulting to 'current'.");
			return os::Thread::current();
		}
	}

	void Engine::attachThread() {
		gc.attachThread();
		currentEngine = this;
	}

	void Engine::detachThread() {
		gc.detachThread(os::Thread::current());
		currentEngine = null;
	}

	vector<os::Thread> Engine::allThreads() {
		vector<os::Thread> threads = threadGroup.threads();

		os::Thread compiler = world.threads[0]->thread();
		if (std::find(threads.begin(), threads.end(), compiler) == threads.end())
			threads.insert(threads.begin(), compiler);

		return threads;
	}

	// Starts at -1 so that the first Engine will get id=0.
	static Nat engineId = -1;

	template <class T>
	struct ValFlags {
		static const nat pod = std::is_pod<T>::value ? typeCppPOD : typeNone;
		static const nat simple =
			(std::is_trivially_copy_constructible<T>::value & std::is_trivially_destructible<T>::value)
			? typeCppSimple : typeNone;

	public:
		static const TypeFlags v = TypeFlags(typeValue | pod | simple);
	};


	Engine::Engine(const Path &root, ThreadMode mode, void *stackBase) :
		id(atomicIncrement(engineId)),
		gc(defaultArena, defaultFinalizer),
		threadGroup(util::memberVoidFn(this, &Engine::attachThread), util::memberVoidFn(this, &Engine::detachThread)),
		world(gc),
		objRoot(null),
		ioThread(null),
		stackInfo(gc) {

		// Handle system errors.
		setupSystemExceptions();

		bootStatus = bootNone;

		stackId = ::stackInfo().attach(stackInfo);

		// Tell the thread system about the 'stackBase' we received.
		os::Thread::setStackBase(stackBase);

		// Initialize the roots we need.
		memset(&o, 0, sizeof(GcRoot));
		objRoot = gc.createRoot(&o, sizeof(GcRoot) / sizeof(void *));

		assert(Compiler::identifier == 0, L"Invalid ID for the compiler thread. Check CppTypes for errors!");

		try {
			// Since all types in the name tree need the Compiler thread, we need to create that a bit early.
			world.threads.resize(1);
			{
				os::Thread t = mainThread(mode, *this);
				// Ensure 'mainThread' is executed before operator new.
				world.threads[0] = new (Thread::First(*this)) Thread(t);
			}

			// Initialize the type system. This loads all types defined in the compiler.
			initTypes(*this, world);

			// Now, we can give the Compiler thread object a proper header with a type. Until this
			// point, doing as<> on that object will crash the system. However, that is not neccessary
			// this early during compiler startup.
			Thread::stormType(*this)->setType(world.threads[0]);

			// Set proper paths for the now created packages.
#ifdef WINDOWS
			o.root->setUrl(parsePath(*this, toS(root).c_str())->updated());
#else
			o.root->setUrl(parsePath(*this, toWChar(*this, toS(root).c_str())->v)->updated());
#endif

			// Done booting.
			advance(bootDone);
		} catch (...) {
			destroy();
			throw;
		}
	}

	Engine::~Engine() {
		// Do not place anything directly here, we need the cleanup when creation fails.
		destroy();
	}

	void Engine::destroy() {
		advance(bootShutdown);
		libs.shutdown();

		// Perform a GC now, to execute as many finalizers as possible now. We might actually need
		// to compile some destructors during shutdown...
		gc.collect();

		advance(bootLateShutdown);
		// We need to remove the root this array implies before the Gc is destroyed.
		world.clear();

		Gc::destroyRoot(objRoot);
		delete ioThread;
		// volatile Engine *volatile me = this;

		::stackInfo().detach(stackId);
		stackInfo.clear();

		gc.destroy();
		libs.unload();
		threadGroup.join();

		// TODO: Execute the sanity check in the UThreadData destructor now. Otherwise we might do
		// it at a point where we can not do cout properly.
	}

	Type *Engine::cppType(Nat id) const {
		return world.types[id];
	}

	TemplateList *Engine::cppTemplate(Nat id) const {
		if (id >= world.templates.count())
			return null;
		return world.templates[id];
	}

	Thread *Engine::cppThread(Nat id) const {
		return world.threads[id];
	}

	void Engine::forNamed(NamedFn fn) {
		world.forNamed(fn);

		// TODO: Do more packages need this?
		package()->lateInit();
	}

	/**
	 * Helpers for the pointer handle.
	 */

	static void tObjToS(const void *obj, StrBuf *to) {
		const TObject *o = *(const TObject **)obj;
		*to << o;
	}

	static Nat tObjHash(const void *obj) {
		const void *ptr = *(const void **)obj;
		return ptrHash(ptr);
	}

	static Bool tObjEqual(const void *a, const void *b) {
		const void *aa = *(const void **)a;
		const void *bb = *(const void **)b;
		return aa == bb;
	}

	const Handle &Engine::tObjHandle() {
		if (!o.tObjHandle) {
			o.tObjHandle = new (*this) Handle();
			o.tObjHandle->size = sizeof(void *);
			o.tObjHandle->locationHash = true;
			o.tObjHandle->gcArrayType = &pointerArrayType;
			o.tObjHandle->copyFn = null; // No special function, use memcpy.
			o.tObjHandle->deepCopyFn = null; // No need for deepCopy.
			o.tObjHandle->toSFn = &tObjToS;
			o.tObjHandle->hashFn = &tObjHash;
			o.tObjHandle->equalFn = &tObjEqual;
		}
		return *o.tObjHandle;
	}

	static void objDeepCopy(void *obj, CloneEnv *env) {
		Object *&o = *(Object **)obj;
		cloned(o, env);
	}

	static void objToS(const void *obj, StrBuf *to) {
		const Object *o = *(const Object **)obj;
		*to << o;
	}

	static Nat objHash(const void *obj) {
		const void *ptr = *(const void **)obj;
		return ptrHash(ptr);
	}

	static Bool objEqual(const void *a, const void *b) {
		const void *aa = *(const void **)a;
		const void *bb = *(const void **)b;
		return aa == bb;
	}

	const Handle &Engine::refObjHandle() {
		if (!o.refObjHandle) {
			o.refObjHandle = new (*this) Handle();
			o.refObjHandle->size = sizeof(void *);
			o.refObjHandle->locationHash = true;
			o.refObjHandle->gcArrayType = &pointerArrayType;
			o.refObjHandle->copyFn = null;
			o.refObjHandle->deepCopyFn = &objDeepCopy;
			o.refObjHandle->toSFn = &objToS;
			o.refObjHandle->hashFn = &objHash;
			o.refObjHandle->equalFn = &objEqual;
		}
		return *o.refObjHandle;
	}

	void Engine::advance(BootStatus to) {
		assert(to >= bootStatus, L"Trying to devolve the boot status.");
		bootStatus = to;
	}

	ScopeLookup *Engine::scopeLookup() {
		if (!o.rootLookup) {
			util::Lock::L z(createLock);
			if (!o.rootLookup)
				o.rootLookup = new (*this) ScopeLookup();
		}

		return o.rootLookup;
	}

	Scope Engine::scope() {
		return Scope(package(), scopeLookup());
	}

	code::Arena *Engine::arena() {
		if (!o.arena)
			o.arena = code::arena(*this);

		return o.arena;
	}

	VTableCalls *Engine::vtableCalls() {
		if (!o.vtableCalls)
			o.vtableCalls = new (*this) VTableCalls();

		return o.vtableCalls;
	}

	static const GcType voidArrayType = {
		GcType::tArray,
		null,
		null,
		0,
		0, {}
	};

	static void voidToS(const void *, StrBuf *to) {
		*to << L"void";
	}

	static Nat voidHash(const void *) {
		return 0;
	}

	static Bool voidEqual(const void *, const void *) {
		return true;
	}

	const Handle &Engine::voidHandle() {
		if (!o.voidHandle) {
			o.voidHandle = new (*this) Handle();
			o.voidHandle->size = 0;
			o.voidHandle->gcArrayType = &voidArrayType;
			o.voidHandle->toSFn = &voidToS;
			o.voidHandle->hashFn = &voidHash;
			o.voidHandle->equalFn = &voidEqual;
		}

		return *o.voidHandle;
	}

	code::Ref Engine::ref(builtin::BuiltIn ref) {
		code::RefSource **r = &o.refs[ref];
		if (!*r)
			*r = createRef(ref);

		return code::Ref(*r);
	}

	static void *allocType(Type *t) {
		return runtime::allocObject(0, t);
	}

	static void *allocArray(Type *t, size_t count) {
		return runtime::allocArray(t->engine, t->gcArrayType(), count);
	}

	static RootObject *stormAs(RootObject *in, Type *type) {
		if (!in)
			return null;
		if (in->isA(type))
			return in;
		return null;
	}

	static void fnNull() {
		// The null function in Storm!
	}

	static void CODECALL throwException(Exception *ex) {
		ex->throwMe();
	}

	static void CODECALL throwAbstractError(Str *identifier) {
		throw new (identifier) AbstractFnCalled(identifier);
	}

	static void CODECALL createValVariant(void *obj, const void *src, Type *type) {
		new (Place(obj)) Variant(src, type);
	}

	static void CODECALL createClassVariant(void *obj, Object *src) {
		new (Place(obj)) Variant(src);
	}

	code::RefSource *Engine::createRef(builtin::BuiltIn ref) {
#define W(x) S(x)
#define FNREF(x) arena()->externalSource(S("C++:") W(#x), address(&x))

		switch (ref) {
		case builtin::engine:
			return arena()->externalSource(S("engine"), this);
		case builtin::lazyCodeUpdate:
			return FNREF(LazyCode::updateCode);
		case builtin::ruleThrow:
			return FNREF(syntax::Node::throwError);
		case builtin::alloc:
			return FNREF(allocType);
		case builtin::allocArray:
			return FNREF(allocArray);
		case builtin::as:
			return FNREF(stormAs);
		case builtin::VTableAllocOffset:
			return arena()->externalSource(S("vtableAllocOffset"), (const void *)VTableCpp::vtableAllocOffset());
		case builtin::TObjectOffset:
			return arena()->externalSource(S("threadOffset"), (const void *)OFFSET_OF(TObject, thread));
		case builtin::mapAtValue:
			return FNREF(MapBase::atRawValue);
		case builtin::mapAtClass:
			return FNREF(MapBase::atRawClass);
		case builtin::mapGetUnsafe:
			return FNREF(MapBase::getUnsafeRaw);
		case builtin::setGetUnsafe:
			return FNREF(SetBase::getUnsafeRaw);
		case builtin::enumToS:
			return FNREF(Enum::toString);
		case builtin::futurePost:
			return FNREF(FutureBase::postRaw);
		case builtin::futureResult:
			return FNREF(FutureBase::resultRaw);
		case builtin::futureNoClone:
			return FNREF(FutureBase::setNoClone);
		case builtin::spawnResult:
			return FNREF(spawnThreadResult);
		case builtin::spawnFuture:
			return FNREF(spawnThreadFuture);
		case builtin::spawnId:
			return FNREF(spawnThreadId);
		case builtin::fnNeedsCopy:
			return FNREF(FnBase::needsCopy);
		case builtin::fnCall:
			return FNREF(fnCallRaw);
		case builtin::fnCreate:
			return FNREF(fnCreateRaw);
		case builtin::fnNull:
			return FNREF(fnNull);
		case builtin::maybeToS:
			return FNREF(MaybeValueType::toSHelper);
		case builtin::globalAddr:
			return FNREF(GlobalVar::dataPtr);
		case builtin::throwAbstractError:
			return FNREF(throwAbstractError);
		case builtin::throwException:
			return FNREF(throwException);
		case builtin::createValVariant:
			return FNREF(createValVariant);
		case builtin::createClassVariant:
			return FNREF(createClassVariant);
		case builtin::createJoin:
			return FNREF(createJoin);
		default:
			assert(false, L"Unknown reference: " + ::toS(ref));
			return null;
		}
	}

	Visibility *Engine::visibility(VisType t) {
		Visibility *&r = o.visibility[t];
		if (!r)
			r = createVisibility(t);
		return r;
	}

	Visibility *Engine::createVisibility(VisType t) {
		switch (t) {
		case vPublic:
			return new (*this) Public();
		case vTypePrivate:
			return new (*this) TypePrivate();
		case vTypeProtected:
			return new (*this) TypeProtected();
		case vPackagePrivate:
			return new (*this) PackagePrivate();
		case vFilePrivate:
			return new (*this) FilePrivate();
		default:
			assert(false, L"Unknown visibility: " + ::toS(t));
			return null;
		}
	}

	StdIo *Engine::stdIo() {
		if (!ioThread) {
			util::Lock::L z(createLock);
			if (!ioThread) {
				ioThread = new StdIo();
			}
		}

		return ioThread;
	}

	TextInput *Engine::stdIn() {
		if (!o.stdIn)
			o.stdIn = new (*this) Utf8Input(proc::in(*this));
		return o.stdIn;
	}

	TextOutput *Engine::stdOut() {
		if (!o.stdOut)
			o.stdOut = new (*this) Utf8Output(proc::out(*this), sysTextInfo());
		return o.stdOut;
	}

	TextOutput *Engine::stdError() {
		if (!o.stdError)
			o.stdError = new (*this) Utf8Output(proc::error(*this), sysTextInfo());
		return o.stdError;
	}

	void Engine::stdIn(TextInput *to) {
		o.stdIn = to;
	}

	void Engine::stdOut(TextOutput *to) {
		o.stdOut = to;
	}

	void Engine::stdError(TextOutput *to) {
		o.stdError = to;
	}

	Array<Str *> *Engine::argv() {
		return atomicRead(o.argv);
	}

	void Engine::argv(Array<Str *> *val) {
		atomicWrite(o.argv, val);
	}

	code::TypeDesc *Engine::ptrDesc() {
		if (!o.ptrDesc)
			o.ptrDesc = code::ptrDesc(*this);
		return o.ptrDesc;
	}

	code::TypeDesc *Engine::voidDesc() {
		if (!o.voidDesc)
			o.voidDesc = code::voidDesc(*this);
		return o.voidDesc;
	}

	static RootObject *CODECALL readObject(RootObject **src) {
		return *src;
	}

	Function *Engine::readObjFn() {
		if (!o.readObj) {
			Value t(Object::stormType(*this));
			o.readObj = nativeFunction(*this, t, S("_read_"), valList(*this, 1, t.asRef()), address(&readObject));
			o.readObj->parentLookup = o.root;
		}
		return o.readObj;
	}

	Function *Engine::readTObjFn() {
		if (!o.readTObj) {
			Value t(TObject::stormType(*this));
			o.readTObj = nativeFunction(*this, t, S("_readT_"), valList(*this, 1, t.asRef()), address(&readObject));
			o.readTObj->parentLookup = o.root;
		}
		return o.readTObj;
	}

	Package *Engine::package() {
		if (!o.root) {
			assert(has(bootTypes), L"Can not create packages yet.");
			o.root = new (*this) Package(new (*this) Str(S("<root>")));
		}
		return o.root;
	}

	Package *Engine::corePackage() {
		if (!o.corePkg)
			o.corePkg = package(S("core"));
		return o.corePkg;
	}

	Package *Engine::package(SimpleName *name, bool create) {
		Package *now = package();

		for (nat i = 0; i < name->count(); i++) {
			SimplePart *p = name->at(i);

			// Not supported.
			if (p->params->any())
				return null;

			Named *next = now->find(p, Scope());
			if (!next) {
				Package *pkg = new (*this) Package(p->name);
				now->add(pkg);
				now = pkg;
			} else if (Package *p = as<Package>(next)) {
				now = p;
			} else {
				return null;
			}
		}

		return now;
	}

	Package *Engine::package(const wchar *name) {
		SimpleName *n = parseSimpleName(*this, name);
		return package(n, false);
	}

	NameSet *Engine::nameSet(SimpleName *name, bool create) {
		return nameSet(name, package(), create);
	}

	NameSet *Engine::nameSet(SimpleName *name, NameSet *now, bool create) {
		for (nat i = 0; i < name->count(); i++) {
			SimplePart *p = name->at(i);

			// Not supported.
			if (p->params->any())
				return null;

			Named *next = now->find(p, Scope());
			if (!next && create) {
				Package *pkg = new (*this) Package(p->name);
				now->add(pkg);
				now = pkg;
			} else if (NameSet *p = as<NameSet>(next)) {
				now = p;
			} else {
				return null;
			}
		}

		return now;
	}

	static Package *findChild(Package *p, Url *path, Nat start) {
		for (Nat i = start; p && i < path->count(); i++) {
			// TODO: Make sure we handle case sensitivity correctly!
			p = as<Package>(p->find(path->at(i), Scope()));
		}
		return p;
	}

	MAYBE(Package *) Engine::package(Url *path) {
		Map<Url *, Package *> *lookup = pkgMap();

		for (Url *at = path; !at->empty(); at = at->parent()) {
			if (Package *p = lookup->get(at, null)) {
				return findChild(p, path, at->count());
			}
		}

		return null;
	}

	Map<Url *, Package *> *Engine::pkgMap() {
		if (!o.pkgMap)
			o.pkgMap = new (*this) Map<Url *, Package *>();
		return o.pkgMap;
	}

	SharedLib *Engine::loadShared(Url *file) {
		return libs.load(file);
	}


	Engine::StormInfo::StormInfo(Gc &gc) : gc(gc) {}

	Engine::StormInfo::~StormInfo() {
		clear();
	}

	void Engine::StormInfo::clear() {
		if (!roots.empty()) {
			WARNING(L"The registered roots for stack traces were not empty. Possible leak!");
		}

		for (RootMap::iterator i = roots.begin(); i != roots.end(); ++i)
			gc.destroyRoot(i->second);
		roots.clear();
	}

	void Engine::StormInfo::alloc(StackFrame *frames, nat count) const {
		Gc::Root *r = gc.createRoot(frames, count * sizeof(StackFrame), true);
		roots.insert(std::make_pair(size_t(frames), r));
	}

	void Engine::StormInfo::free(StackFrame *frames, nat count) const {
		RootMap::iterator i = roots.find(size_t(frames));
		if (i != roots.end()) {
			gc.destroyRoot(i->second);
			roots.erase(i);
		}
	}

	bool Engine::StormInfo::translate(void *ip, void *&fnBase, int &offset) const {
		return false;
	}

	void Engine::StormInfo::format(GenericOutput &to, void *fnBase, int offset) const {}

	/**
	 * Interface which only exists in the compiler. Dynamic libraries have their own implementations
	 * which forward the calls to these implementations.
	 */

	Thread *DeclThread::thread(Engine &e) const {
		return e.cppThread(identifier);
	}

	void gc(EnginePtr e) {
		e.v.gc.collect();
	}

}

// Get the global StackInfoSet.
DEFINE_STACK_INFO()
