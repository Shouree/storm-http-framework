#include "stdafx.h"
#include "SecureChannel.h"
#include "Exception.h"
#include "SecureChannelData.h"
#include "WinErrorMsg.h"
#include "ClientContext.h"
#include "ServerContext.h"

#ifdef WINDOWS

namespace ssl {

	// Inspiration for this implementation:
	// Racket: https://github.com/racket/racket/blob/master/racket/collects/net/win32-ssl.rkt
	// MSDN: https://docs.microsoft.com/en-us/windows/win32/secauthn/creating-a-secure-connection-using-schannel

	// Note: We could use InitSecurityInterface and use the function table returned from there. I'm
	// not sure what is the main benefit of that though.

#ifndef SCH_USE_STRONG_CRYPTO
#define SCH_USE_STRONG_CRYPTO 0x00400000
#endif

	SChannelContext::SChannelContext(CredHandle creds, bool isServer)
		: credentials(creds), certificate(null), isServer(isServer) {}

	SChannelContext::~SChannelContext() {
		FreeCredentialsHandle(&credentials);
	}

	SChannelContext *SChannelContext::createClient(ClientContext *context) {
		SECURITY_STATUS status;
		CredHandle credentials;

		DWORD flags = 0;

		// This is what OpenSSL does - it does not attempt to recover if the client does not send a complete chain.
		flags |= SCH_CRED_NO_DEFAULT_CREDS;

		if (context->strongCiphers())
			flags |= SCH_USE_STRONG_CRYPTO;
		if (!context->verifyHostname())
			flags |= SCH_CRED_NO_SERVERNAME_CHECK;
		if (context->pinnedCertificate())
			flags |= SCH_CRED_MANUAL_CRED_VALIDATION;

		SCHANNEL_CRED data = {
			SCHANNEL_CRED_VERSION,
			0, // Number of private keys
			NULL, // Array of private keys
			NULL, // Root store
			0, // Reserved
			0, // Reserved
			0, // Number of algorithms to use (0 = system default)
			NULL, // Algorithms to use (0 = system default)
			0, // Protocols (we should set it to zero to let the system decide). This is where we specify TLS version.
			0, // Cipher strenght. 0 = system default
			0, // Session lifespan. 0 = default (10 hours)
			flags, // Various flags. We might want to alter this later.
			0, // Kernel format. Should be zero.
		};

		// Acquire credentials.
		status = AcquireCredentialsHandle(NULL, SCHANNEL_NAME, SECPKG_CRED_OUTBOUND,
										NULL, &data, NULL, NULL,
										&credentials, NULL);

		if (status == SEC_E_SECPKG_NOT_FOUND)
			throw new (runtime::someEngine()) SSLError(S("Failed to find the SChannel security package."));
		else if (status != SEC_E_OK)
			throwError(runtime::someEngine(), S("Failed to acquire credentials for SChannel: "), status);

		SChannelContext *result = new SChannelContext(credentials, false);
		try {
			if (Certificate *cert = context->pinnedCertificate())
				result->certificate = cert->get()->windows();
			return result;
		} catch (...) {
			delete result;
			throw;
		}
	}

