#include "HTTPServer.h"

int main()
{

    HTTPServer server(54321, "test", false);

    server.setThreadNum(4);

    // 修改路由处理函数
    server.Get("/", [](const HttpRequest &req, HttpResponse *resp)
               {
    // 记录日志，表明开始处理请求
    LOG_INFO("收到根路径请求，准备发送响应数据");

    try {
        std::ifstream file("/home/webserver/Project/TinyHTTP/TinyHTTP/src/Source/index.html");
        if(!file.is_open()){
            throw std::runtime_error("无法打开 HTML 文件");
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string htmlContent = buffer.str();

        // 明确指定协议版本
        resp->setVersion("HTTP/1.1");
        // 设置响应状态码为 200 OK
        resp->setStatusCode(HttpResponse::k200Ok);
        // 设置响应状态消息
        resp->setStatusMessage("OK");
        // 添加响应头，指定响应内容类型为 HTML，字符编码为 UTF-8
        resp->addHeader("Content-Type", "text/html; charset=utf-8");
        // 强制添加 Content-Length 头（关键修复）
        resp->addHeader("Content-Length", std::to_string(htmlContent.size()));
        // 设置响应体内容为 HTML 文件内容
        resp->setBody(htmlContent);
        // 记录日志，表明响应数据已设置完成
        LOG_INFO ( "响应数据设置完成，准备发送");
       
    } catch (const std::exception& e) 
    {
        // 记录异常信息，以便调试和维护
        LOG_ERROR ("设置响应数据时发生异常: %s",e.what());
        // 设置响应状态码为 500 Internal Server Error
        resp->setStatusCode(HttpResponse::k500InternalServerError);
        // 设置响应状态消息
        resp->setStatusMessage("Internal Server Error");
        // 添加响应头，指定响应内容类型为纯文本，字符编码为 UTF-8
        resp->addHeader("Content-Type", "text/plain; charset=utf-8");
        // 设置响应体内容为错误信息
        resp->setBody("An error occurred while processing your request.");
    } });
    server.start();

    return 0;
}