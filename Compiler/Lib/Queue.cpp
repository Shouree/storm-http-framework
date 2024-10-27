#include "stdafx.h"
#include "Queue.h"
#include "Engine.h"
#include "Core/Queue.h"

namespace storm {

	Type *createQueue(Str *name, ValueArray *params) {
		if (params->count() != 1)
			return null;

		Value param = params->at(0);
		if (param.ref)
			return null;

		return new (params) QueueType(name, param.type);
	}

	Bool isQueue(Value v) {
		return as<QueueType>(v.type) != null;
	}

	Value unwrapQueue(Value v) {
		if (QueueType *t = as<QueueType>(v.type))
			return t->param();
		else
			return v;
	}

	Value wrapQueue(Value v) {
		if (!v.type)
			return v;
		if (v.ref)
			return v;

		Engine &e = v.type->engine;
		TemplateList *l = e.cppTemplate(QueueId);
		NameSet *to = l->addTo();
		assert(to, L"Too early to use 'wrapQueue'.");
		Type *found = as<Type>(to->find(S("Queue"), v, Scope()));
		if (!found)
			throw new (v.type) InternalError(S("Can not find the queue type!"));
		return Value(found);
	}

	QueueType::QueueType(Str *name, Type *contents) : Type(name, typeClass), contents(contents) {
		if (engine.has(bootTemplates))
			lateInit();

		setSuper(QueueBase::stormType(engine));
	}

	void QueueType::lateInit() {
		if (!params)
			params = new (engine) Array<Value>();
		if (params->count() < 1)
			params->push(Value(contents));

		Type::lateInit();
	}

	Value QueueType::param() const {
		return Value(contents);
	}

	static void CODECALL createQueueRaw(void *mem) {
		QueueType *t = (QueueType *)runtime::typeOf((RootObject *)mem);
		QueueBase *o = new (Place(mem)) QueueBase(t->param().type->handle());
		runtime::setVTable(o);
	}

	static void CODECALL copyQueue(void *mem, QueueBase *from) {
		QueueBase *o = new (Place(mem)) QueueBase(*from);
		runtime::setVTable(o);
	}

	static void CODECALL pushQueueClass(QueueBase *q, void *elem) {
		q->pushRaw(&elem);
	}

	static QueueBase *CODECALL addQueueClass(QueueBase *q, void *elem) {
		q->pushRaw(&elem);
		return q;
	}

	static QueueBase *CODECALL addQueueVal(QueueBase *q, void *elem) {
		q->pushRaw(elem);
		return q;
	}


	Bool QueueType::loadAll() {
		Engine &e = engine;
		Value t = thisPtr(this);
		Value ref = param().asRef();

		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 1, t), address(&createQueueRaw))->makePure());
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 2, t, t), address(&copyQueue))->makePure());
		add(nativeFunction(e, ref, S("top"), valList(e, 1, t), address(&QueueBase::topRaw))->makePure());

		if (param().isObject()) {
			add(nativeFunction(e, Value(), S("push"), valList(e, 2, t, param()), address(&pushQueueClass)));
			add(nativeFunction(e, t, S("<<"), valList(e, 2, t, param()), address(&addQueueClass)));
		} else {
			add(nativeFunction(e, Value(), S("push"), valList(e, 2, t, ref), address(&QueueBase::pushRaw)));
			add(nativeFunction(e, t, S("<<"), valList(e, 2, t, ref), address(&addQueueVal)));
		}

		Type *iter = new (this) QueueIterType(contents);
		add(iter);

		add(nativeFunction(e, Value(iter), S("begin"), valList(e, 1, t), address(&QueueBase::beginRaw))->makePure());
		add(nativeFunction(e, Value(iter), S("end"), valList(e, 1, t), address(&QueueBase::endRaw))->makePure());

		return Type::loadAll();
	}

	/**
	 * Iterator.
	 */

	static void CODECALL copyIterator(void *to, const QueueBase::Iter *from) {
		new (Place(to)) QueueBase::Iter(*from);
	}

	static void *CODECALL assignIterator(QueueBase::Iter *to, const QueueBase::Iter *from) {
		*to = *from;
		return to;
	}

	static bool CODECALL iteratorEq(QueueBase::Iter &a, QueueBase::Iter &b) {
		return a == b;
	}

	static bool CODECALL iteratorNeq(QueueBase::Iter &a, QueueBase::Iter &b) {
		return a != b;
	}

	static void *CODECALL iteratorGet(const QueueBase::Iter &v) {
		return v.getRaw();
	}

	static Nat CODECALL iteratorGetKey(const QueueBase::Iter &v) {
		return v.getIndex();
	}

	QueueIterType::QueueIterType(Type *param)
		: Type(new (param) Str(S("Iter")), new (param) Array<Value>(), typeValue),
		  contents(param) {

		setSuper(QueueBase::Iter::stormType(engine));
	}

	Bool QueueIterType::loadAll() {
		Engine &e = engine;
		Value v(this, false);
		Value r(this, true);
		Value param(contents, true);

		Array<Value> *ref = valList(e, 1, r);
		Array<Value> *refref = valList(e, 2, r, r);

		Value vBool = Value(StormInfo<Bool>::type(e));
		Value vNat = Value(StormInfo<Nat>::type(e));
		add(nativeFunction(e, Value(), Type::CTOR, refref, address(&copyIterator))->makePure());
		add(nativeFunction(e, r, S("="), refref, address(&assignIterator))->makePure());
		add(nativeFunction(e, vBool, S("=="), refref, address(&iteratorEq))->makePure());
		add(nativeFunction(e, vBool, S("!="), refref, address(&iteratorNeq))->makePure());
		add(nativeFunction(e, r, S("++*"), ref, address(&QueueBase::Iter::preIncRaw)));
		add(nativeFunction(e, v, S("*++"), ref, address(&QueueBase::Iter::postIncRaw)));
		add(nativeFunction(e, vNat, S("k"), ref, address(&iteratorGetKey))->makePure());
		add(nativeFunction(e, param, S("v"), ref, address(&iteratorGet))->makePure());

		return Type::loadAll();
	}


}
