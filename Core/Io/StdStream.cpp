#include "stdafx.h"
#include "StdStream.h"

namespace storm {

	namespace proc {

		IStream *in(EnginePtr e) {
			return new (e.v) StdIStream();
		}

		OStream *out(EnginePtr e) {
			return new (e.v) StdOStream(stdOut);
		}

		OStream *error(EnginePtr e) {
			return new (e.v) StdOStream(stdError);
		}

	}

	StdIStream::StdIStream() {}

	PeekReadResult StdIStream::doRead(byte *to, Nat count) {
		StdRequest r(stdIn, to, count);
		runtime::postStdRequest(engine(), &r);
		r.wait.down();
		return PeekReadResult::success(r.count);
	}

	StdOStream::StdOStream(StdStream t) : target(t) {}

	void StdOStream::write(Buffer to, Nat start) {
		start = min(start, to.filled());

		while (start < to.filled()) {
			StdRequest r(target, to.dataPtr() + start, to.filled() - start);
			runtime::postStdRequest(engine(), &r);
			r.wait.down();

			if (r.count == 0)
				break;

			start += r.count;
		}
	}

	StdRequest::StdRequest(StdStream stream, byte *to, Nat count)
		: stream(stream),
		  buffer(to),
		  count(count),
		  wait(0),
		  next(null) {}

}
