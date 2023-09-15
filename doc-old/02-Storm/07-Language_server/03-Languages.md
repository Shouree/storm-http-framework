Using the language server for a language
==========================================

In order to highlight a language, the language server expects that the reader for the language
returns a `FileReader` for the given file when the `readFile` function is called. This happens
automatically if the `FilePkgReader` class is used to read the language. The language server then
calls the `createParser` function of the `FileReader` to create a parser containing the syntax of
the language. If it is enough to call the constructor of the parser, it is possible to override the
`rootRule` function to only specify the root rule to use while parsing.

Reading a simple language can be implemented as follows:

```
PkgReader reader(Url[] files, Package pkg) on Compiler {
	FilePkgReader(files, pkg, &createFile(FileInfo));
}

FileReader createFile(FileInfo info) on Compiler {
	FooFile(info);
}

class FooFile extends FileReader {
	init(FileInfo info) {
		init(info) {}
	}

	void readSyntaxRules() {}
	void readSyntaxProductions() {}
	void readTypes() {}
	void resolveTypes() {}
	void readFunctions() {}
	void resolveFunctions() {}

	// Either override this function...
	Rule rootRule() {
		named{SFile};
	}

	// ...or this function.
	InfoParser createParser() {
		InfoParser parser(named{SFile});
		// Add any additional syntax here.
		parser;
	}
}
```