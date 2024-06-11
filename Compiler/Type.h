#pragma once
#include "NameSet.h"
#include "TypeChain.h"
#include "Gc/Gc.h"
#include "RunOn.h"
#include "RefHandle.h"
#include "Layout.h"
#include "VTable.h"
#include "Core/TypeFlags.h"
#include "OverridePart.h"

namespace storm {
	STORM_PKG(core.lang);

	// Description of a type in C++. Found in CppTypes.h
	struct CppType;
	class NamedThread;
	class Function;

	/**
	 * Description of a Type inside Storm.
	 *
	 * A type is either a *value*, a *class* or an *actor* (TObject). These have slightly different
	 * semantics. Most notably, values are always stored on the stack while the other two are
	 * heap-allocated. Furthermore, actors are associated with a specific thread, meaning that the
	 * system ensures that they are only accessed from that particular thread (it is, of course,
	 * possible to handle references to an actor from anywhere). It is convenient to wrap the type
	 * in a Value object to more easily query characteristics of a particular type. This also makes
	 * sure that references are handled properly.
	 *
	 * Furthermore, the class handles inheritance and the creation of various data structures
	 * required in the compiler, such as VTables (for virtual dispatch), type handles (for the
	 * templating mechanism used in Storm) and various wrapper functions.
	 *
	 * There are some peculiarities that need to be considered, mainly with regards to lazy loading:
	 *
	 * - During early compiler boot, all pointer members may be null even if not indicated
     *   otherwise. This includes name and parameters of the object, but also things like VTable and
     *   TypeChain. Code in C++ needs to be careful with regards to this.
	 *
	 * - VTables are only created when an object or any of its child objects are first instantiated.
	 *   Since the creation of VTables require loading all classes in an object hierarchy, eager
	 *   creation of VTables causes strange dependencies that causes seeminly unused types to be
	 *   loaded without reason just because some other type was added to the hierarchy in another
	 *   part of the system. Note that no VTables are created for value types.
	 */
	class Type : public NameSet {
		STORM_CLASS;

		// Let allocation access gcType.
		friend void *runtime::allocObject(size_t s, Type *t);

		// Let VTable access the raw version of our VTable.
		friend MAYBE(VTable *) rawVTable(Type *t);

	public:
		// Create a type declared in Storm.
		STORM_CTOR Type(Str *name, TypeFlags flags);
		STORM_CTOR Type(Str *name, Array<Value> *params, TypeFlags flags);

		STORM_CTOR Type(SrcPos pos, Str *name, TypeFlags flags);
		STORM_CTOR Type(SrcPos pos, Str *name, Array<Value> *params, TypeFlags flags);

	protected:
		// Create a type of a particular size, its contents defined by whatever is returned from
		// 'createDesc' and the supplied size rather than the members defined here. Only usable for
		// value types.
		STORM_CTOR Type(Str *name, Array<Value> *params, TypeFlags flags, Size size);

	public:
		// Create a type declared in C++.
		Type(Str *name, TypeFlags flags, Size size, GcType *gcType, const void *vtable);
		Type(Str *name, Array<Value> *params, TypeFlags flags, Size size, GcType *gcType, const void *vtable);

		// Destroy our resources.
		~Type();

		// Owning engine. Must be the first data member of this class!
		Engine &engine;

		// Make a GcType that describes an object inheriting from 'type'.
		static void makeType(Engine &e, GcType *type);

		// Create the type for Type (as this is special).
		static Type *createType(Engine &e, const CppType *type);

		// Set this type onto another object. This can *not* be used to trick the system into
		// resizing the object. This is only used once during startup.
		void setType(Object *object) const;

		// Set the super-class for this type.
		void STORM_FN setSuper(MAYBE(Type *) to);

		// Get the super-type for this type.
		inline MAYBE(Type *) STORM_FN super() const { return chain ? chain->super() : null; }

		// Get the declared super-type for this type. This ignores the implicit `Object` or `TObject` bases.
		MAYBE(Type *) STORM_FN declaredSuper() const;

		// Set the thread for this type. This will force the super-type to be TObject.
		Bool STORM_FN setThread(NamedThread *t);

		// Get where we want to run.
		RunOn STORM_FN runOn();

		// Is this type abstract? Ie. should we disallow instantiating the object? By default, a
		// type is abstract if it contains any abstract members, but languages may alter this
		// definition to better suit their needs.
		virtual Bool STORM_FN abstract();

		// Check if this type should be possible to instantiate (ie. it is not abstract) and throw
		// an error otherwise.
		void STORM_FN ensureNonAbstract(SrcPos pos);

		// Keep track of what is added.
		using NameSet::add;
		virtual void STORM_FN add(Named *item);

