#include "stdafx.h"
#include "Session.h"

#include "SecureChannel.h"
#include "OpenSSL.h"

namespace ssl {

	// TODO: Platform independence...

	Session::Session(IStream *input, OStream *output, SSLSession *ctx, Str *host) : data(ctx), gcData(null) {
		gcData = data->connect(input, output, host);
	}

	Session::Session(const Session &o) : data(o.data), gcData(o.gcData) {
		data->ref();
	}

	Session::~Session() {
		data->unref();
	}

	void Session::deepCopy(CloneEnv *) {
		// No need.
	}

	IStream *Session::input() {
		return new (this) SessionIStream(this);
	}

	OStream *Session::output() {
		return new (this) SessionOStream(this);
	}

	void Session::close() {
		shutdown();
		data->close(gcData);
	}

	Bool Session::more() {
		return data->more(gcData);
	}

	void Session::read(Buffer &to) {
		if (to.free() > 0)
			data->read(to, gcData);
	}

	void Session::peek(Buffer &to) {
		data->peek(to, gcData);
	}

	Nat Session::write(const Buffer &from, Nat offset) {
		if (offset >= from.filled())
			return 0;

		return data->write(from, offset, gcData);
	}

	Bool Session::flush() {
		return data->flush(gcData);
	}

	void Session::shutdown() {
		data->shutdown(gcData);
	}


	/**
	 * Input stream.
	 */

	SessionIStream::SessionIStream(Session *owner) : owner(owner) {}

	void SessionIStream::close() {}

	Bool SessionIStream::more() {
		return owner->more();
	}

	Buffer SessionIStream::read(Buffer to) {
		owner->read(to);
		return to;
	}

	Buffer SessionIStream::peek(Buffer to) {
		owner->peek(to);
		return to;
	}


	/**
	 * Output stream.
	 */

	SessionOStream::SessionOStream(Session *owner) : owner(owner) {}

	void SessionOStream::close() {
		owner->shutdown();
	}

	Nat SessionOStream::write(Buffer from, Nat offset) {
		return owner->write(from, offset);
	}

	Bool SessionOStream::flush() {
		return owner->flush();
	}

}
