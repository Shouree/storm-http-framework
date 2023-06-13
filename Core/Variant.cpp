#include "stdafx.h"
#include "Variant.h"
#include "Runtime.h"
#include "Handle.h"
#include "StrBuf.h"

namespace storm {

	Variant::Variant() : data(null) {}

	Variant::Variant(const Variant &o) : data(null) {
		if (!o.data)
			return;

		const GcType *t = runtime::gcTypeOf(o.data);
		if (t->kind == GcType::tArray) {
			// Value.
			GcArray<Byte> *alloc = (GcArray<Byte> *)o.data;
			init(alloc->v, t->type);
		} else {
			// Class.
			data = o.data;
		}
	}

	Variant &Variant::operator =(const Variant &o) {
		Variant t(o);
		std::swap(t.data, data);
		return *this;
	}

	Variant::Variant(RootObject *o) : data(o) {}

	Variant::Variant(Object *o) : data(o) {}

	Variant::Variant(TObject *o) : data(o) {}

	Variant::Variant(const void *value, Type *type) {
		init(value, type);
	}

	void Variant::init(const void *value, Type *type) {
		const Handle &h = runtime::typeHandle(type);
		data = runtime::allocArray(h.engine(), h.gcArrayType, 1);

		GcArray<Byte> *alloc = (GcArray<Byte> *)data;
		h.safeCopy(alloc->v, value);
		alloc->filled = 1;
	}

	Variant::~Variant() {
		if (!data)
			return;

		const GcType *t = runtime::gcTypeOf(data);
		if (t->kind != GcType::tArray)
			return;

		GcArray<Byte> *alloc = (GcArray<Byte> *)data;
		if (!alloc->filled)
			return; // Not initialized.

		const Handle &h = runtime::typeHandle(t->type);
		h.safeDestroy(alloc->v);
		alloc->filled = 0;
	}

	void Variant::deepCopy(CloneEnv *env) {
		if (!data)
			return;

		const GcType *t = runtime::gcTypeOf(data);
		if (t->kind == GcType::tArray) {
			// Value.
			const Handle &h = runtime::typeHandle(t->type);
			if (!h.deepCopyFn)
				return;

			GcArray<Byte> *alloc = (GcArray<Byte> *)data;
			(*h.deepCopyFn)(alloc->v, env);
		} else {
			// Class.
			data = runtime::cloneObjectEnv((RootObject *)data, env);
		}
	}

	Bool Variant::empty() const {
		if (!data)
			return true;

		const GcType *t = runtime::gcTypeOf(data);
		if (t->kind == GcType::tArray) {
			// We have data, but is it initialized?
			GcArray<Byte> *alloc = (GcArray<Byte> *)data;
			return alloc->filled == 0;
		} else {
			return false;
		}
	}

	Bool Variant::has(Type *type) const {
		if (!data)
			return false;

		const GcType *t = runtime::gcTypeOf(data);
		return runtime::isA(type, t->type);
	}

	MAYBE(Type *) Variant::type() const {
		if (!data)
			return null;

		const GcType *t = runtime::gcTypeOf(data);
		return t->type;
	}

	void *Variant::getValue() const {
		GcArray<Byte> *alloc = (GcArray<Byte> *)data;
		return alloc->v;
	}

	Variant Variant::uninitializedValue(Type *type) {
		assert(runtime::isValue(type));

		Variant v;

		const Handle &h = runtime::typeHandle(type);
		v.data = runtime::allocArray(h.engine(), h.gcArrayType, 1);

		return v;
	}

	void Variant::valueInitialized() {
		GcArray<Byte> *alloc = (GcArray<Byte> *)data;
		alloc->filled = 1;
	}

	void Variant::valueRemoved() {
		GcArray<Byte> *alloc = (GcArray<Byte> *)data;
		alloc->filled = 0;
	}

	void Variant::moveValue(void *to) {
		const GcType *t = runtime::gcTypeOf(data);
		GcArray<Byte> *alloc = (GcArray<Byte> *)data;
		memcpy(to, alloc->v, t->stride);
		memset(alloc->v, 0, t->stride);
		valueRemoved();
	}

	RootObject *Variant::getObject() const {
		return (RootObject *)data;
	}

	void *Variant::getPointer() {
		if (!data)
			return null;

		const GcType *t = runtime::gcTypeOf(data);
		if (t->kind == GcType::tArray) {
			return getValue();
		} else {
			return &data;
		}
	}

	Engine &Variant::engine() const {
		const GcType *t = runtime::gcTypeOf(data);
		return runtime::allocEngine((RootObject *)t->type);
	}

	StrBuf &operator <<(StrBuf &to, const Variant &v) {
		if (v.empty())
			return to << S("<empty>");

		const GcType *t = runtime::gcTypeOf(v.data);
		const Handle &h = runtime::typeHandle(t->type);
		if (t->kind == GcType::tArray) {
			GcArray<Byte> *alloc = (GcArray<Byte> *)v.data;
			(*h.toSFn)(alloc->v, &to);
		} else {
			(*h.toSFn)(&v.data, &to);
		}
		return to;
	}

	wostream &operator <<(wostream &to, const Variant &v) {
		if (v.empty())
			return to << L"<empty>";

		StrBuf *b = new (v.engine()) StrBuf();
		*b << v;
		return to << ::toS(b);
	}

}
