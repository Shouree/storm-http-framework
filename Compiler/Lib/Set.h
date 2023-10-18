#pragma once
#include "ValueArray.h"
#include "Type.h"

namespace storm {
	STORM_PKG(core.lang);

	class SetBase;
	class SerializeInfo;

	/**
	 * Implements the template interface for the Set<> class in Storm.
	 */

	// Create types for unknown implementations.
	Type *createSet(Str *name, ValueArray *params);

	// Create the reference set type.
	Type *createRefSet(Str *name, ValueArray *params);

	/**
	 * Type for sets.
	 */
	class SetType : public Type {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR SetType(Str *name, Type *k, Bool ref);

		// Notifications.
		virtual void STORM_FN notifyAdded(NameSet *to, Named *added);

	protected:
		// Load members.
		virtual Bool STORM_FN loadAll();

	private:
		// Content type.
		Type *k;

		// Is the key hashed by reference?
		Bool ref;

		// Currently watching for, so that we know when to stop.
		enum {
			watchNone = 0x00,
			watchSerialization = 0x01,
		};
		Nat watchFor;

		// Helpers for creating instances.
		static void createClass(void *mem);
		static void createRefClass(void *mem);
		static void copyClass(void *mem, SetBase *copy);

		// Add the 'at' member.
		void addMaybeAccess();

		// Add serialization.
		void addSerialization(SerializeInfo *info);

		// Generate the 'write' function.
		Function *writeFn(SerializedType *type, SerializeInfo *info);

		// Generate the 'read' function.
		Function *readCtor(SerializeInfo *info);
	};

	/**
	 * The set iterator type.
	 */
	class SetIterType : public Type {
		STORM_CLASS;
	public:
		// Ctor.
		SetIterType(Type *k);

	protected:
		// Lazy loading.
		virtual Bool STORM_FN loadAll();

	private:
		// Content type.
		Type *k;
	};

}
