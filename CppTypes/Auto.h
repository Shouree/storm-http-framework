#pragma once
#include "Utils/Templates.h"
#define IF_CONVERTIBLE(From, To) typename EnableIf<IsConvertible<From, To>::value, bool>::t = false

/**
 * Auto-ptr.
 */
template <class T>
class Auto {
public:
	Auto() : ptr(null) {}

	Auto(T *o) : ptr(o) {}

	Auto(const Auto<T> &o) : ptr(o.ptr) {
		if (ptr)
			ptr->addRef();
	}

	template <class U>
	Auto(const Auto<U> &o, IF_CONVERTIBLE(U, T)) : ptr(&*o) {
		if (ptr)
			ptr->addRef();
	}

	Auto<T> &operator =(const Auto<T> &o) {
		if (o.ptr)
			o.ptr->addRef();
		if (ptr)
			ptr->release();
		ptr = o.ptr;
		return *this;
	}

	~Auto() {
		if (ptr)
			ptr->release();
	}

	template <class U>
	Auto<U> as() const {
		U *o = dynamic_cast<U *>(ptr);
		if (o)
			o->addRef();
		return Auto<U>(o);
	}

	T *borrow() const {
		return ptr;
	}

	operator void *() const {
		return ptr;
	}

	bool operator ==(const Auto<T> &o) const {
		return ptr == o.ptr;
	}

	T *operator ->() const {
		return ptr;
	}

	T &operator *() const {
		return *ptr;
	}

	// Make sure < works as if calling < on the value (good in maps).
	bool operator <(const Auto<T> &o) const {
		return *ptr < *o.ptr;
	}

private:
	T *ptr;
};

template <class T>
wostream &operator <<(wostream &to, const Auto<T> &p) {
	return to << *p;
}

template <class T>
Auto<T> capture(T *ptr) {
	if (ptr)
		ptr->addRef();
	return Auto<T>(ptr);
}


/**
 * Refcounted object.
 */
class Refcount : NoCopy {
public:
	Refcount() : refs(1) {}
	Refcount(const Refcount &) : NoCopy(), refs(1) {}

	void addRef() {
		refs++;
	}

	void release() {
		if (--refs == 0)
			delete this;
	}

private:
	nat refs;
};
