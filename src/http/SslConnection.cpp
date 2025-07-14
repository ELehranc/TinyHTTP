#include "SslConnection.h"
#include <mymuduo/Logger.h>
#include <openssl11/openssl/err.h>

SslConnection::SslConnection(const ConnectionPtr &conn, SslContext *ctx) : ssl_(nullptr),
                                                                           ctx_(ctx),
                                                                           conn_(conn),
                                                                           state_(SSLState::HANDSHAKE),
                                                                           readBio_(nullptr),
                                                                           writeBio_(nullptr),
                                                                           messageCallback_(nullptr)
{
    ssl_ = SSL_new(ctx_->getNativeHandle());
    if (!ssl_)
    {

        LOG_ERROR("Failed to create SSL object: %s", ERR_error_string(ERR_get_error(), nullptr));
        return;
    }

    readBio_ = BIO_new(BIO_s_mem());
    writeBio_ = BIO_new(BIO_s_mem());

    if (!readBio_ || !writeBio_)
    {

        LOG_ERROR("Failed to create BIO bojects");
        SSL_free(ssl_);
        ssl_ = nullptr;
        return;
    }

    SSL_set_bio(ssl_, readBio_, writeBio_);
    SSL_set_accept_state(ssl_);

    SSL_set_mode(ssl_, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
    SSL_set_mode(ssl_, SSL_MODE_ENABLE_PARTIAL_WRITE);

    conn_->setMessageCallback(std::bind(&SslConnection::onRead, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

SslConnection::~SslConnection()
{

    if (ssl_)
    {
        SSL_free(ssl_);
    }
}

void SslConnection::startHandshake()
{

    handleHandshake();
}
/* 发送数据时：
应用数据 → SSL_write（加密）→ 数据存入 writeBio_ → BIO_read（取出加密数据）→ 发送到 TCP 连接 */
void SslConnection::send(const void *data, size_t len)
{

    if (state_ != SSLState::ESTABLISHED)
    {
        LOG_ERROR("Cannot send data before SSL handshake is complete");
        return;
    }

    int written = SSL_write(ssl_, data, len);
    if (written <= 0)
    {
        int err = SSL_get_error(ssl_, written);
        LOG_ERROR("SSL_write failed: %s", ERR_error_string(err, nullptr));
        return;
    }

    char buf[4096];
    int pending;

    while ((pending = BIO_pending(writeBio_)) > 0)
    {
        int bytes = BIO_read(writeBio_, buf, std::min(pending, static_cast<int>(sizeof buf)));

        if (bytes > 0)
        {

            LOG_INFO("数据通过TCPconn发送出去了");
            conn_->send(buf, bytes);
        }
    }
}
/*
接收数据时：
TCP 连接接收加密数据 → BIO_write（存入 readBio_）→ SSL_read（解密）→ 得到应用数据 */
void SslConnection::onRead(const ConnectionPtr &conn, BufferPtr buf, Timestamp time)
{
    LOG_INFO("有新的数据到达了");

    if (state_ == SSLState::HANDSHAKE)
    {
        int bytes = buf->readableBytes();
        LOG_INFO("正在握手，写入BIO的数据长度 %d ", bytes);

        BIO_write(readBio_, buf->peek(), bytes);
        buf->retrieveAll();
        handleHandshake();
    }
    else if (state_ == SSLState::ESTABLISHED)
    {

        int bytes_written = BIO_write(readBio_, buf->peek(), buf->readableBytes());
        if (bytes_written <= 0)
        {
            LOG_ERROR("BIO_write failed");
            return;
        }
        buf->retrieve(bytes_written);
        char decryptedData[4096];
        while (true)
        {
            LOG_INFO("onRead 循环读取引用数据");
            int ret = SSL_read(ssl_, decryptedData, sizeof decryptedData);
            int err = SSL_get_error(ssl_, ret);

            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
            {

                LOG_INFO("SSL_read时发生了WANT_READ/WRITE");

                break;
            }
            else if (ret > 0)
            {
                LOG_INFO("接收到解密数据，长度: %d bytes", ret);

                onDecrypted(decryptedData, ret);

                Buffer decryptedBuffer;
                decryptedBuffer.append(decryptedData, ret);

                if (messageCallback_)
                {
                    LOG_INFO("交上层回调处理数据");
                    messageCallback_(conn, &decryptedBuffer, time);
                }
            }
            else
            {
                int err = SSL_get_error(ssl_, ret);
                if (err == SSL_ERROR_ZERO_RETURN)
                {
                    LOG_INFO("对端关闭连接，SSL 读取返回0");
                    conn->shutdown();
                    return;
                }
                else if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
                {
                    LOG_INFO("等待 I/O 事件 (err= %d)", err);
                    return;
                }
                else
                {
                    LOG_ERROR("SSL_read 错误: %d ", ERR_error_string(err, nullptr));
                    conn->shutdown();
                    return;
                }
            }
        }
    }
}

void SslConnection::handleHandshake()
{

    int ret = SSL_do_handshake(ssl_);

    char buf[4096];
    while (BIO_pending(writeBio_))
    {

        int bytes = BIO_read(writeBio_, buf, sizeof(buf));
        if (bytes > 0)
        {
            conn_->send(buf, bytes);
        }
    }

    if (ret == 1)
    {
        state_ = SSLState::ESTABLISHED;
        LOG_INFO("SSL handshake completed successfully");
        LOG_INFO("Using cipher: %s", SSL_get_cipher(ssl_));
        LOG_INFO("Protocol version: %s", SSL_get_version(ssl_));

        if (!messageCallback_)
        {
            LOG_ERROR(" NO MESSAGE CALLBACK SET");
        }
        return;
    }

    int err = SSL_get_error(ssl_, ret);
    switch (err)
    {

    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
        break;

    case SSL_ERROR_SSL:
    {
        long verifyResult = SSL_get_verify_result(ssl_);
        if (verifyResult == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN)
        {

            state_ = SSLState::ESTABLISHED;
            return;
        }
        break;
    }

    default:
    {
        char errBuf[256];
        unsigned long errCode = ERR_get_error();
        ERR_error_string_n(errCode, errBuf, sizeof(errBuf));
        LOG_ERROR("SSL handshake failed: %s", errBuf);
        conn_->shutdown();
        break;
    }
    }
}

void SslConnection::onDecrypted(const char *data, size_t len)
{
    decryptedBuffer_.append(data, len);
}

SSLError SslConnection::getLastError(int ret)
{
    int err = SSL_get_error(ssl_, ret);
    switch (err)
    {
    case SSL_ERROR_NONE:
        return SSLError::NONE;
    case SSL_ERROR_WANT_READ:
        return SSLError::WANT_READ;
    case SSL_ERROR_WANT_WRITE:
        return SSLError::WANT_WRITE;
    case SSL_ERROR_SYSCALL:
        return SSLError::SYSCALL;
    case SSL_ERROR_SSL:
        return SSLError::SSL;
    default:
        return SSLError::SSL;
    }
}
