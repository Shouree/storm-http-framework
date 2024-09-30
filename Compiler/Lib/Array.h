#pragma once
#include "ValueArray.h"
#include "Type.h"

namespace storm {
	STORM_PKG(core.lang);

	class SerializeInfo;

	/**
	 * Implements the template interface for the Array<> class in Storm.
	 */

	// Create types for unknown implementations.
	Type *createArray(Str *name, ValueArray *params);

	/**
	 * Type for arrays.
	 */
	class ArrayType : public Type {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR ArrayType(Str *name, Type *contents);

		// Late init.
		virtual void lateInit();

		// Parameter.
		Value STORM_FN param() const;

		// Notifications.
		virtual void STORM_FN notifyAdded(NameSet *to, Named *added);

	protected:
		// Lazy loading.
		virtual Bool STORM_FN loadAll();

	private:
		// Content type.
		Type *contents;

		// Added functions, to know when we can stop watching.
		enum {
			watchNone = 0x00,
			watchLess = 0x01,
			watchSerialization = 0x02,
			watchEquality = 0x04,
		};

		Nat watchFor;

		// Helpers.
		void loadClassFns();
		void loadValueFns();

		// Add 'sort' without parameters.
		void addSort();

		// Add serialization functions.
		void addSerialization(SerializeInfo *info);

		// Add 'removeDuplicates'
		void addRemoveDuplicates();

		// Generate the 'write' function.
		Function *writeFn(SerializedType *type, SerializeInfo *info);

		// Generate the 'read' ctor.
		Function *readCtor(SerializeInfo *info);
	};

	/**
	 * The array iterator type.
	 */
	class ArrayIterType : public Type {
		STORM_CLASS;
	public:
		// Ctor.
		ArrayIterType(Type *param);

	protected:
		// Lazy loading.
		virtual Bool STORM_FN loadAll();

	private:
		// Content type.
		Type *contents;
	};

	Bool STORM_FN isArray(Value v);
	Value STORM_FN unwrapArray(Value v);
	Value STORM_FN wrapArray(Value v);

}
