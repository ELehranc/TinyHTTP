#include "HTTPServer.h"
#include "netsocket/base/Logger.h"
#include "SslConfig.h"
#include <fstream>
#include <sstream>

int main(int argc, char *argv[])
{

    auto serverPtr = std::make_unique<HTTPServer>(54321, "HttpsServer", true);

    SslConfig sslConfig;

    std::string cerFile = "/home/webserver/cert/certificate.crt";
    std::string keyFile = "/home/webserver/cert/private.key";

    sslConfig.setCertificateFile(cerFile);
    sslConfig.setPrivateKeyFile(keyFile);
    sslConfig.setProtocolVersion(SSLVersion::TLS_1_2);

    if (access(cerFile.c_str(), R_OK) != 0)
    {

        LOG_FATAL("Cannot read cerificate file : %s", cerFile);
        return 1;
    }

    if (access(keyFile.c_str(), R_OK) != 0)
    {
        LOG_FATAL("Cannot read keyFile file : %s", keyFile);
        return 1;
    }

    serverPtr->setSslConfig(sslConfig);

    serverPtr->setThreadNum(4);

    serverPtr->Get("/", [](const HttpRequest &req, HttpResponse *resp)
                   {
                       LOG_INFO("收到根路径请求，准备发送响应数据");

                       std::ifstream file("/home/webserver/Project/muduo/HttpServer/Source/index.html");
                       if (!file.is_open())
                       {
                           LOG_ERROR("无法打开HTML文件");
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
                       resp->setBody(htmlContent); });
    serverPtr->start();

    return 0;
}
