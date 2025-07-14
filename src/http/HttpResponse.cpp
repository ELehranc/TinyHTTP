#include "HttpResponse.h"
#include <cstring>
#include <sstream>

void HttpResponse::appendToBuffer(Buffer *outputBuf) const
{
    std::stringstream ss;
    ss << httpVersion_ << ' ' << statusCode_ << ' ' << statusMessage_ << "\r\n";

    if (closeConnection_)
    {
        ss << "Connection: close\r\n";
    }
    else
    {
        ss << "Connection: Keep-Alive\r\n"
           << "Content-Length: " << body_.size() << "\r\n";
    }

    for (const auto &header : headers_)
    {
        ss << header.first << ": " << header.second << "\r\n";
    }

    ss << "\r\n"
       << body_;

    std::string output = ss.str();
    outputBuf->append(output.data(), output.size());
}

void HttpResponse::setStatusLine(const std::string &version, HttpStatusCode statusCode, const std::string &statusMessage)
{
    httpVersion_ = version;
    statusCode_ = statusCode;
    statusMessage_ = statusMessage;
}