	SChannelContext *SChannelContext::createServer(ServerContext *context, CertificateKey *key) {
		SECURITY_STATUS status;
		CredHandle credentials;

		// According to the documentation, keys should be in "array of private keys", not "root
		// store".  The root store is used to verify clients.
		RefPtr<WinSSLCertKey> certKey = key->get()->windows();
		RefPtr<WinSSLCert> cert = key->certificate()->get()->windows();

		DWORD flags = 0;
		if (context->strongCiphers())
			flags |= SCH_USE_STRONG_CRYPTO;

		// We (sadly) need to make a copy of the certificate since we will attach it with a
		// key. That modifies the representation, and thus we might run into concurrency issues.
		// I have no idea where the linking is actually stored, as CERT_CONTEXT is always const
		// and seemingly lacks a location for this data to be stored.
		cert = cert->copy();

		{
			PCCERT_CONTEXT certificate = cert->data;

			// Note: We keep this alive for as little as possible, in case we crash somewhere.
			WinKeyStore store(certKey.get(), false);
			store.attach(certificate);

			SCHANNEL_CRED data = {
				SCHANNEL_CRED_VERSION,
				1, // Number of private keys
				&certificate, // Array of private keys
				NULL, // Root store
				0, // Reserved
				0, // Reserved
				0, // Number of algorithms to use (0 = system default)
				NULL, // Algorithms to use (0 = system default)
				0, // Protocols (we should set it to zero to let the system decide). This is where we specify TLS version.
				0, // Cipher strenght. 0 = system default
				0, // Session lifespan. 0 = default (10 hours)
				flags, // Various flags.
				0, // Kernel format. Should be zero.
			};

			// TODO: Add flags.

			status = AcquireCredentialsHandle(NULL, SCHANNEL_NAME, SECPKG_CRED_INBOUND,
											NULL, &data, NULL, NULL,
											&credentials, NULL);
		}

		if (status == SEC_E_SECPKG_NOT_FOUND)
			throw new (runtime::someEngine()) SSLError(S("Failed to find the SChannel security package."));
		else if (status != SEC_E_OK)
			throwError(runtime::someEngine(), S("Failed to acquire credentials for SChannel: "), status);

		return new SChannelContext(credentials, true);
	}

	SSLSession *SChannelContext::createSession() {
		if (isServer)
			return new SChannelServerSession(this);
		else
			return new SChannelClientSession(this);
	}


	SChannelSession::SChannelSession(SChannelContext *data)
		: data(data), maxMessageSize(0), blockSize(0) {

		data->ref();
		SecInvalidateHandle(&context);
	}

	SChannelSession::~SChannelSession() {
		data->unref();
		if (SecIsValidHandle(&context))
			DeleteSecurityContext(&context);
	}

	int SChannelSession::initSession(Engine &e, Buffer &inBuffer, Buffer &outBuffer, const wchar *host) {
		SECURITY_STATUS status;
		// Ask it to allocate output buffers for us. That means we don't have to worry about the size of them.
		ULONG requirements = ISC_REQ_ALLOCATE_MEMORY;
		// Some more good features.
		requirements |= ISC_REQ_REPLAY_DETECT | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_CONFIDENTIALITY | ISC_REQ_STREAM;

		// Don't validate the server automatically if we have provided a pinned certificate:
		if (data->certificate)
			requirements |= ISC_REQ_MANUAL_CRED_VALIDATION;

		ULONG attrsOut = 0;
		CtxtHandle *currentHandle = null;
		SecBufferDesc *inputParam = null;

		SecBuffer inBuffers[2] = {
			{ 0, SECBUFFER_TOKEN, null },
			{ 0, SECBUFFER_EMPTY, null }
		};
		SecBufferDesc input = {
			SECBUFFER_VERSION, 2, inBuffers
		};

		if (!SecIsValidHandle(&context)) {
			// First call.
			currentHandle = null;
			inputParam = null;
		} else {
			// Other calls.
			currentHandle = &context;
			inputParam = &input;
			inBuffers[0].cbBuffer = inBuffer.filled();
			inBuffers[0].pvBuffer = inBuffer.dataPtr();
		}

		// Note: Doc says we need a SECBUFFER_ALERT, but it works anyway.
		SecBuffer outBuffers[1] = { { 0, SECBUFFER_EMPTY, null } };
		SecBufferDesc output = {
			SECBUFFER_VERSION, 1, outBuffers,
		};

		status = InitializeSecurityContext(&data->credentials, currentHandle,
										(SEC_WCHAR *)host, requirements,
										0 /* reserved */, 0 /* not used for schannel */,
										inputParam, 0 /* reserved */,
										&context, &output,
										&attrsOut, NULL /* expires */);

		// See if there is any unconsumed data in the input buffer.
		if (input.pBuffers[1].BufferType == SECBUFFER_EXTRA) {
			// Shift data back and keep unconsumed data in the beginning of "input".
			inBuffer.shift(inBuffer.filled() - input.pBuffers[1].cbBuffer);
		} else if (status == SEC_E_INCOMPLETE_MESSAGE) {
			// It was incomplete, just don't clear the buffer.
		} else {
			// No unconsumed data. Clear it.
			inBuffer.filled(0);
		}

		// Did we get a token to send to the remote peer?
		if (outBuffers[0].BufferType == SECBUFFER_TOKEN) {
			SecBuffer b = output.pBuffers[0];
			if (outBuffer.count() < b.cbBuffer)
				outBuffer = storm::buffer(e, b.cbBuffer);

			memcpy(outBuffer.dataPtr(), b.pvBuffer, b.cbBuffer);
			outBuffer.filled(b.cbBuffer);
		} else {
			outBuffer.filled(0);
		}

		int result = 0;
		bool error = false;
		if (status == SEC_I_CONTINUE_NEEDED) {
			// Not done. More messages.
			result = -1;
		} else if (status == SEC_E_OK) {
			// All done, set the maximum size for encryption/decryption.
			checkCertificate(e);

			initSizes();
			result = 0;
		} else if (status == SEC_E_INCOMPLETE_MESSAGE) {
			if (input.pBuffers[1].BufferType == SECBUFFER_MISSING) {
				result = input.pBuffers[1].cbBuffer;
			} else {
				// Just provide a decent guess if we cant find a guess from the API.
				result = 32;
			}
		} else {
			error = true;
		}

		// Clean up.
		if (output.pBuffers[0].pvBuffer)
			FreeContextBuffer(output.pBuffers[0].pvBuffer);

		if (error)
			throwError(e, S("Failed to initialize an SSL session: "), status);

		return result;
	}

