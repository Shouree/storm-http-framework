#pragma once
#include "Core/Array.h"
#include "Core/Fn.h"
#include "Core/Io/Url.h"
#include "NamedThread.h"
#include "Package.h"
#include "Syntax/Parser.h"

namespace storm {
	STORM_PKG(lang);

	class FileReader;

	/**
	 * Wrapper of an array of URL:s, since we need to be able to express a map of those. This is not
	 * allowed by the preprocessor unless we wrap it inside a class.
	 */
	class PkgFiles : public Object {
		STORM_CLASS;
	public:
		STORM_CTOR PkgFiles();

		virtual void STORM_FN deepCopy(CloneEnv *e);

		Array<Url *> *files;
	};

	// Get the file extension that determines which package to use when parsing/compiling a
	// file. Usually, the last file extension is returned, but if a file is named like foo.so.2,
	// 'so' is used instead, since the last part is a number and there is a previous file
	// extension. If no dots are found at all, 'null' is returned.
	MAYBE(Str *) STORM_FN codeFileType(Url *file);

	// Get the package containing syntax when parsing 'file'. This is the default package, and
	// languages may choose to ignore this.
	MAYBE(SimpleName *) STORM_FN syntaxPkgName(Url *file);

	// Get the name of the 'reader' function for use when reading 'file'. Returns 'null' for files without an extension.
	MAYBE(SimpleName *) STORM_FN readerName(Url *file);
	MAYBE(SimpleName *) STORM_FN readerName(Str *ext);

	// Group files together by which reader they should use. Storm will have a custom implementation
	// of this which does not use a 'PkgFiles' object.
	Map<SimpleName *, PkgFiles *> *readerName(Array<Url *> *files);

	// Create a reader for a given file type.
	MAYBE(PkgReader *) STORM_FN createReader(Array<Url *> *files, Package *pkg);
	MAYBE(PkgReader *) STORM_FN createReader(SimpleName *name, Array<Url *> *files, Package *pkg);

	/**
	 * Load a specific file type from a package. This is the abstract base class that does not do
	 * very much at all. Create a function 'lang.ext.reader(Array<Url>, Package)' which creates an
	 * instance of a Reader, and you are ready to parse a language!
	 *
	 * The process is designed to be implemented as follows:
	 * 1: get syntax rules
	 * 2: get syntax options
	 * 3: get types (just information, should be lazily-loaded!)
	 * 4: resolve types (ie. set up inheritance, figure out how structs look...)
	 * 5: get functions
	 * 6: resolve functions (any other work needed to finalize functions etc.)
	 *
	 * This is to ensure that inter-dependencies are resolved correctly. Within each language, these
	 * rules may be implemented differently.
	 */
	class PkgReader : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create a PkgReader.
		STORM_CTOR PkgReader(Array<Url *> *files, Package *pkg);

		// Owning package.
		Package *pkg;

		// Files to parse.
		Array<Url *> *files;

		// Get the syntax rules.
		virtual void STORM_FN readSyntaxRules();

		// Get the syntax options.
		virtual void STORM_FN readSyntaxProductions();

		// Get the types.
		virtual void STORM_FN readTypes();

		// Resolve types.
		virtual void STORM_FN resolveTypes();

		// Get all functions.
		virtual void STORM_FN readFunctions();

		// Resolve functions.
		virtual void STORM_FN resolveFunctions();

		/**
		 * For language server integration.
		 */

		// Get a file reader for the given Url and the contents of the file.
		virtual MAYBE(FileReader *) STORM_FN readFile(Url *url, Str *src);
	};


	class FileInfo;
	class FileReader;

	/**
	 * Specialization of PkgReader which uses a set of FileReaders. Provide a function which creates
	 * instances of a 'FileReader' to use.
	 */
	class FilePkgReader : public PkgReader {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR FilePkgReader(Array<Url *> *files, Package *pkg, Fn<FileReader *, FileInfo *> *create);

		// Get the syntax rules.
		virtual void STORM_FN readSyntaxRules();

		// Get the syntax productions.
		virtual void STORM_FN readSyntaxProductions();

		// Get the types.
		virtual void STORM_FN readTypes();

		// Resolve types.
		virtual void STORM_FN resolveTypes();

		// Get all functions.
		virtual void STORM_FN readFunctions();

		// Resolve functions.
		virtual void STORM_FN resolveFunctions();

		// Get a file reader for the given Url.
		virtual MAYBE(FileReader *) STORM_FN readFile(Url *url, Str *src);

	private:
		// Store all files in use.
		Array<FileReader *> *readers;

		// Create FileReaders.
		Fn<FileReader *, FileInfo *> *create;

		// Populate 'readers' if it is not already done.
		void loadReaders();
	};

	// Load everything from a number of readers in the proper order.
	// Calls 'readSyntaxRules', 'readSyntaxProductions', ...
	void STORM_FN read(Array<PkgReader *> *readers);

}
