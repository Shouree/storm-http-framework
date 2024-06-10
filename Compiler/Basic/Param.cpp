#include "stdafx.h"
#include "Param.h"
#include "Type.h"

namespace storm {
	namespace bs {

		NameParam::NameParam(SrcName *type, Str *name) : type(type), name(name) {}

		NameParam::NameParam(SrcName *type, syntax::SStr *name) : type(type), name(name->v) {}

		wostream &operator <<(wostream &to, NameParam p) {
			StrBuf *b = new (p.name) StrBuf();
			*b << p;
			return to << b->toS();
		}

		void NameParam::toS(StrBuf *to) const {
			*to << type << S(" ") << name;
		}

		NameParam nameless(SrcName *type) {
			return NameParam(type, new (type) Str(S("")));
		}

		ValParam::ValParam(Value type, Str *name)
			: name(name), t(type.type), ref(type.ref), thisPar(false) {}

		ValParam::ValParam(Value type, syntax::SStr *name)
			: name(name->v), t(type.type), ref(type.ref), thisPar(false) {}

		ValParam::ValParam(Value type, Str *name, Bool thisParam)
			: name(name), t(type.type), ref(type.ref), thisPar(thisParam) {}

		wostream &operator <<(wostream &to, ValParam p) {
			StrBuf *b = new (p.name) StrBuf();
			*b << p;
			return to << b->toS();
		}

		void ValParam::toS(StrBuf *to) const {
			*to << type() << S(" ") << name;
			if (thisParam())
				*to << S(" (this param)");
		}

		ValParam thisParam(Type *me) {
			return thisParam(new (me) Str(S("this")), me);
		}

		ValParam thisParam(Str *name, Type *me) {
			return ValParam(thisPtr(me), name, true);
		}

		ValParam resolve(NameParam param, Scope scope) {
			return ValParam(scope.value(param.type), param.name);
		}

		Array<ValParam> *resolve(Array<NameParam> *params, Scope scope) {
			Array<ValParam> *res = new (params) Array<ValParam>();
			res->reserve(params->count());
			for (nat i = 0; i < params->count(); i++)
				res->push(resolve(params->at(i), scope));
			return res;
		}

		Array<ValParam> *resolve(Array<NameParam> *params, Type *me, Scope scope) {
			Array<ValParam> *res = new (params) Array<ValParam>();
			res->reserve(params->count() + 1);
			res->push(thisParam(me));
			for (nat i = 0; i < params->count(); i++)
				res->push(resolve(params->at(i), scope));
			return res;
		}

		Array<Value> *values(Array<ValParam> *params) {
			Array<Value> *r = new (params) Array<Value>();
			r->reserve(params->count());
			for (nat i = 0; i < params->count(); i++)
				r->push(params->at(i).type());
			return r;
		}

		Array<ValParam> *merge(Array<Value> *val, Array<Str *> *names) {
			if (val->count() != names->count()) {
				WARNING(L"Non-equal size of values and parameters, ignoring some entries!");
			}
			Nat to = min(val->count(), names->count());
			Array<ValParam> *r = new (val) Array<ValParam>();
			for (nat i = 0; i < to; i++) {
				r->push(ValParam(val->at(i), names->at(i)));
			}
			return r;
		}

	}
}