	int SChannelSession::acceptSession(Engine &e, Buffer &inBuffer, Buffer &outBuffer) {
		SECURITY_STATUS status;
		// Ask it to allocate output buffers for us. That means we don't have to worry about the size of them.
		ULONG requirements = ISC_REQ_ALLOCATE_MEMORY;
		// Some more good features.
		requirements |= ISC_REQ_REPLAY_DETECT | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_CONFIDENTIALITY | ISC_REQ_STREAM;

		ULONG attrsOut = 0;
		CtxtHandle *currentHandle = null;

		SecBuffer inBuffers[2] = {
			{ 0, SECBUFFER_TOKEN, null },
			{ 0, SECBUFFER_EMPTY, null }
		};
		SecBufferDesc input = {
			SECBUFFER_VERSION, 2, inBuffers
		};

		if (!SecIsValidHandle(&context)) {
			// First call.
			currentHandle = null;
		} else {
			// Other calls.
			currentHandle = &context;
		}

		inBuffers[0].cbBuffer = inBuffer.filled();
		inBuffers[0].pvBuffer = inBuffer.dataPtr();

		// Note: Doc says we need a SECBUFFER_ALERT, but it works anyway.
		SecBuffer outBuffers[1] = { { 0, SECBUFFER_EMPTY, null } };
		SecBufferDesc output = {
			SECBUFFER_VERSION, 1, outBuffers,
		};

		status = AcceptSecurityContext(&data->credentials, currentHandle,
									&input, requirements,
									0 /* not used for schannel */,
									&context, &output,
									&attrsOut, NULL /* expires */);

		// See if there is any unconsumed data in the input buffer.
		if (input.pBuffers[1].BufferType == SECBUFFER_EXTRA) {
			// Shift data back and keep unconsumed data in the beginning of "input".
			inBuffer.shift(inBuffer.filled() - input.pBuffers[1].cbBuffer);
		} else if (status == SEC_E_INCOMPLETE_MESSAGE) {
			// It was incomplete, just don't clear the buffer.
		} else {
			// No unconsumed data. Clear it.
			inBuffer.filled(0);
		}

		// Did we get a token to send to the remote peer?
		if (outBuffers[0].BufferType == SECBUFFER_TOKEN) {
			SecBuffer b = output.pBuffers[0];
			if (outBuffer.count() < b.cbBuffer)
				outBuffer = storm::buffer(e, b.cbBuffer);

			memcpy(outBuffer.dataPtr(), b.pvBuffer, b.cbBuffer);
			outBuffer.filled(b.cbBuffer);
		} else {
			outBuffer.filled(0);
		}

		int result = 0;
		bool error = false;
		if (status == SEC_I_CONTINUE_NEEDED) {
			// Not done. More messages.
			result = -1;
		} else if (status == SEC_E_OK) {
			// All done, set the maximum size for encryption/decryption.
			initSizes();
			result = 0;
		} else if (status == SEC_E_INCOMPLETE_MESSAGE) {
			if (input.pBuffers[1].BufferType == SECBUFFER_MISSING) {
				result = input.pBuffers[1].cbBuffer;
			} else {
				// Just provide a decent guess if we cant find a guess from the API.
				result = 32;
			}
		} else {
			error = true;
		}

		// Clean up.
		if (output.pBuffers[0].pvBuffer)
			FreeContextBuffer(output.pBuffers[0].pvBuffer);

		if (error) {
			if (status == SEC_E_ALGORITHM_MISMATCH)
				throw new (e) SSLError(S("Failed to initialize an SSL session: SEC_E_ALGORITHM_MISMATCH. This could be due to a missing server side certificate."));
			else
				throwError(e, S("Failed to initialize an SSL session: "), status);
		}

		return result;
	}

