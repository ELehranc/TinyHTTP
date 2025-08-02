

#include "CorsMiddleware.h"
#include "netsocket/base/Logger.h"
#include <sstream>

CorsMiddleware::CorsMiddleware(const CorsConfig &config) : config_(config) {}

void CorsMiddleware::before(HttpRequest &request)
{

    if (request.getMethod() == HttpRequest::Method::kOptions)
    {

        LOG_INFO("Processing CORS preflight request");
        HttpResponse response;
        handlePreflightRequest(request, response);
        throw response;
    }
}

void CorsMiddleware ::after(HttpResponse &response)
{

    if (!config_.allowedOrigins.empty())
    {
        if (std::find(config_.allowedOrigins.begin(), config_.allowedOrigins.end(), "*") != config_.allowedOrigins.end())
        {
            addCorsHeaders(response, "*");
        }
        else
        {
            addCorsHeaders(response, config_.allowedOrigins[0]); // 此处可能需要修改为目标指定的域名
        }
    }
}

bool CorsMiddleware::isOriginAllowed(const std::string &origin) const
{

    return config_.allowedOrigins.empty() ||
           std::find(config_.allowedOrigins.begin(), config_.allowedOrigins.end(), "*") != config_.allowedOrigins.end() ||
           std::find(config_.allowedOrigins.begin(), config_.allowedOrigins.end(), origin) != config_.allowedOrigins.end();
}

void CorsMiddleware::handlePreflightRequest(const HttpRequest &request, HttpResponse &response)
{

    const std::string &origin = request.getHeader("Origin");

    if (!isOriginAllowed(origin))
    {
        response.setStatusCode(HttpResponse::k403Forbidden);
        return;
    }

    addCorsHeaders(response, origin);
    response.setStatusCode((HttpResponse::k204NoContent));
    LOG_INFO("Preflight request processed successfully");
}

void CorsMiddleware::addCorsHeaders(HttpResponse &response, const std::string &origin)
{

    try
    {
        response.addHeader("Access-Control-Allow-Origin", origin);

        if (config_.allowCredentials)
        {

            response.addHeader("Access-Control-Allow-Credentials", "true");
        }
        if (!config_.allowedMethods.empty())
        {

            response.addHeader("Access-Control-Allow-Headers", join(config_.allowedHeaders, ","));
        }

        response.addHeader("Acess-Control-Max-Age", std::to_string(config_.maxAge));
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Error adding CORS headers : %s", e.what());
    }
}

std::string CorsMiddleware::join(const std::vector<std::string> &strings, const std::string &delimiter)
{

    std::ostringstream result;
    for (size_t i = 0; i < strings.size(); i++)
    {

        if (i > 0)
        {

            result << delimiter;
            result << strings[i];
        }
    }
    return result.str();
}