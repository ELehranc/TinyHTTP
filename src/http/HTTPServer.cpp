#include "HTTPServer.h"

// void defaultHttpCallback(const HttpRequest &req, HttpResponse *resp)
// {
//     resp->setStatusCode(HttpResponse::k404NotFound);
//     resp->setStatusMessage("Not Found");
//     resp->setCloseConnection(true); // 短链接关闭
// }

HTTPServer::HTTPServer(int port, const std::string &name, bool useSSL, TcpServer::Option option)
    : listenAddr_("192.168.88.133", port),
      server_(&mainLoop_, listenAddr_, name, option),
      useSSL_(useSSL), // 是否使用 SSL
      httpCallback_(std::bind(&HTTPServer::handleRequest, this, std::placeholders::_1, std::placeholders::_2))
{
    initialize();
}

void HTTPServer::start()
{

    LOG_INFO("HttpServer[ %s ] starts listening on %s ", server_.getName(), server_.getIpPort());

    server_.start();
    mainLoop_.loop();
}

void HTTPServer::initialize()
{

    server_.setConnectionCallback(std::bind(&HTTPServer::onConnection, this, std::placeholders::_1));

    server_.setMessageCallback(std::bind(&HTTPServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void HTTPServer::setSslConfig(const SslConfig &config)
{
    if (useSSL_)
    {

        sslCtx_ = std::make_unique<SslContext>(config);
        if (!sslCtx_->initialize())
        {
            LOG_ERROR("Failed to initialize SSL context");
            abort();
        }
        LOG_INFO("ssl 上下文初始化成功");
    }
}

void HTTPServer::onConnection(const ConnectionPtr &conn)
{
    if (conn->connected())
    {
        if (useSSL_)
        {
            LOG_INFO("SLL连接已建立");
            auto sslConn = std::make_shared<SslConnection>(conn, sslCtx_.get());
            sslConn->setMessageCallback(std::bind(&HTTPServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

            sslConns_[conn] = sslConn;
            sslConns_[conn]->startHandshake();
        }
        auto httpConn = std::make_shared<HttpConnection>(conn);

        httpConnections_[conn->name()] = httpConn;
        httpConn->setContext(HttpContext());
    }
    else
    {
        if (useSSL_)
        {

            auto it = sslConns_.find(conn);
            if (it != sslConns_.end())
            {
                sslConns_.erase(it);
            }
        }
    }
}

void HTTPServer::onMessage(const ConnectionPtr &conn, Buffer *buf, Timestamp receiveTime)
{
    LOG_INFO("有信息到达了");
    try
    {

        if (useSSL_)
        {

            LOG_INFO("onMessage useSSL_ is true");

            auto it = sslConns_.find(conn);
            if (it != sslConns_.end())
            {

                if (!it->second->isHandshakeCompleted())
                {
                    LOG_INFO("握手未完成，返回");
                    return;
                }

                Buffer *decryptedBuf = it->second->getDecryptedBuffer();

                if (decryptedBuf->readableBytes() == 0)
                {
                    LOG_INFO("解密缓冲区为空");
                    return;
                }

                buf = decryptedBuf;

                LOG_INFO("解密缓冲区不为空");
            }
        }

        auto it = httpConnections_.find(conn->name());
        if (it != httpConnections_.end())
        {
            // 获取 HttpConnection 的上下文（HttpContext*）
            HttpContext *context = it->second->getMutableContext();

            if (!context->parseRequest(buf, receiveTime))
            {

                conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
                conn->shutdown();
            }

            if (context->gotAll())
            {
                LOG_INFO("响应解析完成，封装响应报文");
                onRequest(conn, context->request());
                context->reset();
            }
            else
            {
                LOG_INFO("封装响应失败");
            }
        }
        else
        {
            LOG_ERROR("No HttpConnection found for connection: {}", conn->name());
        }
    }
    catch (const std::exception &err)
    {

        LOG_ERROR("Expection in onMessage: %s", err.what());
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->shutdown();
    }
}

void HTTPServer::onRequest(const ConnectionPtr &conn, const HttpRequest &req)
{

    const std::string &connection = req.getHeader("Connection"); // 查看是否为短链接
    bool close = ((connection == "close") || (req.getVersion() == "HTTP/1.0" && connection != "Keep-Alive"));
    HttpResponse response(close);

    httpCallback_(req, &response); // 开始填充响应

    Buffer buf;
    response.appendToBuffer(&buf);

    if (useSSL_)
    {
        auto it = sslConns_.find(conn);
        if (it != sslConns_.end())
        {
            it->second->send(buf.peek(), buf.readableBytes()); // 通过 SslConnection 加密发送
        }
    }
    else
    {
        conn->send(buf.retrieveAllAsString());
    }

    if (response.getCloseConnection())
    {
        conn->shutdown();
    }
}

void HTTPServer::handleRequest(const HttpRequest &req, HttpResponse *resp)
{

    try
    {
        LOG_INFO("开始路由处理");

        HttpRequest mutableReq = req;
        middlewareChain_.processBefore(mutableReq);

        if (!router_.route(mutableReq, resp))
        {

            LOG_INFO("未找到路由,返回404");
            resp->setStatusCode(HttpResponse::k404NotFound);
            resp->setStatusMessage("Not Found");
            resp->setCloseConnection(true);
        }

        // 处理响应后的中间件
        middlewareChain_.processAfter(*resp);
    }
    catch (const HttpResponse &res)
    {
        *resp = res;
    }
    catch (const std::exception &err)
    {

        resp->setStatusCode(HttpResponse::k500InternalServerError);
        resp->setBody(err.what());
    }
}
