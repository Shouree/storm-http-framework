#include "stdafx.h"
#include "BufferedOStream.h"

namespace storm {

	BufferedOStream::BufferedOStream(OStream *output) : output(output) {
		init(4096);
	}

	BufferedOStream::BufferedOStream(OStream *output, Nat bufferSize) : output(output) {
		init(bufferSize);
	}

	void BufferedOStream::init(Nat bufferSize) {
		buffer = storm::buffer(engine(), bufferSize);
	}

	Nat BufferedOStream::write(Buffer input, Nat start) {
		if (start >= input.filled())
			return 0;

		// 0: Fast path: if our buffer is empty, and number of bytes are larger than the buffer,
		// just output the buffer as is.
		Nat inputCount = input.filled() - start;
		if (buffer.empty() && inputCount >= buffer.count()) {
			return output->write(input, start);
		}

		// 1: Fill the buffer.
		Nat consumed = 0;
		Nat toCopy = min(inputCount, buffer.free());
		memcpy(buffer.dataPtr() + buffer.filled(), input.dataPtr() + start, toCopy);
		buffer.filled(buffer.filled() + toCopy);
		start += toCopy;
		inputCount -= toCopy;
		consumed += toCopy;

		// 2: Time to flush? If not, then we are done.
		if (!buffer.full())
			return consumed;
		output->write(buffer);
		buffer.filled(0);

		// 3: More data. Either, we can copy the remaining data to the buffer, or it is enough that
		// we can just output the rest as it is (as in the fast-path).
		if (inputCount >= buffer.count()) {
			// Output everything.
			consumed += output->write(input, start);
		} else if (inputCount > 0) {
			// Copy the remaining data. Note: 'inputCount' is less than 'buffer.count()', and the
			// buffer is flushed.
			memcpy(buffer.dataPtr(), input.dataPtr() + start, inputCount);
			buffer.filled(inputCount);
			consumed += inputCount;
		}
		return consumed;
	}

	Bool BufferedOStream::flush() {
		Bool ok = true;
		if (buffer.filled() > 0) {
			ok = output->write(buffer) == buffer.filled();
			buffer.filled(0);
		}
		return ok;
	}

	void BufferedOStream::close() {
		flush();
		output->close();
	}

	sys::ErrorCode BufferedOStream::error() const {
		return output->error();
	}

}