	void SChannelSession::checkCertificate(Engine &e) {
		// We use auto-mode!
		if (!data->certificate)
			return;

		PCCERT_CONTEXT pinned = data->certificate->data;

		PCCERT_CONTEXT check = 0;
		SECURITY_STATUS status = QueryContextAttributes(&context, SECPKG_ATTR_REMOTE_CERT_CONTEXT, &check);

		bool ok = true;
		if (pinned->dwCertEncodingType != check->dwCertEncodingType)
			ok = false;
		if (pinned->cbCertEncoded != check->cbCertEncoded)
			ok = false;

		if (ok)
			ok = memcmp(pinned->pbCertEncoded, check->pbCertEncoded, pinned->cbCertEncoded) == 0;

		CertFreeCertificateContext(check);

		if (!ok)
			throw new (e) SSLError(S("The certificate presented by the remote peer does not match the one we expected."));
	}

	void SChannelSession::encrypt(Engine &e, const Buffer &input, Nat offset, Nat inputSize, Buffer &output) {
		// "reserve" size for header and trailer.
		Nat size = headerSize + trailerSize + inputSize;
		if (output.count() < size)
			output = buffer(e, size);

		output.filled(size);
		memcpy(output.dataPtr() + headerSize, input.dataPtr() + offset, inputSize);

		SecBuffer buffers[4] = {
			{ headerSize, SECBUFFER_STREAM_HEADER, output.dataPtr() },
			{ inputSize, SECBUFFER_DATA, output.dataPtr() + headerSize },
			{ trailerSize, SECBUFFER_STREAM_TRAILER, output.dataPtr() + headerSize + inputSize },
			{ 0, SECBUFFER_EMPTY, null },
		};
		SecBufferDesc bufferDesc = {
			SECBUFFER_VERSION, ARRAY_COUNT(buffers), buffers
		};
		SECURITY_STATUS status = EncryptMessage(&context, 0, &bufferDesc, 0);
		if (status != SEC_E_OK)
			throwError(e, S("Failed to encrypt a message: "), status);
	}

	static SecBuffer *findBuffer(SecBufferDesc &in, unsigned long type) {
		for (unsigned long i = 0; i < in.cBuffers; i++) {
			if (in.pBuffers[i].BufferType == type)
				return &in.pBuffers[i];
		}
		return null;
	}

