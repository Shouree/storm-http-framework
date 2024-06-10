#include "stdafx.h"
#include "Init.h"
#include "CppLoader.h"
#include "Type.h"
#include "Engine.h"
#include "Core/Map.h"
#include "Core/Array.h"
#include "Core/Str.h"
#include "Code/Reference.h"
#include "Lib/Clone.h"
#include "Lib/Enum.h"
#include "Lib/ToS.h"
#include "Lib/RawPtr.h"
#include "Lib/PinnedSet.h"
#include "Lib/Variant.h"
#include "Lib/Join.h"
#include "NameSet.h"
#include "Package.h"
#include "License.h"

namespace storm {

	static void lateInit(Named *n) {
		n->lateInit();
	}

	void initTypes(Engine &e, World &into) {
		const CppWorld *world = cppWorld();
		CppLoader loader(e, world, into, null, null);

		into.types.resize(1);

		// We need to start by creating the Type-type.
		into.types[0] = Type::createType(e, &world->types[0]);

		// Then we can go on loading the rest of the types.
		loader.loadTypes();
		loader.loadThreads();
		loader.loadSuper();

		e.advance(bootTypes);

		// Load templates.
		loader.loadTemplates();

		// Poke at some template types needed to instantiate a Type properly. This causes them to be
		// properly instantiated before we report that templates are fully functional.
		Array<Value>::stormType(e);
		Map<Str *, NameOverloads *>::stormType(e);
		Map<Function *, VTableUpdater *>::stormType(e);
		WeakSet<TypeChain>::stormType(e);
		WeakSet<code::Reference>::stormType(e);

		// Now we can declare templates fully functional.
		e.advance(bootTemplates);

		// Do the late initialization on all previously created types.
		e.forNamed(&lateInit);

		// Insert everything into their packages.
		loader.loadPackages();

		// Do late initialization again for types that wish to utilize packages.
		e.forNamed(&lateInit);

		// Load functions and variables.
		loader.loadVariables();
		loader.loadFunctions();
		loader.loadMeta();

		// Load the system-wide toS implementation and other global templates.
		Package *core = e.package(S("core"));
		core->add(new (e) CloneTemplate());
		core->add(new (e) ToSTemplate());
		core->add(new (e) OutputOperatorTemplate());
		core->add(new (e) JoinTemplate());

		// Add the GC license, if any.
		if (const GcLicense *l = e.gc.license()) {
			Str *id = new (e) Str(l->id);
			Str *title = new (e) Str(l->title);
			Str *author = new (e) Str(l->author);
			Str *body = new (e) Str(l->body);
			core->add(new (e) License(id, title, author, body));
		}

		// Load other types.
		Package *unsafe = e.package(S("core.unsafe"));
		RawPtrType *rawPtr = new (e) RawPtrType(e);
		unsafe->add(rawPtr);
		addPinned(rawPtr);

		// Add dynamic parts of the Variant interface.
		addVariant(e.nameSet(parseSimpleName(e, S("core.Variant"))));
	}

}
