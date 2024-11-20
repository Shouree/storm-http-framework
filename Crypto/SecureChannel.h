#pragma once

#ifdef WINDOWS

// Common includes for Windows.
#define SECURITY_WIN32
#include <Security.h>
#include <Schannel.h>

// Missing from time to time.
#ifndef SECBUFFER_ALERT
#define SECBUFFER_ALERT 17
#endif

#include "Data.h"
#include "WinCert.h"
#include "Core/Io/Buffer.h"

namespace ssl {

	class SChannelData;
	class ClientContext;
	class ServerContext;
	class CertificateKey;

	/**
	 * Various types we use on Windows.
	 */

	/**
	 * SSL credentials for SChannel.
	 */
	class SChannelContext : public SSLContext {
	public:
		virtual ~SChannelContext();

		// Acquired credentials.
		CredHandle credentials;

		// SSL Certificate to use, if any.
		RefPtr<WinSSLCert> certificate;

		// Create for a client context.
		static SChannelContext *createClient(ClientContext *context);

		// Create for a server context.
		static SChannelContext *createServer(ServerContext *context, CertificateKey *key);

		// Create a session for this context.
		virtual SSLSession *createSession();

	private:
		// Create.
		SChannelContext(CredHandle handle, bool isServer);

		// Server- or client context?
		bool isServer;
	};


	/**
	 * Data for a SSL session.
	 */
	class SChannelSession : public SSLSession {
	public:
		// Create.
		SChannelSession(SChannelContext *data);

		virtual ~SChannelSession();

		// Implementation of the generic interface.
		virtual Bool more(void *gcData);
		virtual void read(Buffer &to, void *gcData);
		virtual void peek(Buffer &to, void *gcData);
		virtual Nat write(const Buffer &from, Nat start, void *gcData);
		virtual Bool flush(void *gcData);
		virtual void close(void *gcData);

	protected:
		// Owning data, so we don't free it too early.
		SChannelContext *data;

		// Session (called a context here). Initialized when we first update the context.
		CtxtHandle context;

		// Maximum message size for encryption/decryption. Set when the session is established.
		Nat maxMessageSize;

		// Block size for the cipher. Optimal encoding if messages are a size modulo this size (seems to be 16 often)
		Nat blockSize;

		// Buffer sizes for headers and trailers.
		Nat headerSize;
		Nat trailerSize;

		// Initialize a session. Reads data from "input" (empty at first), and writes to
		// "output". The function clears the part of "input" that was consumed, and may leave parts
		// of it there. "output" is to be sent to the remote peer if it contains more than zero bytes.
		// Returns 0 if we're done, <0 if we need to continue, and >0 if we need
		// to get more data. Note: remaining size is just a guess.
		// Note that "output" might need to be sent even if we are done.
		int initSession(Engine &e, Buffer &input, Buffer &output, const wchar *host);

		// Accept a session. Works much the same as "initSession", but does not expect "input" to be
		// empty initially.
		int acceptSession(Engine &e, Buffer &input, Buffer &output);

		// Encrypt a message using this session.
		void encrypt(Engine &e, const Buffer &input, Nat inputOffset, Nat inputLength, Buffer &output);

		// Markers from 'decrypt'.
		struct Markers {
			// Start and end of decrypted plaintext.
			Nat plaintextStart;
			Nat plaintextEnd;

			// Start of any remaining data that was not decrypted.
			Nat remainingStart;

			// Is this the end of the connection?
			Bool shutdown;
		};

		// Decrypt a message using this session. Decryption is performed in-place. Returns a set of
		// markers indicating where in the data different parts can be found.
		Markers decrypt(Engine &e, Buffer &data, Nat offset);

		// Shutdown the channel. Returns a message to send to the remote peer.
		Buffer shutdown(Engine &e);

		// Initialize sizes. Called when the context is established.
		void initSizes();

		// Fill the buffers in "data".
		void fill(SChannelData *data);

		// Check the validity of the remote peer's certificate.
		void checkCertificate(Engine &e);
	};


	/**
	 * Client session.
	 */
	class SChannelClientSession : public SChannelSession {
	public:
		SChannelClientSession(SChannelContext *data);

		// Client-side connect.
		virtual void *connect(IStream *input, OStream *output, Str *host);

		// Shutdown.
		virtual void shutdown(void *gcData);
	};

	/**
	 * Server session.
	 */
	class SChannelServerSession : public SChannelSession {
	public:
		SChannelServerSession(SChannelContext *data);

		// Client-side connect.
		virtual void *connect(IStream *input, OStream *output, Str *host);

		// Shutdown.
		virtual void shutdown(void *gcData);
	};
}

#endif
