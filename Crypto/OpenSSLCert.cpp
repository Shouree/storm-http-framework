#include "stdafx.h"
#include "OpenSSLCert.h"
#include "OpenSSLError.h"
#include "Core/Convert.h"
#include "Exception.h"

#ifdef POSIX

#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/dh.h>
#include <openssl/ec.h>

namespace ssl {

	/**
	 * Create a BIO with memory from a Str.
	 */
	class StrBIO {
	public:
		StrBIO(Str *data) {
			size_t count = convert(data->c_str(), (char *)0, 0);
			// Remove null-terminator.
			count = max(count, 1) - 1;
			buffer = std::vector<char>(count, 0);
			convert(data->c_str(), &buffer[0], count);

			bio = BIO_new_mem_buf(&buffer[0], int(count));
		}

		~StrBIO() {
			BIO_free(bio);
		}

		std::vector<char> buffer;

		BIO *bio;

		operator BIO *() const {
			return bio;
		}
	};

	// We don't support passwords at the moment.
	static int passwordCallback(char *buffer, int size, int rwflag, void *data) {
		(void)buffer;
		(void)size;
		(void)rwflag;
		(void)data;
		return 0;
	}

	OpenSSLCert *OpenSSLCert::fromPEM(Str *data) {
		StrBIO bio(data);

		X509 *cert = PEM_read_bio_X509(bio, NULL, &passwordCallback, NULL);
		if (!cert)
			throwError();

		return new OpenSSLCert(cert);
	}

	OpenSSLCert::OpenSSLCert(X509 *cert) : data(cert) {}

	OpenSSLCert::~OpenSSLCert() {
		if (data)
			X509_free(data);
	}

	WinSSLCert *OpenSSLCert::windows() {
		throw new (runtime::someEngine()) SSLError(S("Windows certificates are not supported."));
	}

	OpenSSLCert *OpenSSLCert::openSSL() {
		ref();
		return this;
	}

	static void output(StrBuf *to, X509_NAME *name) {
		// Note: This is not exactly as on Windows, but it is good enough for toS.
		char *buffer = X509_NAME_oneline(name, NULL, 0);
		*to << toWChar(to->engine(), buffer)->v;
		free(buffer);
	}

	void OpenSSLCert::output(StrBuf *to) {
		// Note: These should *not* be freed according to the manpage.
		X509_NAME *subject = X509_get_subject_name(data);
		X509_NAME *issuer = X509_get_issuer_name(data);

		*to << S("subject: ");
		ssl::output(to, subject);
		*to << S(", issuer: ");
		ssl::output(to, issuer);
	}


	OpenSSLCertKey *OpenSSLCertKey::fromPEM(Str *data) {
		StrBIO bio(data);

		EVP_PKEY *key = PEM_read_bio_PrivateKey(bio, NULL, &passwordCallback, NULL);
		if (!key)
			throwError();

		return new OpenSSLCertKey(key);
	}

	OpenSSLCertKey::OpenSSLCertKey(EVP_PKEY *key) : data(key) {}

	OpenSSLCertKey::~OpenSSLCertKey() {
		if (data)
			EVP_PKEY_free(data);
	}

	WinSSLCertKey *OpenSSLCertKey::windows() {
		throw new (runtime::someEngine()) SSLError(S("Windows is not supported here."));
	}

	OpenSSLCertKey *OpenSSLCertKey::openSSL() {
		ref();
		return this;
	}

	const wchar *OpenSSLCertKey::validate(SSLCert *cert) {
		RefPtr<OpenSSLCert> c = cert->openSSL();

		if (!X509_verify(c->data, data))
			return S("Certificate and key does not match.");

		return null;
	}

}

#endif
