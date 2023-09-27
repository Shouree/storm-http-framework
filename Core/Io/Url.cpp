#include "stdafx.h"
#include "Url.h"
#include "Str.h"
#include "StrBuf.h"
#include "CloneEnv.h"
#include "Protocol.h"
#include "Serialization.h"
#include "SerializationUtils.h"
#include "Core/Convert.h"
#include "Core/Exception.h"

#if defined(WINDOWS)
#include <Shlobj.h>
#pragma comment (lib, "Shell32.lib")
#endif

namespace storm {

	static inline bool isDot(const wchar *str) {
		return str[0] == '.' && str[1] == '\0';
	}

	static inline bool isParent(const wchar *str) {
		return str[0] == '.' && str[1] == '.' && str[2] == '\0';
	}

	static inline bool separator(wchar c) {
		return (c == '/') || (c == '\\');
	}

	// Make sure 'str' do not contain any forbidden characters, and is not empty.
	static void validate(Str *str) {
		if (str->empty())
			throw new (str) InvalidName();

		for (const wchar *s = str->c_str(); *s; s++) {
			// Now, we only disallow separators in parts.
			if (separator(*s))
				throw new (str) InvalidName(str);
		}
	}

	static void validate(Array<Str *> *data) {
		for (Nat i = 0; i < data->count(); i++)
			validate(data->at(i));
	}

	static Array<Str *> *simplify(Array<Str *> *parts) {
		Array<Str *> *result = new (parts) Array<Str *>();
		result->reserve(parts->count());

		for (Nat i = 0; i < parts->count(); i++) {
			Str *p = parts->at(i);
			if (isDot(p->c_str())) {
				// Ignore it.
			} else if (isParent(p->c_str()) && result->any() && !isParent(result->last()->c_str())) {
				result->remove(result->count() - 1);
			} else {
				result->push(p);
			}
		}

		return result;
	}

	static void simplifyInplace(Array<Str *> *&parts) {
		for (Nat i = 0; i < parts->count(); i++) {
			Str *p = parts->at(i);
			if (isDot(p->c_str()) || isParent(p->c_str())) {
				parts = simplify(parts);
				return;
			}
		}
	}

	Url::Url() : flags(nothing) {
		parts = new (this) Array<Str *>();
		proto = new (this) RelativeProtocol();
	}

	Url::Url(Protocol *p, Array<Str *> *parts) : proto(p), parts(parts), flags(nothing) {
		dbg_assert(p, L"Need a protocol!");
		validate(this->parts);
		simplifyInplace(this->parts);
	}

	Url::Url(Array<Str *> *parts) : proto(new (engine()) RelativeProtocol()), parts(parts), flags(nothing) {
		validate(this->parts);
		simplifyInplace(this->parts);
	}

	Url::Url(Protocol *p, Array<Str *> *parts, UrlFlags flags) : proto(p), parts(parts), flags(flags) {
		dbg_assert(p, L"Need a protocol!");
		validate(this->parts);
		simplifyInplace(this->parts);
	}

	Url::Url(Array<Str *> *parts, UrlFlags flags) : proto(new (engine()) RelativeProtocol()), parts(parts), flags(flags) {
		validate(this->parts);
		simplifyInplace(this->parts);
	}

	void Url::toS(StrBuf *to) const {
		*to << proto;

		if (parts->count() > 0)
			*to << parts->at(0);

		for (Nat i = 1; i < parts->count(); i++)
			*to << L"/" << parts->at(i);

		if (flags & isDir)
			*to << L"/";
	}

	Url::Url(ObjIStream *from) {
		// Read data here so that we can initialize Url later.
		proto = (Protocol *)Protocol::read(from);
		parts = Serialize<Array<Str *> *>::read(from);
		flags = UrlFlags(Serialize<Nat>::read(from));
		from->end();
	}

	SerializedType *Url::serializedType(EnginePtr e) {
		SerializedStdType *t = serializedStdType<Url>(e.v);
		t->add(S("protocol"), StormInfo<Protocol>::type(e.v));
		t->add(S("parts"), StormInfo<Array<Str *>>::type(e.v));
		t->add(S("flags"), StormInfo<Nat>::type(e.v));
		return t;
	}

