#include "stdafx.h"
#include "OpenSSL.h"
#include "OpenSSLError.h"
#include "ClientContext.h"
#include "ServerContext.h"

#ifdef POSIX

#include "Exception.h"
#include "OpenSSLCert.h"
#include "Core/Convert.h"
#include "RuntimeSSL.h"

#include <openssl/err.h>
#include <openssl/opensslv.h>
#include <openssl/opensslconf.h>
#include <openssl/x509v3.h>

#ifndef OPENSSL_THREADS
#error "No thread support in OpenSSL!"
#endif
#if OPENSSL_VERSION_NUMBER < 0x10100000
#error "Too old OpenSSL version, we rely on some locking support!"
#endif

namespace ssl {

	/**
	 * Initialze the library properly.
	 */
	class Initializer {
	public:
		Initializer() {
			initRuntimeSSL();

			SSL_library_init();
			SSL_load_error_strings();
			CONF_modules_load_file(NULL, NULL, 0);

			// From the headers of OpenSSL, it seems like the requirements of locking functions was
			// removed earlier. As such, we don't need to give them anymore.
		}
	};

	void init() {
		static Initializer i;
	}

	static int verifyCallback(int preverifyOk, X509_STORE_CTX *x509) {
		// This is where we would check any of our own certificates.
		// See the manpage for SSL_CTX_set_verify for an example of how to get access to some context in here.

		// 0 - fail immediately
		// 1 - continue
		if (preverifyOk != 1)
			return 0;
		return 1;
	}

#define EXCLUDE ":!aNULL:!kRSA:!PSK:!MD5";
	static const char *defaultCiphers = "DEFAULT" EXCLUDE;
	static const char *strongCiphers = "HIGH:!RC4" EXCLUDE;

	OpenSSLContext *OpenSSLContext::createClient(ClientContext *context) {
		init();
		// Note: This means SSLv3 or TLSv1.x, as available.
		OpenSSLContext *ctx = new OpenSSLContext(SSL_CTX_new(TLS_client_method()), false);

		SSL_CTX_set_verify(ctx->context, SSL_VERIFY_PEER, verifyCallback);
		SSL_CTX_set_verify_depth(ctx->context, 10); // Should be enough.

		long options = SSL_OP_NO_SSLv2;
		if (context->strongCiphers())
			options |= SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1;
		SSL_CTX_set_options(ctx->context, options);

		// Allowed ciphers. We should probably modify this list a bit...
		if (context->strongCiphers())
			SSL_CTX_set_cipher_list(ctx->context, strongCiphers);
		else
			SSL_CTX_set_cipher_list(ctx->context, defaultCiphers);


		if (Certificate *cert = context->pinnedCertificate()) {
			// Only trust the supplied certificate:
			RefPtr<OpenSSLCert> pinned = cert->get()->openSSL();

			X509_STORE *store = X509_STORE_new();

			// Does not take ownership of the cert.
			X509_STORE_add_cert(store, pinned->data);

			// Takes ownership of "store".
			SSL_CTX_set0_verify_cert_store(ctx->context, store);
		} else {
			// Use default paths for certificates.
			SSL_CTX_set_default_verify_paths(ctx->context);
		}

		ctx->checkHostname = context->verifyHostname();

		return ctx;
	}

	OpenSSLContext *OpenSSLContext::createServer(ServerContext *context, CertificateKey *key) {
		init();

		RefPtr<OpenSSLCert> oCert = key->certificate()->get()->openSSL();
		RefPtr<OpenSSLCertKey> oKey = key->get()->openSSL();

		OpenSSLContext *ctx = new OpenSSLContext(SSL_CTX_new(TLS_server_method()), true);

		SSL_CTX_set_verify(ctx->context, SSL_VERIFY_NONE, NULL);

		long options = SSL_OP_NO_SSLv2;
		if (context->strongCiphers())
			options |= SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1;
		SSL_CTX_set_options(ctx->context, options);

		// Allowed ciphers. We should probably modify this list a bit...
		if (context->strongCiphers())
			SSL_CTX_set_cipher_list(ctx->context, strongCiphers);
		else
			SSL_CTX_set_cipher_list(ctx->context, defaultCiphers);

		SSL_CTX_use_certificate(ctx->context, oCert->data);
		SSL_CTX_use_PrivateKey(ctx->context, oKey->data);

		return ctx;
	}

