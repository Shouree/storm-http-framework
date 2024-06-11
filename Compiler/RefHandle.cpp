#include "stdafx.h"
#include "RefHandle.h"
#include "Function.h"
#include "Engine.h"
#include "Utils/Memory.h"
#include "Code/Listing.h"

namespace storm {

	RefHandle::RefHandle(code::Content *content) : content(content) {}

	void RefHandle::setCopyCtor(code::Ref ref) {
		if (copyRef)
			copyRef->disable();
		copyRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, copyFn), ref, content);
	}

	void RefHandle::setCopyCtor(code::Binary *thunk) {
		if (copyRef)
			copyRef->disable();
		// No need to use references for this to work. The code and metadata is kept alive by just
		// storing a pointer in the appropriate variable:
		copyFn = (Handle::CopyFn)thunk->address();
	}

	void RefHandle::setDestroy(code::Ref ref) {
		if (destroyRef)
			destroyRef->disable();
		destroyRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, destroyFn), ref, content);
	}

	void RefHandle::setDestroy(code::Binary *thunk) {
		if (copyRef)
			copyRef->disable();
		// No need to use references for this to work. The code and metadata is kept alive by just
		// storing a pointer in the appropriate variable:
		destroyFn = (Handle::DestroyFn)thunk->address();
	}

	void RefHandle::setDeepCopy(code::Ref ref) {
		if (deepCopyRef)
			deepCopyRef->disable();
		deepCopyRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, deepCopyFn), ref, content);
	}

	void RefHandle::setDeepCopy(code::Binary *thunk) {
		if (copyRef)
			copyRef->disable();
		// No need to use references for this to work. The code and metadata is kept alive by just
		// storing a pointer in the appropriate variable:
		deepCopyFn = (Handle::DeepCopyFn)thunk->address();
	}

	void RefHandle::setToS(code::Ref ref) {
		if (toSRef)
			toSRef->disable();
		toSRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, toSFn), ref, content);
	}

	void RefHandle::setToS(code::Binary *thunk) {
		if (copyRef)
			copyRef->disable();
		// No need to use references for this to work. The code and metadata is kept alive by just
		// storing a pointer in the appropriate variable:
		toSFn = (Handle::ToSFn)thunk->address();
	}

	void RefHandle::setHash(code::Ref ref) {
		if (hashRef)
			hashRef->disable();
		hashRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, hashFn), ref, content);
	}

	void RefHandle::setHash(code::Binary *thunk) {
		if (copyRef)
			copyRef->disable();
		// No need to use references for this to work. The code and metadata is kept alive by just
		// storing a pointer in the appropriate variable:
		hashFn = (Handle::HashFn)thunk->address();
	}

	void RefHandle::setEqual(code::Ref ref) {
		if (equalRef)
			equalRef->disable();
		equalRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, equalFn), ref, content);
	}

	void RefHandle::setEqual(code::Binary *thunk) {
		if (copyRef)
			copyRef->disable();
		// No need to use references for this to work. The code and metadata is kept alive by just
		// storing a pointer in the appropriate variable:
		equalFn = (Handle::EqualFn)thunk->address();
	}

	void RefHandle::setLess(code::Ref ref) {
		if (lessRef)
			lessRef->disable();
		lessRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, lessFn), ref, content);
	}

	void RefHandle::setLess(code::Binary *thunk) {
		if (copyRef)
			copyRef->disable();
		// No need to use references for this to work. The code and metadata is kept alive by just
		// storing a pointer in the appropriate variable:
		lessFn = (Handle::LessFn)thunk->address();
	}

	void RefHandle::setSerializedType(code::Ref ref) {
		if (serializedTypeRef)
			serializedTypeRef->disable();
		serializedTypeRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, serializedTypeFn), ref, content);
	}

	void RefHandle::setSerializedType(code::Binary *thunk) {
		if (copyRef)
			copyRef->disable();
		// No need to use references for this to work. The code and metadata is kept alive by just
		// storing a pointer in the appropriate variable:
		serializedTypeFn = (Handle::SerializedTypeFn)thunk->address();
	}

	template <class T>
	static Nat wrapHash(const void *obj) {
		const T *&o = *(const T **)obj;
		return o->hash();
	}

	template <class T>
	static Bool wrapEquals(const void *a, const void *b) {
		const T *&oa = *(const T **)a;
		const T *&ob = *(const T **)b;
		return *oa == *ob;
	}

	template <class T>
	static Bool wrapLess(const void *a, const void *b) {
		const T *&oa = *(const T **)a;
		const T *&ob = *(const T **)b;
		return *oa < *ob;
	}


	static void populateStr(Handle *h) {
		h->hashFn = &wrapHash<Str>;
		h->equalFn = &wrapEquals<Str>;
		h->lessFn = &wrapLess<Str>;
	}

	void populateHandle(Type *type, Handle *h) {
		Engine &e = type->engine;
		if (type == Str::stormType(e))
			populateStr(h);
	}
}
