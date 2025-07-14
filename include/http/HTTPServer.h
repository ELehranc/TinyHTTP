#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <unordered_map>
#include <mymuduo/TcpServer.h>
#include <mymuduo/EventLoop.h>
#include <mymuduo/Logger.h>

#include "HttpContext.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Router.h"
#include "RouterHandler.h"
#include "HttpConnection.h"

#include "SslConfig.h"
#include "SslContext.h"
#include "SslConnection.h"

#include "SessionManager.h"
#include "MiddlewareChain.h"
#include "CorsMiddleware.h"
class HTTPServer
{

public:
    using HttpCallback = std::function<void(const HttpRequest &, HttpResponse *)>;

    HTTPServer(int port, const std::string &name, bool useSSL = false, TcpServer::Option option = TcpServer::kNoReusePort);

    void setThreadNum(int numThreads) { server_.setThreadNum(numThreads); }

    void start();
    void start(const std::string &cerFile, const std::string &keyFile);
    EventLoop *getLoop() const { return server_.getLoop(); }

    void setHttpCallback(const HttpCallback &cb) { httpCallback_ = cb; }

    // 注册静态路由处理器
    void Get(const std::string &path, const HttpCallback &cb) { router_.registerCallback(HttpRequest::kGet, path, cb); }
    // 注册静态路由回调函数
    void Get(const std::string &path, Router::HandlerPtr handler) { router_.registerHandler(HttpRequest::kGet, path, handler); }
    // 注册静态路由处理器
    void Post(const std::string &path, const HttpCallback &cb) { router_.registerCallback(HttpRequest::kPost, path, cb); }
    // 注册静态路由回调函数
    void Post(const std::string &path, Router::HandlerPtr handler) { router_.registerHandler(HttpRequest::kPost, path, handler); }

    // 注册动态路由处理器
    void addRoute(HttpRequest::Method method, const std::string path, Router::HandlerPtr handler) { router_.addRegexHandler(method, path, handler); }
    void addRoute(HttpRequest::Method method, const std::string path, Router::HandlerCallback &callback) { router_.addRegexCallback(method, path, callback); }

    void addMiddleware(std::shared_ptr<Middleware> middleware) { middlewareChain_.addMiddleware(middleware); }

    // 设置会话管理器
    void setSessionManager(std::unique_ptr<SessionManager> manager) { sessionManager_ = std::move(manager); }
    // 获取会话管理器
    SessionManager *getSessionManager() const { return sessionManager_.get(); }
    // 添加中间件

    // 使能SSL
    void enableSSL(bool enable)
    {
        useSSL_ = enable;
    }
    // 设置SSL配置
    void setSslConfig(const SslConfig &config);

private:
    void initialize();
    void onConnection(const ConnectionPtr &conn);
    void onMessage(const ConnectionPtr &conn, Buffer *buf, Timestamp receiveTime);
    void onRequest(const ConnectionPtr &conn, const HttpRequest &req);
    void handleRequest(const HttpRequest &req, HttpResponse *resp);

private:
    InetAddress listenAddr_; // 监听地址
    EventLoop mainLoop_;     // 主循环
    TcpServer server_;
    bool useSSL_;                                    // 是否使用 SSL
    HttpCallback httpCallback_;                      // http处理回调函数
    Router router_;                                  // 路由
    std::unique_ptr<SessionManager> sessionManager_; // 会话管理器
    MiddlewareChain middlewareChain_;                // 中间件链
    std::unique_ptr<SslContext> sslCtx_;             // SSL 上下文

    std::map<ConnectionPtr, std::shared_ptr<SslConnection>> sslConns_; // ssl连接管理容器

    std::unordered_map<std::string, HttpConnection::HttpConnectionPtr> httpConnections_; // 管理httpconn连接
};