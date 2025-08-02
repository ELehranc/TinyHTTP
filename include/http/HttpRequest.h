#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <netsocket/base/Timestamp.h>

class HttpRequest
{

public:
    enum Method
    {

        kInvalid,
        kGet,
        kPost,
        kHead,
        kPut,
        kDelete,
        kOptions
    };

    HttpRequest() : method_(kInvalid), version_("UnKnown") {}

    void setReceiveTime(Timestamp time);
    Timestamp getReceiveTime() const { return receiveTime_; }

    bool setMethod(const char *start, const char *end);
    Method getMethod() const { return method_; }

    void setPath(const char *start, const char *end);
    std::string getPath() const { return path_; }

    void setPathParameters(const std::string &key, const std::string &value);
    std::string getPathParameters(const std::string &key) const;

    void setQueryParameters(const char *start, const char *end);
    std::string getQueryParameters(const std::string &key) const;

    void setVersion(std::string ver) { version_ = ver; }
    std::string getVersion() const { return version_; }

    void addHeader(const char *start, const char *colon, const char *end);
    std::string getHeader(const std::string &field) const;

    const std::map<std::string, std::string> &getHeaders() const { return headers_; }

    void setBody(const std::string &body) { content_ = body; }
    void setBody(const char *start, const char *end)
    {
        if (end >= start)
        {
            content_.assign(start, end - start);
        }
    }
    std::string getBody() const { return content_; }

    void setContentLength(uint64_t length) { contentLength_ = length; }
    uint64_t getContentLength() const { return contentLength_; }

    void swap(HttpRequest &other);
private:
    Method method_;                                                // 请求方法
    std::string version_;                                          // http版本
    std::string path_;                                             // 请求路径
    std::unordered_map<std::string, std::string> pathParameters_;  // 路径参数
    std::unordered_map<std::string, std::string> queryParameters_; // 查询参数
    Timestamp receiveTime_;                                        // 接收时间
    std::map<std::string, std::string> headers_;                   // 请求头
    std::string content_;                                          // 请求体
    uint64_t contentLength_{0};                                    // 请求体长度
};