	OpenSSLContext::OpenSSLContext(SSL_CTX *ctx, bool server)
		: context(ctx), checkHostname(false), isServer(server) {

		if (!ctx)
			throw new (runtime::someEngine()) SSLError(S("Failed to create OpenSSL context."));
	}

	OpenSSLContext::~OpenSSLContext() {
		SSL_CTX_free(context);
	}

	SSLSession *OpenSSLContext::createSession() {
		if (isServer)
			return new OpenSSLServerSession(this);
		else
			return new OpenSSLClientSession(this);
	}

	/**
	 * Data structure containing GC pointers that we allocate for our BIO class.
	 */
	struct BIO_data {
		IStream *input;
		OStream *output;

		// Buffer to support "peek".
		GcArray<byte> *buffer;

		// Number of bytes we have consumed inside 'buffer'.
		Nat consumed;
	};

	// Pointer offsets in BIO_data
	static const size_t BIO_data_offsets[] = {
		OFFSET_OF(BIO_data, input),
		OFFSET_OF(BIO_data, output),
		OFFSET_OF(BIO_data, buffer),
	};

	// GCtype for the data.
	class BioType {
	public:
		BioType() {
			type = (GcType *)malloc(gcTypeSize(ARRAY_COUNT(BIO_data_offsets)));
			type->kind = GcType::tFixed;
			type->type = null;
			type->finalizer = null;
			type->stride = sizeof(BIO_data);
			type->count = ARRAY_COUNT(BIO_data_offsets);
			for (size_t i = 0; i < ARRAY_COUNT(BIO_data_offsets); i++)
				type->offset[i] = BIO_data_offsets[i];
		}
		~BioType() {
			free(type);
		}

		GcType *type;
	};

	// Allocate BIO data for streams. Ensures that this data is not moved by the GC. It still needs
	// to be kept alive somehow.
	static BIO_data *allocData(IStream *input, OStream *output) {
		static BioType t;
		BIO_data *data = (BIO_data *)runtime::allocStaticRaw(input->engine(), t.type);
		data->input = input;
		data->output = output;
		return data;
	}

	/**
	 * A BIO wrapper that forwards to/from our streams.
	 */

	// Maximum number of bytes to read/write in one go.
	static int bioWrite(BIO *bio, const char *src, int length) {
		BIO_data *data = (BIO_data *)BIO_get_data(bio);
		GcPreArray<Byte, 4096> buffer;

		size_t offset = 0;
		while (offset < size_t(length)) {
			size_t toWrite = min(buffer.count, size_t(length) - offset);
			memcpy(buffer.v, src + offset, toWrite);

			Buffer b = fullBuffer(buffer);
			b.filled(toWrite);
			data->output->write(b);

			offset += toWrite;
		}

		return length;
	}

	static int bioRead(BIO *bio, char *dest, int length) {
		BIO_data *data = (BIO_data *)BIO_get_data(bio);

		// Note: Will ensure so that 'size' is not too large.
		GcPreArray<Byte, 4096> buffer(length);

		// Note: This implementation might give to little data back, but that is fine.
		Buffer b = emptyBuffer(buffer);
		b = data->input->read(b);
		memcpy(dest, b.dataPtr(), b.filled());

		return int(b.filled());
	}

	static int bioPuts(BIO *bio, const char *buffer) {
		return bioWrite(bio, buffer, strlen(buffer));
	}

	static int bioGets(BIO *bio, char *buffer, int size) {
		// TODO: Implement this... It should work like BIO_puts()
		// I don't think it is strictly necessary though. The network BIOs do not
		// support gets, so we're probably fine.
		return 0;
	}

	static long int bioCtrl(BIO *bio, int cmd, long larg, void *parg) {
		BIO_data *data = (BIO_data *)BIO_get_data(bio);

		switch (cmd) {
		case BIO_CTRL_RESET:
			// PLN("BIO_CTRL_RESET");
			break;
		case BIO_CTRL_EOF:
			// PLN("BIO_CTRL_EOF");
			break;
		case BIO_CTRL_SET_CLOSE:
			// PLN("BIO_CTRL_SET_CLOSE");
			break;
		case BIO_CTRL_GET_CLOSE:
			// PLN("BIO_CTRL_GET_CLOSE");
			break;
		case BIO_CTRL_PENDING:
			// PLN("BIO_CTRL_PENDING");
			break;
		case BIO_CTRL_WPENDING:
			// PLN("BIO_CTRL_WPENDING");
			break;
		case BIO_CTRL_FLUSH:
			data->output->flush();
			return 1;
		case BIO_CTRL_GET_CALLBACK:
			// PLN("BIO_CTRL_GET_CALLBACK");
			break;
		case BIO_CTRL_SET_CALLBACK:
			// PLN("BIO_CTRL_SET_CALLBACK");
			break;
		}
		return 0;
	}

