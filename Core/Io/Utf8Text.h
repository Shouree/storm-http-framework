#pragma once
#include "Text.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * Reading and decoding UTF-8 text.
	 */
	class Utf8Input : public TextInput {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR Utf8Input(IStream *src);

		// Create with initial buffer contents.
		STORM_CTOR Utf8Input(IStream *src, Buffer start);

		// Close the stream.
		virtual void STORM_FN close();

	protected:
		virtual Char STORM_FN readChar();

	private:
		// Source stream.
		IStream *src;

		// Input buffer.
		Buffer buf;

		// Position in the buffer.
		Nat pos;

		// Read a character.
		Byte readByte();

		// Reset one byte.
		void ungetByte();

		// Buffer size.
		enum {
			bufSize = 1024
		};
	};


	/**
	 * Encoding and writing UTF-8 text.
	 */
	class Utf8Output : public TextOutput {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR Utf8Output(OStream *to);
		STORM_CTOR Utf8Output(OStream *to, TextInfo info);

		// Flush output.
		virtual void STORM_FN flush();

		// Close.
		virtual void STORM_FN close();

	protected:
		// Write character.
		virtual void STORM_FN writeChar(Char ch);

	private:
		// Target stream.
		OStream *dest;

		// Output buffer.
		Buffer buf;

		// Position in the buffer.
		Nat pos;

		// Write bytes to the buffer (automatically flushes it if needed).
		void writeBytes(const byte *data, nat count);

		// Initialize.
		void init();

		// Buffer size.
		enum {
			bufSize = 1024
		};
	};

}
