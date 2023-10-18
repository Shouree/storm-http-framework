#pragma once
#include "ValueArray.h"
#include "Type.h"

namespace storm {
	STORM_PKG(core.lang);

	class MapBase;
	class SerializeInfo;

	/**
	 * Implements the template interface for the Map<> class in Storm.
	 */

	// Create types for unknown implementations.
	Type *createMap(Str *name, ValueArray *params);

	// Create types of RefMap.
	Type *createRefMap(Str *name, ValueArray *params);

	/**
	 * Type for maps.
	 */
	class MapType : public Type {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR MapType(Str *name, Type *k, Type *v, Bool refKeys);

		// Late initialization.
		virtual void lateInit();

		// Notifications.
		virtual void STORM_FN notifyAdded(NameSet *to, Named *added);

	protected:
		// Load members.
		virtual Bool STORM_FN loadAll();

	private:
		// Content types.
		Type *k;
		Type *v;

		// Treat keys as references always.
		Bool refKeys;

		// Has the 'at' operator been added?
		Bool atAdded;

		// What we're watching for, to know when we can stop watching.
		enum {
			watchNone = 0x00,

			watchKeySerialization = 0x01,
			watchKeyDefaultCtor = 0x02,
			watchKeyMask = 0x0F,

			watchValueSerialization = 0x10,
			watchValueMask = 0xF0,
		};
		Nat watchFor;

		// Add the '[]' member if applicable.
		void addAccess();

		// Add the 'at' member.
		void addMaybeAccess();

		// Add serialization.
		void addSerialization(SerializeInfo *kInfo, SerializeInfo *vInfo);

		// Create a 'write' function.
		Function *writeFn(SerializedType *type, SerializeInfo *kInfo, SerializeInfo *vInfo);

		// Create a 'read' constructor.
		Function *readCtor(SerializeInfo *kInfo, SerializeInfo *vInfo);

		// Helpers for creating instances.
		static void createClass(void *mem);
		static void createRefClass(void *mem);
		static void copyClass(void *mem, MapBase *copy);
	};

	/**
	 * The map iterator type.
	 */
	class MapIterType : public Type {
		STORM_CLASS;
	public:
		// Ctor.
		MapIterType(Type *k, Type *v);

	protected:
		// Lazy loading.
		virtual Bool STORM_FN loadAll();

	private:
		// Content type.
		Type *k;
		Type *v;
	};

}
