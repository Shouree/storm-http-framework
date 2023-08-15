#pragma once
#include "Named.h"
#include "Reader.h"

namespace storm {
	STORM_PKG(core.info);

	/**
	 * Holds version information about something. Inspired by semantic versioning: http://semver.org/
	 */
	class Version : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR Version(Nat major, Nat minor, Nat patch);

		// Copy.
		Version(const Version &o);

		// Major, minor and patch levels.
		Nat major;
		Nat minor;
		Nat patch;

		// Pre-release?
		Array<Str *> *pre;

		// Build info?
		Array<Str *> *build;

		// Compare versions.
		Bool STORM_FN operator <(const Version &o) const;
		Bool STORM_FN operator <=(const Version &o) const;
		Bool STORM_FN operator >(const Version &o) const;
		Bool STORM_FN operator >=(const Version &o) const;
		Bool STORM_FN operator ==(const Version &o) const;
		Bool STORM_FN operator !=(const Version &o) const;

		// Hash.
		virtual Nat STORM_FN hash() const;

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Serialization.
		static SerializedType *STORM_FN serializedType(EnginePtr e);
		static Version *STORM_FN read(ObjIStream *from);
		Version(ObjIStream *from);
		void STORM_FN write(ObjOStream *to) const;

	protected:
		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;
	};

	// Parse a version string.
	MAYBE(Version *) STORM_FN parseVersion(Str *src);


	/**
	 * Stores a Version object in the name tree so that it can be associated with some part of the system easily.
	 */
	class VersionTag : public Named {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR VersionTag(Str *name, Version *version);

		// The version.
		Version *version;

	protected:
		virtual void STORM_FN toS(StrBuf *to) const;
	};

	// Find all currently loaded licenses in some part of the name tree.
	Array<VersionTag *> *STORM_FN versions(EnginePtr e) ON(Compiler);
	Array<VersionTag *> *STORM_FN versions(Named *root) ON(Compiler);



	STORM_PKG(lang.version);

	namespace version {
		// Reader for version annotations.
		PkgReader *STORM_FN reader(Array<Url *> *files, Package *pkg) ON(Compiler);
	}

	/**
	 * Reader for version files.
	 *
	 * We don't bother implementing individual file readers, as syntax highlighting is not very
	 * important for this file type.
	 */
	class VersionReader : public PkgReader {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR VersionReader(Array<Url *> *files, Package *pkg);

		// We read versions as types.
		virtual void STORM_FN readTypes();

	private:
		// Load a single license.
		VersionTag *STORM_FN readVersion(Url *file);
	};
}
