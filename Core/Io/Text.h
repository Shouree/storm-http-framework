#pragma once
#include "Core/Object.h"
#include "Core/Char.h"
#include "Url.h"
#include "Stream.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * Text IO. Supports UTF8 and UTF16, little and big endian. Use 'readText' to detect which
	 * encoding is used and create the correct reader for that encoding.
	 *
	 * Line endings are always converted into a single \n (ie. Unix line endings). If the original
	 * line endings are desired, use 'readAllRaw' on a TextReader. Supports \r\n (Windows), \n
	 * (Unix) and \r (Macintosh).
	 *
	 * TODO: This should also be extensible.
	 * TODO: Better handling of line endings.
	 * TODO: Find a good way of duplicating an input format to an output format.
	 * TODO: Add TextInfo to a TextReader as well!
	 * TODO: It is quite natural to have 'readLine' return MAYBE(Str *), then we can do:
	 *   while (line = x.readLine()) {}
	 */

	/**
	 * Information about encoded text.
	 */
	class TextInfo {
		STORM_VALUE;
	public:
		// Create the default configuration (all members set to false).
		STORM_CTOR TextInfo();

		// Use windows-style line endings.
		Bool useCrLf;

		// Output a byte order mark first.
		Bool useBom;
	};

	// Create the default text information for the current system.
	TextInfo STORM_FN sysTextInfo();

	// Create a text information that produces Unix-style line endings.
	TextInfo STORM_FN unixTextInfo();

	// Create a text information that procuces Windows-style line endings.
	TextInfo STORM_FN windowsTextInfo();

	// Create a text information that produces Windows-style line endings, and specify whether a BOM
	// should be outputted.
	TextInfo STORM_FN windowsTextInfo(Bool bom);


	/**
	 * Base interface for reading text. Caches one character. When implementing your own version,
	 * override 'readChar' to read a code point in UTF32. Manages line endings so that the 'read'
	 * functions will always only see '\n' as line endings. 'readRaw' and 'peek' may observe a '\r'
	 * sometimes.
	 */
	class TextInput : public Object {
		STORM_ABSTRACT_CLASS;
	public:
		// Create.
		STORM_CTOR TextInput();

		// Read a single character from the stream. Returns `Char(0)` on failure.
		Char STORM_FN read();

		// Read a single character from the stream without line-ending conversion. Returns `Char(0)` on failure.
		Char STORM_FN readRaw();

		// Peek a single character. Returns `Char(0)` on failure.
		Char STORM_FN peek();

		// Read an entire line from the file. Removes any line endings.
		Str *STORM_FN readLine();

		// Read the entire file into a string.
		Str *STORM_FN readAll();

		// Read the entire file without any conversions of line endings (still ignores any BOM).
		Str *STORM_FN readAllRaw();

		// Does the file contain any more data?
		Bool STORM_FN more();

		// Close the underlying stream.
		virtual void STORM_FN close();

	protected:
		// Override in derived readers, read one character.
		virtual Char STORM_FN readChar() ABSTRACT;

	private:
		// Cached code point. 0 if at end of stream.
		Char next;

		// Is 'next' valid?
		Bool hasNext;

		// First character?
		Bool first;

		// At end of file.
		Bool eof;

		// Helper for reading which removes any BOM.
		Char doRead();
	};


	// Create a text reader. Identifies the encoding automatically and creates an appropriate reader.
	TextInput *STORM_FN readText(IStream *stream);

	// Create a text reader from an `Url`. Equivalent to calling `readText(file.read())`.
	TextInput *STORM_FN readText(Url *file);

	// Create a text reader that reads data from a string. Utilizes `StrInput`.
	TextInput *STORM_FN readStr(Str *from);

	// Read the text from a file into a string. Equivalent to calling `readText(file).readAll()`.
	Str *STORM_FN readAllText(Url *file);


	/**
	 * Base interface for writing text. Buffers entire lines. When implementing your own version,
	 * override 'writeChar' and 'flush' to write a code point in UTF32.
	 */
	class TextOutput : public Object {
		STORM_ABSTRACT_CLASS;
	public:
		// Create. Outputs plain Unix line endings.
		STORM_CTOR TextOutput();

		// Create. Specify line endings.
		STORM_CTOR TextOutput(TextInfo info);

		// Automatic flush on newline? (on by default)
		Bool autoFlush;

		// Write a character.
		void STORM_FN write(Char c);

		// Write a string.
		void STORM_FN write(Str *s);

		// Write a string, add any line endings.
		void STORM_FN writeLine(Str *s);

		// Write a new-line character.
		void STORM_FN writeLine();

		// Flush all buffered output to the underlying stream.
		virtual void STORM_FN flush();

		// Close the underlying stream.
		virtual void STORM_FN close();

	protected:
		// Override in derived writers. Write one character.
		virtual void STORM_FN writeChar(Char ch) ABSTRACT;

	private:
		// Text config.
		TextInfo config;

		// Write a bom if needed.
		void writeBom();
	};

}
