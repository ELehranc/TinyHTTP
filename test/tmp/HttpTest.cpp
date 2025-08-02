#include "HttpContext.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include <netsocket/net/uffer.h>
#include <netsocket/base/Timestamp.h>

#include <cassert>
#include <sstream>
#include <cstring>
#include <iostream>
void testRequestParsing_Get()
{
    const char *requestData =
        "GET /path?key1=value1&key2=value2 HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "User-Agent: TestAgent\r\n"
        "\r\n"; // 空行表示头部结束

    Buffer buf;
    buf.append(requestData, std::strlen(requestData));
// 
    HttpContext context;
    Timestamp receiveTime = Timestamp::now();

    bool ok = context.parseRequest(&buf, receiveTime);

    assert(ok && context.gotAll());
    const HttpRequest &req = context.request();

    assert(req.getMethod() == HttpRequest::kGet);
    assert(req.getPath() == "/path");
    assert(req.getQueryParameters("key1") == "value1");
    assert(req.getQueryParameters("key2") == "value2");
    assert(req.getVersion() == "HTTP/1.1");

    assert(req.getHeader("Host") == "example.com");
    assert(req.getHeader("User-Agent") == "TestAgent");
    assert(req.getContentLength() == 0);
    assert(req.getBody().empty());
}

// 测试 HTTP 请求解析（POST 方法带请求体）
void testRequestParsing_POST()
{
    const char *requestData =
        "POST /api/upload HTTP/1.1\r\n"
        "Host: api.example.com\r\n"
        "Content-Length: 16\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "\r\n"
        "name=test&age=20"; // 请求体

    Buffer buf;
    buf.append(requestData, strlen(requestData));

    HttpContext context;
    bool ok = context.parseRequest(&buf, Timestamp::now());

    assert(ok && context.gotAll());
    const HttpRequest &req = context.request();

    assert(req.getMethod() == HttpRequest::kPost);
    assert(req.getContentLength() == 16);
    std::cout << req.getBody() << std::endl;
    assert(req.getBody() == "name=test&age=20");

    assert(req.getHeader("Content-Type") == "application/x-www-form-urlencoded");
}

void testResponseBuilding_200OK()
{
    HttpResponse response(false); // 长连接
    response.setVersion("HTTP/1.1");
    response.setStatusCode(HttpResponse::k200Ok); // 自动对应 "OK"，无需手动设置
    response.setContentType("text/plain; charset=utf-8");
    response.setBody("hello world");                    // 正确长度：11（h e l l o   w o r l d）
    response.addHeader("X-Custom-Header", "TestValue"); // 修正拼写："Custom" 而非 "Custon"

    Buffer buf;
    response.appendToBuffer(&buf);
    std::string output = buf.retrieveAllAsString();

    std::cout << "===== 响应内容开始 =====" << std::endl;
    std::cout << output << std::endl;
    std::cout << "===== 响应内容结束 =====" << std::endl;
}

// 测试错误请求解析（格式错误的请求行）
void testRequestParsing_Error()
{
    const char *badRequestData = "INVALID-METHOD /path HTTP/1.1\r\n\r\n";
    Buffer buf;
    buf.append(badRequestData, strlen(badRequestData));

    HttpContext context;
    bool ok = context.parseRequest(&buf, Timestamp::now());

    assert(!ok); // 方法无效，解析失败
    assert(context.request().getMethod() == HttpRequest::kInvalid);
}

int main()
{
    testRequestParsing_Get();
    testRequestParsing_POST();
    testResponseBuilding_200OK();
    testRequestParsing_Error();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}