#include "stdafx.h"

// Make all exposed classes non-abstract.
#undef ABSTRACT
#define ABSTRACT

#include "Utils/Memory.h"
#include "Utils/TypeInfo.h"
#include "Core/Gen/CppTypes.h"
#include "Core/Runtime.h"
#include "Core/Exception.h"

/**
 * Template for the generated cpp file.
 */

// INCLUDES

// PRIMITIVE_GLOBALS

// TYPE_GLOBALS

// TEMPLATE_GLOBALS

// THREAD_GLOBALS

// VTABLE_DECLS

// ABSTRACT_IMPLS

// Turn off optimizations in this file. It takes quite a long time, and since it is only executed
// once during compiler startup, it is not very useful to optimize these functions. Especially not
// during testing!
#pragma optimize("", off)
#pragma GCC optimize ("O0")

// Turn of some warnings from GCC in this file...
#pragma GCC diagnostic ignored "-Wnarrowing"
#pragma GCC diagnostic ignored "-Wunused-variable"

// Someone defines VOID...
#ifdef VOID
#undef VOID
#endif

namespace storm {

	static const nat NONE = nat(-1);
	static const nat VOID = nat(-2);

	/**
	 * Add type flags depending on characteristics of T.
	 */
	template <class T>
	struct ValFlags {
	private:
		static const nat pod;
		static const nat simple;

	public:
		static const TypeFlags v;
	};

	template <class T>
	const nat ValFlags<T>::pod = std::is_pod<T>::value ? typeCppPOD : typeNone;

	template <class T>
	const nat ValFlags<T>::simple =
		(std::is_trivially_copy_constructible<T>::value & std::is_trivially_destructible<T>::value)
		? typeCppSimple : typeNone;

	template <class T>
	const TypeFlags ValFlags<T>::v = TypeFlags(typeValue | pod | simple);


	/**
	 * Struct for all metadata. Declared as a friend to all Storm classes through the STORM_CLASS and STORM_VALUE macro.
	 */
	struct CppMeta {
		static const CppType *cppTypes();
		static const CppFunction *cppFunctions();
		static const CppVariable *cppVariables();
		static const CppEnumValue *cppEnumValues();
		static const CppTemplate *cppTemplates();
		static const CppThread *cppThreads();
		static const CppLicense *cppLicenses();
		static const CppVersion *cppVersions();
		static const wchar *const *cppSources();
		static const wchar *cppLibName();
		static const wchar *cppDocName();
		static const CppRefType *cppRefTypes();

		/**
		 * Constructor for built-in classes.
		 */
		template <class T>
		static void CODECALL create1(void *mem) {
			new (Place(mem))T();
		}

		template <class T, class P>
		static void CODECALL create2(void *mem, P p) {
			new (Place(mem))T(p);
		}

		template <class T, class P, class Q>
		static void CODECALL create3(void *mem, P p, Q q) {
			new (Place(mem))T(p, q);
		}

		template <class T, class P, class Q, class R>
		static void CODECALL create4(void *mem, P p, Q q, R r) {
			new (Place(mem))T(p, q, r);
		}

		template <class T, class P, class Q, class R, class S>
		static void CODECALL create5(void *mem, P p, Q q, R r, S s) {
			new (Place(mem))T(p, q, r, s);
		}

		template <class T, class P, class Q, class R, class S, class U>
		static void CODECALL create6(void *mem, P p, Q q, R r, S s, U u) {
			new (Place(mem))T(p, q, r, s, u);
		}

		template <class T, class P, class Q, class R, class S, class U, class V>
		static void CODECALL create7(void *mem, P p, Q q, R r, S s, U u, V v) {
			new (Place(mem))T(p, q, r, s, u, v);
		}

		template <class T, class P, class Q, class R, class S, class U, class V, class W>
		static void CODECALL create8(void *mem, P p, Q q, R r, S s, U u, V v, W w) {
			new (Place(mem))T(p, q, r, s, u, v, w);
		}