		// Modify the 'find' behaviour slightly so it also considers superclasses.
		virtual MAYBE(Named *) STORM_FN find(SimplePart *part, Scope source);
		using NameSet::find;

		// Get a reference to this object that can be used in code generation.
		virtual code::Ref STORM_FN typeRef();

		/**
		 * These functions are safe to call from any thread.
		 */

		// Get a handle for this type.
		const Handle &CODECALL handle();

		// Get the GcType for instances of this object. Lazily creates the information if it is
		// needed. For values, this is the same as gcArrayType.
		const GcType *CODECALL gcType();

		// Get the raw GcType for arrays of this type. Usually, the handle is preferred, but
		// sometimes in early boot that is not possible.
		const GcType *CODECALL gcArrayType();

		// Get this class's size. (NOTE: This function is safe to call from any thread. Use
		// Value::size() from Storm if you don't want to switch to the Compiler thread).
		Size STORM_FN size();

		// Get the VTable for this type.
		MAYBE(VTable *) STORM_FN vtable();

		/**
		 * The threadsafe part ends here.
		 */

		// Late initialization.
		virtual void lateInit();

		// Force layout of all member variables.
		void STORM_FN doLayout();

		// Get a list of all variables in the type.
		Array<MemberVar *> *STORM_FN variables() const;

		// Inspect the inheritance chain, and perform quick membership lookups.
		TypeChain *chain;

		// If the type was initialized during early boot, we need to initialize vtables through here
		// at a suitable time, before any Storm-defined types are created.
		void vtableInit(const void *vtable);

		// Find only in this type.
		MAYBE(Named *) STORM_FN findHere(SimplePart *part, Scope source);

		// Find only in this type, not trying to lazily load any missing content. Useful when one
		// wants to see what has happened so far without triggering excessive compilation too early
		// in the compilation process.
		MAYBE(Named *) STORM_FN tryFindHere(SimplePart *part, Scope source);

		// Helpers for the chain. Storm uses the free functions for this, as they are threadsafe.
		inline Bool isA(const Type *o) const { return chain->isA(o); }
		inline Int distanceFrom(const Type *from) const { return chain->distance(from); }

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Flags for this type.
		TypeFlags STORM_FN typeFlags() const { return myTypeFlags; }

		// Add a flag to the set of type flags.
		void STORM_FN addTypeFlag(TypeFlags flags);

		// Get a compact description of this type, used to know how this type shall be passed to
		// functions in the system.
		code::TypeDesc *STORM_FN typeDesc();

		// Names for constructors and destructors.
		static const wchar *CTOR;
		static const wchar *DTOR;

		// Find some nice-to-have functions. TODO: Make these throw on error?
		MAYBE(Function *) STORM_FN defaultCtor();
		MAYBE(Function *) STORM_FN copyCtor();
		MAYBE(Function *) STORM_FN assignFn();
		MAYBE(Function *) STORM_FN deepCopyFn();
		MAYBE(Function *) STORM_FN destructor();

		// Get a function that reads an instance of this type from a reference and returns a
		// value. This functionality is used to make sure accesses to variables in other threads are
		// safe. This function is created on demand.
		Function *STORM_FN readRefFn();

		// Get the raw destructor to be used for this type. Mainly used by the GC for finalization.
		typedef void (*DtorFn)(void *);
		DtorFn rawDestructor();

		// Get the raw copy constructor for this type. This differs from the one found in the handle
		// if this Type represents a GC:d object, as this function alwas operates on the actual
		// object and not just the pointer (as the Handle does).
		// Safe to call on threads other than the Compiler thread.
		typedef void (*CopyCtorFn)(void *, const void *);
		CopyCtorFn CODECALL rawCopyConstructor();

	protected:
		// Use the 'gcType' of the super class. Use only if no additional fields are introduced into
		// this class.
		void useSuperGcType();

		// Create a 'TypeDesc' for this type. Called the first time the 'TypeDesc' is needed, the
		// result is cached.
		virtual code::TypeDesc *STORM_FN createTypeDesc();

		// Modify the type handle, if desired. Called when the default construction is complete.
		virtual void STORM_FN modifyHandle(Handle *handle);

	private:
		// Special constructor for creating the first type.
		Type(Engine &e, TypeFlags flags, Size size, GcType *gcType);

		// Type flags. Private copy to prevent arbitrary modifications but allow a limited set of
		// modifications.
		TypeFlags myTypeFlags;

		// The description of the type we maintain for the GC. If we're a value type,
		// this will have 'kind' set to 'tArray'.
		// Note: the type member of the GcType this is pointing to must never be changed after this
		// member is changed. Otherwise we confuse the GC.
		GcType *myGcType;

		// Handle (lazily created). If we're a value, this will be a RefHandle.
		const Handle *tHandle;

