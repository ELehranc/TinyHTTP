#include "SslContext.h"
#include <mymuduo/Logger.h>

SslContext::SslContext(const SslConfig &config) : ctx_(nullptr), config_(config)
{
}

SslContext::~SslContext()
{
    if (ctx_)
    {
        SSL_CTX_free(ctx_);
    }
}

bool SslContext::initialize()
{

    // 新版初始化方法（OpenSSL 1.1.0+）：
    OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS | OPENSSL_INIT_LOAD_CRYPTO_STRINGS, nullptr);

    const SSL_METHOD *method = TLS_server_method();
    ctx_ = SSL_CTX_new(method);
    if (!ctx_)
    {
        handleSslError("Failed to create SSL context");
        return false;
    }

    long options = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION | SSL_OP_CIPHER_SERVER_PREFERENCE;
    SSL_CTX_set_options(ctx_, options);

    if (!loadCerificates())
    {
        return false;
    }

    if (!setupProtocol())
    {
        return false;
    }

    setupSessionCache();

    LOG_INFO("SSL context initialized successfully");
    return true;
}

bool SslContext::loadCerificates()
{
    // 加载证书
    if (SSL_CTX_use_certificate_file(ctx_, config_.getCerificateFile().c_str(), SSL_FILETYPE_PEM) <= 0)
    {

        handleSslError("Failed to load server certificate");
        return false;
    }
    // 加载私钥
    if (SSL_CTX_use_PrivateKey_file(ctx_, config_.getPrivateKeyFile().c_str(), SSL_FILETYPE_PEM) <= 0)
    {

        handleSslError("Failed to load private key");
        return false;
    }

    // 验证私钥
    if (!SSL_CTX_check_private_key(ctx_))
    {
        handleSslError("Private key does not match the certificate");
        return false;
    }

    // 加载证书链
    if (!config_.getCertificateChainFile().empty())
    {
        if (SSL_CTX_use_certificate_chain_file(ctx_,
                                               config_.getCertificateChainFile().c_str()) <= 0)
        {
            handleSslError("Failed to load certificate chain");
            return false;
        }
    }

    return true;
}

bool SslContext::setupProtocol()
{

    long options = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3;
    switch (config_.getProtocolVersion())
    {
    case SSLVersion::TLS_1_0:
        options |= SSL_OP_NO_TLSv1;
        break;
    case SSLVersion::TLS_1_1:
        options |= SSL_OP_NO_TLSv1_1;
        break;
    case SSLVersion::TLS_1_2:
        options |= SSL_OP_NO_TLSv1_2;
        break;
    case SSLVersion::TLS_1_3:
        options |= SSL_OP_NO_TLSv1_3;
        break;
    }
    SSL_CTX_set_options(ctx_, options);

    // 设置加密套件
    if (!config_.getCipherList().empty())
    {
        if (SSL_CTX_set_cipher_list(ctx_,
                                    config_.getCipherList().c_str()) <= 0)
        {
            handleSslError("Failed to set cipher list");
            return false;
        }
    }

    return true;
}

void SslContext::setupSessionCache()
{
    SSL_CTX_set_session_cache_mode(ctx_, SSL_SESS_CACHE_SERVER);
    SSL_CTX_sess_set_cache_size(ctx_, config_.getSessionCacheSize());
    SSL_CTX_set_timeout(ctx_, config_.getSessionTimeout());
}

void SslContext::handleSslError(const char *msg)
{
    char buf[256];
    ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
    LOG_ERROR("%s << :  << %s", msg, buf);
}
