#pragma once
#include "Core/EnginePtr.h"
#include "Code/Reference.h"

namespace storm {
	STORM_PKG(core.lang);

	namespace builtin {
		/**
		 * Global references to built-in functions in the compiler. These are needed from generated code,
		 * but are not intended to be visible as functions in the name tree. Either as they have semantics
		 * that do not match the expected semantics in the rest of the system, or because their signatures
		 * are not easily expressible in the type system, or for other reasons.
		 *
		 * Use ref() to get a reference to these things.
		 */
		enum BuiltIn {
			// Reference to the engine itself.
			engine,
			// Reference to the entry-point for lazy code updates.
			lazyCodeUpdate,
			// Reference to the throw function for a rule.
			ruleThrow,
			// Allocate an object of the type given.
			alloc,
			// Allocate an array of the given type and size.
			allocArray,
			// Execute as<T>.
			as,
			// # of bytes inside a vtable the object's vtable ptr is pointing.
			VTableAllocOffset,
			// # of bytes inside TObject the thread is stored
			TObjectOffset,
			// Access the 'atRaw' member of Map.
			mapAtValue,
			mapAtClass,
			// Acces to the 'getUnsafeRaw' member of Map.
			mapGetUnsafe,
			// Acces to the 'getUnsafeRaw' member of Set.
			setGetUnsafe,
			// Acces the generic 'EnumType::toString'.
			enumToS,
			// Access to 'postRaw' and 'resultRaw' in Future.
			futurePost,
			futureResult,
			// Set a future into "no-clone"-mode.
			futureNoClone,
			// Low-level helpers for spawning threads.
			spawnResult,
			spawnFuture,
			spawnFutureId,
			spawnVoid,
			spawnVoidId,
			// Access to things inside FnBase.
			fnNeedsCopy,
			fnCall,
			fnCreate,
			// A null function (does nothing).
			fnNull,
			// ToS helper for Maybe<T>.
			maybeToS,
			// Get the address of a global variable.
			globalAddr,
			// Throw an "Abstract function called"-exception.
			throwAbstractError,
			// Throw a generic exception (an instance of storm::Exception).
			throwException,
			// Create a Variant instance from a value (a constructor).
			createValVariant,
			// Create a Variant instance from a class (a constructor).
			createClassVariant,
			// Create a Join instance.
			createJoin,

			// Should be the last one.
			count,
		};

		// Get a reference to a built-in thing.
		code::Ref STORM_FN ref(EnginePtr e, BuiltIn which);
	}
}
