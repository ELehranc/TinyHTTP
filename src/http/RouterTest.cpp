#include "Router.h"
#include "RouterHandler.h"

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "HttpContext.h"

#include <iostream>

class TestHandler : public RouterHandler
{

public:
    void handle(const HttpRequest &req, HttpResponse *resp)
    {

        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
        resp->setContentType("text/plain");
        resp->setVersion("HTTP/1.1");
        resp->setBody("Handler: Hello world");
    }
};

void testCallback(const HttpRequest &req, HttpResponse *resp)
{
    resp->setStatusCode(HttpResponse::k200Ok);
    resp->setStatusMessage("OK");
    resp->setContentType("text/plain");
    resp->setVersion("HTTP/1.1");
    resp->setBody("Callback: Hello world");
}

int main()
{

    Router router;

    router.registerHandler(HttpRequest::Method::kGet, "/static/handler", std::make_shared<TestHandler>());

    router.registerCallback(HttpRequest::Method::kGet, "/static/callback", testCallback);

    router.addRegexHandler(HttpRequest::Method::kGet, "/dynamic/handler/:param", std::make_shared<TestHandler>());

    router.addRegexCallback(HttpRequest::Method::kGet, "/dynamic/callback/:param", testCallback);
    
    // 测试静态路由（处理器）
    const char *requestData1 =
        "GET /static/handler HTTP/1.1\r\n"
        "Host: api.example.com\r\n"
        "Content-Length: 16\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "\r\n"
        "name=test&age=20"; // 请求体

    Buffer buf;
    buf.append(requestData1, strlen(requestData1));

    HttpContext context;
    context.parseRequest(&buf, Timestamp::now());

    HttpRequest staticHandlerReq = context.request();
    HttpResponse staticHandlerResp;
    bool handled = router.route(staticHandlerReq, &staticHandlerResp);
    if (handled)
    {
        std::cout << "Static Handler Response: " << staticHandlerResp.getStatusCode() << std::endl;
    }
    else
    {
        std::cout << "Static Handler Route NOT Found" << std::endl;
    }


    // 测试静态路由（回调函数）
    const char *requestData2 =
        "GET /static/callback HTTP/1.1\r\n"
        "Host: api.example.com\r\n"
        "Content-Length: 16\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "\r\n"
        "name=test&age=20"; // 请求体
    Buffer buf2;
    buf2.append(requestData2, strlen(requestData2));

    HttpContext context2;
    context2.parseRequest(&buf2, Timestamp::now());

    HttpRequest staticCallbackReq = context2.request();
    HttpResponse staticCallbackResp;
    bool handled2 = router.route(staticCallbackReq, &staticCallbackResp);
    if (handled2)
    {
        std::cout << "Static Callback Response: " << staticCallbackResp.getStatusCode() << std::endl;
    }
    else
    {
        std::cout << "Static Callback Route NOT Found" << std::endl;
    }

    // 测试动态路由（处理器）
    const char *requestData3 =
        "GET /dynamic/handler/123 HTTP/1.1\r\n"
        "Host: api.example.com\r\n"
        "Content-Length: 16\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "\r\n"
        "name=test&age=20"; // 请求体
    Buffer buf3;
    buf3.append(requestData3, strlen(requestData3));

    HttpContext context3;
    context3.parseRequest(&buf3, Timestamp::now());

    HttpRequest dynamicHandlerReq = context3.request();
    HttpResponse dynamicHandlerResp;
    bool handled3 = router.route(dynamicHandlerReq, &dynamicHandlerResp);
    if (handled3)
    {
        std::cout << "dynamic Handler Response: " << dynamicHandlerResp.getStatusCode() << std::endl;
    }
    else
    {
        std::cout << "dynamic Handler Route NOT Found" << std::endl;
    }

    // 测试动态路由（处理器）
    const char *requestData4 =
        "GET /dynamic/callback/123 HTTP/1.1\r\n"
        "Host: api.example.com\r\n"
        "Content-Length: 16\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "\r\n"
        "name=test&age=20"; // 请求体
    Buffer buf4;
    buf4.append(requestData4, strlen(requestData4));

    HttpContext context4;
    context4.parseRequest(&buf4, Timestamp::now());

    HttpRequest dynamicCallbackReq = context4.request();
    HttpResponse dynamicCallbackResp;
    bool handled4 = router.route(dynamicCallbackReq, &dynamicCallbackResp);
    if (handled4)
    {
        std::cout << "dynamic Callback Response: " << dynamicCallbackResp.getStatusCode() << std::endl;
    }
    else
    {
        std::cout << "dynamic Callback Route NOT Found" << std::endl;
    }

    return 0;
}