		// The content we're using for all references in the handle, and in the type. TODO: Place it inside a RefSource?
		code::Content *myContent;

		// Get the content.
		code::Content *refContent();

		enum {
			// No toS function has been found yet.
			toSMissing = 0,
			// A toS function of the form toS() is used.
			toSNoParam = 1,
			// A toS function of the form toS(StrBuf) is used.
			toSWithParam = 2,
		};

		// What is the status of the toS function in the handle? Only valid whenever the handle has
		// been initialized.
		Byte handleToS;

		enum {
			abstractUnknown = 0,
			abstractNo = 1,
			abstractYes = 2,
		};

		// Cache for the 'abstract' function.
		Byte isAbstract;

		// Thread we should be running on if we indirectly inherit from TObject.
		NamedThread *useThread;

		// Generated type description. 'null' means that it has not yet been computed.
		code::TypeDesc *myTypeDesc;

		// Create a SimpleTypeDesc based on this type.
		code::SimpleDesc *createSimpleDesc();

		// Populate a SimpleTypeDesc based on this type.
		Nat populateSimpleDesc(MAYBE(code::SimpleDesc *) into);

		// Our size (including base classes). If zero, we need to re-compute it.
		Size mySize;

		// Compute the size of our super-class.
		Size superSize();

		// Get the default super type for us.
		Type *defaultSuper() const;

		// Internal helper for setSuper. Does the work associated with switching super classes, but
		// does not change 'super'.
		void updateSuper();

		// Special case for the first Type.
		static void *operator new(size_t size, Engine &e, GcType *type);
		static void operator delete(void *mem, Engine &e, GcType *type);

		// Common initialization. For C++ types, passes the vtable. May be null.
		void init(const void *vtable);

		// Is this a value type?
		inline bool value() const { return (myTypeFlags & typeValue) == typeValue; }

		// Generate a handle for this type.
		void buildHandle();

		// The current destructor to be used. Updated by 'rawDtorRef'.
		UNKNOWN(PTR_GC) DtorFn rawDtor;

		// Reference to the dtor to track changes. Possibly null.
		code::MemberRef * rawDtorRef;

		// Called whenever a new destructor is added.
		void updateDtor(Function *dtor);

		// Update finalizers in all child classes.
		void updateChildFinalizers();

		// Cache of the copy constructor for this type (if any). Updated by 'rawCtorRef'.
		UNKNOWN(PTR_GC) CopyCtorFn rawCtor;

		// Reference to the ctor to track changes. Possibly null.
		code::MemberRef *rawCtorRef;

		// Called whenever a new constructor is added.
		void updateCtor(Function *ctor);

		// Update the handle with the potentially relevant function 'fn'.
		void updateHandle(Function *fn);

		// Notify that the thread changed.
		void notifyThread(NamedThread *t);

		// The member variable layout for this type. Not used for types declared in C++.
		Layout *layout;

		// Reference to us.
		code::RefSource *selfRef;

		// Function used to read instances of this type. Created on demand.
		Function *readRef;

		/**
		 * Helpers for deciding which functions shall be virtual.
		 *
		 * This logic decides which functions shall be presented to the vtable but we let the vtable
		 * decide which functions shall use lookup.
		 */

		// The VTable for this class. Value types do not have a vtable, so 'vtable' is null for a
		// value type.
		MAYBE(VTable *) myVTable;

		// Called when a function has been added here. Decides if 'f' should be virtual or not and
		// acts accordingly.
		void vtableFnAdded(Function *added);

		// Search towards the super class for a function overridden by 'fn'. Returns 'true' if an
		// overridden function was found.
		Bool vtableInsertSuper(OverridePart *added, Function *original);

		// Search towards the subclasses for an overriding function and make sure to insert it into
		// the vtable in 'slot'. Assuming 'parentFn' is the corresponding function in the closest
		// parent. Returns true if any function was inserted.
		Bool vtableInsertSubclasses(OverridePart *added, Function *original);

		// Called when we have been attached to a new parent.
		void vtableNewSuper();

		// Called when we lost our previous parent.
		void vtableDetachedSuper(Type *oldSuper);

		// Invalidate 'isAbstract' for all child classes.
		void invalidateAbstract();

	};

	// Allocate an instance of 'type' (slow).
	RootObject *alloc(Type *type);

	// Get the raw VTable (should only be used inside VTable).
	inline MAYBE(VTable *) rawVTable(Type *t) { return t->myVTable; }

	// Check type relations without switching to the compiler thread.
	inline Bool STORM_FN isA(const Type *derived, MAYBE(const Type *) base) { return derived->chain->isA(base); }

	// Check how many levels of indirection separates the two types.
	inline Int STORM_FN distanceFrom(const Type *derived, const Type *base) { return derived->chain->distance(base); }

}
