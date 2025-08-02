#include "netsocket/net/Connection.h"
#include "HttpContext.h"

class HttpConnection
{
public:
    using HttpConnectionPtr = std::shared_ptr<HttpConnection>;

    explicit HttpConnection(const std::shared_ptr<Connection> &conn)
        : conn_(conn) {}


    void setContext(const HttpContext &context){
        context_ = context;
    }

    HttpContext *getMutableContext() { return &context_; }

private:
    ConnectionPtr conn_; // 原TcpConnection
    HttpContext context_;
};
