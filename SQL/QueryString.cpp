#include "stdafx.h"
#include "QueryString.h"

namespace sql {

	static const Nat tagValue = 0x80000000;
	static const Nat textValue = tagValue - 1;
	static const Nat nameValue = textValue - 1;
	static const Nat placeholderValue = nameValue - 1;

	static Bool hasTag(Nat val) {
		return (val & tagValue) != 0;
	}


	QueryString::QueryString(GcArray<Nat> *ops, GcArray<Str *> *text)
		: opData(ops), textData(text) {}

	QueryString::Visitor::Visitor() {}

	void QueryString::Visitor::put(StrBuf *to, Str *text) {
		*to << text;
	}

	void QueryString::Visitor::name(StrBuf *to, Str *name) {
		*to << S("'") << text << S("'");
	}

	void QueryString::Visitor::placeholder(StrBuf *to) {
		*to << S("?");
	}

	void QueryString::Visitor::type(StrBuf *to, Type type, Nat size) {
		switch (type) {
		case text:
			*to << S("TEXT");
			break;
		case integer:
			*to << S("INTEGER");
			break;
		case real:
			*to << S("FLOAT");
			break;
		default:
			*to << S("UNKNOWN ") << type;
			break;
		}
		if (size != Nat(-1))
			*to << S("(") << size << S(")");
	}

	Str *QueryString::generate(Visitor *v) const {
		StrBuf *out = new (this) StrBuf();

		size_t textPos = 0;
		for (size_t i = 0; i < opData->count; i++) {
			if (opData->v[i] == textValue) {
				v->put(out, textData->v[textPos++]);
			} else if (opData->v[i] == nameValue) {
				v->name(out, textData->v[textPos++]);
			} else if (opData->v[i] == placeholderValue) {
				v->placeholder(out);
			} else if (hasTag(opData->v[i])) {
				Nat type = opData->v[i++] & ~tagValue;
				v->type(out, Type(type), opData->v[i]);
			} else {
				v->type(out, Type(opData->v[i]), Nat(-1));
			}
		}

		return out->toS();
	}

	void QueryString::toS(StrBuf *to) const {
		*to << generate(new (this) Visitor());
	}


	/**
	 * The builder.
	 */

	QueryStringBuilder::QueryStringBuilder() {
		currentStr = new (this) StrBuf();
		ops = new (this) Array<Nat>();
		strings = new (this) Array<Str *>();
	}

	void QueryStringBuilder::deepCopy(CloneEnv *env) {
		cloned(currentStr, env);
		cloned(ops, env);
		cloned(strings, env);
	}

	void QueryStringBuilder::put(Str *str) {
		*currentStr << str;
	}

	void QueryStringBuilder::flush() {
		*strings << currentStr->toS();
		*ops << textValue;
		currentStr->clear();
	}

	void QueryStringBuilder::name(Str *str) {
		flush();
		*strings << str;
		*ops << nameValue;
	}

	void QueryStringBuilder::placeholder() {
		flush();
		*ops << placeholderValue;
	}

	void QueryStringBuilder::type(QueryString::Type type) {
		flush();
		*ops << Nat(type);
	}

	void QueryStringBuilder::type(QueryString::Type type, Nat size) {
		flush();
		*ops << (Nat(type) | tagValue);
		*ops << size;
	}

	QueryString *QueryStringBuilder::build() {
		flush();

		GcArray<Nat> *o = runtime::allocArray<Nat>(engine(), &natArrayType, ops->count());
		for (Nat i = 0; i < ops->count(); i++)
			o->v[i] = ops->at(i);

		GcArray<Str *> *s = runtime::allocArray<Str *>(engine(), &pointerArrayType, strings->count());
		for (Nat i = 0; i < strings->count(); i++)
			s->v[i] = strings->at(i);

		return new (this) QueryString(o, s);
	}

}
