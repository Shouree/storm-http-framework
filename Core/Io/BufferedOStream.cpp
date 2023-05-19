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

	void BufferedOStream::write(Buffer input, Nat start) {
		if (start >= input.filled())
			return;

		// 0: Fast path: if our buffer is empty, and number of bytes are larger than the buffer,
		// just output the buffer as is.
		Nat inputCount = input.filled() - start;
		if (buffer.empty() && inputCount >= buffer.count()) {
			output->write(input, start);
			return;
		}

		// 1: Fill the buffer.
		Nat toCopy = min(inputCount, buffer.free());
		memcpy(buffer.dataPtr() + buffer.filled(), input.dataPtr() + start, toCopy);
		buffer.filled(buffer.filled() + toCopy);
		start += toCopy;
		inputCount -= toCopy;

		// 2: Time to flush? If not, then we are done.
		if (!buffer.full())
			return;
		output->write(buffer);
		buffer.filled(0);

		// 3: More data. Either, we can copy the remaining data to the buffer, or it is enough that
		// we can just output the rest as it is (as in the fast-path).
		if (inputCount >= buffer.count()) {
			// Output everything.
			output->write(input, start);
		} else if (inputCount > 0) {
			// Copy the remaining data. Note: 'inputCount' is less than 'buffer.count()', and the
			// buffer is flushed.
			memcpy(buffer.dataPtr(), input.dataPtr() + start, inputCount);
			buffer.filled(inputCount);
		}
	}

	void BufferedOStream::flush() {
		if (buffer.filled() > 0) {
			output->write(buffer);
			buffer.filled(0);
		}
	}

	void BufferedOStream::close() {
		flush();
		output->close();
	}

}
