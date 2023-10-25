#include "stdafx.h"
#include "Debug.h"
#include "Exception.h"
#include "Variable.h"
#include "Type.h"
#include "Core/Str.h"
#include "Core/Set.h"
#include "Code/Debug.h"
#include "Utils/Memory.h"
#include "Engine.h"
#include "Lib/MultiStackTrace.h"
#include "Core/Io/Text.h"

#ifdef POSIX
#include <signal.h>
#endif

namespace storm {
	namespace debug {

		Link *createList(EnginePtr ep, Nat n) {
			Link *start = null;
			Link *prev = null;
			Engine &e = ep.v;

			for (nat i = 0; i < n; i++) {
				Link *now = new (e) Link;
				now->value = i;

				if (prev == null) {
					start = now;
				} else {
					prev->next = now;
				}
				prev = now;
			}

			return start;
		}

		Bool checkList(Link *first, Nat n) {
			Link *at = first;
			for (nat i = 0; i < n; i++) {
				if (!at)
					return false;
				if (at->value != i)
					return false;

				at = at->next;
			}

			return at == null;
		}


		Value::Value() : value(0), list(null) {}

		DtorClass::DtorClass(int v) : value(v) {}

		DtorClass::~DtorClass() {
			PVAR(value);
		}

		PtrKey::PtrKey() {
			reset();
		}

		void PtrKey::reset() {
			oldPos = size_t(this);
		}

		bool PtrKey::moved() const {
			return size_t(this) != oldPos;
		}

		Extend::Extend() : v(1) {}

		Extend::Extend(Int v) : v(v) {}

		Int Extend::value() {
			return v;
		}

		void dbgBreak() {
#ifdef WINDOWS
			DebugBreak();
#else
			raise(SIGINT);
#endif
		}

		Float dbgFloat() {
			return 23.0f;
		}

		void dumpObject(Object *obj) {
			PLN((void *)obj);
		}

		void dumpObject(TObject *obj) {
			PLN((void *)obj);
		}

		void stackTrace(EnginePtr e) {
			PLN(collectStackTrace(e).format());
		}

		void threadSummary(EnginePtr e) {
			e.v.stdOut()->writeLine(collectAllStackTraces(e)->toS());
		}

		void throwError(EnginePtr e) {
			throw new (e.v) DebugError();
		}

		/**
		 * Basically an alias for Set<Object>, but uses object hashes for all operations.
		 */
		class ObjSet : public SetBase {
		public:
			static Type *stormType(Engine &e) {
				return SetBase::stormType(e);
			}

			ObjSet() : SetBase(StormInfo<TObject *>::handle(engine())) {}

			void put(RootObject *r) {
				putRaw(&r);
			}

			Bool has(RootObject *r) {
				return hasRaw(&r);
			}

			class Iter : public SetBase::Iter {
			public:
				Iter() : SetBase::Iter() {}
				Iter(ObjSet *owner) : SetBase::Iter(owner) {}

				ObjSet *v() const {
					return *(ObjSet **)rawVal();
				}
			};

			Iter begin() {
				return Iter(this);
			}

			Iter end() {
				return Iter();
			}

		};

		// Generic object traversal function.
		typedef void (*FoundFn)(void *, MemberVar *, nat, void *);

		// Call 'foundFn' for all objects reachable from 'base'.
		static void traverse(void *base, Type *type, ObjSet *visited, nat depth, FoundFn fn, void *data) {
			// Examine super class' members.
			if (Type *super = type->super())
				traverse(base, super, visited, depth, fn, data);

			Array<MemberVar *> *vars = type->variables();
			for (nat i = 0; i < vars->count(); i++) {
				MemberVar *v = vars->at(i);
				const storm::Value &t = v->type;
				Offset offset = v->offset();

				if (t.ref) {
					WARNING(L"References are not yet supported and therefore ignored!");
				} else if (t.isClass()) {
					// Class.
					RootObject *o = OFFSET_IN(base, offset.current(), Object *);
					if (o != null && !visited->has(o)) {
						visited->put(o);
						if (fn)
							(*fn)(o, v, depth, data);
						traverse(o, runtime::typeOf(o), visited, depth + 1, fn, data);
					}
				} else if (t.isPrimitive()) {
					// Built-in type, never a problem. Ignore.
					void *o = &OFFSET_IN(base, offset.current(), void *);
					if (fn)
						(*fn)(o, v, depth, data);
				} else {
					// Value. The value itself is not a problem, but it may contain references that we need to examine.
					void *o = &OFFSET_IN(base, offset.current(), void *);
					if (fn)
						(*fn)(o, v, depth, data);
					traverse(o, t.type, visited, depth + 1, fn, data);
				}
			}
		}

