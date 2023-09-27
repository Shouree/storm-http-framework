#pragma once
#include "Core/Object.h"
#include "Core/Array.h"
#include "Core/Exception.h"
#include "Buffer.h"

namespace storm {
	STORM_PKG(core.io);

	class Url;
	class IStream;
	class OStream;

	/**
	 * Error thrown when an operation is not supported by a protocol.
	 */
	class EXCEPTION_EXPORT ProtocolNotSupported : public Exception {
		STORM_EXCEPTION;
	public:
		ProtocolNotSupported(const wchar *operation, const wchar *protocol);
		ProtocolNotSupported(const wchar *operation, Str *protocol);
		STORM_CTOR ProtocolNotSupported(Str *operation, Str *protocol);

		virtual void STORM_FN message(StrBuf *to) const;
	private:
		Str *operation;
		Str *protocol;
	};

	/**
	 * File flags.
	 */
	enum StatType {
		// File not found.
		STORM_NAME(sNotFound, notFound),

		// File found. It is a file.
		STORM_NAME(sFile, file),

		// Found. It is a directory.
		STORM_NAME(sDirectory, directory),
	};


	/**
	 * Defines a protocol to use with a Url. The protocol itself defines how
	 * to access the underlying data. For example, the 'file' protocol opens
	 * regular files.
	 *
	 * The protocol below is an interface. A concrete implementation is provided
	 * for the file system.
	 *
	 * TODO: How to extend?
	 */
	class Protocol : public Object {
		STORM_CLASS;
	public:
		// Ctor.
		STORM_CTOR Protocol();

		// Compare two parts of a filename for equality.
		// Implemented here as a simple bitwise comparision.
		virtual Bool STORM_FN partEq(Str *a, Str *b);

		// Hash a part of a filename.
		virtual Nat STORM_FN partHash(Str *a);

		// This is only overridden in "RelativeProtocol".
		virtual Bool absolute() { return true; }

		/**
		 * These should be implemented as far as possible.
		 */

		// Children of this directory.
		virtual Array<Url *> *STORM_FN children(Url *url);

		// Read a file.
		virtual IStream *STORM_FN read(Url *url);

		// Write a file.
		virtual OStream *STORM_FN write(Url *url);

		// Exists?
		virtual StatType STORM_FN stat(Url *url);

		// Create a directory.
		virtual Bool STORM_FN createDir(Url *url);

		// Delete a file or directory.
		virtual Bool STORM_FN STORM_NAME(unlink,delete)(Url *url);

		// Convert to a string suitable for other C-api:s
		virtual Str *STORM_FN format(Url *url);

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

		// The same protocol as some other?
		virtual Bool STORM_FN operator ==(const Protocol &o) const;

		// Serialization:
		static SerializedType *STORM_FN serializedType(EnginePtr e);
		static Protocol *STORM_FN read(ObjIStream *from);
		virtual void STORM_FN write(ObjOStream *to);
		STORM_CTOR Protocol(ObjIStream *from);
	};

	/**
	 * The protocol used internally for relative paths.
	 */
	class RelativeProtocol : public Protocol {
		STORM_CLASS;
	public:
		// Ctor.
		STORM_CTOR RelativeProtocol();

		// We're the only relative one out there!
		virtual Bool absolute() { return false; }

		// Compare parts.
		virtual Bool STORM_FN partEq(Str *a, Str *b);

		// Hash parts.
		virtual Nat STORM_FN partHash(Str *a);

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Formatting.
		virtual Str *STORM_FN format(Url *url);

		// Serialization:
		static SerializedType *STORM_FN serializedType(EnginePtr e);
		static RelativeProtocol *STORM_FN read(ObjIStream *from);
		virtual void STORM_FN write(ObjOStream *to);
		STORM_CTOR RelativeProtocol(ObjIStream *from);
	};


	/**
	 * The 'file' protocol. On Windows, this causes case-insensitive file name compares.
	 */
	class FileProtocol : public Protocol {
		STORM_CLASS;
	public:
		// Ctor.
		STORM_CTOR FileProtocol();

		// Compare parts.
		virtual Bool STORM_FN partEq(Str *a, Str *b);

		// Hash parts.
		virtual Nat STORM_FN partHash(Str *a);

		// Children of this directory.
		virtual Array<Url *> *STORM_FN children(Url *url);

		// Read a file.
		virtual IStream *STORM_FN read(Url *url);

		// Write a file.
		virtual OStream *STORM_FN write(Url *url);

		// Exists?
		virtual StatType STORM_FN stat(Url *url);

		// Create a directory.
		virtual Bool STORM_FN createDir(Url *url);

		// Delete a file or directory.
		virtual Bool STORM_FN STORM_NAME(unlink,delete)(Url *url);

		// Convert an Url to a string suitable for other C-api:s.
		virtual Str *STORM_FN format(Url *url);

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Serialization:
		static SerializedType *STORM_FN serializedType(EnginePtr e);
		static FileProtocol *STORM_FN read(ObjIStream *from);
		virtual void STORM_FN write(ObjOStream *to);
		STORM_CTOR FileProtocol(ObjIStream *from);
	};


	/**
	 * HTTP/HTTPS protocol.
	 *
	 * This protocol does not support accessing the underlying data.
	 */
	class HttpProtocol : public Protocol {
		STORM_CLASS;
	public:
		// Ctor.
		STORM_CTOR HttpProtocol(Bool secure);

		// Compare parts.
		virtual Bool STORM_FN partEq(Str *a, Str *b);

		// Hash parts.
		virtual Nat STORM_FN partHash(Str *a);

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Compare.
		virtual Bool STORM_FN operator ==(const Protocol &o) const;

		// Serialization:
		static SerializedType *STORM_FN serializedType(EnginePtr e);
		static HttpProtocol *STORM_FN read(ObjIStream *from);
		virtual void STORM_FN write(ObjOStream *to);
		STORM_CTOR HttpProtocol(ObjIStream *from);

	private:
		// Is this https?
		Bool secure;
	};


	/**
	 * Protocol for memory-backed files.
	 *
	 * A single protocol may contain multiple "files" that are accessible from the protocol. The
	 * hierarchy in the memory protocol currently needs to be flat (i.e. no support for
	 * directories). Currently, writing data back to the files is not supported.
	 */
	class MemoryProtocol : public Protocol {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR MemoryProtocol();

		// Compare parts.
		virtual Bool STORM_FN partEq(Str *a, Str *b);

		// Hash parts.
		virtual Nat STORM_FN partHash(Str *a);

		// Get child items. Only makes sense on the "root" Url.
		virtual Array<Url *> *STORM_FN children(Url *url);

		// Read a file.
		virtual IStream *STORM_FN read(Url *url);

		// Write a file.
		virtual OStream *STORM_FN write(Url *url);

		// Exists?
		virtual StatType STORM_FN stat(Url *url);

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Set file contents.
		Url *STORM_FN put(Str *name, Buffer content);

		// Serialization.
		static SerializedType *STORM_FN serializedType(EnginePtr e);
		static MemoryProtocol *STORM_FN read(ObjIStream *from);
		virtual void STORM_FN write(ObjOStream *to);
		STORM_CTOR MemoryProtocol(ObjIStream *from);

	private:
		// All "files" in this protocol.
		Map<Str *, Buffer> *files;
	};

}