	SChannelSession::Markers SChannelSession::decrypt(Engine &e, Buffer &data, Nat offset) {
		SecBuffer buffers[4] = {
			{ data.filled() - offset, SECBUFFER_DATA, data.dataPtr() + offset },
			{ 0, SECBUFFER_EMPTY, null },
			{ 0, SECBUFFER_EMPTY, null },
			{ 0, SECBUFFER_EMPTY, null },
		};
		SecBufferDesc bufferDesc = {
			SECBUFFER_VERSION, ARRAY_COUNT(buffers), buffers
		};
		SECURITY_STATUS status = DecryptMessage(&context, &bufferDesc, 0, NULL);

		Markers m = { offset, offset, offset, false };

		if (status == SEC_E_OK) {
			{
				SecBuffer *plaintext = findBuffer(bufferDesc, SECBUFFER_DATA);
				m.plaintextStart = Nat((byte *)plaintext->pvBuffer - data.dataPtr());
				m.plaintextEnd = m.plaintextStart + plaintext->cbBuffer;
			}

			if (SecBuffer *ciphertext = findBuffer(bufferDesc, SECBUFFER_EXTRA)) {
				m.remainingStart = Nat((byte *)ciphertext->pvBuffer - data.dataPtr());
			} else {
				m.remainingStart = data.filled();
			}
		} else if (status == SEC_E_INCOMPLETE_MESSAGE) {
			// Tell the caller that they need to get more data.
			// We don't need to do anything here due to the initialization above.
		} else if (status == SEC_I_CONTEXT_EXPIRED) {
			// The remote end shut the connection down.
			m.shutdown = true;
		} else {
			throwError(e, S("Failed to decrypt a message: "), status);
		}

		return m;
	}

	Buffer SChannelSession::shutdown(Engine &e) {
		{
			DWORD token = SCHANNEL_SHUTDOWN;
			SecBuffer buffers[1] = {
				{ sizeof(DWORD), SECBUFFER_TOKEN, &token }
			};
			SecBufferDesc bufferDesc = {
				SECBUFFER_VERSION, ARRAY_COUNT(buffers), buffers
			};
			SECURITY_STATUS status = ApplyControlToken(&context, &bufferDesc);
			if (status != SEC_E_OK)
				throwError(e, S("Failed to shutdown: "), status);
		}

		// Now, we just need to call initSession(?) to get what to reply!
		// TODO: We should call initSession for server/client as appropriate.
		// TODO: Doc says we might need to loop this. Based on the example
		// from http://www.coastrd.com/c-schannel-smtp, it seems unlikely for SChannel.
		Buffer input;
		Buffer output;
		int ok = initSession(e, input, output, null);
		if (ok != 0)
			WARNING(L"We might need to do something differently!");
		return output;
	}

	void SChannelSession::initSizes() {
		SecPkgContext_StreamSizes ss;
		QueryContextAttributes(&context, SECPKG_ATTR_STREAM_SIZES, &ss);
		maxMessageSize = ss.cbMaximumMessage;
		blockSize = ss.cbBlockSize;
		headerSize = ss.cbHeader;
		trailerSize = ss.cbTrailer;
	}

	/**
	 * The "public" API:
	 */

	Bool SChannelSession::more(void *gcData) {
		SChannelData *data = (SChannelData *)gcData;
		os::Lock::L z(lock);

		// More decrypted data?
		if (data->decryptedStart < data->decryptedEnd)
			return true;

		// If the remote end is shutdown, we won't get more data.
		if (data->incomingEnd)
			return false;

		// Otherwise, see if we have more data to decrypt, or if there is more in the source.
		return (data->remainingStart < data->incoming.filled()) // more encrypted data?
			|| data->src->more(); // more from the source stream?

		return true;
	}

	void SChannelSession::read(Buffer &to, void *gcData) {
		SChannelData *data = (SChannelData *)gcData;
		os::Lock::L z(lock);

		// Nothing to read?
		if (to.free() == 0)
			return;

		if (data->decryptedStart >= data->decryptedEnd)
			fill(data);

		Nat toFill = min(to.free(), data->decryptedEnd - data->decryptedStart);
		memcpy(to.dataPtr() + to.filled(), data->incoming.dataPtr() + data->decryptedStart, toFill);
		to.filled(to.filled() + toFill);
		data->decryptedStart += toFill;
	}

