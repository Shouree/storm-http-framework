#include "stdafx.h"
#include "RuntimeSSL.h"
#include "Exception.h"

#ifdef POSIX

#ifdef RUNTIME_OPENSSL_LINK
#include <dlfcn.h>

#include <openssl/ssl.h>
#include <openssl/conf.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/opensslv.h>
#include <openssl/opensslconf.h>
#include <openssl/x509v3.h>

#define SSL_FN(lib, ret, name, params, names)	\
	static ret (*ptr_ ## name) params;			\
	extern "C" ret name params {				\
		return (*ptr_ ## name)names;			\
	}
#include "OpenSSLFunctions.inc"
#undef SSL_FN

namespace ssl {

	static util::Lock initLock;
	static bool initialized = false;

	void *loadFn(void *from, const char *name) {
		void *r = dlsym(from, name);
		std::cout << name << ": " << r << std::endl;
		if (!r) {
			StrBuf *msg = new (runtime::someEngine()) StrBuf();
			*msg << S("Can not find the function ") << name << S(" in the SSL or crypto library.");
			throw new (msg) SSLError(msg->toS());
		}
		return r;
	}

	void initRuntimeSSL() {
		util::Lock::L z(initLock);
		if (initialized)
			return;
		initialized = true;

		void *libCrypto = dlopen("libcrypto.so", RTLD_NOW | RTLD_GLOBAL);
		void *libSSL = dlopen("libssl.so", RTLD_NOW);
		PVAR(libCrypto);
		PVAR(libSSL);

		if (!libSSL || !libCrypto)
			throw new (runtime::someEngine()) SSLError(S("Unable to load libssl.so and/or libcrypto.so. Make sure they are installed!"));

		// ptr_SSL_CTX_free = (void (*)(SSL_CTX *ctx))loadFn(
#define SSL_FN(lib, ret, name, params, names)	\
		ptr_ ## name = (ret (*) params)loadFn(lib, #name);
#include "OpenSSLFunctions.inc"
#undef SSL_FN
	}
}

#else

namespace ssl {

	void initRuntimeSSL() {}

}

#endif

#endif
