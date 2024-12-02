#pragma once
#include "OS/Sync.h"
#include "Core/Str.h"
#include "Core/Io/Stream.h"

namespace ssl {

	/**
	 * Generic data utilities.
	 */

	class SSLSession;
	class WinSSLCert;
	class WinSSLCertKey;
	class OpenSSLCert;
	class OpenSSLCertKey;

	/**
	 * Generic refcount class.
	 */
	class RefCount {
	public:
		// Create, initializes refs to 1.
		RefCount() : refs(1) {}

		// Destroy.
		virtual ~RefCount() {}

		// Add a reference.
		void ref() {
			atomicIncrement(refs);
		}

		// Release a reference.
		void unref() {
			if (atomicDecrement(refs) == 0) {
				delete this;
			}
		}

	private:
		// References.
		size_t refs;
	};

	/**
	 * Auto ptr for refcounts. Useful in implementation code, but might not work as data members in Storm classes.
	 */
	template <class T>
	class RefPtr {
	public:
		// Create for null.
		RefPtr() : data(null) {}

		// Create. Takes ownership.
		RefPtr(T *p) : data(p) {}

		// Destroy.
		~RefPtr() {
			if (data)
				data->unref();
		}

		// Copy.
		RefPtr(const RefPtr &o) : data(o.data) {
			if (data)
				data->ref();
		}

		// Assign.
		RefPtr &operator =(const RefPtr &o) {
			if (o.data)
				o.data->ref();

			if (data)
				data->unref();
			data = o.data;

			return *this;
		}

#ifdef USE_MOVE
		RefPtr(RefPtr &&o) : data(o.data) {
			o.data = null;
		}

		RefPtr &operator =(RefPtr &&o) {
			std::swap(data, o.data);
			return *this;
		}
#endif

		// Access the data.
		T &operator *() const {
			return *data;
		}
		T *operator ->() const {
			return data;
		}
		T *get() const {
			return data;
		}

		operator bool() const {
			return data != null;
		}

	private:
		T *data;
	};


	/**
	 * Data for an SSL context. Internal to the Context, but convenient to have outside the class
	 * declaration.
	 */
	class SSLContext : public RefCount {
	public:
		// Create a session for this context.
		virtual SSLSession *createSession() = 0;
	};

	/**
	 * Data for an individual session.
	 */
	class SSLSession : public RefCount {
	public:
		// Lock for the session.
		os::Lock lock;

		// Connect this session. Returns any additional data needed to be kept alive by the GC.
		virtual void *connect(IStream *input, OStream *output, Str *host) = 0;

		// More data available?
		virtual Bool more(void *gcData) = 0;

		// Read data.
		virtual void read(Buffer &to, void *gcData) = 0;

		// Peek data.
		virtual void peek(Buffer &to, void *gcData) = 0;

		// Write data.
		virtual Nat write(const Buffer &from, Nat start, void *gcData) = 0;

		// Flush the stream.
		virtual Bool flush(void *gcData) = 0;

		// Shut down the session.
		virtual void shutdown(void *gcData) = 0;

		// Close the underlying streams.
		virtual void close(void *gcData) = 0;
	};


	/**
	 * Some representation of a certificate.
	 *
	 * The certificate inside is expected to be read-only after creation.
	 */
	class SSLCert : public RefCount {
	public:
		// Get the windows SSL implementation (might convert it, might throw).
		virtual WinSSLCert *windows() = 0;

		// Get the OpenSSL implementation version (might convert it, might throw).
		virtual OpenSSLCert *openSSL() = 0;

		// Extract some sensible string data.
		virtual void output(StrBuf *to) = 0;
	};


	/**
	 * Some representation of a certificate key.
	 */
	class SSLCertKey : public RefCount {
	public:
		// Check so that the key is valid for a particular certificate. Returns an error message on error, or null.
		virtual const wchar *validate(SSLCert *cert) = 0;

		// Get a windows version of the data.
		virtual WinSSLCertKey *windows() = 0;

		// Get the OpenSSL version of the data.
		virtual OpenSSLCertKey *openSSL() = 0;
	};

}
