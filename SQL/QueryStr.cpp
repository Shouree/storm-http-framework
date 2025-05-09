#include "stdafx.h"
#include "QueryStr.h"

namespace sql {

	static const Nat tagValue = 0x80000000;
	static const Nat textValue = tagValue - 1;
	static const Nat nameValue = textValue - 1;
	static const Nat placeholderValue = nameValue - 1;
	static const Nat autoIncrementValue = placeholderValue - 1;

	static Bool hasTag(Nat val) {
		return (val & tagValue) != 0;
	}


	QueryStr::QueryStr(GcArray<Nat> *ops, GcArray<Str *> *text)
		: opData(ops), textData(text) {}

	QueryStr::Visitor::Visitor() {}

	void QueryStr::Visitor::put(StrBuf *to, Str *text) {
		*to << text;
	}

	void QueryStr::Visitor::name(StrBuf *to, Str *name) {
		*to << S("\"") << name << S("\"");
	}

	void QueryStr::Visitor::placeholder(StrBuf *to) {
		*to << S("?");
	}

	void QueryStr::Visitor::autoIncrement(StrBuf *to) {
		*to << S("AUTOINCREMENT");
	}

	void QueryStr::Visitor::type(StrBuf *to, QueryType type) {
		*to << type;
	}

	Str *QueryStr::generate(Visitor *v) const {
		StrBuf *out = new (this) StrBuf();

		size_t textPos = 0;
		for (size_t i = 0; i < opData->count; i++) {
			if (opData->v[i] == textValue) {
				v->put(out, textData->v[textPos++]);
			} else if (opData->v[i] == nameValue) {
				v->name(out, textData->v[textPos++]);
			} else if (opData->v[i] == placeholderValue) {
				v->placeholder(out);
			} else if (opData->v[i] == autoIncrementValue) {
				v->autoIncrement(out);
			} else if (hasTag(opData->v[i])) {
				Nat type = opData->v[i++] & ~tagValue;
				v->type(out, QueryType(type, opData->v[i]));
			} else {
				v->type(out, QueryType(opData->v[i]));
			}
		}

		return out->toS();
	}

	void QueryStr::toS(StrBuf *to) const {
		*to << generate(new (this) Visitor());
	}


	/**
	 * The builder.
	 */

	QueryStrBuilder::QueryStrBuilder() {
		currentStr = new (this) StrBuf();
		ops = new (this) Array<Nat>();
		strings = new (this) Array<Str *>();
	}

	void QueryStrBuilder::deepCopy(CloneEnv *env) {
		cloned(currentStr, env);
		cloned(ops, env);
		cloned(strings, env);
	}

	void QueryStrBuilder::put(Str *str) {
		*currentStr << str;
	}

	void QueryStrBuilder::flush() {
		*strings << currentStr->toS();
		*ops << textValue;
		currentStr->clear();
	}

	void QueryStrBuilder::name(Str *str) {
		flush();
		*strings << str;
		*ops << nameValue;
	}

	void QueryStrBuilder::placeholder() {
		flush();
		*ops << placeholderValue;
	}

	void QueryStrBuilder::autoIncrement() {
		flush();
		*ops << autoIncrementValue;
	}

	void QueryStrBuilder::type(QueryType type) {
		flush();
		if (type.typeSize == 0) {
			*ops << type.type;
		} else {
			*ops << (type.type | tagValue);
			*ops << type.typeSize;
		}
	}

	QueryStr *QueryStrBuilder::build() {
		flush();

		GcArray<Nat> *o = runtime::allocArray<Nat>(engine(), &natArrayType, ops->count());
		for (Nat i = 0; i < ops->count(); i++)
			o->v[i] = ops->at(i);

		GcArray<Str *> *s = runtime::allocArray<Str *>(engine(), &pointerArrayType, strings->count());
		for (Nat i = 0; i < strings->count(); i++)
			s->v[i] = strings->at(i);

		return new (this) QueryStr(o, s);
	}

	void QueryStrBuilder::clear() {
		currentStr->clear();
		ops->clear();
		strings->clear();
	}

}
