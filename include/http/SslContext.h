#pragma once

#include "SslConfig.h"
#define OPENSSL_API_COMPAT 0x10100000L
#define OPENSSL_NO_DEPRECATED

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/conf.h>

#include <memory>

class SslContext
{

public:
    explicit SslContext(const SslConfig &config);
    ~SslContext();

    bool initialize();
    SSL_CTX *getNativeHandle() { return ctx_; }

private:
    bool loadCerificates();
    bool setupProtocol();
    void setupSessionCache();
    static void handleSslError(const char *msg);

private:
    SSL_CTX *ctx_;     // SSL上下文
    SslConfig config_; // SSL配置
};