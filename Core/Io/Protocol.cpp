#include "stdafx.h"
#include "Protocol.h"
#include "Url.h"
#include "Core/Convert.h"
#include "Str.h"
#include "StrBuf.h"
#include "FileStream.h"
#include "MemStream.h"
#include "Serialization.h"
#include "SerializationUtils.h"

#ifdef WINDOWS
#include <Shlwapi.h>
#endif

namespace storm {

	ProtocolNotSupported::ProtocolNotSupported(const wchar *operation, const wchar *protocol)
		: operation(new (engine()) Str(operation)), protocol(new (engine()) Str(protocol)) {
		saveTrace();
	}

	ProtocolNotSupported::ProtocolNotSupported(const wchar *operation, Str *protocol)
		: operation(new (engine()) Str(operation)), protocol(protocol) {
		saveTrace();
	}

	ProtocolNotSupported::ProtocolNotSupported(Str *operation, Str *protocol)
		: operation(operation), protocol(protocol) {
		saveTrace();
	}

	void ProtocolNotSupported::message(StrBuf *to) const {
		*to << operation << S(" is not supported by the protocol ") << protocol;
	}


	Protocol::Protocol() {}

	Bool Protocol::partEq(Str *a, Str *b) {
		return *a == *b;
	}

	Nat Protocol::partHash(Str *part) {
		return part->hash();
	}

	Array<Url *> *Protocol::children(Url *url) {
		throw new (this) ProtocolNotSupported(S("children"), toS());
	}

	IStream *Protocol::read(Url *url) {
		throw new (this) ProtocolNotSupported(S("read"), toS());
	}

	OStream *Protocol::write(Url *url) {
		throw new (this) ProtocolNotSupported(S("write"), toS());
	}

	StatType Protocol::stat(Url *url) {
		throw new (this) ProtocolNotSupported(S("stat"), toS());
	}

	Bool Protocol::createDir(Url *url) {
		throw new (this) ProtocolNotSupported(S("createDir"), toS());
	}

	Bool Protocol::unlink(Url *url) {
		throw new (this) ProtocolNotSupported(S("delete"), toS());
	}

	Str *Protocol::format(Url *url) {
		throw new (this) ProtocolNotSupported(S("format"), toS());
	}

	void Protocol::toS(StrBuf *to) const {
		*to << S("<unknown protocol>");
	}

	Bool Protocol::operator ==(const Protocol &o) const {
		return runtime::typeOf(this) == runtime::typeOf(&o);
	}

	SerializedType *Protocol::serializedType(EnginePtr e) {
		SerializedStdType *type = serializedStdType<Protocol>(e.v);
		// No data.
		return type;
	}

	Protocol *Protocol::read(ObjIStream *from) {
		return (Protocol *)from->readClass(StormInfo<Protocol>::type(from->engine()));
	}

	void Protocol::write(ObjOStream *to) {
		if (to->startClass(StormInfo<Protocol>::type(engine()), this)) {
			to->end();
		}
	}

	Protocol::Protocol(ObjIStream *from) {
		from->end();
	}


	/**
	 * Protocol for relative paths.
	 */

	RelativeProtocol::RelativeProtocol() {}

	Bool RelativeProtocol::partEq(Str *a, Str *b) {
		return *a == *b;
	}

	Nat RelativeProtocol::partHash(Str *part) {
		return part->hash();
	}

	void RelativeProtocol::toS(StrBuf *to) const {
		*to << S("./");
	}

	Str *RelativeProtocol::format(Url *url) {
		StrBuf *to = new (this) StrBuf();

		Array<Str *> *parts = url->getParts();
		if (parts->count() == 0)
			return to->toS();

		*to << parts->at(0);
		for (Nat i = 1; i < parts->count(); i++)
			*to << S("/") << parts->at(i);

		if (url->dir())
			*to << L"/";

		return to->toS();
	}

	SerializedType *RelativeProtocol::serializedType(EnginePtr e) {
		SerializedStdType *type = serializedStdType<RelativeProtocol, Protocol>(e.v);
		// No data.
		return type;
	}

	RelativeProtocol *RelativeProtocol::read(ObjIStream *from) {
		return (RelativeProtocol *)from->readClass(StormInfo<RelativeProtocol>::type(from->engine()));
	}

	void RelativeProtocol::write(ObjOStream *to) {
		if (to->startClass(StormInfo<RelativeProtocol>::type(engine()), this)) {
			Protocol::write(to);
			to->end();
		}
	}

	RelativeProtocol::RelativeProtocol(ObjIStream *from) : Protocol(from) {
		from->end();
	}

	/**
	 * File protocol.
	 */

	FileProtocol::FileProtocol() {}

	void FileProtocol::toS(StrBuf *to) const {
#ifndef WINDOWS
		*to << L"/";
#endif
	}

