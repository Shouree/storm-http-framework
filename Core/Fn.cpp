#include "stdafx.h"
#include "Fn.h"
#include "StrBuf.h"
#include "CloneEnv.h"

namespace storm {

	RawFn::RawFn() : fn(null) {}

	void RawFn::call(FnBase *ptr, void *out, const void *params) const {
		typedef void (*Fn)(FnBase *ptr, void *out, const void *params);
		Fn fn = (Fn)this->fn;
		(*fn)(ptr, out, params);
	}

	/**
	 * Targets.
	 */

	RawFnTarget::RawFnTarget(const void *ptr) : data(ptr) {}

	void RawFnTarget::cloneTo(void *to, size_t size) const {
		assert(size >= sizeof(*this));
		new (to) RawFnTarget(data);
	}

	const void *RawFnTarget::ptr() const {
		return data;
	}

	void RawFnTarget::toS(StrBuf *to) const {
		*to << L"C++ function @" << hex(data);
	}


	/**
	 * The function pointer!
	 */

	FnBase::FnBase(const void *fn, const RootObject *thisPtr, Bool member, Thread *thread) {
		callMember = member;
		this->thisPtr = thisPtr;
		this->thread = thread;
		new (target()) RawFnTarget(fn);
	}

	FnBase::FnBase(const FnTarget &target, const RootObject *thisPtr, Bool member, Thread *thread) {
		callMember = member;
		this->thisPtr = thisPtr;
		this->thread = thread;
		target.cloneTo(this->target(), targetSize * sizeof(size_t));
	}

	FnBase::FnBase(const FnBase &o) {
		callMember = o.callMember;
		thisPtr = o.thisPtr;
		thread = o.thread;
		o.target()->cloneTo(target(), targetSize * sizeof(size_t));
	}

	void FnBase::deepCopy(CloneEnv *env) {
		if (thisPtr) {
			if (const Object *o = as<const Object>(thisPtr)) {
				// Yes, we need to clone it!
				cloned(o, env);
				thisPtr = o;
			}
		}
	}

	bool FnBase::needsCopy(const TObject *first) const {
		Thread *t = runOn(first);
		if (t)
			return t->thread() != os::Thread::current();
		else
			return false;
	}

	Thread *FnBase::runOn(const TObject *first) const {
		const TObject *tObj;
		if (callMember && (tObj = as<const TObject>(thisPtr)))
			return tObj->associatedThread();
		else if (callMember && !thread && first)
			return first->associatedThread();
		else
			return thread;
	}

	void FnBase::callRawI(void *out, const os::FnCallRaw &params, const TObject *first, CloneEnv *env) const {
		const void *toCall = target()->ptr();

		Thread *thread = runOn(first);
		bool spawn = needsCopy(first);

		void *addFirst = null;
		if (thisPtr) {
			addFirst = (void *)thisPtr;
			if (spawn) {
				if (!env)
					env = new (this) CloneEnv();
				// In this case, 'p' has to be derived from Object.
				addFirst = clone((Object *)addFirst, env);
			}
		}

		// Dispatch to the proper thread.
		if (spawn) {
			os::FutureSema<os::Sema> future;
			os::UThread::spawnRaw(toCall, callMember, addFirst, params, future, out, &thread->thread());
			future.result();
		} else {
			params.callRaw(toCall, callMember, addFirst, out);
		}
	}

	void FnBase::toS(StrBuf *to) const {
		target()->toS(to);
	}

	RawFn FnBase::rawCall() {
		assert(false, L"Override 'rawCall'!");
		return RawFn();
	}


}
