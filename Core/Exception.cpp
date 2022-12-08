#include "stdafx.h"
#include "Exception.h"
#include "StrBuf.h"

namespace storm {

	Exception::Exception() : stackTrace(engine()) {}

	Exception::Exception(const Exception &o) : stackTrace(o.stackTrace) {}

	void Exception::deepCopy(CloneEnv *env) {}

	Str *Exception::message() const {
		StrBuf *b = new (this) StrBuf();
		message(b);
		return b->toS();
	}

	void Exception::toS(StrBuf *to) const {
		message(to);
		if (stackTrace.any()) {
			*to << S("\n");
			stackTrace.format(to);
		}
	}

	Exception *Exception::saveTrace() {
		if (stackTrace.empty()) {
			try {
				stackTrace = collectStackTrace(engine());
			} catch (...) {
				// If we get an error, we just ignore it here. Better that we can throw an exception
				// at all than crashing when trying to grab a stack trace!
			}
		}
		return this;
	}

	GcError::GcError(const wchar *msg) : msg(msg) {
		saveTrace();
	}

	void GcError::message(StrBuf *to) const {
		*to << S("GC error: ") << msg;
	}


	NumericError::NumericError() {}

	DivisionByZero::DivisionByZero() {}

	void DivisionByZero::message(StrBuf *to) const {
		*to << S("Integer division by zero");
	}


	NotSupported::NotSupported(const wchar *msg) {
		this->msg = new (this) Str(msg);
		saveTrace();
	}

	NotSupported::NotSupported(Str *msg) {
		this->msg = msg;
		saveTrace();
	}

	void NotSupported::message(StrBuf *to) const {
		*to << S("Operation not supported: ") << msg;
	}

	InternalError::InternalError(const wchar *msg) {
		this->msg = new (this) Str(msg);
		saveTrace();
	}

	InternalError::InternalError(Str *msg) {
		this->msg = msg;
		saveTrace();
	}

	void InternalError::message(StrBuf *to) const {
		*to << S("Internal error: ") << msg;
	}

	RuntimeError::RuntimeError(const wchar *msg) {
		this->msg = new (this) Str(msg);
		saveTrace();
	}

	RuntimeError::RuntimeError(Str *msg) {
		this->msg = msg;
		saveTrace();
	}

	void RuntimeError::message(StrBuf *to) const {
		*to << msg;
	}

	AbstractFnCalled::AbstractFnCalled(const wchar *name) : RuntimeError(name) {}

	AbstractFnCalled::AbstractFnCalled(Str *name) : RuntimeError(name) {}

	void AbstractFnCalled::message(StrBuf *to) const {
		*to << S("Abstract function called: ");
		RuntimeError::message(to);
	}

	StrError::StrError(const wchar *msg) {
		this->msg = new (this) Str(msg);
		saveTrace();
	}

	StrError::StrError(Str *msg) {
		this->msg = msg;
		saveTrace();
	}

	void StrError::message(StrBuf *to) const {
		*to << msg;
	}

	MapError::MapError(const wchar *msg) {
		this->msg = new (this) Str(msg);
		saveTrace();
	}

	MapError::MapError(Str *msg) {
		this->msg = msg;
		saveTrace();
	}

	void MapError::message(StrBuf *to) const {
		*to << S("Map error: ") << msg;
	}

	SetError::SetError(const wchar *msg) {
		this->msg = new (this) Str(msg);
		saveTrace();
	}

	SetError::SetError(Str *msg) {
		this->msg = msg;
		saveTrace();
	}

	void SetError::message(StrBuf *to) const {
		*to << S("Set error: ") << msg;
	}

	ArrayError::ArrayError(Nat id, Nat count) : id(id), count(count), msg(null) {
		saveTrace();
	}

	ArrayError::ArrayError(Nat id, Nat count, Str *msg) : id(id), count(count), msg(msg) {
		saveTrace();
	}

	void ArrayError::message(StrBuf *to) const {
		*to << S("Array error: Index ") << id << S(" out of bounds (of ") << count << S(").");
		if (msg)
			*to << S(" During ") << msg << S(".");
	}

}
