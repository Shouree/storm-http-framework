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

	RuntimeError::RuntimeError() {}

	GcError::GcError(const wchar *msg) : msg(msg) {
		saveTrace();
	}

	void GcError::message(StrBuf *to) const {
		*to << S("GC error: ") << msg;
	}


	NumericError::NumericError() {}

	DivisionByZero::DivisionByZero() {
		saveTrace();
	}

	void DivisionByZero::message(StrBuf *to) const {
		*to << S("Integer division by zero");
	}


	MemoryAccessError::MemoryAccessError(Word address, Type type) : address(address), type(type) {
		saveTrace();
	}

	void MemoryAccessError::message(StrBuf *to) const {
		*to << S("Memory access error: ");
		switch (type) {
		case notMapped:
			*to << S("address 0x") << hex(size_t(address)) << S(" is not valid.");
			break;
		case invalidAccess:
			*to << S("access to address 0x") << hex(size_t(address)) << S(" does not match memory permissions.");
			break;
		case invalidAlignment:
			*to << S("address 0x") << hex(size_t(address)) << S(" is not properly aligned.");
			break;
		case kernel:
			*to << S("address 0x") << hex(size_t(address)) << S(" is reserved by the kernel (reported address might be incorrect).");
			break;
		}
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

	UsageError::UsageError(const wchar *msg) {
		this->msg = new (this) Str(msg);
		saveTrace();
	}

	UsageError::UsageError(Str *msg) : msg(msg) {
		saveTrace();
	}

	void UsageError::message(StrBuf *to) const {
		*to << msg;
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

	AbstractFnCalled::AbstractFnCalled(const wchar *name) : RuntimeError() {
		this->name = new (this) Str(name);
	}

	AbstractFnCalled::AbstractFnCalled(Str *name) : RuntimeError(), name(name) {}

	void AbstractFnCalled::message(StrBuf *to) const {
		*to << S("Abstract function called: ") << name;
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
