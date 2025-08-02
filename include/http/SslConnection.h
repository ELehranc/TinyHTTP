#pragma once
#include "SslContext.h"
#include <netsocket/net/Connection.h>
#include <netsocket/net/Buffer.h>
#include <openssl/ssl.h>
#include <memory>

using MessageCallback = std::function<void(const std::shared_ptr<Connection> &, Buffer *, Timestamp)>;
using WriteCompleteCallback = std::function<void( std::shared_ptr<Connection> &)>;
class SslConnection
{

public:
    using ConnectionPtr = std::shared_ptr<Connection>;
    using BufferPtr = Buffer *;

    SslConnection(const ConnectionPtr &conn, SslContext *ctx);
    ~SslConnection();

    void startHandshake();
    void send(const void *data, size_t len);
    void onRead(const ConnectionPtr &conn, BufferPtr buf, Timestamp time);
    bool isHandshakeCompleted() const { return state_ == SSLState::ESTABLISHED; }
    Buffer *getDecryptedBuffer() { return &decryptedBuffer_; }

    static int bioWrite(BIO *bio, const char *data, int len);
    static int bioRead(BIO *bio, char *data, int len);
    static long bioCtrl(BIO *bio, int cmd, long num, void *ptr);

    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

    void shutdown();
    void flushWriteBuffer();
    void handleWriteComplete(const ConnectionPtr &conn);

private:
    void handleHandshake();
    void onEncrypted(const char *data, size_t len);
    void onDecrypted(const char *data, size_t len);

    SSLError getLastError(int ret);
    void handleError(SSLError error);

private:
    SSL *ssl_;                                    // SSL 连接
    SslContext *ctx_;                             // SSL 上下文
    ConnectionPtr conn_;                          // tcp 连接
    SSLState state_;                              // SSL状态
    BIO *readBio_;                                // 网络数据 -> SSL
    BIO *writeBio_;                               // SSL -> 网络数据
    Buffer readBuffer_;                           // 读缓冲区
    Buffer writeBuffer_;                          // 写缓冲区
    Buffer decryptedBuffer_;                      // 解密后的数据
    MessageCallback messageCallback_;             // 消息回调
    WriteCompleteCallback writeCompleteCallback_; // 数据写完回调
};