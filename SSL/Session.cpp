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
		data->read(to, gcData);
	}

	void Session::peek(Buffer &to) {
		data->peek(to, gcData);
	}

	void Session::write(const Buffer &from, Nat offset) {
		if (offset >= from.filled())
			return;

		data->write(from, offset, gcData);
	}

	void Session::flush() {
		data->flush(gcData);
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

	void SessionOStream::write(Buffer from, Nat offset) {
		owner->write(from, offset);
	}

	void SessionOStream::flush() {
		owner->flush();
	}

}
