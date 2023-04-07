#pragma once

#ifdef POSIX
#include "Data.h"
#include "Core/Io/Buffer.h"
#include "Core/Io/Stream.h"

#include <openssl/ssl.h>
#include <openssl/conf.h>
#include <openssl/crypto.h>

namespace ssl {

	class ClientContext;
	class ServerContext;
	class CertificateKey;

	// Ensure the library is initialized.
	void init();

	/**
	 * OpenSSL context.
	 */
	class OpenSSLContext : public SSLContext {
	public:
		// Destroy.
		~OpenSSLContext();

		// Create a client context.
		static OpenSSLContext *createClient(ClientContext *context);

		// Create a server context.
		static OpenSSLContext *createServer(ServerContext *context, CertificateKey *key);

		// Our OpenSSL context.
		SSL_CTX *context;

		// Verify hostname?
		bool checkHostname;

		// Create a session.
		virtual SSLSession *createSession();

	private:
		// Create.
		OpenSSLContext(SSL_CTX *ctx, bool isServer);

		// Server context?
		bool isServer;
	};

	/**
	 * OpenSSL session.
	 */
	class OpenSSLSession : public SSLSession {
	public:
		// Create from a context.
		OpenSSLSession(OpenSSLContext *ctx);

		// Destroy.
		virtual ~OpenSSLSession();

		// Implementation of the generic interface.
		virtual Bool more(void *gcData);
		virtual void read(Buffer &to, void *gcData);
		virtual void peek(Buffer &to, void *gcData);
		virtual void write(const Buffer &from, Nat start, void *gcData);
		virtual void flush(void *gcData);
		virtual void shutdown(void *gcData);
		virtual void close(void *gcData);

	protected:
		// Session.
		OpenSSLContext *context;

		// Allocated BIO for the SSL connection.
		BIO *connection;

		// Did we see an end-of-file?
		Bool eof;

		// Fill the buffer.
		void fillBuffer(Nat bytes, void *data);
	};

	/**
	 * OpenSSL client session.
	 */
	class OpenSSLClientSession : public OpenSSLSession {
	public:
		// Create.
		OpenSSLClientSession(OpenSSLContext *ctx);

		// Overrides.
		virtual void *connect(IStream *input, OStream *output, Str *host);
	};

	/**
	 * OpenSSL server session.
	 */
	class OpenSSLServerSession : public OpenSSLSession {
	public:
		// Create.
		OpenSSLServerSession(OpenSSLContext *ctx);

		// Overrides.
		virtual void *connect(IStream *input, OStream *output, Str *host);
	};

}

#endif