	void SChannelSession::peek(Buffer &to, void *gcData) {
		// TODO: More generous peeking, perhaps?

		SChannelData *data = (SChannelData *)gcData;
		os::Lock::L z(lock);

		if (data->decryptedStart >= data->decryptedEnd)
			fill(data);

		Nat toFill = min(to.free(), data->decryptedEnd - data->decryptedStart);
		memcpy(to.dataPtr() + to.filled(), data->incoming.dataPtr() + data->decryptedStart, toFill);
		to.filled(to.filled() + toFill);
	}

	void SChannelSession::write(const Buffer &from, Nat offset, void *gcData) {
		SChannelData *data = (SChannelData *)gcData;
		os::Lock::L z(lock);

		if (data->outgoingEnd)
			return;

		// Don't send more data than the max message size.
		// For SChannel, this seems to be 64k, so it is unlikely, but we shall handle it properly.
		while (offset < from.filled()) {
			Nat chunkSz = min(from.filled() - offset, maxMessageSize);
			encrypt(data->engine(), from, offset, chunkSz, data->outgoing);
			data->dst->write(data->outgoing);

			offset += chunkSz;
		}

		// Clear the buffer if it becomes too large.
		if (data->outgoing.count() > data->bufferSizes * 2)
			data->outgoing = Buffer();
	}

	void SChannelSession::flush(void *gcData) {
		SChannelData *data = (SChannelData *)gcData;
		os::Lock::L z(lock);
		data->dst->flush();
	}

	void SChannelSession::close(void *gcData) {
		SChannelData *data = (SChannelData *)gcData;
		os::Lock::L z(lock);

		data->src->close();
		data->dst->close();
	}

	void SChannelSession::fill(SChannelData *data) {
		// In case we fail somewhere...
		data->decryptedStart = data->decryptedEnd = 0;

		// Anything left in the buffer?
		if (data->remainingStart >= data->incoming.filled()) {
			// Nope. Get more. But this means that the buffer is entirely empty, use that.
			data->incoming.filled(0);
			data->remainingStart = 0;
			data->incoming = data->src->read(data->incoming);
		}

		// Should we shrink the buffer a bit?
		Nat used = data->incoming.filled() - data->remainingStart;
		if ((used * 2 < data->incoming.count()) && (data->incoming.count() > data->bufferSizes)) {
			Buffer t = buffer(data->engine(), max(used, data->bufferSizes));
			memcpy(t.dataPtr(), data->incoming.dataPtr() + data->remainingStart, used);
			data->incoming = t;
			data->incoming.filled(used);
			data->remainingStart = 0;
		}

		while (true) {
			SChannelSession::Markers markers = decrypt(data->engine(), data->incoming, data->remainingStart);
			data->decryptedStart = markers.plaintextStart;
			data->decryptedEnd = markers.plaintextEnd;
			data->remainingStart = markers.remainingStart;

			// Did we decrypt something? Good, we're done for now.
			if (data->decryptedStart != data->decryptedEnd)
				break;

			// End of transmission?
			if (markers.shutdown) {
				data->incomingEnd = true;
				break;
			}

			// Is there a large "hole" in the beginning of the array? If so, we want to move it back
			// into place to use the space more efficiently.
			if (data->remainingStart > data->incoming.count() / 4) {
				data->incoming.shift(data->remainingStart);
				data->remainingStart = 0;
				data->decryptedStart = data->decryptedEnd = 0;
			}

			// If we didn't get any data back, try to get more data and try again.
			Nat margin = data->incoming.filled() / 2;
			if (data->incoming.free() < margin)
				data->incoming = grow(data->engine(), data->incoming, data->incoming.filled() * 2);

			Nat before = data->incoming.filled();
			data->incoming = data->src->read(data->incoming);

			if (data->incoming.filled() == before) {
				// Failed to receive data, give up.
				break;
			}
		}
	}


	/**
	 * Client-specific parts:
	 */