	SerializedType *FileProtocol::serializedType(EnginePtr e) {
		SerializedStdType *type = serializedStdType<FileProtocol, Protocol>(e.v);
		// No data.
		return type;
	}

	FileProtocol *FileProtocol::read(ObjIStream *from) {
		return (FileProtocol *)from->readClass(StormInfo<FileProtocol>::type(from->engine()));
	}

	void FileProtocol::write(ObjOStream *to) {
		if (to->startClass(StormInfo<FileProtocol>::type(engine()), this)) {
			Protocol::write(to);
			to->end();
		}
	}

	FileProtocol::FileProtocol(ObjIStream *from) : Protocol(from) {
		from->end();
	}

#ifdef WINDOWS

	Bool FileProtocol::partEq(Str *a, Str *b) {
		return _wcsicmp(a->c_str(), b->c_str()) == 0;
	}

	Nat FileProtocol::partHash(Str *a) {
		const wchar *str = a->c_str();

		Nat r = 5381;
		for (const wchar *at = str; *at; at++)
			r = ((r << 5) + r) + towlower(*at);

		return r;
	}

	Str *FileProtocol::format(Url *url) {
		StrBuf *to = new (this) StrBuf();

		Array<Str *> *parts = url->getParts();
		if (parts->count() == 0)
			return to->toS();

		Str *first = parts->at(0);
		if (wcslen(first->c_str()) == 2 && first->c_str()[1] == ':') {
			*to << first;
		} else {
			*to << L"\\\\" << first;
		}

		for (nat i = 1; i < parts->count(); i++) {
			*to << L"\\" << parts->at(i);
		}

		return to->toS();
	}

	Array<Url *> *FileProtocol::children(Url *url) {
		Array<Url *> *result = new (this) Array<Url *>();
		Str *search = *format(url) + new (url) Str(L"\\*");

		WIN32_FIND_DATA data;
		HANDLE h = FindFirstFile(search->c_str(), &data);
		if (h == INVALID_HANDLE_VALUE)
			return result;

		do {
			const wchar *name = data.cFileName;
			if (wcscmp(name, L".") == 0 || wcscmp(name, L"..") == 0)
				continue;

			Str *sName = new (this) Str(name);
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				result->push(url->pushDir(sName));
			} else {
				result->push(url->push(sName));
			}
		} while (FindNextFile(h, &data));

		FindClose(h);

		return result;
	}

	IStream *FileProtocol::read(Url *url) {
		return new (url) FileIStream(format(url));
	}

	OStream *FileProtocol::write(Url *url) {
		return new (url) FileOStream(format(url));
	}

	StatType FileProtocol::stat(Url *url) {
		DWORD result = GetFileAttributes(format(url)->c_str());
		if (result == INVALID_FILE_ATTRIBUTES)
			return sNotFound;

		if (result & FILE_ATTRIBUTE_DIRECTORY)
			return sDirectory;
		else
			return sFile;
	}

	Bool FileProtocol::createDir(Url *url) {
		return CreateDirectory(format(url)->c_str(), NULL) == TRUE;
	}

	Bool FileProtocol::unlink(Url *url) {
		Str *file = format(url);
		DWORD attrs = GetFileAttributes(file->c_str());
		if (attrs == INVALID_FILE_ATTRIBUTES)
			return false;

		if (attrs & FILE_ATTRIBUTE_DIRECTORY)
			return RemoveDirectory(file->c_str()) != FALSE;
		else
			return DeleteFile(file->c_str()) != FALSE;
	}

#elif defined(POSIX)

	Bool FileProtocol::partEq(Str *a, Str *b) {
		return *a == *b;
	}

	Nat FileProtocol::partHash(Str *a) {
		return a->hash();
	}

	Str *FileProtocol::format(Url *url) {
		StrBuf *to = new (this) StrBuf();

		Array<Str *> *parts = url->getParts();
		if (parts->count() == 0)
			return to->toS();

		for (Nat i = 0; i < parts->count(); i++) {
			*to << S("/") << parts->at(i);
		}

		// if (url->dir)
		// 	*to << L"/";

		return to->toS();
	}

	Array<Url *> *FileProtocol::children(Url *url) {
		Array<Url *> *result = new (url) Array<Url *>();

		DIR *h = opendir(format(url)->utf8_str());
		if (!h)
			return result;

		dirent *d;
		while ((d = readdir(h)) != null) {
			if (strcmp(d->d_name, "..") == 0 || strcmp(d->d_name, ".") == 0)
				continue;

			Str *name = new (this) Str(toWChar(engine(), d->d_name)->v);
			if (d->d_type == DT_DIR) {
				result->push(url->pushDir(name));
				// TODO: Handle 'DT_UNKNOWN' properly...
			} else {
				result->push(url->push(name));
			}
		}

		closedir(h);

		return result;
	}

	IStream *FileProtocol::read(Url *url) {
		return new (url) FileIStream(format(url));
	}

	OStream *FileProtocol::write(Url *url) {
		return new (url) FileOStream(format(url));
	}

	StatType FileProtocol::stat(Url *url) {
		struct stat s;
		if (::stat(format(url)->utf8_str(), &s) != 0)
			return sNotFound;

		if (S_ISDIR(s.st_mode))
			return sDirectory;
		else
			return sFile;
	}

	Bool FileProtocol::createDir(Url *url) {
		return mkdir(format(url)->utf8_str(), 0777) == 0;
	}

	Bool FileProtocol::unlink(Url *url) {
		const char *name = format(url)->utf8_str();

		struct stat s;
		if (::stat(name, &s) != 0)
			return false;

		if (S_ISDIR(s.st_mode))
			return ::rmdir(name) == 0;
		else
			return ::unlink(name) == 0;
	}