		// Entry point.
		static ObjSet *traverse(Object *o, FoundFn fn, void *data) {
			ObjSet *visited = new (o) ObjSet();
			visited->put(o);
			traverse(o, runtime::typeOf(o), visited, 0, fn, data);
			return visited;
		}


		static void printLayout(void *object, MemberVar *var, nat depth, void *data) {
			ObjSet *o = (ObjSet *)data;

			bool hilight = false;
			String value = L"?";
			const storm::Value &v = var->type;
			if (v.isClass()) {
				RootObject *obj = (RootObject *)object;
				value = ::toS(obj) + L" @" + toHex(obj);
				if (o->has(obj))
					hilight = true;
			} else if (v.isPrimitive()) {
				// Built in type.
				if (v.type == StormInfo<Int>::type(var->engine())) {
					value = ::toS(*(Int *)object);
				} else if (v.type == StormInfo<Nat>::type(var->engine())) {
					value = ::toS(*(Nat *)object);
				}
			} else {
				// Value
				value = L"value:";
			}

			String type = ::toS(v) + L" = " + ::toS(var->name);
			if (hilight)
				PNN("->");
			else
				depth++;

			for (nat i = 0; i < depth; i++)
				PNN("  ");
			PLN(type << " " << value);
		}

		// Layout with highlights.
		static void layout(Object *root, ObjSet *hilight) {
			PLN(runtime::typeOf(root)->identifier() << " " << root);
			traverse(root, &printLayout, hilight);
		}

		void layout(Object *obj) {
			layout(obj, new (obj) ObjSet());
		}

		Bool disjoint(Object *a, Object *b) {
			ObjSet *aObjs = traverse(a, null, null);
			ObjSet *bObjs = traverse(b, null, null);
			ObjSet *same = new (a) ObjSet();
			for (ObjSet::Iter i = aObjs->begin(), end = aObjs->end(); i != end; ++i) {
				if (bObjs->has(i.v()))
					same->put(i.v());
			}

			if (same->any()) {
				PLN("The objects " << a << " and " << b << " are not disjoint!");
				PLN("Reachable from a: " << ::toS(aObjs));
				PLN("Reachable from b: " << ::toS(bObjs));
				PLN("Common: " << ::toS(same));
				PLN("--------- LAYOUT OF A ---------");
				layout(a, same);
				PLN("--------- LAYOUT OF B ---------");
				layout(b, same);
				return false;
			}

			return true;
		}

		Bool same(Object *a, Object *b) {
			return a == b;
		}

		void dbgSleep(Int ms) {
			if (ms < 0)
				return;
#ifdef WINDOWS
			Sleep(ms);
#else
			usleep(useconds_t(ms) * 1000);
#endif
		}

		void dbgYield() {
			os::UThread::leave();
		}

		Dbg::Dbg() : v(10) {}

		Dbg::Dbg(Dbg *o) : v(o->v) {}

		Dbg::Dbg(Int v) : v(v) {}

		Dbg::~Dbg() {}

		void Dbg::set(Int v) {
			this->v = v;
		}

		Int Dbg::get() {
			return v;
		}

		DbgVal Dbg::asDbgVal() {
			return DbgVal(v);
		}

		void Dbg::dbg() {
			PLN("Debug object. Type: " << runtime::typeOf(this) << " value: " << v);
		}

