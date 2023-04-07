/**
 * List of functions used in OpenSSL.
 */
SSL_FN(libSSL, int, OPENSSL_init_ssl, (uint64_t opts, const OPENSSL_INIT_SETTINGS *settings), (opts, settings))
SSL_FN(libSSL, int, CONF_modules_load_file, (const char *fn, const char *an, long unsigned int x), (fn, an, x))
SSL_FN(libSSL, unsigned long, ERR_get_error, (), ())
SSL_FN(libSSL, char *, ERR_error_string, (unsigned long e, char *buf), (e, buf))

SSL_FN(libSSL, void, SSL_CTX_free, (SSL_CTX *ctx), (ctx))
SSL_FN(libSSL, SSL_CTX *, SSL_CTX_new, (const SSL_METHOD *meth), (meth))
SSL_FN(libSSL, void, SSL_CTX_set_verify, (SSL_CTX *ctx, int mode, SSL_verify_cb cb), (ctx, mode, cb))
SSL_FN(libSSL, void, SSL_CTX_set_verify_depth, (SSL_CTX *ctx, int depth), (ctx, depth))
SSL_FN(libSSL, uint64_t, SSL_CTX_set_options, (SSL_CTX *ctx, uint64_t op), (ctx, op))
SSL_FN(libSSL, int, SSL_CTX_set_cipher_list, (SSL_CTX *ctx, const char *str), (ctx, str))
SSL_FN(libSSL, long, SSL_CTX_ctrl, (SSL_CTX *ctx, int cmd, long larg, void *parg), (ctx, cmd, larg, parg))
SSL_FN(libSSL, int, SSL_CTX_set_default_verify_paths, (SSL_CTX *ctx), (ctx))
SSL_FN(libSSL, int, SSL_CTX_use_certificate, (SSL_CTX *ctx, X509 *c), (ctx, c))
SSL_FN(libSSL, int, SSL_CTX_use_PrivateKey, (SSL_CTX *ctx, EVP_PKEY *pk), (ctx, pk))

SSL_FN(libSSL, int, SSL_shutdown, (SSL *ssl), (ssl))
SSL_FN(libSSL, X509_VERIFY_PARAM *, SSL_get0_param, (SSL *ssl), (ssl))
SSL_FN(libSSL, long, SSL_ctrl, (SSL *ssl, int cmd, long larg, void *parg), (ssl, cmd, larg, parg))
SSL_FN(libSSL, X509 *, SSL_get1_peer_certificate, (const SSL *ssl), (ssl))
SSL_FN(libSSL, long, SSL_get_verify_result, (const SSL *ssl), (ssl))

SSL_FN(libSSL, void, X509_free, (X509 *x), (x))
SSL_FN(libSSL, X509_NAME *, X509_get_subject_name, (const X509 *x), (x))
SSL_FN(libSSL, X509_NAME *, X509_get_issuer_name, (const X509 *x), (x))
SSL_FN(libSSL, int, X509_verify, (X509 *x, EVP_PKEY *k), (x, k))

SSL_FN(libSSL, X509_STORE *, X509_STORE_new, (), ())
SSL_FN(libSSL, int, X509_STORE_add_cert, (X509_STORE *s, X509 *c), (s, c))

SSL_FN(libSSL, void, X509_VERIFY_PARAM_set_hostflags, (X509_VERIFY_PARAM *p, unsigned int x), (p, x))
SSL_FN(libSSL, int, X509_VERIFY_PARAM_set1_host, (X509_VERIFY_PARAM *p, const char *host, size_t s), (p, host, s))
SSL_FN(libSSL, char *, X509_NAME_oneline, (const X509_NAME *a, char *buf, int size), (a, buf, size))

SSL_FN(libSSL, void, EVP_PKEY_free, (EVP_PKEY *pk), (pk))

SSL_FN(libSSL, const SSL_METHOD *, TLS_client_method, (), ())
SSL_FN(libSSL, const SSL_METHOD *, TLS_server_method, (), ())

SSL_FN(libSSL, BIO *, BIO_new, (const BIO_METHOD *type), (type))
SSL_FN(libSSL, BIO *, BIO_new_mem_buf, (const void *buf, int len), (buf, len))
SSL_FN(libSSL, int, BIO_free, (BIO *b), (b))
SSL_FN(libSSL, BIO *, BIO_new_ssl, (SSL_CTX *ctx, int client), (ctx, client));
SSL_FN(libSSL, void, BIO_set_data, (BIO *b, void *ptr), (b, ptr))
SSL_FN(libSSL, int, BIO_read, (BIO *b, void *data, int dlen), (b, data, dlen))
SSL_FN(libSSL, int, BIO_write, (BIO *b, const void *data, int dlen), (b, data, dlen))
SSL_FN(libSSL, long, BIO_ctrl, (BIO *b, int cmd, long larg, void *parg), (b, cmd, larg, parg))
SSL_FN(libSSL, int, BIO_get_new_index, (), ())
SSL_FN(libSSL, BIO *, BIO_push, (BIO *b, BIO *next), (b, next))
SSL_FN(libSSL, void *, BIO_get_data, (BIO *b), (b))

SSL_FN(libSSL, BIO_METHOD *, BIO_meth_new, (int type, const char *name), (type, name))
SSL_FN(libSSL, void, BIO_meth_free, (BIO_METHOD *m), (m))
SSL_FN(libSSL, int, BIO_meth_set_write, (BIO_METHOD *m, int (*fn)(BIO *, const char *, int)), (m, fn))
SSL_FN(libSSL, int, BIO_meth_set_read, (BIO_METHOD *m, int (*fn)(BIO *, char *, int)), (m, fn))
SSL_FN(libSSL, int, BIO_meth_set_puts, (BIO_METHOD *m, int (*fn)(BIO *, const char *)), (m, fn))
SSL_FN(libSSL, int, BIO_meth_set_gets, (BIO_METHOD *m, int (*fn)(BIO *, char *, int)), (m, fn))
SSL_FN(libSSL, int, BIO_meth_set_ctrl, (BIO_METHOD *m, long (*fn)(BIO *, int, long, void *)), (m, fn))
SSL_FN(libSSL, int, BIO_meth_set_callback_ctrl, (BIO_METHOD *m, long (*fn)(BIO *, int, BIO_info_cb *)), (m, fn))

SSL_FN(libSSL, X509 *, PEM_read_bio_X509, (BIO *b, X509 **out, int (*cb)(char *, int, int, void *), void *data), (b, out, cb, data));
SSL_FN(libSSL, EVP_PKEY *, PEM_read_bio_PrivateKey, (BIO *b, EVP_PKEY **out, int (*cb)(char *, int, int, void *), void *data), (b, out, cb, data));