#else
#error "Please implement FileProtocol for your OS!"
#endif


	/**
	 * HTTP/HTTPS
	 */

	HttpProtocol::HttpProtocol(Bool secure) : secure(secure) {}

	Bool HttpProtocol::partEq(Str *a, Str *b) {
		return *a == *b;
	}

	Nat HttpProtocol::partHash(Str *a) {
		return a->hash();
	}

	void HttpProtocol::toS(StrBuf *to) const {
		if (secure)
			*to << S("https://");
		else
			*to << S("http://");
	}

	Bool HttpProtocol::operator ==(const Protocol &o) const {
		if (!Protocol::operator ==(o))
			return false;

		const HttpProtocol &other = (const HttpProtocol &)o;
		return secure == other.secure;
	}

	SerializedType *HttpProtocol::serializedType(EnginePtr e) {
		SerializedStdType *type = serializedStdType<HttpProtocol, Protocol>(e.v);
		type->add(S("secure"), StormInfo<Bool>::type(e.v));
		return type;
	}

	HttpProtocol *HttpProtocol::read(ObjIStream *from) {
		return (HttpProtocol *)from->readClass(StormInfo<HttpProtocol>::type(from->engine()));
	}

	void HttpProtocol::write(ObjOStream *to) {
		if (to->startClass(StormInfo<HttpProtocol>::type(engine()), this)) {
			Protocol::write(to);
			Serialize<Bool>::write(secure, to);
			to->end();
		}
	}

	HttpProtocol::HttpProtocol(ObjIStream *from) : Protocol(from) {
		secure = Serialize<Bool>::read(from);
		from->end();
	}


	MemoryProtocol::MemoryProtocol() {
		files = new (this) Map<Str *, Buffer>();
	}

	Bool MemoryProtocol::partEq(Str *a, Str *b) {
		return *a == *b;
	}

	Nat MemoryProtocol::partHash(Str *a) {
		return a->hash();
	}

	Array<Url *> *MemoryProtocol::children(Url *url) {
		Array<Url *> *result = new (this) Array<Url *>();

		if (url->empty()) {
			for (Map<Str *, Buffer>::Iter i = files->begin(), e = files->end(); i != e; ++i)
				*result << url->push(i.k());
		}

		return result;
	}

	IStream *MemoryProtocol::read(Url *url) {
		Buffer buffer;

		if (url->count() == 1) {
			Str *name = url->at(0);
			if (files->has(name))
				buffer = files->get(name);
		}

		return new (this) MemIStream(buffer);
	}

	OStream *MemoryProtocol::write(Url *url) {
		// We could support it, but then we need a more custom OStream implementation.
		throw new (this) ProtocolNotSupported(S("write"), toS());
	}

	StatType MemoryProtocol::stat(Url *url) {
		if (url->count() != 1)
			return sNotFound;

		if (files->has(url->at(0)))
			return sFile;

		return sNotFound;
	}

	void MemoryProtocol::toS(StrBuf *to) const {
		*to << S("<memory>:");
	}

	Url *MemoryProtocol::put(Str *name, Buffer content) {
		files->put(name, content);
		return new (this) Url(this, new (this) Array<Str *>(1, name));
	}

	SerializedType *MemoryProtocol::serializedType(EnginePtr e) {
		SerializedStdType *type = serializedStdType<MemoryProtocol, Protocol>(e.v);
		return type;
	}

	MemoryProtocol *MemoryProtocol::read(ObjIStream *from) {
		return (MemoryProtocol *)from->readClass(StormInfo<MemoryProtocol>::type(from->engine()));
	}

	void MemoryProtocol::write(ObjOStream *to) {
		if (to->startClass(StormInfo<MemoryProtocol>::type(engine()), this)) {
			Protocol::write(to);
			to->end();
		}
	}

	MemoryProtocol::MemoryProtocol(ObjIStream *from) {
		// Note: Serialization does not save files in the memory protocol, just like paths are not
		// transferred between different systems.
		from->end();
	}

}
