#pragma once
#include "Core/TypeFlags.h"

namespace storm {

	/**
	 * Describes the types declared in C++ which are garbage collected and exposed to Storm.
	 */

	// Classes which are not always present.
	class ValueArray;
	class Type;

	/**
	 * Access mode from C++.
	 */
	enum STORM_HIDDEN(CppAccess) {
		cppPublic,
		cppProtected,
		cppPrivate,
	};

	/**
	 * A size. Describes both x86 and x64 sizes.
	 */
	struct CppSize {
		nat size32;
		nat size64;
		nat align32;
		nat align64;

#ifdef STORM_COMPILER
		// We do not know about code::Size if we're not the compiler.
		inline operator Size() const {
			return Size(size32, align32, size64, align64);
		}
#endif

		static const CppSize invalid;
	};

	/**
	 * An offset. Describes both x86 and x64 sizes.
	 */
	struct CppOffset {
		int s32;
		int s64;

		inline bool operator ==(const CppOffset &o) const {
			return s32 == o.s32 && s64 == o.s64;
		}

		inline bool operator !=(const CppOffset &o) const {
			return !(*this == o);
		}

#ifdef STORM_COMPILER
		// We do not know about code::Offset if we're not the compiler.
		inline operator Offset() const {
			return Offset(s32, s64);
		}
#endif

		// Invalid mask of (-1, -1). Usable only whenever we expect positive offsets.
		static const CppOffset invalid;
	};

	/**
	 * A source position.
	 */
	struct CppSrcPos {
		// File id. If negative, then no position is available.
		int id;

		// File position (only start, no end).
		nat pos;
	};

	/**
	 * List of C++ types.
	 */
	struct CppType {
		// Name of the type (null if last element).
		const wchar *name;

		// Package the type is located inside (eg. a.b.c).
		const wchar *pkg;

		// What kind of parent do we have?
		enum Kind {
			// No parent.
			tNone,

			// 'super' is a class in this table.
			tSuperClass,

			// 'super' is a class in this table, direct or indirect descendant of Type (id 0).
			tSuperClassType,

			// 'super' is a thread id (we inherit from TObject).
			tSuperThread,

			// 'super' is a pointer to a function CreateFn that generates the type to use.
			tCustom,

			// This is an enum.
			tEnum,

			// This is a bitmask enum.
			tBitmaskEnum,

			// This type is external to this library. Storm should try to find it using regular type
			// lookup with 'name' as an absolute package name.
			tExternal,

			// Mask for additional flags.
			tMask = 0xFF,

			// Is this type private to the enclosing type?
			tPrivate = 0x100
		};

		// Kind of type, super type
		Kind kind;

#ifdef STORM_COMPILER
		// The create function that 'super' is if 'kind == superCustom'.
		typedef Type *(*CreateFn)(Str *name, Size size, GcType *type);
#endif

		// Super class' type id, see 'kind' for the exact meaning.
		size_t super;

		// Documentation id.
		nat doc;

		// Total size of the type.
		CppSize size;

		// Pointer offsets. Ends with 'CppSize::invalid'.
		CppOffset *ptrOffsets;

		// Type flags (final? value type? etc.)
		TypeFlags flags;

		// C++ generated VTable for this type (if any).
		typedef const void *(*VTableFn)();
		VTableFn vtable;

		// Source location.
		CppSrcPos pos;
	};

	// Extract the regular part of the 'kind' enum.
	inline CppType::Kind typeKind(const CppType &t) {
		return CppType::Kind(nat(t.kind) & nat(CppType::tMask));
	}

	// See if a function has one or more specific flags.
	inline bool typeHasFlag(const CppType &t, CppType::Kind flag) {
		return (nat(t.kind) & nat(flag)) == nat(flag);
	}

	// Combine flags properly. Required by the generated code.
	inline CppType::Kind operator |(CppType::Kind a, CppType::Kind b) {
		return CppType::Kind(nat(a) | nat(b));
	}

	/**
	 * Reference to a type from C++.
	 */
	struct CppTypeRef {
		// Invalid index.
		static const nat invalid = -1;

		// Index used for 'void'.
		static const nat tVoid = -2;

		// Type or template index. If 'params' is null, this is a type index, otherwise it is a
		// template index and 'params' indicates the parameters to that template.
		nat id;

		// If we're a template, this array is populated. It ends with 'invalid', but may also contain 'tVoid'.
		const nat *params;

		// Is this a reference to the type?
		bool ref;

		// Is this a maybe-type?
		bool maybe;
	};

	/**
	 * Parameter to a function in C++.
	 */
	struct CppParam {
		// The parameter name (null if last parameter).
		const wchar *name;

		// The type of the parameter.
		CppTypeRef type;
	};

	/**
	 * List of C++ functions.
	 */
	struct CppFunction {
		// Name of the function (null if last element).
		const wchar *name;

		// Package. This is 'null' if this is a member function.
		const wchar *pkg;

		// Kind of function and misc. flags.
		enum FnKind {
			// Free function. Nothing strange.
			fnFree,

			// Free function, but a EnginePtr is to be inserted as the first parameter.
			fnFreeEngine,

			// Member function. The first parameter indicates the type we're a member of. 'pkg' is null.
			fnMember,

