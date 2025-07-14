#include "HttpContext.h"
#include <mymuduo/Logger.h>

// 将报文解析出来，将关键信息封装到HttpRequest对象当中
bool HttpContext::parseRequest(Buffer *buf, Timestamp receiveTime)
{
    LOG_INFO("开始解析http请求");
    bool ok = true; // 解析每行请求格式是否正确
    bool hasMore = true;
    std::string temp(buf->peek(), buf->readableBytes());
    std::cout << temp << std::endl;

    while (hasMore)
    {
        if (state_ == kExpectRequestLine)
        {
            LOG_INFO("正在处理请求行");
            const char *crlf = buf->findCRLF();
            // 查找请求行的结束位置（即 \r\n）
            // 比方说找到 "GET /index.html?param1=value1&param2=value2 HTTP/1.1\r\n" 中 \r 的位置

            if (crlf)
            {

                ok = processRequestLine(buf->peek(), crlf);
                // 传递 “GET /index.html?param1=value1&param2=value2 HTTP/1.1” 给函数

                if (ok)
                {
                    LOG_INFO("请求行解析成功");
                    request_.setReceiveTime(receiveTime);
                    buf->retrieveUntil(crlf + 2);
                    // 使得buf的读指针向前移动至下一行，跳过/r/n
                    state_ = kExpectHeaders;
                }
                else
                {
                    LOG_INFO("请求行解析失败");
                    hasMore = false;
                }
            }
            else
            {
                LOG_INFO("未找到请求行CRLF位置");
                hasMore = false;
            }
        }
        else if (state_ == kExpectHeaders)
        {

            const char *crlf = buf->findCRLF();
            // 查找请求头这一行的结束位置（\r\n）
            // 比方说会找到“Host: example.com\r\n”，会找到\r的位置

            if (crlf)
            {
                const char *colon = std::find(buf->peek(), crlf, ':');
                // 找到：的位置

                if (colon < crlf)
                {
                    request_.addHeader(buf->peek(), colon, crlf);
                    // 这里会将Host和example.com 作为键值对添加到headrs_里
                }
                else if (buf->peek() == crlf)
                {
                    // 空行，结束Headr的解析
                    // 根据请求方法和Content-Length判断是否继续读取body
                    if (request_.getMethod() == HttpRequest::kPost || request_.getMethod() == HttpRequest::kPut)
                    {

                        std::string contentLength = request_.getHeader("Content-Length");
                        if (!contentLength.empty())
                        {
                            request_.setContentLength(std::stoi(contentLength));
                            // 如果存在请求体，将请求体的长度存入request中

                            if (request_.getContentLength() > 0)
                            {
                                state_ = kExpectBody;
                                // 如果请求体的长度不为0，则说明有请求体，需要进行对请求体的解析
                            }
                            else
                            {

                                state_ = kGotAll;
                                hasMore = false;

                                // 如果没有请求体，那么对于请求的解析就完成了
                            }
                        }
                        else
                        {

                            // POST和PUT 请求没有Content-Length 是语法错误，需要直接返回
                            ok = false;
                            hasMore = false;
                        }
                    }
                    else
                    {
                        // GET/HEAD/DELETE 等方法没有消息体，直接返回就好
                        state_ = kGotAll;
                        hasMore = false;
                    }
                }
                else
                { // ：符号大于\r的位置，是HTTP语法错误
                    ok = false;
                    hasMore = false;
                }
                buf->retrieveUntil(crlf + 2); // 开始读取下一行的数据了
            }
            else
            {
                hasMore = false;
                // 没有找到\r\n，说明请求头行不完整，停止解析
            }
        }
        else if (state_ == kExpectBody)
        {

            if (buf->readableBytes() < request_.getContentLength())
            {
                hasMore = false; // 数据不完整，等待更多的数据
                return true;
            }

            // 只读取 Content-Length 指定的长度
            std::string body(buf->peek(), buf->peek() + request_.getContentLength());
            LOG_INFO("BODY_LEN : %d,body: %s", body.size(), body);
            request_.setBody(body);

            // 移动buf指针
            buf->retrieve(request_.getContentLength());

            state_ = kGotAll;
            hasMore = false;
        }
    }
    return ok; // ok为false则报文语法解析错误，为true则报文解析完成
}
// 解析请求行
bool HttpContext::processRequestLine(const char *begin, const char *end)
{
    bool succeed = false;
    const char *start = begin;
    const char *space = std::find(start, end, ' ');
    // 查找请求行中的第一个空格
    // 在 "GET /index.html?param1=value1&param2=value2 HTTP/1.1" 中，会找到第一个空格的位置
    if (space != end && request_.setMethod(start, space))
    {
        // 设置请求方法
        // 将"GET"设置为请求方法

        start = space + 1;
        space = std::find(start, end, ' ');
        // 查找请求行中的第二个空格
        // 在 "GET /index.html?param1=value1&param2=value2 HTTP/1.1" 中，会找到第二个空格的位置

        if (space != end)
        {
            const char *argumentStart = std::find(start, space, '?');
            // 查找请求路径中的问好，用于分割路径和查询参数
            // “/index.html?param1=value&param2=value2”中找到？的位置

            if (argumentStart != space) // 也就是找到了？，不等于最后一位的空格
            {
                request_.setPath(start, argumentStart); // 左闭又开
                // 设置路径“/index.html”
                request_.setQueryParameters(argumentStart + 1, space);
                // 设置参数“param1=value&param2=value2”
            }
            else
            {
                request_.setPath(start, space); // 没有找到？，所以没有参数，直接设置路径
            }

            start = space + 1;
            succeed = ((end - start == 8) && std::equal(start, end - 1, "HTTP/1."));
            // 查找最后的一个版本号
            // "GET /index.html?param1=value1&param2=value2 HTTP/1.1"
            // start位于'H'的位置
            if (succeed)
            {
                if (*(end - 1) == '1')
                {
                    request_.setVersion("HTTP/1.1");
                }
                else if (*(end - 1) == '0')
                {
                    request_.setVersion("HTTP/1.0");
                }
                else
                {
                    succeed = false;
                }
            }
        }
    }
    return succeed;
    // 请求行的解析完毕
}