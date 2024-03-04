#include "stdafx.h"
#include "Package.h"
#include "Core/StrBuf.h"
#include "Core/Str.h"
#include "Core/Io/Text.h"
#include "Core/Set.h"
#include "Engine.h"
#include "Reader.h"
#include "Exception.h"

namespace storm {

	Package::Package(Str *name) : NameSet(name), discardOnLoad(true), exportedLoaded(false) {}

	Package::Package(Url *path) : NameSet(path->name()), pkgPath(path), discardOnLoad(true), exportedLoaded(false) {
		engine().pkgMap()->put(pkgPath, this);

		documentation = new (this) PackageDoc(this);
	}

	Package::Package(Str *name, Url *path) : NameSet(name), pkgPath(path), discardOnLoad(true), exportedLoaded(false) {
		engine().pkgMap()->put(pkgPath, this);

		documentation = new (this) PackageDoc(this);
	}

	NameLookup *Package::parent() const {
		// We need to be able to return null.
		return parentLookup;
	}

	Url *Package::url() const {
		return pkgPath;
	}

	void Package::setUrl(Url *url) {
		assert(!engine().has(bootDone), L"Shall not be done after boot is complete!");
		if (pkgPath)
			engine().pkgMap()->remove(pkgPath);
		pkgPath = url;
		engine().pkgMap()->put(pkgPath, this);

		if (pkgPath->dir()) {
			for (Iter i = begin(), e = end(); i != e; ++i) {
				if (Package *p = as<Package>(i.v())) {
					if (p->params->count() > 0)
						continue;
					Url *sub = url->pushDir(p->name);
					if (!sub->exists())
						continue;

					p->setUrl(sub);
				}
			}
		}

		if (!documentation)
			documentation = new (this) PackageDoc(this);
	}

	MAYBE(Named *) Package::find(SimplePart *part, Scope source) {
		if (Named *found = NameSet::find(part, source))
			return found;

		// TODO: Consider if this is interesting to do.
		// Only consider exports if the call originated from another package than this one. The
		// feature is named "exports" after all.
		// if (source.top && ScopeLookup::firstPkg(source.top) == this)
		// 	return null;

		loadExports();
		if (exported) {
			// Look inside exported packages, beware of cycles.
			ExportSet examined(engine());
			examined.push(this);

			for (Nat i = 0; i < exported->count(); i++)
				if (Named *found = exported->at(i)->recursiveFind(examined, part, source))
					return found;
		}

		return null;
	}

	MAYBE(Named *) Package::recursiveFind(ExportSet &examined, SimplePart *part, Scope source) {
		// See if this package is already examined.
		for (nat i = 0; i < examined.count(); i++)
			if (examined[i] == this)
				return null;

		// Remember that we have been examined.
		examined.push(this);

		// Look here. Note: We can not call 'find' directly, then we would lose 'examined'.
		if (Named *found = NameSet::find(part, source))
			return found;

		if (exported) {
			// Look at exports.
			for (Nat i = 0; i < exported->count(); i++)
				if (Named *found = exported->at(i)->recursiveFind(examined, part, source))
					return found;
		}

		// Note: We don't have to remove this package from 'examined'. Doing so seems intuitive, but
		// would result in us examining the same package from multiple paths. The big benefit of
		// removing ourselves is that the 'examined' array would be smaller in size, and thus the
		// loop in the top of this function would be faster. Calling 'find' more times is likely
		// more expensive than a few extra iterations in a linear scan, however.

		return null;
	}

	Bool Package::loadName(SimplePart *part) {
		// We're only loading packages this way.
		if (part->params->empty()) {
			if (Package *pkg = loadPackage(part->name)) {
				add(pkg);
				return true;
			}
		}

		// There may be other things to load!
		return false;
	}

	Bool Package::loadAll() {
		// Do not load things before the compiler has properly started. As everything is a chaos at
		// that time, loading anything external will most likely fail. So we retry later.
		if (!engine().has(bootDone))
			return false;

		// Nothing to load.
		if (!pkgPath)
			return true;

		// Load exports if needed.
		loadExports();

		Array<Url *> *files = new (this) Array<Url *>();

		if (pkgPath->dir()) {
			// A directory, the normal case:
			Array<Url *> *children = pkgPath->children();

			// Load any remaining packages:
			for (Nat i = 0; i < children->count(); i++) {
				Url *now = children->at(i);
				if (now->dir()) {
					Str *name = now->name();
					if (!tryFind(new (this) SimplePart(name), Scope()))
						add(loadPackage(name));
				} else {
					files->push(now);
				}
			}
		} else {
			// It is a file. Just load it. This is used when specifying files on the command line for example.
			files->push(pkgPath);
		}

		// Load all code.
		loadFiles(files);

		return true;
	}

	void Package::addExport(Package *pkg) {
		if (!exported)
			exported = new (this) Array<Package *>();

		// Remove duplicates: generally quite cheap to do, and prevents lookups from being expensive.
		for (Nat i = 0; i < exported->count(); i++)
			if (exported->at(i) == pkg)
				return;

		exported->push(pkg);
	}

	Array<Package *> *Package::exports() {
		loadExports();

		if (exported)
			return new (this) Array<Package *>(*exported);
		else
			return new (this) Array<Package *>();
	}

	Array<Package *> *Package::recursiveExports() {
		Array<Package *> *result = exports();
		Set<Package *> *seen = new (this) Set<Package *>();

		// Note: 'result->count()' will change.
		for (Nat at = 0; at < result->count(); at++) {
			Package *examine = result->at(at);

			examine->loadExports();
			if (examine->exported) {
				for (Nat i = 0; i < examine->exported->count(); i++) {
					Package *add = examine->exported->at(i);
					if (!seen->has(add)) {
						seen->put(add);
						result->push(add);
					}
				}
			}
		}

		return result;
	}

