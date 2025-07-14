#include "MiddlewareChain.h"
#include <mymuduo/Logger.h>

void MiddlewareChain::addMiddleware(std::shared_ptr<Middleware> middleware)
{
    middlewares_.push_back(middleware);
}

void MiddlewareChain::processBefore(HttpRequest &request)
{

    for (auto &middleware : middlewares_)
    {

        middleware->before(request);
    }
}

void MiddlewareChain::processAfter(HttpResponse &response)
{

    try
    {

        for (auto it = middlewares_.rbegin(); it != middlewares_.rend(); ++it)
        {

            if ((*it))
            {

                (*it)->after(response);

                /* code */
            }
        }
    }
    catch (const std::exception &e)
    {

        LOG_ERROR("Error in middleware after processing : %s", e.what());
    }
}