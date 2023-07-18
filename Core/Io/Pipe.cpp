#include "stdafx.h"
#include "Pipe.h"

namespace storm {

	Pipe::Pipe() {
		init(1024 * 4);
	}

	Pipe::Pipe(Nat size) {
		init(size);
	}

	void Pipe::init(Nat size) {
		data = runtime::allocArray<Byte>(engine(), &byteArrayType, size);
		readPos = 0;
		filled = 0;
		readClosed = false;
		writeClosed = false;

		lock = new (this) Lock();
		waitRead = new (this) Event();
		waitWrite = new (this) Event();

		waitWrite->set();
		waitRead->clear();
	}

	IStream *Pipe::input() {
		return new (this) PipeIStream(this);
	}

	OStream *Pipe::output() {
		return new (this) PipeOStream(this);
	}

	void Pipe::close() {
		closeRead();
		closeWrite();
	}

	void Pipe::closeRead() {
		readClosed = true;
		waitWrite->set();
	}

	void Pipe::closeWrite() {
		writeClosed = true;
		waitRead->set();
	}

	Bool Pipe::more() {
		Lock::Guard z(lock);
		return !writeClosed;
	}

	void Pipe::read(Buffer to) {
		readCommon(to, true);
	}

	void Pipe::peek(Buffer to) {
		readCommon(to, false);
	}

	void Pipe::readCommon(Buffer to, Bool updatePos) {
		while (true) {
			// Wait until we can read...
			waitRead->wait();

			Lock::Guard z(lock);

			// Spurious wakeup or closed?
			if (filled <= 0) {
				if (writeClosed)
					return;

				// Make sure we wait.
				waitRead->clear();
				continue;
			}

			Nat toRead = min(to.free(), filled);
			if (readPos + toRead > data->count) {
				// Need two memcpy.
				Nat first = Nat(data->count) - readPos;
				memcpy(to.dataPtr() + to.filled(), data->v + readPos, first);
				memcpy(to.dataPtr() + to.filled() + first, data->v, toRead - first);
			} else {
				// One is enough.
				memcpy(to.dataPtr() + to.filled(), data->v + readPos, toRead);
			}
			to.filled(to.filled() + toRead);

			if (updatePos) {
				filled -= toRead;
				readPos += toRead;
				if (readPos >= data->count)
					readPos -= Nat(data->count);

				// Now we can write more data!
				waitWrite->set();

				// Maybe we can't read anymore.
				if (filled == 0)
					waitRead->clear();
			}

			// Done!
			return;
		}
	}

	void Pipe::write(Buffer from, Nat start) {
		while (from.filled() > start) {
			waitWrite->wait();

			Lock::Guard z(lock);

			// If the read end is closed, just stop writing.
			if (readClosed)
				return;

			// Any room to write? If not, wait a bit more. It could be a spurious wakeup.
			if (filled >= data->count) {
				// Make sure we wait.
				waitWrite->clear();
				continue;
			}

			Nat toWrite = min(Nat(data->count) - filled, from.filled() - start);

			Nat tail = readPos + filled;
			if (tail >= data->count)
				tail -= Nat(data->count);

			// Take wrapping into account.
			toWrite = min(Nat(data->count) - tail, toWrite);

			memcpy(data->v + tail, from.dataPtr() + start, toWrite);

			filled += toWrite;
			start += toWrite;

			// Update our state:
			if (filled >= data->count)
				waitWrite->clear();

			// It is possible to read now, we put data there.
			waitRead->set();
		}
	}

}