	void Package::noDiscard() {
		discardOnLoad = false;
	}

	void Package::discardSource() {
		// We don't need to propagate this message, we emit it ourselves.
	}

	void Package::toS(StrBuf *to) const {
		if (parent())
			*to << L"Package " << identifier();
		else
			*to << L"Root package (" << name << L")";

		if (pkgPath)
			*to << L"(in " << pkgPath << L")";
		else
			*to << L"(virtual)";

		if (false) {
			*to << L"\n";
			Indent z(to);
			NameSet::toS(to);
		}
	}

	void Package::loadExports() {
		if (exportedLoaded)
			return;
		exportedLoaded = true;

		if (!pkgPath)
			return;

		Url *exports = pkgPath->push(new (this) Str(S("export")));
		if (!exports->exists())
			return;

		if (!exported)
			exported = new (this) Array<Package *>();

		Scope root = engine().scope();
		TextInput *input = readText(exports);
		SrcPos pos(exports, 0, 0);
		while (input->more()) {
			Str *line = input->readLine();

			// Update position. We only count newlines as 1 character, regardless of how they are
			// represented in the file.
			if (pos.end != 0)
				pos.start = pos.end + 1;
			pos.end = pos.start + line->peekLength(); // Not always exact, but good enough.

			// Trim whitespace to make "parsing" easier.
			line = trimWhitespace(line);

			// Empty line, ignore.
			if (line->begin() == line->end())
				continue;

			// Comment, ignore.
			if (line->begin().v() == Char('#'))
				continue;
			if (line->begin().v() == Char('/') && (line->begin() + 1).v() == Char('/'))
				continue;

			Name *name = parseComplexName(line);
			if (!name)
				throw new (this) SyntaxError(pos, TO_S(this, "Invalid format of names. Use: 'x.y.z(a, b.c)'. Comments start with '#' or '//'"));

			Named *found = root.find(name);
			if (!found)
				throw new (this) SyntaxError(pos, TO_S(this, S("The name ") << name << S(" does not refer to anything.")));

			Package *p = as<Package>(found);
			if (!p)
				throw new (this) SyntaxError(pos, TO_S(this, S("The name ") << found << S(" does not refer to a package.")));

			exported->push(p);
		}
	}

	Package *Package::loadPackage(Str *name) {
		// Virtual package, can not be auto loaded.
		if (!pkgPath)
			return null;

		// If pkgPath was a file, then we can't auto-load any sub-packages.
		if (!pkgPath->dir()) {
			return null;
		}

		// Name parts that are empty do not play well with the Url.
		if (name->empty())
			return null;

		// TODO: Make sure this is case sensitive!
		Url *sub = pkgPath->pushDir(name);
		if (!sub->exists())
			return null;

		return new (this) Package(sub);
	}

	void Package::loadFiles(Array<Url *> *files) {
		// TODO: Remember previous contents if things go wrong...
		// Array<Named *> *prev = new (this) Array<Named *>();
		// for (Iter i = begin(), to = end(); i != to; ++i)
		// 	prev->push(i.v());

		try {
			Map<SimpleName *, PkgFiles *> *readers = readerName(files);
			Array<PkgReader *> *load = createReaders(readers);

			// Load everything!
			read(load);

			// Ask functions to discard their sources.
			if (discardOnLoad)
				NameSet::discardSource();

		} catch (...) {
			TODO(L"Try to restore!");
			throw;
		}
	}

	static Str *noReaderWarning(SimpleName *name, Array<Url *> *files) {
		StrBuf *msg = new (name) StrBuf();
		*msg << L"No reader for [";
		Url *rootUrl = name->engine().package()->url();
		for (nat i = 0; i < files->count(); i++) {
			if (i > 0)
				*msg << L", ";
			*msg << files->at(i)->relative(rootUrl);
		}
		*msg << L"] (should be " << name << L")";

		return msg->toS();
	}

	Array<PkgReader *> *Package::createReaders(Map<SimpleName *, PkgFiles *> *readers) {
		typedef Map<SimpleName *, PkgFiles *> ReaderMap;
		Array<PkgReader *> *r = new (this) Array<PkgReader *>();
		SimpleName *me = path();

		SimpleName *delayName = null;
		PkgFiles *delayFiles = null;

		for (ReaderMap::Iter i = readers->begin(), end = readers->end(); i != end; ++i) {
			SimpleName *name = i.k();

			// Load ourselves last.
			if (*name->parent() == *me) {
				delayName = name;
				delayFiles = i.v();
				continue;
			}

			PkgReader *reader = createReader(i.k(), i.v()->files, this);
			if (reader) {
				r->push(reader);
			} else {
				WARNING(noReaderWarning(i.k(), i.v()->files)->c_str());
			}
		}

		if (delayName && delayFiles) {
			PkgReader *reader = createReader(delayName, delayFiles->files, this);
			if (reader)
				r->push(reader);
		}

		return r;
	}


	/**
	 * Documentation
	 */

	PackageDoc::PackageDoc(Package *owner) : pkg(owner) {}

	Doc *PackageDoc::get() {
		Doc *result = doc(pkg);

		if (Url *url = pkg->url()) {
			Url *file = url->push(new (this) Str(S("README")));
			if (file->exists())
				result->body = readAllText(file);
		}

		return result;
	}


	/**
	 * Helpers for Storm.
	 */

	MAYBE(Package *) package(Url *path) {
		return path->engine().package(path);
	}

	Package *rootPkg(EnginePtr e) {
		return e.v.package();
	}

}
