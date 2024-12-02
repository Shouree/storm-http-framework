#include "stdafx.h"
#include "MeterStream.h"

namespace storm {

	MeterOStream::MeterOStream(OStream *to) : to(to), pos(0) {}

	Nat MeterOStream::write(Buffer buf, Nat start) {
		Nat c = 0;
		if (start <= buf.filled())
			c = buf.filled() - start;
		Nat written = to->write(buf, start);
		pos += c;
		return written;
	}

	Bool MeterOStream::flush() {
		return to->flush();
	}

	void MeterOStream::close() {
		to->close();
	}

	sys::ErrorCode MeterOStream::error() const {
		return to->error();
	}

	Word MeterOStream::tell() {
		return pos;
	}

	void MeterOStream::reset() {
		pos = 0;
	}

}
