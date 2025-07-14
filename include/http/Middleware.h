#pragma once

#include "HttpRequest.h"
#include "HttpResponse.h"

class Middleware
{

public:
    virtual ~Middleware() = default;

    virtual void before(HttpRequest &request) = 0;
    virtual void after(HttpResponse &response) = 0;

    void setNext(std::shared_ptr<Middleware> next) { nextMiddleware_ = next; }

protected:
    std::shared_ptr<Middleware> nextMiddleware_;
};