	Url *Url::read(ObjIStream *from) {
		return from->readClass<Url>();
	}

	void Url::write(ObjOStream *to) const {
		if (to->startClass(this)) {
			proto->write(to);
			Serialize<Array<Str *> *>::write(parts, to);
			Serialize<Nat>::write(Nat(flags), to);
			to->end();
		}
	}

	void Url::deepCopy(CloneEnv *e) {
		cloned(parts, e);
	}

	Bool Url::operator ==(const Url &o) const {
		if (!sameType(this, &o))
			return false;

		if (*proto != *o.proto)
			return false;

		if (parts->count() != o.parts->count())
			return false;

		for (Nat i = 0; i < parts->count(); i++) {
			if (!proto->partEq(parts->at(i), o.parts->at(i)))
				return false;
		}

		return true;
	}

	Nat Url::hash() const {
		Nat r = 5381;
		for (Nat i = 0; i < parts->count(); i++) {
			r = ((r << 5) + r) + proto->partHash(parts->at(i));
		}
		return r;
	}

	Array<Str *> *Url::getParts() const {
		return new (this) Array<Str *>(*parts);
	}

	Url *Url::copy() const {
		Url *c = new (this) Url(*this);
		c->parts = new (this) Array<Str *>(*c->parts);
		return c;
	}

	Url *Url::push(Str *p) const {
		validate(p);

		Url *c = copy();
		if (p->empty())
			return c;

		c->parts->push(p);
		simplifyInplace(c->parts);
		c->flags &= ~isDir;
		return c;
	}

	Url *Url::operator /(Str *p) const {
		return push(p);
	}

	Url *Url::pushDir(Str *p) const {
		validate(p);

		Url *c = copy();
		if (p->empty())
			return c;

		c->parts->push(p);
		simplifyInplace(c->parts);
		c->flags |= isDir;
		return c;
	}

	Url *Url::makeDir() const {
		return new (this) Url(proto, parts, isDir);
	}

	Url *Url::makeAbsolute(Url *base) {
		if (absolute())
			return this;
		return base->push(this);
	}

	Url *Url::push(Url *url) const {
		if (url->absolute())
			throw new (this) InvalidName(url->toS());

		Url *c = copy();
		for (Nat i = 0; i < url->parts->count(); i++)
			c->parts->push(url->parts->at(i));

		simplifyInplace(c->parts);
		c->flags = (flags & ~isDir) | (url->flags & isDir);
		return c;
	}

	Url *Url::operator /(Url *p) const {
		return push(p);
	}

	Url *Url::parent() const {
		Array<Str *> *p = new (this) Array<Str *>();

		if (parts->any())
			for (Nat i = 0; i < parts->count() - 1; i++)
				p->push(parts->at(i));

		return new (this) Url(proto, p, flags | isDir);
	}

	Bool Url::dir() const {
		return (flags & isDir) != nothing;
	}

	Bool Url::absolute() const {
		return proto->absolute();
	}

	Str *Url::name() const {
		if (parts->any())
			return parts->last();
		else
			return new (this) Str(L"");
	}

	static Str::Iter divideName(Str *str) {
		if (str->empty())
			return str->begin();

		Str::Iter last = str->end();
		for (Str::Iter at = str->begin(); at != str->end(); at++) {
			if (at.v() == Char(L'.'))
				last = at;
		}

		if (last == str->begin())
			last = str->end();
		return last;
	}

	Str *Url::ext() const {
		Str *n = name();
		Str::Iter d = divideName(n);
		d++;
		return n->substr(d);
	}

	Url *Url::withExt(Str *ext) const {
		Url *c = copy();
		if (parts->empty())
			return c;

		// Sorry about the derefs, the + operator is intended for use in Storm...
		c->parts->last() = *(*this->title() + S(".")) + ext;
		return c;
	}

	Str *Url::title() const {
		Str *n = name();
		Str::Iter d = divideName(n);
		return n->substr(n->begin(), d);
	}

