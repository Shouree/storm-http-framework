#include "stdafx.h"
#include "Version.h"
#include "NameSet.h"
#include "Package.h"
#include "Engine.h"
#include "Core/Str.h"
#include "Core/Hash.h"
#include "Core/Join.h"
#include "Core/Io/Text.h"
#include "Core/Io/Serialization.h"
#include "Core/Io/SerializationUtils.h"

#ifdef major
#undef major
#endif

#ifdef minor
#undef minor
#endif

namespace storm {

	Version::Version(Nat major, Nat minor, Nat patch)
		: major(major), minor(minor), patch(patch),
		  pre(new (engine()) Array<Str *>()),
		  build(new (engine()) Array<Str *>()) {}

	Version::Version(const Version &src) {
		this->major = src.major;
		this->minor = src.minor;
		this->patch = src.patch;
		// Note: Strings are OK not to deep copy since they are immutable.
		this->pre = new (this) Array<Str *>(*src.pre);
		this->build = new (this) Array<Str *>(*src.build);
	}

	void Version::deepCopy(CloneEnv *env) {
		// We do a deep copy in the copy constructor, so we do not need to do anything here.
	}

	void Version::toS(StrBuf *to) const {
		*to << major << S(".") << minor << S(".") << patch;
		if (pre->any())
			*to << S("-") << join(pre, S("."));

		if (build->any())
			*to << S("+") << join(build, S("."));
	}

	static int compare(Str *a, Str *b) {
		Bool aNr = a->isNat();
		Bool bNr = b->isNat();
		if (aNr && bNr) {
			Nat aa = a->toNat();
			Nat bb = b->toNat();
			if (aa == bb)
				return 0;
			return aa < bb ? -1 : 1;
		} else if (aNr != bNr) {
			// Numbers have a lower priority than strings.
			return aNr ? -1 : 1;
		} else {
			// Just compare the strings.
			return wcscmp(a->c_str(), b->c_str());
		}
	}

	Bool Version::operator <(const Version &o) const {
		if (major != o.major)
			return major < o.major;
		if (minor != o.minor)
			return minor < o.minor;
		if (patch != o.patch)
			return patch < o.patch;

		if (pre->empty() != o.pre->empty()) {
			// One of them are empty. That one is the later one.
			return o.pre->empty();
		}

		// Compare the pre-release statuses lexiographically.
		Nat to = min(pre->count(), o.pre->count());
		for (Nat i = 0; i < to; i++) {
			int r = compare(pre->at(i), o.pre->at(i));
			if (r != 0)
				return r < 0;
		}

		return pre->count() < o.pre->count();
	}

	Bool Version::operator <=(const Version &o) const {
		return !(*this > o);
	}

	Bool Version::operator >(const Version &o) const {
		return o < *this;
	}

	Bool Version::operator >=(const Version &o) const {
		return !(*this < o);
	}

	Bool Version::operator ==(const Version &o) const {
		if (major != o.major)
			return false;
		if (minor != o.minor)
			return false;
		if (patch != o.patch)
			return false;

		if (pre->count() != o.pre->count())
			return false;

		for (Nat i = 0; i < pre->count(); i++)
			if (*pre->at(i) != *o.pre->at(i))
				return false;

		return true;
	}

	Bool Version::operator !=(const Version &o) const {
		return !(*this == o);
	}

	Nat Version::hash() const {
		Nat result = natHash(major);
		result ^= natHash(minor);
		result ^= natHash(patch);

		for (Nat i = 0; i < pre->count(); i++) {
			result ^= pre->at(i)->hash();
		}

		// Note 'build' are ignored in comparisons, so we shall ignore it here as well!
		return result;
	}

	SerializedType *Version::serializedType(EnginePtr e) {
		SerializedStdType *t = serializedStdType<Version>(e.v);
		Type *natType = StormInfo<Nat>::type(e.v);
		Type *strArray = StormInfo<Array<Str *>>::type(e.v);
		t->add(S("major"), natType);
		t->add(S("minor"), natType);
		t->add(S("patch"), natType);
		t->add(S("pre"), strArray);
		t->add(S("build"), strArray);
		return t;
	}