			// Constructor usable for casting.
			fnCastMember,

			// This is a static member, it is stored as a free function.
			fnStatic,

			// This is a static member, taking an EnginePtr as its first parameter.
			fnStaticEngine,

			/**
			 * Flags part of the enum. Use 'fnMask' to mask out the regular members below.
			 */
			fnMask = 0xFF,

			// This function is usable as an assignment function.
			fnAssign = 0x100,

			// This function is final (ie. non-virtual in C++).
			fnFinal = 0x200,

			// This function is abstract.
			fnAbstract = 0x400,
		};

		// Kind.
		FnKind kind;

		// Access modifier.
		CppAccess access;

		// Thread to run this function on.
		nat threadId;

		// Documentation id.
		nat doc;

		// Pointer to the function.
		const void *ptr;

		// Parameters.
		const CppParam *params;

		// Result.
		CppTypeRef result;

		// Source position.
		CppSrcPos pos;
	};

	// Extract the regular part of the 'kind' enum.
	inline CppFunction::FnKind fnKind(const CppFunction &f) {
		return CppFunction::FnKind(nat(f.kind) & nat(CppFunction::fnMask));
	}

	// See if a function has one or more specific flags.
	inline bool fnHasFlag(const CppFunction &f, CppFunction::FnKind flag) {
		return (nat(f.kind) & nat(flag)) == nat(flag);
	}

	// Combine flags properly. Required by the generated code.
	inline CppFunction::FnKind operator |(CppFunction::FnKind a, CppFunction::FnKind b) {
		return CppFunction::FnKind(nat(a) | nat(b));
	}


	/**
	 * List of C++ member variables.
	 */
	struct CppVariable {
		// Name of the variable (null if last element).
		const wchar *name;

		// Type this variable is a member of.
		nat memberOf;

		// Documentation.
		nat doc;

		// Access mode.
		CppAccess access;

		// Type of this variable.
		CppTypeRef type;

		// Offset into the type.
		CppOffset offset;

		// Source position.
		CppSrcPos pos;
	};

	/**
	 * List of enum members and their values.
	 */
	struct CppEnumValue {
		// Name (null if last element).
		const wchar *name;

		// Member of.
		nat memberOf;

		// Value.
		nat value;

		// Documentation.
		nat doc;
	};

	/**
	 * List of C++ templates.
	 */
	struct CppTemplate {
		// Name of the template (null if last element).
		const wchar *name;

		// Package the template is located inside (eg. a.b.c).
		const wchar *pkg;

		// Generate templates using this function. If null, this template is located externally.
		typedef Type *(*GenerateFn)(Str *name, ValueArray *);
		GenerateFn generate;

		// Documentation id.
		nat doc;
	};

	/**
	 * List of C++ named threads.
	 */
	struct CppThread {
		// Name of the thread (null if last element).
		const wchar *name;

		// Package the thread is located inside.
		const wchar *pkg;

		// Thread declaration.
		DeclThread *decl;

		// Documentation id.
		nat doc;

		// Position.
		CppSrcPos pos;

		// Located externally?
		bool external;

		// ...
	};

	/**
	 * List of licenses from C++.
	 */
	struct CppLicense {
		// Name of the license object (null if last element).
		const wchar *name;

		// Package.
		const wchar *pkg;

		// Title.
		const wchar *title;

		// Author(s).
		const wchar *author;

		// Contents.
		const wchar *body;
	};

	/**
	 * List of versions from C++.
	 */
	struct CppVersion {
		// Name of the version object (null if last element).
		const wchar *name;

		// Packages.
		const wchar *pkg;

		// Version string. Parsed by 'parseVersion' on load.
		const wchar *version;
	};

	/**
	 * List of sizes and pointer offsets for all types generated by C++ (ie. they are the 'reference
	 * implementation'). These are generated in debug builds so that GC-related issues can be found
	 * more easily.
	 */
	struct CppRefType {
#ifdef DEBUG
		// The size of this type.
		size_t size;

		// The pointer offsets of this type. -1 marks a member we can not get an offset for from C++
		// (ie. references). -1 also marks the end of the array.
		const size_t *offsets;
#endif
	};

	/**
	 * Contains all information about C++ types and functions required by Storm.
	 */
	struct CppWorld {
		// List of types.
		const CppType *types;

		// List of functions.
		const CppFunction *functions;

		// List of member variables.
		const CppVariable *variables;

		// List of enum values.
		const CppEnumValue *enumValues;

		// List of templates.
		const CppTemplate *templates;

		// List of named threads.
		const CppThread *threads;

		// List of licenses.
		const CppLicense *licenses;

		// List of versions.
		const CppVersion *versions;

		// List of source files that are referred to by types, functions, etc.
		const wchar *const *sources;

		// Name of the current library (or NULL for the compiler). Used to locate source code
		// relative to the compiler root directory.
		const wchar *libName;

		// Name of the file containing documentation for this world.
		const wchar *docFile;

		// List of the actual offsets for all pointers inside all types. Useful to diagnose
		// GC-related issues. This is empty in release builds.
		const CppRefType *refTypes;
	};

	// Get the CppWorld for this module.
	const CppWorld *cppWorld();

}
