#include "stdafx.h"
#include "CppDoc.h"
#include "Engine.h"
#include "Package.h"

#include "Core/Io/Stream.h"
#include "Core/Io/MemStream.h"
#include "Core/Io/Text.h"

namespace storm {

	/**
	 * Read the documentation file.
	 */

	static Nat readNat(IStream *src) {
		GcPreArray<byte, sizeof(Nat)> b;
		Buffer buf = src->fill(emptyBuffer(b));

		if (!buf.full())
			return 0;

		Nat r = 0;
		memcpy(&r, buf.dataPtr(), sizeof(Nat));
		return r;
	}

	static MAYBE(Str *) readDoc(RIStream *src, Nat entry) {
		if (entry <= 0)
			return null;

		Nat first = readNat(src);

		// Too large index?
		if (entry * sizeof(Nat) >= first)
			return null;

		src->seek((entry - 1) * sizeof(Nat));
		Nat from = readNat(src);
		Nat to = readNat(src);

		// Otherwise corrupted data?
		if (from > to)
			return null;

		src->seek(from);

		return readText(new (src) MemIStream(src->fill(to - from)))->readAll();
	}

	static Str *readBody(Url *src, Nat entry) {
		if (!src->exists())
			throw new (src) DocError(TO_S(src, S("The file ") << src << S(" does not exist.")));

		IStream *in = src->read();
		Str *body = readDoc(in->randomAccess(), entry);
		in->close();

		if (!body) {
			Str *msg = TO_S(src, S("Unable to extract documentation entry ") << entry
							<< S(" from ") << src << S(". Is the file corrupted?"));
			throw new (src) DocError(msg);
		}

		return body;
	}

	static Array<DocParam> *createParams(Named *entity, const CppParam *params) {
		Array<Value> *src = entity->params;
		Array<DocParam> *result = new (entity) Array<DocParam>();
		result->reserve(src->count());

		if (params) {
			for (Nat i = 0; i < src->count(); i++, params++) {
				if (!params->name) {
					Str *msg = TO_S(entity, S("Number of parameters for ") << entity->identifier()
									<< S(" does not match."));
					throw new (entity) DocError(msg);
				}

				result->push(DocParam(new (src) Str(params->name), src->at(i)));
			}
		} else {
			Str *empty = new (src) Str(S(""));
			for (Nat i = 0; i < src->count(); i++)
				result->push(DocParam(empty, src->at(i)));
		}

		return result;
	}


	/**
	 * CppDoc.
	 */

	CppDoc::CppDoc(Named *entity, Url *file, Nat entry, const CppParam *params) :
		entity(entity), data(file), entryInfo(entry << 1), params(params) {}

	CppDoc::CppDoc(Named *entity, const wchar *name, Nat entry, const CppParam *params) :
		entity(entity), data(name), entryInfo((entry << 1) | 0x1), params(params) {}

	Url *CppDoc::file() {
		if (entryInfo & 0x1) {
			const wchar *name = (const wchar *)data;

			Package *root = engine().package();
			return root->url()->push(new (root) Str(name));
		} else {
			return (Url *)data;
		}
	}

	Nat CppDoc::entry() {
		return entryInfo >> 1;
	}

	Doc *CppDoc::get() {
		Str *body = readBody(file(), entry());
		Array<DocParam> *params = createParams(entity, this->params);

		return doc(entity, params, body);
	}

}
