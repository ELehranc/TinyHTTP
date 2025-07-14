#include "Router.h"
#include "mymuduo/Logger.h"

void Router::registerHandler(HttpRequest::Method method, const std::string &path, HandlerPtr handler)
{
    RouteKey key{method, path};
    handlers_[key] = std::move(handler);
}

void Router::registerCallback(HttpRequest::Method method, const std::string &path, const HandlerCallback &callback)
{

    RouteKey key{method, path};
    callbacks_[key] = std::move(callback);
}

bool Router::route(const HttpRequest &req, HttpResponse *resp)
{

    RouteKey key{req.getMethod(), req.getPath()};

    // 先查处理器
    auto handlerIt = handlers_.find(key);
    if (handlerIt != handlers_.end())
    {

        handlerIt->second->handle(req, resp);
        return true;
    }
    // 在查回调函数
    auto callbackIt = callbacks_.find(key);
    if (callbackIt != callbacks_.end())
    {
        callbackIt->second(req, resp);
        return true;
    }

    for (const auto &[method, pathRegex, handler] : regexHandlers_)
    {

        std::smatch match;
        std::string pathStr(req.getPath());
        if (method == req.getMethod() && std::regex_match(pathStr, match, pathRegex))
        {

            HttpRequest newReq(req);
            extractPathParameters(match, newReq);

            handler->handle(newReq, resp);
            return true;
        }
    }

    // 查找动态路由回调函数
    for (const auto &[method, pathRegex, callback] : regexCallbacks_)
    {
        std::smatch match;
        std::string pathStr(req.getPath());
        // 如果方法匹配并且动态路由匹配，则执行回调函数
        if (method == req.getMethod() && std::regex_match(pathStr, match, pathRegex))
        {
            // Extract path parameters and add them to the request
            HttpRequest newReq(req); // 因为这里需要用这一次所以是可以改的
            extractPathParameters(match, newReq);

            callback(newReq, resp);
            return true;
        }
    }
    return false;
}