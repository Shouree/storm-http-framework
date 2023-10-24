#include "stdafx.h"
#include "Future.h"
#include "CloneEnv.h"
#include "Exception.h"
#include "Utils/Memory.h"

namespace storm {

	FutureBase::FutureBase(const Handle &type) : data(null), noClone(false) {
		GcArray<byte> *result = (GcArray<byte> *)runtime::allocArray(engine(), type.gcArrayType, 1);
		data = new (runtime::allocStaticRaw(engine(), &Data::gcType.type)) Data(type, result);
	}

	FutureBase::FutureBase(const FutureBase &o) : data(o.data), noClone(o.noClone) {
		data->addRef();
	}

	FutureBase::~FutureBase() {
		data->release();
	}

	void FutureBase::deepCopy(CloneEnv *env) {
		// We need to clone the result now that we have been cloned.
		noClone = false;
	}

	void FutureBase::setNoClone() {
		noClone = true;
	}

	void FutureBase::postRaw(const void *value) {
		// Do not post multiple times.
		if (atomicCAS(data->result->filled, 0, 1) == 0) {
			data->handle->safeCopy(data->result->v, value);
			if (!noClone && data->handle->deepCopyFn) {
				CloneEnv *e = new (this) CloneEnv();
				(*data->handle->deepCopyFn)(data->result->v, e);
			}
			data->future.posted();
		} else {
			WARNING(L"Trying to re-use a future!");
		}
	}

	void FutureBase::error() {
		if (atomicCAS(data->result->filled, 0, 1) == 0) {
			data->future.error();
		} else {
			WARNING(L"Trying to re-use a future!");
		}
	}

	void FutureBase::error(Exception *exception) {
		try {
			exception->throwMe();
		} catch (...) {
			error();
		}
	}

	static os::PtrThrowable *cloneEx(os::PtrThrowable *src, void *) {
		if (Object *o = dynamic_cast<Object *>(src)) {
			return clone(o);
		} else {
			return src;
		}
	}

	void FutureBase::resultRaw(void *to) {
		data->future.result(&cloneEx, null);

		data->handle->safeCopy(to, data->result->v);
		if (!noClone && data->handle->deepCopyFn) {
			CloneEnv *e = new (this) CloneEnv();
			(*data->handle->deepCopyFn)(to, e);
		}
	}

	void FutureBase::errorResult() {
		data->future.result(&cloneEx, null);
	}

	void FutureBase::detach() {
		data->future.detach();
	}

	os::FutureBase *FutureBase::rawFuture() {
		if (atomicCAS(data->releaseOnResult, 0, 1) == 0)
			data->addRef();

		return &data->future;
	}

	FutureBase::FutureSema::FutureSema() : os::FutureSema<os::Sema>() {}

	void FutureBase::FutureSema::notify() {
		os::FutureSema<os::Sema>::notify();
		Data::resultPosted(this);
	}

	const GcTypeStore<3> FutureBase::Data::gcType = {
		{
			GcType::tFixed,
			null,
			&FutureBase::Data::finalize,
			sizeof(FutureBase::Data),
			3,
			// First pointer offset.
			{ OFFSET_OF(FutureBase::Data, future.ptrException) }
		},
		// Remaining two pointer offsets.
		{
			OFFSET_OF(FutureBase::Data, handle),
			OFFSET_OF(FutureBase::Data, result),
		}
	};

	FutureBase::Data::Data(const Handle &type, GcArray<byte> *result)
		: handle(&type), result(result), refs(1), releaseOnResult(0) {}

	FutureBase::Data::~Data() {}

	void FutureBase::Data::addRef() {
		atomicIncrement(refs);
	}

	void FutureBase::Data::release() {
		if (atomicDecrement(refs) == 0)
			this->~Data();
	}

	void FutureBase::Data::resultPosted(FutureSema *sema) {
		Data *me = BASE_PTR(Data, sema, future);

		if (atomicCAS(me->releaseOnResult, 1, 0) == 1)
			me->release();
	}

	void FutureBase::Data::finalize(void *object, os::Thread *) {
		// If the refcount is larger than one, someone forgot to update the refcount, and we need to
		// call the destructor.
		Data *d = static_cast<Data *>(object);
		if (atomicRead(d->refs) != 0)
			d->~Data();
	}

}