	Url *Url::relative(Url *to) {
		if (absolute() != to->absolute())
			throw new (this) UrlError(new (this) Str(S("Both paths to 'relative' must be either absolute or relative.")));

		// Different protocols, not possible...
		if (*proto != *to->proto)
			return this;

		Array<Str *> *rel = new (this) Array<Str *>();
		Str *up = new (this) Str(L"..");

		Nat equalTo = 0; // Equal up to a position
		for (Nat i = 0; i < to->parts->count(); i++) {
			if (equalTo == i) {
				if (i >= parts->count()) {
				} else if (proto->partEq(to->parts->at(i), parts->at(i))) {
					equalTo = i + 1;
				}
			}

			if (equalTo <= i)
				rel->push(up);
		}

		for (Nat i = equalTo; i < parts->count(); i++)
			rel->push(parts->at(i));

		return new (this) Url(rel, flags);
	}

	Url *Url::relativeIfBelow(Url *to) {
		if (absolute() != to->absolute())
			throw new (this) UrlError(new (this) Str(S("Both paths to 'relativeIfBelow' must be either absolute or relative.")));

		// Different protocols?
		if (*proto != *to->proto)
			return this;

		// Check so that "to" is equal to "this" for all elements in "to".
		if (to->parts->count() > parts->count())
			return this;

		for (Nat i = 0; i < to->parts->count(); i++) {
			if (!proto->partEq(to->parts->at(i), parts->at(i)))
				return this;
		}

		Array<Str *> *rel = new (this) Array<Str *>();
		for (Nat i = to->parts->count(); i < parts->count(); i++)
			rel->push(parts->at(i));

		return new (this) Url(rel, flags);
	}

	Url *Url::updated() {
		UrlFlags f = flags;
		switch (proto->stat(this)) {
		case sNotFound:
			return this;
		case sDirectory:
			f |= isDir;
			break;
		case sFile:
			f &= ~isDir;
			break;
		}

		if (f != flags)
			return new (this) Url(proto, parts, f);
		else
			return this;
	}

	/**
	 * Forward to the protocol.
	 */

	// Find all children URL:s.
	Array<Url *> *Url::children() {
		return proto->children(this);
	}

	// Open this Url for reading.
	IStream *Url::read() {
		return proto->read(this);
	}

	// Open this Url for writing.
	OStream *Url::write() {
		return proto->write(this);
	}

	// Does this Url exist?
	Bool Url::exists() {
		return proto->stat(this) != sNotFound;
	}

	// Create a directory.
	Bool Url::createDir() {
		return proto->createDir(this);
	}

	// Create a directory, recursive.
	Bool Url::createDirTree() {
		if (!exists()) {
			if (!parent()->createDirTree())
				return false;

			return createDir();
		} else {
			return true;
		}
	}

	// Delete.
	Bool Url::unlink() {
		return proto->unlink(this);
	}

	// Recursive delete.
	Bool Url::deleteTree() {
		Bool ok = true;
		Array<Url *> *children = this->children();
		for (Nat i = 0; i < children->count(); i++) {
			ok &= children->at(i)->deleteTree();
		}

		if (ok) {
			ok &= unlink();
		}

		return ok;
	}

	// Format.
	Str *Url::format() {
		return proto->format(this);
	}


	/**
	 * Parsing
	 */


	Url *parsePath(Str *s) {
		return parsePath(s->engine(), s->c_str());
	}

	Url *parsePathAsDir(Str *s) {
		return parsePathAsDir(s->engine(), s->c_str());
	}

	static Url *parsePathI(Engine &e, const wchar *src, UrlFlags flags) {
		Array<Str *> *parts = new (e) Array<Str *>();
		Protocol *protocol = new (e) FileProtocol();

		if (*src == 0)
			return new (e) Url(null, parts, flags);

		const wchar *start = src;
		// UNIX absolute path?
		if (separator(*start))
			start++;
		// Windows absulute path?
		else if (src[0] != 0 && src[1] == ':')
			start = src;
		// Windows network share?
		else if (separator(src[0]) && separator(src[1]))
			start = src + 2;
		// Relative path?
		else
			protocol = new (e) RelativeProtocol();

		if (*start == 0)
			return new (e) Url(protocol, parts, flags);

		const wchar *end = start;
		for (; end[1]; end++)
			;

		if (separator(*end)) {
			flags |= isDir;
			end--;
		}

		const wchar *last = start;
		for (const wchar *i = start; i < end; i++) {
			if (separator(*i)) {
				if (i > last) {
					parts->push(new (e) Str(last, i));
				}
				last = i + 1;
			}
		}
		if (end + 1 > last)
			parts->push(new (e) Str(last, end + 1));

		return new (e) Url(protocol, parts, flags);
	}

