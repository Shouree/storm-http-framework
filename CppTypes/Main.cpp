#include "stdafx.h"
#include "Config.h"
#include "SrcPos.h"
#include "World.h"
#include "Parse.h"
#include "Write.h"
#include "Output.h"
#include "Utils/StackInfoSet.h"

void findHeaders(const Path &in, vector<Path> &out) {
	vector<Path> c = in.children();
	for (nat i = 0; i < c.size(); i++) {
		if (c[i].isDir()) {
			findHeaders(c[i], out);
		} else if (c[i].hasExt(L"h")) {
			out.push_back(c[i]);
		}
	}
}

// Find headers to process.
vector<Path> findHeaders(const vector<Path> &in) {
	vector<Path> result;
	for (nat i = 0; i < in.size(); i++)
		findHeaders(in[i], result);
	return result;
}

void findLicenses(const Path &in, vector<Path> &out) {
	vector<Path> c = in.children();
	for (nat i = 0; i < c.size(); i++) {
		if (c[i].isDir()) {
			findLicenses(c[i], out);
		} else if (c[i].hasExt(L"license")) {
			out.push_back(c[i]);
		}
	}
}

// Find license files.
vector<Path> findLicenses(const vector<Path> &in) {
	vector<Path> result;
	for (nat i = 0; i < in.size(); i++)
		findLicenses(in[i], result);
	return result;
}

void findVersions(const Path &in, vector<Path> &out) {
	vector<Path> c = in.children();
	for (nat i = 0; i < c.size(); i++) {
		if (c[i].isDir()) {
			findVersions(c[i], out);
		} else if (c[i].hasExt(L"version")) {
			out.push_back(c[i]);
		}
	}
}

// Find version files.
vector<Path> findVersions(const vector<Path> &in) {
	vector<Path> result;
	for (nat i = 0; i < in.size(); i++)
		findVersions(in[i], result);
	return result;
}

// Find the latest last-modified time for any files.
Timestamp lastModified(const vector<Path> &files) {
	Timestamp last = files[0].mTime();
	for (nat i = 1; i < files.size(); i++)
		last = max(last, files[i].mTime());
	return last;
}

// Update a timestamp if 'file' exists.
Timestamp max(const Timestamp &t, const Path &file) {
	if (file.exists())
		return max(t, file.mTime());
	else
		return t;
}

// Do we need to update the file?
bool oldFile(const Timestamp &input, const Path &file) {
	if (file.isEmpty())
		return false;

	if (!file.exists())
		return true;

	return file.mTime() < input;
}

template <class T>
vector<T> &operator +=(vector<T> &to, const vector<T> &from) {
	to.reserve(to.size() + from.size());
	to.insert(to.end(), from.begin(), from.end());
	return to;
}

int _tmain(int argc, const wchar_t *argv[]) {
	initDebug();

	if (!parse(argc, argv)) {
		usage(argv[0]);
		return 1;
	}

	Timestamp start;

	// Put the files in our global, so we can use SrcPos later on.
	SrcPos::files = findHeaders(config.useDirs);
	SrcPos::firstExport = nat(SrcPos::files.size());
	SrcPos::files += findHeaders(config.dirs);
	if (SrcPos::files.empty()) {
		PLN("No header files found.");
		return 1;
	}

	// Find all license and version files.
	vector<Path> licenses = findLicenses(config.dirs);
	vector<Path> versions = findVersions(config.dirs);

	Timestamp modified = lastModified(SrcPos::files);
	if (!licenses.empty())
		modified = max(modified, lastModified(licenses));
	if (!versions.empty())
		modified = max(modified, lastModified(versions));
	modified = max(modified, config.cppSrc);
	if (config.genAsm)
		modified = max(modified, config.asmSrc);

	{
		Path me = Path::executableFile();
		if (me.exists())
			modified = max(modified, me.mTime());
	}

	bool update = oldFile(modified, config.cppOut);
	if (config.genAsm)
		update |= oldFile(modified, config.asmOut);
	if (!config.docOut.isEmpty())
		update |= oldFile(modified, config.docOut);

	if (update) {
		try {
			World world;
			parseWorld(world, licenses, versions);

			// TODO: More!
			world.usingDecl.push_back(CppName(L"storm"));
			for (nat i = 0; i < config.usingDecl.size(); i++)
				world.usingDecl.push_back(CppName(config.usingDecl[i]));

			world.prepare();
			generateFile(config.cppSrc, config.cppOut, genMap(), world);

			if (config.genAsm)
				generateFile(config.asmSrc, config.asmOut, asmMap(), world);
			if (!config.docOut.isEmpty())
				generateDoc(config.docOut, world);

		} catch (const Exception &e) {
			PLN(e);
			// PLN(e.what());
			// PVAR(e);
			PLN("FAILED");
			return 1;
		}
	}

	Timestamp end;
	PLN(L"Total time: " << (end - start));

	return 0;
}

DEFINE_STACK_INFO()
