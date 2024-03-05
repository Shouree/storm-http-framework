#pragma once
#include "NameSet.h"
#include "Core/Io/Url.h"
#include "Core/PODArray.h"

namespace storm {
	STORM_PKG(core.lang);

	class PkgReader;
	class PkgFiles;

	/**
	 * Defines the contents of a package. A package is a NameSet associated with a path, which
	 * allows it to load things from there.
	 */
	class Package : public NameSet {
		STORM_CLASS;
	public:
		// Create a virtual package, ie. a package that is not present on disk. These packages must
		// be eagerly loaded by 'add'ing things to them.
		STORM_CTOR Package(Str *name);

		// Create a package located in 'path'.
		STORM_CTOR Package(Url *path);

		// Create a package located in 'path' with the given name.
		STORM_CTOR Package(Str *name, Url *path);

		// Get parent.
		virtual MAYBE(NameLookup *) STORM_FN parent() const;

		// Get an `Url` that represents the location where this package loads code from. If the
		// package is virtual, returns `null`.
		virtual MAYBE(Url *) STORM_FN url() const;

		// Set url. Only done by Engine during startup and is therefore not exposed to storm.
		void setUrl(Url *to);

		// Lazy-loading.
		virtual Bool STORM_FN loadName(SimplePart *part);
		virtual Bool STORM_FN loadAll();

		// Add an exported package.
		// This is most useful when creating virtual packages. Non-virtual packages read this
		// information automatically as necessary.
		void STORM_FN STORM_NAME(addExport, export)(Package *pkg);

		// Get all exports for this package.
		Array<Package *> *STORM_FN exports();

		// Get all exports from this package, taking recursive exports into account.
		Array<Package *> *STORM_FN recursiveExports();

		// Inhibit discard messages from propagating.
		void STORM_FN noDiscard();

		// We don't need to propagate the discard source message in general.
		virtual void STORM_FN discardSource();

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// Our path. Points to null if we're a virtual package.
		Url *pkgPath;

		// Exports for this package. Might be null.
		Array<Package *> *exported;

		// Shall we emit 'discardSource' messages on load?
		Bool discardOnLoad;

		// Are exported packages loaded?
		Bool exportedLoaded;

		/**
		 * Loading of sub-packages.
		 */

		// Load all files given.
		void loadFiles(Array<Url *> *files);

		// Create a reader for each element in 'readers'.
		Array<PkgReader *> *createReaders(Map<SimpleName *, PkgFiles *> *readers);

		// Try to load a sub-package. Returns null on failure.
		Package *loadPackage(Str *name);

		// Load exports if necessary.
		void loadExports();

		// Type for keeping track of recursive package lookups. Pre-allocated enough so that we
		// won't have to heap allocate too often. Typically, this should not be very large. If it
		// would be, then we would need a set instead for performance.
		typedef PODArray<Package *, 32> ExportSet;
	};


	/**
	 * Documentation for a package.
	 *
	 * Looks for a file named README and uses the text in that file as documentation.
	 */
	class PackageDoc : public NamedDoc {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR PackageDoc(Package *pkg);

		// Generate documentation.
		virtual Doc *STORM_FN get();

	private:
		// Owner.
		Package *pkg;
	};


	// Find a package from a path.
	MAYBE(Package *) STORM_FN package(Url *path);

	// Get the root package.
	Package *STORM_FN rootPkg(EnginePtr e);

}