	Url *parsePath(Engine &e, const wchar *src) {
		return parsePathI(e, src, nothing);
	}

	Url *parsePathAsDir(Engine &e, const wchar *src) {
		return parsePathI(e, src, isDir);
	}

#if defined(WINDOWS)
	Url *cwdUrl(EnginePtr e) {
		DWORD written = GetCurrentDirectory(0, NULL);
		std::vector<wchar_t> buffer;
		// Loop is only necessary if another thread modifies CWD while we are getting it.
		do {
			// +1 is not really needed, just to have some margin since GetCurrentDirectory sometimes
			// returns length not including null terminator...
			buffer.resize(written + 1);
			buffer[0] = 0;
			written = GetCurrentDirectory(DWORD(buffer.size()), &buffer[0]);
		} while (written > buffer.size());

		return parsePathAsDir(e.v, &buffer[0]);
	}

	Url *userConfigUrl(Str *appName) {
		Engine &e = appName->engine();

		wchar_t tmp[MAX_PATH + 1];
		if (SHGetFolderPath(NULL, CSIDL_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, tmp) != S_OK)
			throw new (e) InternalError(S("Failed to retrieve the AppData folder for the current user."));

		Url *url = parsePath(e, tmp);
		url = url->pushDir(appName);
		if (!url->exists())
			url->createDir();
		return url;
	}

	Url *executableFileUrl(Engine &e) {
		std::vector<wchar_t> buffer(MAX_PATH + 1, 0);
		do {
			GetModuleFileName(NULL, &buffer[0], DWORD(buffer.size()));
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
				buffer.resize(buffer.size() * 2);
				continue;
			}
		} while (false);

		return parsePath(e, &buffer[0]);
	}

#elif defined(POSIX)
	Url *cwdUrl(EnginePtr e) {
		char path[PATH_MAX + 1] = { 0 };
		if (!getcwd(path, PATH_MAX))
			throw new (e.v) InternalError(S("Failed to get the current working directory."));
		return parsePathAsDir(e.v, toWChar(e.v, path)->v);
	}

	Url *userConfigUrl(Str *appName) {
		Engine &e = appName->engine();
		Url *url = null;

		const char *base = getenv("XDG_CONFIG_HOME");
		if (base && *base != '\0') {
			url = parsePath(e, toWChar(e, base)->v);
		} else {
			const char *home = getenv("HOME");
			if (home == null)
				throw new (e) InternalError(S("Failed to find the current user's HOME directory."));
			url = parsePath(e, toWChar(e, home)->v);
			url = url->pushDir(new (e) Str(S(".config")));
		}

		if (!url->exists())
			url->createDir();

		url = url->pushDir(appName);

		if (!url->exists())
			url->createDir();

		return url;
	}

	Url *executableFileUrl(Engine &e) {
		char path[PATH_MAX + 1] = { 0 };
		ssize_t r = readlink("/proc/self/exe", path, PATH_MAX);
		if (r >= PATH_MAX || r < 0)
			throw new (e) InternalError(S("Failed to get the path of the executable."));
		return parsePath(e, toWChar(e, path)->v);
	}
#else
#error "Please implement executableFileUrl and cwdUrl for your OS!"
#endif

	Url *executableUrl(Engine &e) {
		Url *v = executableFileUrl(e);
		return v->parent();
	}

	Url *dbgRootUrl(Engine &e) {
#ifndef DEBUG
		WARNING(L"Using dbgRoot in release!");
#endif
		Url *v = executableUrl(e);
		return v->parent();
	}

	Url *httpUrl(Str *host) {
		Protocol *p = new (host) HttpProtocol(false);
		return new (host) Url(p, new (host) Array<Str *>(1, host));
	}

	Url *httpsUrl(Str *host) {
		Protocol *p = new (host) HttpProtocol(true);
		return new (host) Url(p, new (host) Array<Str *>(1, host));
	}

}