	static long int bioCtrlCb(BIO *bio, int cmd, BIO_info_cb *cb) {
		// PVAR(cmd);
		return 0;
	}

	class StormBioMethod {
	public:
		StormBioMethod() {
			int type = BIO_get_new_index();
			method = BIO_meth_new(BIO_TYPE_SOURCE_SINK | type, "STORM I/O");

			// We could call BIO_meth_free() at shutdown...

			// Note: There are *_ex versions of read and write. Maybe we should use them instead?
			BIO_meth_set_write(method, bioWrite);
			BIO_meth_set_read(method, bioRead);
			BIO_meth_set_puts(method, bioPuts);
			BIO_meth_set_gets(method, bioGets);
			BIO_meth_set_ctrl(method, bioCtrl);
			BIO_meth_set_callback_ctrl(method, bioCtrlCb);
			// There are create and destroy as well, but I don't think we need them.
		}

		~StormBioMethod() {
			BIO_meth_free(method);
		}

		BIO_METHOD *method;
	};

	// Create a BIO that wraps a Storm istream and an ostream.
	// Assumes that "data" is kept alive by storing a reference to it somewhere else.
	static BIO *BIO_new_storm(BIO_data *data) {
		static StormBioMethod m;
		BIO *bio = BIO_new(m.method);
		BIO_set_data(bio, data);
		return bio;
	}

	// We can chain BIOs using:
	// con = BIO_new(stormBio)
	// ssl = BIO_new_ssl(ctx, 1) // 1 is for clients
	// result = BIO_push(ssl, con)

	OpenSSLSession::OpenSSLSession(OpenSSLContext *ctx) : context(ctx), connection(null), eof(false) {
		context->ref();
	}

	OpenSSLSession::~OpenSSLSession() {
		context->unref();
	}

	Bool OpenSSLSession::more(void *data) {
		os::Lock::L z(lock);
		BIO_data *d = (BIO_data *)data;

		if (d->buffer != null && d->consumed < d->buffer->filled)
			return true;

		return !eof;
	}

	void OpenSSLSession::read(Buffer &to, void *data) {
		os::Lock::L z(lock);
		BIO_data *d = (BIO_data *)data;

		if (d->buffer != null && d->consumed < d->buffer->filled) {
			// Read data from the buffer first.
			Nat copy = min(to.free(), Nat(d->buffer->filled) - d->consumed);
			memcpy(to.dataPtr() + to.filled(), d->buffer->v + d->consumed, copy);
			to.filled(to.filled() + copy);
			d->consumed += copy;

			if (d->consumed >= d->buffer->filled) {
				// We don't need the buffer anymore. Free it.
				d->consumed = 0;
				d->buffer = null;
			}
		}

		// Still more to read?
		if (to.free() > 0) {
			int bytes = BIO_read(connection, to.dataPtr() + to.filled(), to.free());
			if (bytes > 0) {
				to.filled(to.filled() + bytes);
			} else if (bytes == 0) {
				// We are at EOF!
				eof = true;
			} else {
				// Error.
				checkError();
			}
		}
	}

	void OpenSSLSession::peek(Buffer &to, void *data) {
		os::Lock::L z(lock);
		BIO_data *d = (BIO_data *)data;

		if (d->buffer == null || d->buffer->filled - d->consumed < to.free()) {
			fillBuffer(to.free(), data);
		}

		Nat copy = min(to.free(), Nat(d->buffer->filled) - d->consumed);
		memcpy(to.dataPtr() + to.filled(), d->buffer->v + d->consumed, copy);
		to.filled(to.filled() + copy);
	}

