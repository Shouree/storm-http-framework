#include "stdafx.h"
#include "Row.h"
#include "Exception.h"

namespace sql {

	const GcType Row::elemType = {
		GcType::tArray,
		null,
		null,
		sizeof(Row::Element),
		1,
		{ OFFSET_OF(Row::Element, object) },
	};

	Row::Row() : data(null) {}

	Row::Row(Builder b) : data(b.data) {
		assert(data->filled == data->count, L"Data array was not properly filled before creating a row!");
	}

	void Row::deepCopy(CloneEnv *env) {
		GcArray<Element> *copy = runtime::allocArray<Element>(env->engine(), &elemType, data->count);
		memcpy(&copy->v[0], &data->v[0], sizeof(Element) * data->count);
		data = copy;
	}

	const Row::Element &Row::checkIndex(Nat index) const {
		if (!data || index >= data->count)
			throw new (runtime::someEngine()) ArrayError(index, data ? Nat(data->count) : 0);
		return data->v[index];
	}

	void Row::throwTypeError(const wchar *expected, const Element &found) const {
		Engine &e = runtime::someEngine();
		StrBuf *msg = new (e) StrBuf();
		*msg << S("Expected type of column: ") << expected
			 << S(", but column has type: ");

		switch (size_t(found.object)) {
		case COL_NULL:
			*msg << S("NULL");
			break;
		case COL_LONG:
			*msg << S("Long");
			break;
		case COL_DOUBLE:
			*msg << S("Double");
			break;
		default:
			if (size_t(found.object) < COL_LAST) {
				*msg << S("<unknown>");
			} else {
				*msg << runtime::typeName(runtime::typeOf((RootObject *)found.object));
			}
			break;
		}

		throw new (e) SQLError(msg->toS());
	}

	Str *Row::getStr(Nat index) const {
		const Element &e = checkIndex(index);
		if (size_t(e.object) < COL_LAST)
			throwTypeError(S("Str"), e);

		Object *o = (Object *)e.object;
		if (Str *s = as<Str>(o))
			return s;

		throwTypeError(S("Str"), e);
		return null;
	}

	Bool Row::getBool(Nat index) const {
		return getLong(index) != 0;
	}

	Int Row::getInt(Nat index) const {
		return Int(getLong(index));
	}

	Long Row::getLong(Nat index) const {
		const Element &e = checkIndex(index);
		if (size_t(e.object) != COL_LONG)
			throwTypeError(S("Long"), e);

		return e.l;
	}

	Float Row::getFloat(Nat index) const {
		return Float(getDouble(index));
	}

	Double Row::getDouble(Nat index) const {
		const Element &e = checkIndex(index);
		if (size_t(e.object) != COL_DOUBLE)
			throwTypeError(S("Double"), e);

		return e.d;
	}

	Bool Row::isNull(Nat index) const {
		const Element &e = checkIndex(index);
		return e.object == null;
	}

	Variant at(EnginePtr engine, const Row &row, Nat index) {
		const Row::Element &e = row.checkIndex(index);
		switch (size_t(e.object)) {
		case Row::COL_NULL:
			return Variant();
		case Row::COL_LONG:
			return Variant(e.l, engine.v);
		case Row::COL_DOUBLE:
			return Variant(e.d, engine.v);
		default:
			return Variant((RootObject *)e.object);
		}
	}


	Row::Builder::Builder(GcArray<Element> *elems) : data(elems) {
		elems->filled = 0;
	}

	Row::Builder Row::builder(EnginePtr e, Nat columns) {
		return Builder(runtime::allocArray<Row::Element>(e.v, &elemType, columns));
	}

	Row::Element &Row::Builder::next() {
		if (data->filled >= data->count)
			throw new (runtime::someEngine()) ArrayError(Nat(data->filled), Nat(data->count));

		return data->v[data->filled++];
	}

	void Row::Builder::push(Str *v) {
		Element &e = next();
		e.object = v;
	}

	void Row::Builder::push(Long l) {
		Element &e = next();
		e.object = (void *)COL_LONG;
		e.l = l;
	}

	void Row::Builder::push(Double d) {
		Element &e = next();
		e.object = (void *)COL_DOUBLE;
		e.d = d;
	}

	void Row::Builder::pushNull() {
		Element &e = next();
		e.object = null;
	}

	StrBuf *operator <<(StrBuf *to, const Row &row) {
		*to << S("row: ");
		for (Nat i = 0; i < row.count(); i++) {
			if (i > 0)
				*to << S(", ");
			// Note: slower than necessary, but OK for debugging.
			*to << at(to->engine(), row, i);
		}
		return to;
	}

}