	SChannelClientSession::SChannelClientSession(SChannelContext *c) : SChannelSession(c) {}

	void *SChannelClientSession::connect(IStream *input, OStream *output, Str *host) {
		Engine &e = input->engine();
		SChannelData *data = new (e) SChannelData(input, output);

		// Minimum "free" when calling receive.
		Nat minFree = 128;

		// Input and output buffers.
		Buffer inBuffer = storm::buffer(e, 2*1024);
		Buffer outBuffer = storm::buffer(e, 2*1024);

		while (true) {
			int result = initSession(e, inBuffer, outBuffer, host->c_str());

			// Send to server and receive more.
			if (outBuffer.filled() > 0) {
				data->dst->write(outBuffer);
				data->dst->flush();
			}

			if (result == 0) {
				break;
			}

			// Read more data.
			Nat more = max(minFree, Nat(abs(result)));
			if (inBuffer.free() < more)
				inBuffer = grow(e, inBuffer, inBuffer.filled() + more);

			inBuffer = data->src->read(inBuffer);
			if (inBuffer.filled() == 0 && !data->src->more())
				throw new (e) SSLError(S("Failed to authenticate to the server: End of stream."));
		}

		// From here on, we can use EncryptMessage and DecryptMessage.
		// We must use cbMaximumMessage from QueryContextAttributes to find max message size.

		// Resize the input and output buffers so that we don't have to resize them in the
		// future. We can't reallocate them and keep inter-thread consistency.
		data->bufferSizes = min(Nat(4*1024), maxMessageSize);
		data->incoming = storm::buffer(e, data->bufferSizes);

		return data;
	}

	void SChannelClientSession::shutdown(void *gcData) {
		SChannelData *data = (SChannelData *)gcData;
		os::Lock::L z(lock);

		if (data->outgoingEnd)
			return;

		Buffer msg = SChannelSession::shutdown(data->engine());
		data->dst->write(msg);

		data->outgoingEnd = true;
	}

	/**
	 * Server-specific parts:
	 */

	SChannelServerSession::SChannelServerSession(SChannelContext *c) : SChannelSession(c) {}

	void *SChannelServerSession::connect(IStream *input, OStream *output, Str *host) {
		Engine &e = input->engine();
		SChannelData *data = new (e) SChannelData(input, output);

		// Minimum "free" when calling receive.
		Nat minFree = 128;

		// Input and output buffers.
		Buffer inBuffer = storm::buffer(e, 2*1024);
		Buffer outBuffer = storm::buffer(e, 2*1024);

		while (true) {
			// Read more data.
			inBuffer = data->src->read(inBuffer);
			if (inBuffer.filled() == 0 && !data->src->more())
				throw new (e) SSLError(S("Failed to authenticate to the client: End of stream."));

			// Try to accept the connection...
			int result = acceptSession(e, inBuffer, outBuffer);

			// Should we send something?
			if (outBuffer.filled() > 0) {
				data->dst->write(outBuffer);
				data->dst->flush();
			}

			// Send messages back if we're not done.
			if (result == 0) {
				break;
			}

			// Adjust buffer size if necessary.
			Nat more = max(minFree, Nat(abs(result)));
			if (inBuffer.free() < more)
				inBuffer = grow(e, inBuffer, inBuffer.filled() + more);
		}

		// From here on, we can use EncryptMessage and DecryptMessage.
		// We must use cbMaximumMessage from QueryContextAttributes to find max message size.

		// Resize the input and output buffers so that we don't have to resize them in the
		// future. We can't reallocate them and keep inter-thread consistency.
		data->bufferSizes = min(Nat(4*1024), maxMessageSize);
		data->incoming = storm::buffer(e, data->bufferSizes);

		return data;
	}

	void SChannelServerSession::shutdown(void *gcData) {
		SChannelData *data = (SChannelData *)gcData;
		os::Lock::L z(lock);

		if (data->outgoingEnd)
			return;

		// TODO!

		data->outgoingEnd = true;
	}
}

#endif