	void OpenSSLSession::fillBuffer(Nat bytes, void *data) {
		BIO_data *d = (BIO_data *)data;
		Engine &e = d->input->engine();

		if (d->buffer == null) {
			d->buffer = runtime::allocArray<byte>(e, &byteArrayType, bytes);
			d->consumed = 0;
		} else if (d->buffer->count < bytes) {
			GcArray<byte> *b = runtime::allocArray<byte>(e, &byteArrayType, bytes);
			memcpy(b->v, d->buffer->v + d->consumed, d->buffer->filled - d->consumed);
			b->filled = d->buffer->filled - d->consumed;
			d->consumed = 0;
			d->buffer = b;
		} else {
			memmove(d->buffer->v, d->buffer->v + d->consumed, d->buffer->filled - d->consumed);
			d->buffer->filled -= d->consumed;
			d->consumed = 0;
		}

		// Now, we have enough free space!
		// Note: "consumed" is zero by now.
		if (bytes > d->buffer->filled) {
			int read = BIO_read(connection, d->buffer->v + d->buffer->filled, bytes - d->buffer->filled);
			if (read > 0) {
				d->buffer->filled += read;
			} else if (read == 0) {
				eof = true;
			} else {
				checkError();
			}
		}
	}

	void OpenSSLSession::write(const Buffer &from, Nat offset, void *) {
		os::Lock::L z(lock);

		BIO_write(connection, from.dataPtr() + offset, from.filled() - offset);
	}

	void OpenSSLSession::flush(void *) {
		os::Lock::L z(lock);

		BIO_flush(connection);
	}

	void OpenSSLSession::shutdown(void *gcData) {
		os::Lock::L z(lock);

		SSL *ssl = null;
		BIO_get_ssl(connection, &ssl);
		SSL_shutdown(ssl);

		// Note: We might get more data from the remote peer. This only shuts down our end essentially.
	}

	void OpenSSLSession::close(void *gcData) {
		os::Lock::L z(lock);

		// TODO: We could do this through the BIO interface.
		BIO_data *data = (BIO_data *)gcData;
		data->input->close();
		data->output->close();
	}


	/**
	 * Client session.
	 */

	OpenSSLClientSession::OpenSSLClientSession(OpenSSLContext *c) : OpenSSLSession(c) {}

	void *OpenSSLClientSession::connect(IStream *input, OStream *output, Str *host) {
		os::Lock::L z(lock);

		BIO_data *data = allocData(input, output);
		BIO *io = BIO_new_storm(data);
		BIO *sslBio = BIO_new_ssl(context->context, 1); // 1 means "client".
		// According to what OpenSSL is doing, we don't need to free "io" manually. The docs are a
		// bit unclear about that though (at least the "BIO_push" manpage).
		connection = BIO_push(sslBio, io);

		SSL *ssl = null;
		BIO_get_ssl(connection, &ssl);

		const char *utf8Host = host->utf8_str();

		if (context->checkHostname) {
			X509_VERIFY_PARAM *param = SSL_get0_param(ssl);
			X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
			X509_VERIFY_PARAM_set1_host(param, utf8Host, 0);
		}

		SSL_set_tlsext_host_name(ssl, utf8Host);
		checkError();

		// Do the handshake!
		if (BIO_do_handshake(connection) != 1) {
			checkError();
		}

		X509 *cert = SSL_get_peer_certificate(ssl);
		if (!cert)
			throw new (input) SSLError(S("The remote host did not present a certificate!"));
		X509_free(cert);

		long certOk = SSL_get_verify_result(ssl);
		if (certOk != X509_V_OK) {
			// TODO: We might want to get more descriptive errors here.
			throw new (input) SSLError(TO_S(input, S("Failed to verify the remote peer: ") << certError(certOk)));
		}

		return data;
	}


	/**
	 * Server session.
	 */

	OpenSSLServerSession::OpenSSLServerSession(OpenSSLContext *c) : OpenSSLSession(c) {}

	void *OpenSSLServerSession::connect(IStream *input, OStream *output, Str *host) {
		os::Lock::L z(lock);

		BIO_data *data = allocData(input, output);
		BIO *io = BIO_new_storm(data);
		BIO *sslBio = BIO_new_ssl(context->context, 0); // 0 means "server".
		// According to what OpenSSL is doing, we don't need to free "io" manually. The docs are a
		// bit unclear about that though (at least the "BIO_push" manpage).
		connection = BIO_push(sslBio, io);

		checkError();

		// Do the handshake!
		if (BIO_do_handshake(connection) != 1) {
			checkError();
		}

		return data;
	}

}

#endif
