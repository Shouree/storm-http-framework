#pragma once
#include "Handle.h"
#include "Utils/Templates.h"
#include "Core/Object.h"
#include "Core/TObject.h"
#include "Core/Exception.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * A class that represents an instance of some type in the system. Similar to "void *" in C.
	 *
	 * The interface to this class in Storm is not yet complete; this class was created as a useful
	 * tool for implementing serialization and other generic facilities in C++.
	 */
	class Variant {
		STORM_VALUE;
	public:
		// Create an empty variant.
		STORM_CTOR Variant();

		// Copy the variant.
		Variant(const Variant &o);
		Variant &operator =(const Variant &o);

		// Create a variant referring to an object.
		explicit Variant(RootObject *t);
		STORM_CAST_CTOR Variant(Object *o);
		STORM_CAST_CTOR Variant(TObject *o);

		// Create a variant referring to some known type known to be a value.
		Variant(const void *value, Type *type);

		// Generic creation. The overloads make sure to act correctly regardless if the contained
		// type is a value or an object. The second parameter is Engine &. Sorry for the template mess...
		template <class T>
		Variant(const T &v, typename EnableIf<!IsPointer<T>::value, Engine &>::t e) {
			init(&v, StormInfo<T>::type(e));
		}

		template <class T>
		Variant(T v, typename EnableIf<IsPointer<T>::value, Engine &>::t) : data(v) {}

		// Destroy. Call the destructor of the contained element, if any.
		~Variant();

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *env);

		// Empty/any value?
		Bool STORM_FN empty() const;
		Bool STORM_FN any() const { return !empty(); }

		// Does this variant contain the specified type?
		Bool STORM_FN has(Type *type) const;

		// Get contained type.
		MAYBE(Type *) STORM_FN type() const;

	public:

		/**
		 * Low-level API for C++.
		 *
		 * Make sure to differentiate usage depending on wether the contained type is a value or a
		 * class! This API does no checking of the contents of the variant, and may crash horribly
		 * if care is not taken externally.
		 */

		// Create a uninitialized variant referring to a type. Call 'getValue' and 'valueInitialized'
		// to initialize the value.
		static Variant uninitializedValue(Type *type);

		// Get a pointer to the value stored in here, assuming it is a value.
		void *getValue() const;

		// Note that we have initialized the value.
		void valueInitialized();

		// Note that we have moved the value somewhere else, clearing the variant without calling
		// the destructor. It is possible to call 'valueInitialized' again.
		void valueRemoved();

		// Move the value somewhere else.
		void moveValue(void *to);

		// Get the object stored in here.
		RootObject *getObject() const;

		// Get a pointer to the contained data. If it is an object, get a pointer to the object
		// pointer. If it is a value, get a pointer to the value. This is useful when calling
		// functions through the FnCall interface.
		void *getPointer();


		/**
		 * High-level API for C++.
		 */

		// Templated function for extracting the contained type. Works regardless if the contained
		// type is a value or an object. Assumes that the variant actually contains a value unless T
		// is a pointer, in which case null is returned if the variant is empty. Returns T, sorry
		// for the template mess...
		template <class T>
		typename EnableIf<!IsPointer<T>::value, T>::t get() const {
			if (!data)
				throw new (runtime::someEngine()) InternalError(S("Attempting to get a value from an empty variant."));
			if (!has(StormInfo<T>::type(engine())))
				throw new (engine()) InternalError(S("Attempting to get an incorrect type from a variant."));

			assert(runtime::gcTypeOf(data)->kind == GcType::tArray, L"Should specify a pointer with this type to 'get'.");

			return *(T *)getValue();
		}

		template <class T>
		typename EnableIf<IsPointer<T>::value, T>::t get() const {
			if (data == null)
				return null;

			Type *t = StormInfo<typename BaseType<T>::Type>::type(engine());
			if (!has(t))
				return null;

			const GcType *g = runtime::gcTypeOf(data);
			if (g->kind == GcType::tArray) {
				return (T)getValue();
			} else {
				return (T)getObject();
			}
		}

	private:
		// The stored data. Either an array with one element, or a pointer to an object.
		UNKNOWN(PTR_GC) void *data;

		// Initialize for a value.
		void init(const void *value, Type *type);

		// Find an Engine instance. Assumes we're not empty.
		Engine &engine() const;

		friend StrBuf &operator <<(StrBuf &to, const Variant &v);
		friend wostream &operator <<(wostream &to, const Variant &v);
	};

	// Output.
	StrBuf &STORM_FN operator <<(StrBuf &to, const Variant &v);
	wostream &operator <<(wostream &to, const Variant &v);
}
