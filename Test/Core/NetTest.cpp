#include "stdafx.h"
#include "Core/Net/Address.h"
#include "Core/Net/Socket.h"
#include "Core/Net/NetStream.h"
#include "Core/Net/Listener.h"
#include "Core/Io/Text.h"
#include "Core/Io/FileStream.h"
#include "Core/Timing.h"

BEGIN_TEST(NetAddrTest, Core) {
	Engine &e = gEngine();

	Address *addr = new (e) Inet4Address(0, 0xC0A80101);
	CHECK_EQ(toS(addr), L"192.168.1.1");

	{
		sockaddr_in in;
		in.sin_family = AF_INET;
		in.sin_port = 0;
		inet_pton(AF_INET, "192.168.1.1", &in.sin_addr);
		Address *o = toStorm(e, (sockaddr *)&in);
		CHECK_EQ(toS(o), L"192.168.1.1");
		CHECK_EQ(*o, *addr);
	}

	addr = new (e) Inet6Address(0, 0, 0, 0, 0);
	CHECK_EQ(toS(addr), L"::");
	addr = new (e) Inet6Address(0, 0, 0, 0, 1);
	CHECK_EQ(toS(addr), L"::1");
	addr = new (e) Inet6Address(0, 0xEFEF000, 0, 0, 0);
	CHECK_EQ(toS(addr), L"efe:f000::");
	addr = new (e) Inet6Address(0, 0xFE0000, 0, 0, 1);
	CHECK_EQ(toS(addr), L"fe::1");

	{
		sockaddr_in6 in;
		memset(&in, 0, sizeof(in));
		in.sin6_family = AF_INET6;
		inet_pton(AF_INET6, "fe::1", &in.sin6_addr);
		Address *o = toStorm(e, (sockaddr *)&in);
		CHECK_EQ(toS(o), L"fe::1");
		CHECK_EQ(*o, *addr);
	}


	CHECK_EQ(toS(toAddress(new (e) Str(S("192.168.0.1")))), L"192.168.0.1");
	CHECK_EQ(toS(toAddress(new (e) Str(S("192.168.0.1:31337")))), L"192.168.0.1:31337");

	CHECK_EQ(toS(toAddress(new (e) Str(S("abcd:ef01:2345:6789:1234:5678:9ABC:1230")))), L"abcd:ef01:2345:6789:1234:5678:9abc:1230");
	CHECK_EQ(toS(toAddress(new (e) Str(S("::")))), L"::");
	CHECK_EQ(toS(toAddress(new (e) Str(S("::1")))), L"::1");
	CHECK_EQ(toS(toAddress(new (e) Str(S("efe:f::1")))), L"efe:f::1");
	CHECK_EQ(toS(toAddress(new (e) Str(S("1::fed:1")))), L"1::fed:1");

	CHECK_EQ(toS(toAddress(new (e) Str(S("[1::fed:1]:31337")))), L"[1::fed:1]:31337");

	// Name resolution.
	CHECK_GTE(lookupAddress(new (e) Str(S("storm-lang.org")))->count(), Nat(1));

} END_TEST

struct NetServer {
	Listener *l;
	bool done;

	NetServer() : l(null), done(false) {}

	void server() {
		Engine &e = gEngine();

		l = listen(e, 31337);

		while (NetStream *s = l->accept()) {
			Buffer r = s->input()->read(50);
			s->output()->write(r);

			s->close();
		}

		l->close();
		done = true;
	}
};

BEGIN_TEST(NetConnectTest, Core) {
	Engine &e = gEngine();
	NetServer server;

	os::UThread::spawn(util::memberVoidFn(&server, &NetServer::server));

	os::UThread::leave();

	VERIFY(server.l);

	NetStream *sock = connect(new (e) Str(S("localhost")), 31337);
	VERIFY(sock);

	const char *data = "Hello!";
	sock->output()->write(buffer(e, (const Byte *)data, nat(strlen(data))));

	// We should get the same data back!
	Buffer r = sock->input()->read(50);
	CHECK_EQ(String((const char *)r.dataPtr()), L"Hello!");

	sock->close();

	// Make sure it starts calling 'accept' again, so that we see if it is aborted properly!
	os::UThread::leave();

	// Exit the server by calling 'close'.
	server.l->close();

	// Wait for the server to terminate.
	while (!server.done)
		os::UThread::leave();

} END_TEST

struct NetDuplex {
	NetStream *client;
	Nat id;
	Nat errorAt;
	Bool choked;

	NetDuplex() : client(null), id(0), errorAt(0), choked(false) {}

	void start() {
		Engine &e = gEngine();
		Listener *l = listen(e, 31338);

		client = l->accept();

		// Make it easier to choke the connection.
		client->outputBufferSize(32);

		check(1);
		os::UThread::spawn(util::memberVoidFn(this, &NetDuplex::send));
		os::UThread::spawn(util::memberVoidFn(this, &NetDuplex::checkFull));

		check(2);
		Buffer b = client->input()->read(50);

		// Let the other one finish first.
		os::UThread::leave();

		check(6);

		if (b.filled() != 6)
			errorAt = 10;

		client->close();
		l->close();

		// Signal we're done!
		id = 0;
	}

	void send() {
		Engine &e = gEngine();

		check(3);

		// On Linux, writing to a socket with a non-full output buffer will not block, we need to
		// fill the buffer to verify the behaviour of the queuing system!
		const char *data = "World!";
		Buffer b = buffer(e, (const Byte *)data, nat(strlen(data)));

		while (!choked)
			client->output()->write(b);

		check(5);
	}

	void checkFull() {
		// Executed whenever 'send' starts blocking.
		check(4);
		choked = true;
	}

	void check(Nat expected) {
		if (++id != expected)
			if (errorAt == 0)
				errorAt = id;
	}

};

BEGIN_TEST(NetDuplexTest, Core) {
	Engine &e = gEngine();
	NetDuplex ctx;

	os::UThread::spawn(util::memberVoidFn(&ctx, &NetDuplex::start));

	NetStream *s = connect(new (e) Str(S("localhost")), 31338);
	VERIFY(s);

	// Make it easier to choke the connection.
	s->inputBufferSize(32);

	// Wait until it filled the output buffer.
	while (!ctx.choked)
		os::UThread::leave();

	Buffer msg = s->input()->read(6);
	s->output()->write(msg);

	s->close();

	while (ctx.id != 0)
		os::UThread::leave();

	CHECK_EQ(ctx.errorAt, 0);

} END_TEST