	Version::Version(ObjIStream *from) {
		major = Serialize<Nat>::read(from);
		minor = Serialize<Nat>::read(from);
		patch = Serialize<Nat>::read(from);
		pre = Serialize<Array<Str *> *>::read(from);
		build = Serialize<Array<Str *> *>::read(from);
		from->end();
	}

	Version *Version::read(ObjIStream *from) {
		return from->readClass<Version>();
	}

	void Version::write(ObjOStream *to) const {
		if (to->startClass(this)) {
			Serialize<Nat>::write(major, to);
			Serialize<Nat>::write(minor, to);
			Serialize<Nat>::write(patch, to);
			Serialize<Array<Str *> *>::write(pre, to);
			Serialize<Array<Str *> *>::write(build, to);
			to->end();
		}
	}

	static bool parseNat(const wchar *&at, Nat &out) {
		bool one = false;
		out = 0;
		for (; *at; at++) {
			if (*at >= '0' && *at <= '9') {
				one = true;
				out *= 10;
				out += *at - '0';
			} else {
				break;
			}
		}
		return one;
	}

	static bool parseList(const wchar *&at, Array<Str *> *out) {
		const wchar *start = at;
		bool exit = false;
		while (true) {
			bool add = false;

			switch (*at) {
			case '-':
			case '+':
			case '\0':
				add = true;
				exit = true;
				break;
			case '.':
				add = true;
				break;
			}

			if (add) {
				if (start == at)
					return false;
				out->push(new (out) Str(start, at));
				start = at + 1;
			}

			if (exit)
				break;
			else
				at++;
		}

		return out->any();
	}

	MAYBE(Version *) parseVersion(Str *src) {
		Nat major = 0;
		Nat minor = 0;
		Nat patch = 0;

		const wchar *at = src->c_str();
		if (!parseNat(at, major))
			return null;
		if (*at != '.')
			return null;
		at++;
		if (!parseNat(at, minor))
			return null;
		// It is OK to skip the last digit.
		if (*at == '.') {
			at++;
			if (!parseNat(at, patch))
				return null;
		}

		Version *result = new (src) Version(major, minor, patch);

		if (*at == '-') {
			at++;
			if (!parseList(at, result->pre))
				return null;
		}

		if (*at == '+') {
			at++;
			if (!parseList(at, result->build))
				return null;
		}

		if (*at != 0)
			return null;
		return result;
	}


	/**
	 * Version tag.
	 */

	VersionTag::VersionTag(Str *name, Version *version) : Named(name), version(version) {}

	void VersionTag::toS(StrBuf *to) const {
		*to << identifier() << S(": ") << version;
	}

	static void versions(Array<VersionTag *> *r, Named *root) {
		if (VersionTag *v = as<VersionTag>(root)) {
			r->push(v);
			return;
		}

		NameSet *search = as<NameSet>(root);
		if (!search)
			return;

		for (NameSet::Iter i = search->begin(), e = search->end(); i != e; ++i) {
			versions(r, i.v());
		}
	}

	Array<VersionTag *> *versions(Named *root) {
		Array<VersionTag *> *r = new (root) Array<VersionTag *>();
		versions(r, root);
		return r;
	}

	Array<VersionTag *> *versions(EnginePtr e) {
		return versions(e.v.package());
	}


	/**
	 * Reader.
	 */

	namespace version {
		PkgReader *reader(Array<Url *> *files, Package *pkg) {
			return new (pkg->engine()) VersionReader(files, pkg);
		}
	}

	VersionReader::VersionReader(Array<Url *> *files, Package *pkg) : PkgReader(files, pkg) {}

	void VersionReader::readTypes() {
		for (Nat i = 0; i < files->count(); i++) {
			pkg->add(readVersion(files->at(i)));
		}
	}

	VersionTag *VersionReader::readVersion(Url *file) {
		TextInput *text = readText(file);
		Str *ver = text->readLine();
		text->close();

		Version *version = parseVersion(ver);
		if (!version)
			throw new (this) SyntaxError(SrcPos(file, 0, ver->peekLength()), S("Invalid version syntax."));

		return new (this) VersionTag(file->title(), version);
	}

}