		// Wrap an assignment operator call.
		template <class T>
		static T &CODECALL assign(T &to, const T &from) {
			return to = from;
		}

		// Wrap a destructor call.
		template <class T>
		static void CODECALL destroy(T *obj) {
			obj->~T();
		}

	};

	// Invalid size.
	const CppSize CppSize::invalid = { nat(-1), nat(-1), nat(-1), nat(-1) };

	// Invalid offset.
	const CppOffset CppOffset::invalid = { nat(-1), nat(-1) };

	const CppType *CppMeta::cppTypes() {
		// PTR_OFFSETS

		static const CppType types[] = {
			// CPP_TYPES
			{ null, null, CppType::tNone, 0, 0, CppSize::invalid, null, typeNone, null, { -1, 0 } },
		};
		return types;
	}

	// TEMPLATE_ARRAYS

	const CppFunction *CppMeta::cppFunctions() {
		// FN_PARAMETERS

		static const CppFunction functions[] = {
			// CPP_FUNCTIONS
			{ null, null, CppFunction::fnFree, cppPublic, 0, 0, null, null, { CppTypeRef::tVoid, null, false, false }, { -1, 0 } },
		};

		return functions;
	}

	const CppVariable *CppMeta::cppVariables() {
		static const CppVariable variables[] = {
			// CPP_VARIABLES
			{ null, 0, 0, cppPublic, { CppTypeRef::invalid }, CppOffset::invalid, { -1, 0 } },
		};

		return variables;
	}

	const CppEnumValue *CppMeta::cppEnumValues() {
		static const CppEnumValue values[] = {
			// CPP_ENUM_VALUES
			{ null, 0, 0, 0 },
		};

		return values;
	}

	const CppTemplate *CppMeta::cppTemplates() {
		static const CppTemplate templates[] = {
			// CPP_TEMPLATES
			{ null, null, null, 0 },
		};
		return templates;
	}

	const CppThread *CppMeta::cppThreads() {
		static const CppThread threads[] = {
			// CPP_THREADS
			{ null, null, null, 0, { -1, 0 }, false },
		};
		return threads;
	}

	const CppLicense *CppMeta::cppLicenses() {
		static const CppLicense licenses[] = {
			// LICENSES
			{ null, null, null, null, null },
		};
		return licenses;
	}

	const CppVersion *CppMeta::cppVersions() {
		static const CppVersion versions[] = {
			// VERSIONS
			{ null, null, null },
		};
		return versions;
	}

	const wchar *const *CppMeta::cppSources() {
		static const wchar *sources[] = {
			// SOURCES
			null,
		};
		return sources;
	}

	const wchar *CppMeta::cppLibName() {
		static const wchar *name[] = {
			// LIB_NAME
			null,
		};
		return name[0];
	}

	const wchar *CppMeta::cppDocName() {
		static const wchar *name[] = {
			// DOC_NAME
		};
		return name[0];
	}

	const CppRefType *CppMeta::cppRefTypes() {
#ifdef DEBUG
		// REF_PTR_OFFSETS

		static const CppRefType types[] = {
			// REF_TYPES
			{ NONE, null },
		};
		return types;
#else
		return null;
#endif
	}

	const CppWorld *cppWorld() {
		static CppWorld w = {
			CppMeta::cppTypes(),
			CppMeta::cppFunctions(),
			CppMeta::cppVariables(),
			CppMeta::cppEnumValues(),
			CppMeta::cppTemplates(),
			CppMeta::cppThreads(),
			CppMeta::cppLicenses(),
			CppMeta::cppVersions(),
			CppMeta::cppSources(),
			CppMeta::cppLibName(),
			CppMeta::cppDocName(),
			CppMeta::cppRefTypes(),
		};
		return &w;
	}

}

#pragma optimize ("", off)