		Int Dbg::returnOne() {
			return 1;
		}

		Int Dbg::returnTwo() {
			return 2;
		}

		void Dbg::deepCopy(CloneEnv *e) {
			// Nothing needed here...
		}

		DbgVal::LiveSet DbgVal::live;

		os::Lock DbgVal::liveLock;

		static void dbg_dumpNolock() {
			for (DbgVal::LiveSet::iterator i = DbgVal::live.begin(); i != DbgVal::live.end(); ++i) {
				PLN(L"Live DbgVal at " << *i << L" (" << (*i)->v << L")");
			}
		}

		void DbgVal::dbg_dump() {
			os::Lock::L z(liveLock);
			dbg_dumpNolock();
		}

		bool DbgVal::clear() {
			os::Lock::L z(liveLock);

			if (live.empty())
				return true;

			dbg_dumpNolock();
			return false;
		}

		static void created(DbgVal *v) {
			os::Lock::L z(DbgVal::liveLock);

			if (DbgVal::live.count(v) != 0) {
				PLN("Trying to re-create a live instance at " << v);
				dbg_dumpNolock();
				assert(false);
			}
			DbgVal::live.insert(v);
		}

		DbgVal::DbgVal() : v(10) {
			created(this);
		}

		DbgVal::DbgVal(Int v) : v(v) {
			created(this);
		}

		DbgVal::DbgVal(const DbgVal &o) : v(o.get()) {
			created(this);
		}

		DbgVal &DbgVal::operator =(const DbgVal &o) {
			os::Lock::L z(liveLock);

			if (live.count(this) == 0) {
				PLN("Trying to assign to a dead object at " << this);
				dbg_dumpNolock();
				assert(false);
			}
			if (live.count(&o) == 0) {
				PLN("Trying to copy from a dead object at " << &o);
				dbg_dumpNolock();
				assert(false);
			}
			v = o.v;
			return *this;
		}

		DbgVal::~DbgVal() {
			os::Lock::L z(liveLock);

			if (live.count(this) == 0) {
				PLN("Trying to destroy a non-live object at " << this);
				PLN(format(::stackTrace()));
				dbg_dumpNolock();
				dbg_assert(false, L"Trying to destroy a non-live object!");
			}
			live.erase(this);
		}

		void DbgVal::set(Int v) {
			os::Lock::L z(liveLock);

			if (live.count(this) == 0) {
				PLN("Trying to write to a dead object at " << this);
				dbg_dumpNolock();
				assert(false);
			}
			this->v = v;
		}

		Int DbgVal::get() const {
			os::Lock::L z(liveLock);

			if (live.count(this) == 0) {
				PLN("Trying to read from a dead object at " << this);
				dbg_dumpNolock();
				assert(false);
			}
			return v;
		}

		bool DbgVal::operator ==(const DbgVal &o) const {
			return get() == o.get();
		}

		wostream &operator <<(wostream &to, const DbgVal &v) {
			return to << L"DbgVal:" << v.get();
		}

		void DbgVal::deepCopy(CloneEnv *env) {
			// Nothing needed.
		}

		Str *toS(EnginePtr e, DbgVal v) {
			return new (e.v) Str(::toS(v).c_str());
		}

		DbgActor::DbgActor() : v(0) {}

		DbgActor::DbgActor(Int v) : v(v) {}

		void DbgActor::set(Int v) {
			this->v = v;
		}

		Int DbgActor::get() const {
			return v;
		}

		Str *DbgActor::echo(Str *v) {
			return v;
		}

		DbgVal DbgActor::asDbgVal() {
			return DbgVal(v);
		}


		DbgNoToS::DbgNoToS() : dummy(0) {}

		void docFunction(Int param) {}

		DbgAbstract::DbgAbstract() {}

		Int DbgAbstract::real() {
			return 100;
		}

		Int DbgAbstractDtor::destroyCount = 0;

		DbgAbstractDtor::DbgAbstractDtor() {}

		DbgAbstractDtor::~DbgAbstractDtor() {
			destroyCount++;
		}

	}
}

