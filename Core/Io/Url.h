#pragma once
#include "Core/Object.h"
#include "Core/Array.h"
#include "Core/EnginePtr.h"
#include "Core/Exception.h"
#include "Utils/Bitmask.h"

namespace storm {
	STORM_PKG(core.io);

	class Protocol;
	class IStream;
	class OStream;

	// Flags describing the surroundings of 'parts'.
	enum UrlFlags {
		nothing = 0x00,

		// Is this a directory (ie trailing backslash?). It is, however, always possible to
		// append another part to a non-directory for convenience reasons.
		isDir = 0x01,
	};

	BITMASK_OPERATORS(UrlFlags);

	/**
	 * An URL represents a URL in a system-independent format that is easy to manipulate
	 * programmatically. The 'protocol' part in the URL specifies what backend that should
	 * handle requests to open the specified URL. The object is designed to be treated
	 * as an immutable object.
	 * To keep it simple, this object will always output the unix path separator (/). Correctly
	 * formatting URL:s for use with file systems is left to the implementation of protocols.
	 */
	class Url : public Object {
		STORM_CLASS;
	public:

		// Empty url.
		STORM_CTOR Url();

		// Create from fundamentals.
		STORM_CTOR Url(Protocol* p, Array<Str *> *parts, UrlFlags flags);

		// Relative path creation.
		STORM_CTOR Url(Array<Str *> *parts, UrlFlags flags);

		// Ctor for STORM.
		STORM_CTOR Url(Protocol *p, Array<Str *> *parts);

		// Relative path.
		STORM_CTOR Url(Array<Str *> *parts);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *o);

		// Equals. Note that this is always a bitwise equality, multiple paths may name
		// the same file!
		virtual Bool STORM_FN operator ==(const Url &o) const;

		// Hash of this url.
		virtual Nat STORM_FN hash() const;

		// Append another path to this one. The other one has to be a relative path.
		Url *STORM_FN push(Url *url) const;
		Url *STORM_FN operator /(Url *url) const;

		// Append another part to this one.
		Url *STORM_FN push(Str *part) const;
		Url *STORM_FN operator /(Str *url) const;

		// Append another part to this one, making the resulting Url into a directory.
		Url *STORM_FN pushDir(Str *part) const;

		// Make into a directory.
		Url *STORM_FN makeDir() const;

		// Make the Url absolute if it is not already absolute.
		Url *STORM_FN makeAbsolute(Url *base);

		// Get all parts.
		Array<Str *> *STORM_FN getParts() const;

		// Is this a directory? Note: This only depends on the pathname.
		Bool STORM_FN dir() const;

		// Absolute path?
		Bool STORM_FN absolute() const;
		Bool STORM_FN relative() const { return !absolute(); }

		// Parent directory.
		Url *STORM_FN parent() const;

		// Get the file title. The title does not contain the file extension.
		Str *STORM_FN title() const;

		// Get the file name, this includes the extension.
		Str *STORM_FN name() const;

		// Get the file extension.
		Str *STORM_FN ext() const;

		// Generate an URL with the given extension. Replaces the original one.
		Url *STORM_FN withExt(Str *ext) const;

		// Generate a relative path (is spiritually const, but may return itself).
		Url *STORM_FN relative(Url *to);

		// Generate a relative path if the current path is below "to" (i.e. they share a common prefix).
		Url *STORM_FN relativeIfBelow(Url *to);

		// Update this Url, that is: check if it is a file or a directory and update the status
		// accordingly. You typically only need to call this function if you constructed the Url
		// yourself and are unsure of whether it actually was a file or directory (e.g. because it
		// was from user input). Does not modify the object, but may return itself.
		Url *STORM_FN updated();

		/**
		 * Low-level operations.
		 */

		// Get the number of parts in this URL.
		inline Nat STORM_FN count() const { return parts->count(); }
		inline Bool STORM_FN empty() const { return parts->empty(); }
		inline Bool STORM_FN any() const { return parts->any(); }

		// Get a specific part.
		inline Str *at(Nat i) const { return parts->at(i); }
		inline Str *STORM_FN operator[](Nat i) const { return parts->at(i); }

		/**
		 * Find out things about this URL. All operations are not always supported
		 * by all protocols. Note that these are generally assumed to be run on non-relative urls.
		 */

		// Find all children URL:s.
		Array<Url *> *STORM_FN children();

		// Open this Url for reading.
		IStream *STORM_FN read();

		// Open this Url for writing.
		OStream *STORM_FN write();

		// Does this Url exist?
		Bool STORM_FN exists();

		// Create this path as a directory. Does not attempt to create parent directories.
		Bool STORM_FN createDir();

		// Recursively create the directories for this path.
		Bool STORM_FN createDirTree();

		// Delete this path. Directories need to be empty.
		Bool STORM_FN STORM_NAME(unlink,delete)();

		// Delete this path. Recurses into sub-folders if necessary.
		Bool STORM_FN deleteTree();

		// Format for other C-api:s. May not work for all kind of URL:s.
		Str *STORM_FN format();

		// Get the protocol used.
		Protocol *STORM_FN protocol() const {
			return proto;
		}

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Serialization.
		static SerializedType *STORM_FN serializedType(EnginePtr e);
		static Url *STORM_FN read(ObjIStream *from);
		Url(ObjIStream *from);
		void STORM_FN write(ObjOStream *to) const;

	private:
		// Protocol.
		Protocol *proto;

		// Parts of the path.
		Array<Str *> *parts;

		// Flags
		UrlFlags flags;

		// Make a copy we can modify.
		Url *copy() const;
	};

	// Parse a path into a Url. The path is assumed to be on the local file system.
	Url *STORM_FN parsePath(Str *s);
	Url *parsePath(Engine &e, const wchar *str);

	// Parse a path into a Url. The path is assumed to be on the local file system, and refer to a directory.
	Url *STORM_FN parsePathAsDir(Str *s);
	Url *parsePathAsDir(Engine &e, const wchar *str);

	// Get an url that points to the current Storm executable.
	Url *STORM_FN executableFileUrl(EnginePtr e);

	// Get an url that points to the path where the current Storm binary is located.
	Url *STORM_FN executableUrl(EnginePtr e);

	// Path of the root in debug mode.
	Url *dbgRootUrl(Engine &e);

	// Get the current working directory.
	Url *STORM_FN cwdUrl(EnginePtr e);

	// Get the user's config directory for an application with the name `appName`. Created if not
	// already present.
	Url *STORM_FN userConfigUrl(Str *appName);

	// Create the url `http://` for the specified host.
	Url *STORM_FN httpUrl(Str *host);

	// Create the url `https://` for the specified host.
	Url *STORM_FN httpsUrl(Str *host);

	// Some exceptions.
	class EXCEPTION_EXPORT InvalidName : public Exception {
		STORM_EXCEPTION;
	public:
		STORM_CTOR InvalidName() : name(null) {}
		STORM_CTOR InvalidName(Str *name) : name(name) {}

		virtual void STORM_FN message(StrBuf *to) const {
			if (!name)
				*to << S("Empty name segments are not allowed.");
			else
				*to << S("The url part ") << name << S(" is not acceptable.");
		}
	private:
		MAYBE(Str *) name;
	};

	class EXCEPTION_EXPORT UrlError : public Exception {
		STORM_EXCEPTION;
	public:
		STORM_CTOR UrlError(Str *name) : name(name) {}

		virtual void STORM_FN message(StrBuf *to) const {
			*to << S("Url error: ") << name;
		}

	private:
		Str *name;
	};
}
