#include "stdafx.h"
#include "RuntimeSSL.h"
#include "Exception.h"
#include "Core/Convert.h"

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
#define SSL_FN_ALT(lib, ret, name, params, names, alt)	\
	static ret (*ptr_ ## name) params;					\
	extern "C" ret name params {						\
		return (*ptr_ ## name)names;					\
	}
#include "OpenSSLFunctions.inc"
#undef SSL_FN
#undef SSL_FN_ALT

namespace ssl {

	static util::Lock initLock;
	static bool initialized = false;

	void *loadFn(void *from, const char *name, const char *altName = null) {
		void *r = dlsym(from, name);
		if (altName && !r)
			r = dlsym(from, altName);

		if (!r) {
			StrBuf *msg = new (runtime::someEngine()) StrBuf();
			*msg << S("Can not find the function ")
				 << new (msg) Str(toWChar(msg->engine(), name))
				 << S(" in the SSL or crypto library.");
			throw new (msg) SSLError(msg->toS());
		}
		return r;
	}

	void *loadLib(const char *base, int flags) {
		// Attempted names to append to the library name. If the "-dev" package is installed, the
		// symlink from ".so" to the real library is not always present, so we attempt a few known
		// good names.
		const char *options[] = {
			".so",
			".so.3",
			".so.1.1",
		};

		for (size_t i = 0; i < ARRAY_COUNT(options); i++) {
			std::string name = std::string(base) + options[i];
			void *result = dlopen(name.c_str(), flags);
			if (result)
				return result;
		}

		return null;
	}

	void initRuntimeSSL() {
		util::Lock::L z(initLock);
		if (initialized)
			return;
		initialized = true;

		void *libCrypto = loadLib("libcrypto", RTLD_NOW | RTLD_GLOBAL);
		if (!libCrypto) {
			throw new (runtime::someEngine()) SSLError(S("Unable to load libcrypto.so. Make sure it is installed. In some cases you might need to install the -dev package."));
		}

		void *libSSL = loadLib("libssl", RTLD_NOW);

		if (!libSSL) {
			dlclose(libCrypto);
			throw new (runtime::someEngine()) SSLError(S("Unable to load libssl.so. Make sure it is installed! In some cases you might need to install the -dev package."));
		}

#define SSL_FN(lib, ret, name, params, names)			\
		ptr_ ## name = (ret (*) params)loadFn(lib, #name);
#define SSL_FN_ALT(lib, ret, name, params, names, alt)			\
		ptr_ ## name = (ret (*) params)loadFn(lib, #name, alt);
#include "OpenSSLFunctions.inc"
#undef SSL_FN
#undef SSL_FN_ALT
	}
}

#else

namespace ssl {

	void initRuntimeSSL() {}

}

#endif

#endif
