#include "stdafx.h"
#include "Stream.h"
#include "Exception.h"
#include "LazyMemStream.h"

namespace storm {

	/**
	 * IStream.
	 */

	IStream::IStream() {}

	Bool IStream::more() {
		return false;
	}

	Buffer IStream::read(Nat c) {
		return read(buffer(engine(), c));
	}

	Buffer IStream::read(Buffer to) {
		return to;
	}

	Buffer IStream::peek(Nat c) {
		return peek(buffer(engine(), c));
	}

	Buffer IStream::peek(Buffer to) {
		return to;
	}

	Buffer IStream::fill(Nat c) {
		return fill(buffer(engine(), c));
	}

	Buffer IStream::fill(Buffer b) {
		b.filled(0);
		while (!b.full()) {
			Nat old = b.filled();
			b = read(b);

			// End of stream reached?
			if (b.filled() == old)
				break;
		}
		return b;
	}

	RIStream *IStream::randomAccess() {
		return new (this) LazyMemIStream(this);
	}

	void IStream::close() {}

	/**
	 * RIStream.
	 */

	RIStream::RIStream() {}

	void RIStream::seek(Word to) {}

	Word RIStream::tell() { return 0; }

	Word RIStream::length() { return 0; }

	RIStream *RIStream::randomAccess() {
		return this;
	}


	/**
	 * OStream.
	 */

	OStream::OStream() {}

	void OStream::write(Buffer buf) {
		write(buf, 0);
	}

	void OStream::write(Buffer buf, Nat start) {}

	void OStream::flush() {}

	void OStream::close() {}


	/**
	 * Read/write primitive types.
	 */

	static void checkBuffer(Engine &e, const Buffer &b) {
		if (!b.full())
			throw new (e) IoError(S("Not enough data to read primitive."));
	}

	Bool IStream::readBool() {
		return readByte() != 0;
	}

	Byte IStream::readByte() {
		GcPreArray<Byte, 1> d;
		Buffer b = fill(emptyBuffer(d));
		checkBuffer(engine(), b);
		return d.v[0];
	}

	Int IStream::readInt() {
		GcPreArray<Byte, 4> d;
		Buffer b = fill(emptyBuffer(d));
		checkBuffer(engine(), b);
		Int r = Int(b[0]) << 24;
		r |= Int(b[1]) << 16;
		r |= Int(b[2]) << 8;
		r |= Int(b[3]) << 0;
		return r;
	}

	Nat IStream::readNat() {
		GcPreArray<Byte, 4> d;
		Buffer b = fill(emptyBuffer(d));
		checkBuffer(engine(), b);
		Nat r = Nat(b[0]) << 24;
		r |= Nat(b[1]) << 16;
		r |= Nat(b[2]) << 8;
		r |= Nat(b[3]) << 0;
		return r;
	}

	Long IStream::readLong() {
		GcPreArray<Byte, 8> d;
		Buffer b = fill(emptyBuffer(d));
		checkBuffer(engine(), b);
		Long r = Long(b[0]) << 56;
		r |= Long(b[1]) << 48;
		r |= Long(b[2]) << 40;
		r |= Long(b[3]) << 32;
		r |= Long(b[4]) << 24;
		r |= Long(b[5]) << 16;
		r |= Long(b[6]) << 8;
		r |= Long(b[7]) << 0;
		return r;
	}

	Word IStream::readWord() {
		GcPreArray<Byte, 8> d;
		Buffer b = fill(emptyBuffer(d));
		checkBuffer(engine(), b);
		Long r = Long(b[0]) << 56;
		r |= Long(b[1]) << 48;
		r |= Long(b[2]) << 40;
		r |= Long(b[3]) << 32;
		r |= Long(b[4]) << 24;
		r |= Long(b[5]) << 16;
		r |= Long(b[6]) << 8;
		r |= Long(b[7]) << 0;
		return r;
	}

	Float IStream::readFloat() {
		Nat repr = readNat();
		Float r;
		memcpy(&r, &repr, sizeof(repr));
		return r;
	}

	Double IStream::readDouble() {
		Word repr = readWord();
		Double r;
		memcpy(&r, &repr, sizeof(repr));
		return r;
	}


	void OStream::writeBool(Bool v) {
		writeByte(v ? 1 : 0);
	}

	void OStream::writeByte(Byte v) {
		GcPreArray<Byte, 1> d;
		d.v[0] = v;
		write(fullBuffer(d));
	}

	void OStream::writeInt(Int v) {
		GcPreArray<Byte, 4> d;
		d.v[0] = Byte((v >> 24) & 0xFF);
		d.v[1] = Byte((v >> 16) & 0xFF);
		d.v[2] = Byte((v >>  8) & 0xFF);
		d.v[3] = Byte((v >>  0) & 0xFF);
		write(fullBuffer(d));
	}

	void OStream::writeNat(Nat v) {
		GcPreArray<Byte, 4> d;
		d.v[0] = Byte((v >> 24) & 0xFF);
		d.v[1] = Byte((v >> 16) & 0xFF);
		d.v[2] = Byte((v >>  8) & 0xFF);
		d.v[3] = Byte((v >>  0) & 0xFF);
		write(fullBuffer(d));
	}

	void OStream::writeLong(Long v) {
		GcPreArray<Byte, 8> d;
		d.v[0] = Byte((v >> 56) & 0xFF);
		d.v[1] = Byte((v >> 48) & 0xFF);
		d.v[2] = Byte((v >> 40) & 0xFF);
		d.v[3] = Byte((v >> 32) & 0xFF);
		d.v[4] = Byte((v >> 24) & 0xFF);
		d.v[5] = Byte((v >> 16) & 0xFF);
		d.v[6] = Byte((v >>  8) & 0xFF);
		d.v[7] = Byte((v >>  0) & 0xFF);
		write(fullBuffer(d));
	}

	void OStream::writeWord(Word v) {
		GcPreArray<Byte, 8> d;
		d.v[0] = Byte((v >> 56) & 0xFF);
		d.v[1] = Byte((v >> 48) & 0xFF);
		d.v[2] = Byte((v >> 40) & 0xFF);
		d.v[3] = Byte((v >> 32) & 0xFF);
		d.v[4] = Byte((v >> 24) & 0xFF);
		d.v[5] = Byte((v >> 16) & 0xFF);
		d.v[6] = Byte((v >>  8) & 0xFF);
		d.v[7] = Byte((v >>  0) & 0xFF);
		write(fullBuffer(d));
	}

	void OStream::writeFloat(Float v) {
		Nat repr;
		memcpy(&repr, &v, sizeof(v));
		writeNat(repr);
	}

	void OStream::writeDouble(Double v) {
		Word repr;
		memcpy(&repr, &v, sizeof(v));
		writeWord(repr);
	}

}
