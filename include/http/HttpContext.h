#pragma once
#include <iostream>
#include <netsocket/net/TcpServer.h>
#include "HttpRequest.h"

class HttpRequest;

class HttpContext
{

public:
    enum HttpRequestParseState
    {

        kExpectRequestLine, // 解析请求行
        kExpectHeaders,     // 解析请求头
        kExpectBody,        // 解析请求体
        kGotAll,            // 解析完成
    };

    HttpContext() : state_(kExpectRequestLine) {}

    bool parseRequest(Buffer *buf, Timestamp receiveTime);
    bool gotAll() const { return state_ == kGotAll; }

    void reset()
    {

        state_ = kExpectRequestLine;
        HttpRequest dummyData;
        request_.swap(dummyData);
    }

    const HttpRequest &request() const
    {
        return request_;
    }

    HttpRequest &request()
    {
        return request_;
    }

private:
    bool processRequestLine(const char *begin, const char *end);

private:
    HttpRequestParseState state_;
    HttpRequest request_;
};
