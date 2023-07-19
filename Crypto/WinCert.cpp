#include "stdafx.h"
#include "WinCert.h"

#ifdef WINDOWS

#include "WinErrorMsg.h"
#include "Exception.h"
#include "Core/Io/Text.h"
#pragma comment (lib, "crypt32.lib")
#pragma comment (lib, "advapi32.lib")

namespace ssl {

	static std::vector<BYTE> decodeBase64(Str *data) {
		DWORD size = 0;
		if (!CryptStringToBinaryW(data->c_str(), NULL, CRYPT_STRING_BASE64_ANY, NULL, &size, NULL, NULL))
			throwError(data->engine(), S("Failed to decode PEM data: "), GetLastError());

		std::vector<BYTE> result(size, 0);
		if (!CryptStringToBinaryW(data->c_str(), NULL, CRYPT_STRING_BASE64_ANY, &result[0], &size, NULL, NULL))
			throwError(data->engine(), S("Failed to decode PEM data: "), GetLastError());

		return result;
	}

	WinSSLCert *WinSSLCert::fromPEM(Str *data) {
		std::vector<BYTE> raw(decodeBase64(data));

		const CERT_CONTEXT *c = CertCreateCertificateContext(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, &raw[0], DWORD(raw.size()));
		if (!c)
			throwError(data->engine(), S("Failed to decode certificate data: "), GetLastError());

		return new WinSSLCert(c);
	}

	WinSSLCert::WinSSLCert(const CERT_CONTEXT *c) : data(c) {}

	WinSSLCert::~WinSSLCert() {
		CertFreeCertificateContext(data);
	}

	WinSSLCert *WinSSLCert::windows() {
		ref();
		return this;
	}

	OpenSSLCert *WinSSLCert::openSSL() {
		throw new (runtime::someEngine()) SSLError(S("OpenSSL is not supported on Windows."));
	}

	WinSSLCert *WinSSLCert::copy() {
		const CERT_CONTEXT *c = CertCreateCertificateContext(data->dwCertEncodingType, data->pbCertEncoded, data->cbCertEncoded);
		if (!c)
			throwError(runtime::someEngine(), S("Failed to copy a certificate: "), GetLastError());

		return new WinSSLCert(c);
	}

	static GcArray<wchar> *decodeName(Engine &e, CERT_NAME_BLOB &blob, DWORD stringType) {
		DWORD size = CertNameToStrW(X509_ASN_ENCODING, &blob, stringType, NULL, 0);

		GcArray<wchar> *result = runtime::allocArray<wchar>(e, &wcharArrayType, size);
		CertNameToStrW(X509_ASN_ENCODING, &blob, stringType, result->v, size);
		return result;
	}

	void WinSSLCert::output(StrBuf *to) {
		CERT_INFO *info = data->pCertInfo;
		*to << S("Subject: ") << decodeName(to->engine(), info->Subject, CERT_X500_NAME_STR)->v;
		*to << S(", issuer: ") << decodeName(to->engine(), info->Issuer, CERT_X500_NAME_STR)->v;
	}


	WinSSLCertKey *WinSSLCertKey::fromPEM(Str *data) {
		std::vector<BYTE> raw(decodeBase64(data));

		DWORD size = 0;
		if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, PKCS_RSA_PRIVATE_KEY,
									&raw[0], DWORD(raw.size()), 0, NULL, NULL, &size))
			throwError(data->engine(), S("Failed to decode the key: "), GetLastError());

		std::vector<BYTE> decoded(size, 0);
		if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, PKCS_RSA_PRIVATE_KEY,
									&raw[0], DWORD(raw.size()), 0, NULL, &decoded[0], &size))
			throwError(data->engine(), S("Failed to decode the key: "), GetLastError());

		return new WinSSLCertKey(decoded);
	}

	WinSSLCertKey::WinSSLCertKey(const std::vector<byte> &data) : data(data) {}

	WinSSLCertKey::~WinSSLCertKey() {}

	const wchar *WinSSLCertKey::validate(SSLCert *cert) {
		RefPtr<WinSSLCert> c = cert->windows();

		// Try to import it into a temporary store and associate it with the certificate?
		TODO(L"Implement early validation");

		return NULL;
	}

	WinSSLCertKey *WinSSLCertKey::windows() {
		ref();
		return this;
	}

	OpenSSLCertKey *WinSSLCertKey::openSSL() {
		// This is probably fairly easy to do, but useless since we can not do it for certificates at the moment.
		throw new (runtime::someEngine()) SSLError(S("OpenSSL is not supported on Windows."));
	}


	WinKeyStore::WinKeyStore(WinSSLCertKey *k, bool anonymous) : provider(0), key(0) {
		memset(name, 0, sizeof(name));

		if (anonymous) {
			if (!CryptAcquireContext(&provider, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
				throwError(runtime::someEngine(), S("Failed to acquire a temporary keystore: "), GetLastError());
		} else {
			DWORD pid = GetCurrentProcessId();
			Nat id = 0;
			while (true) {
				id++;
				_snwprintf_s(name, sizeof(name), sizeof(name), S("STORM_%u_%u"), pid, id);

				if (CryptAcquireContext(&provider, name, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_NEWKEYSET))
					break;

				DWORD error = GetLastError();
				if (error != NTE_EXISTS)
					throwError(runtime::someEngine(), S("Failed to acquire a persistent keystore (note: this can not be done as a guest user): "), error);
			}
		}

		// Add the key!
		if (!CryptImportKey(provider, &k->data[0], DWORD(k->data.size()), NULL, 0, &key)) {
			DWORD error = GetLastError();
			CryptReleaseContext(provider, 0);
			throwError(runtime::someEngine(), S("Failed to import the key: "), error);
		}
	}

	WinKeyStore::~WinKeyStore() {
		CryptDestroyKey(key);
		CryptReleaseContext(provider, 0);

		if (name[0] != '\0') {
			if (!CryptAcquireContext(&provider, name, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_DELETEKEYSET)) {
				Nat error = GetLastError();
				WARNING(L"Failed to destroy the keystore: " << name << S(" (") << toHex(error) << S(")"));
			}
		}
	}

	void WinKeyStore::attach(PCCERT_CONTEXT certificate) {
		CRYPT_KEY_PROV_INFO info = { 0 };
		info.pwszContainerName = name;
		info.pwszProvName = MS_ENHANCED_PROV;
		info.dwProvType = PROV_RSA_FULL;
		info.dwKeySpec = AT_KEYEXCHANGE;
		info.dwFlags = CERT_SET_KEY_PROV_HANDLE_PROP_ID;

		if (!CertSetCertificateContextProperty(certificate, CERT_KEY_PROV_INFO_PROP_ID, 0, &info))
			throwError(runtime::someEngine(), S("Failed to attach the certificate to the key: "), GetLastError());
	}

}

#endif
