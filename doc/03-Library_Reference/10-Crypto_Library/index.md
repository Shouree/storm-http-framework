Crypto Library
==============

The `crypto` library provides various cryptographic utilities.

In particular, one of these is the ability to wrap streams inside SSL (or rather TLS) streams. Among
other things, this allows secure connections over the Internet (for example: HTTPS). As SSL is
designed to be an interactive communication protocol, it is not suitable for encryption to disk, for
example.

The library itself does not implement any cryptographic primitives itself. Instead, it relies on the
default cryptographic libraries of the operating system. On Linux and other POSIX systems, OpenSSL
(>= 1.1.0) is used. To facilitate differences between versions, `libssl` and `libcrypt` are loaded
dynamically. As such, it might be necessary to install these versions manually through the system's
package manager if initialization of the crypto library fails. On Windows, the standard
cryptographic library (SChannel) is used instead. This way, the SSL library benefits from any
security patches in these critical libraries that are provided by your operating system. Note: SSL
support does not work on Windows 7.


Usage
-----

The crypto library provides the following classes that are needed to establish and accept SSL
connections:

- [stormname:crypto.Certificate]

  Represents a X509 certificate of some entity. Note that a certificate only encompassess the public
  parts of the certificate.

- [stormname:crypto.CertificateKey]

  Represents the private key required to assert that one owns a particular certificate. It is thus
  required to create an SSL server.

- [stormname:crypto.Context]

  A context encompassess all the SSL configuration parameters required for either server or client
  connections. As these differs, there are different classes for server context and client contexts.

* [stormname:crypto.Session]

  A session can be created from a `Context` and represents an active SSL session (= connection). The
  session itself looks much like a socket and one can get input- and output streams from it in order
  to send and receive data.


In order to illustrate how these concepts belong together, consider the following example where we
perform a HTTP GET request over HTTPS:

```bs
use crypto;
use core:net;
use core:io;

void httpClient() {
    // Get a context with system default settings (i.e. the system's certificate store, sane defaults).
    var context = ClientContext:systemDefault();

    // Connect using the regular core:net functionality.
    unless (socket = connect("storm-lang.org", 443))
        throw InternalError("Failed to connect!");

    // Wrap the socket (NetStream) inside a SSL layer. The second parameter is the hostname and is
    // used to verify the certificate.
    var session = context.connect(socket, "storm-lang.org");

    // Create text streams so that we can speak HTTP. "windowsTextInfo" gives us the proper CR LF.
    var in = Utf8Input(session.input);
    var out = Utf8Output(session.output, windowsTextInfo());
    out.autoFlush = false;

    // Do the HTTP request.
    out.write("GET / HTTP/1.0\n");
    out.write("Host: storm-lang.org\n");
    out.write("\n\n");
    out.flush();

    // Get all data in the response.
    print(in.readAll());

    // Close everything. This propagates to the underlying stream as well.
    session.close();
}
```

If you know exactly which certificate is present on the remote peer, you can use
`ClientContext.pinnedTo`. Then only that particular certificate is trusted (even if it is
self-signed). Certificates themselves can be loaded using `Certificate:loadPEM`.

For a server, the process is similar. The exception is that the `ServerContext` needs a
`CertificateKey` for the certificate that it should use. This might once again be a self-signed
certificate if desired.

It is not necessary to use the crypto library to communicate over sockets. The `connect` function in
the contexts support passing arbitrary input and output streams to use for the encrypted
communications. For example, it is possible to create two `core:io:Pipe` instances, wrap them in an
SSL layer and have two threads (or UThreads) communicate that way (even though it is not very useful
to encrypt communications in the same process).


Known limitations
-----------------

The current implementation have some known limitations:

- It is not possible to supply a certificate chain to the `ServerContext`. This is not an issue for
  self-signed certificates, but for certificates from a certificate authority.

- It is not possible to start a server on a guest account on Windows. This is due to how SChannel is
  designed (we need to store the key in a system store, these are not available in guest accounts).

- Password-protected certificate keys are not supported. This is easy to implement in OpenSSL, but I
  have not yet found support in the Windows API.

- It is not possible to inspect the loaded certificates in any meaningful way aside from using them
  in an SSL connection.

Windows also provides a fairly rich library for cryptography that might be interesting to expose to
Storm in some way. In particular, the preferred way fo managing keys in Windows is to store them in
the system's key store. That way, the keys are encrypted with the user's credentials and only
accessible to that particular user. Furthermore, this allows storing the keys in special hardware
(e.g. in smartcards) and other useful things. One of the main benefits of this is that the keys are
not even visible to the Storm process that are using them.
