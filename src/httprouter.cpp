#include "httprouter.h"

#include <fmt/format.h>

Router::Router()
    : _logger([](const std::string &) {})
{}

void Router::SetLogger(
    std::function<void(const std::string &)> logger)
{
    this->_logger = logger;
}

void Router::Get(
    const std::string &pattern,
    RouteHandler handler)
{
    auto r = std::regex(pattern);

    _getRoutes.push_back(std::make_pair(std::move(r), handler));
}

void Router::Post(
    const std::string &pattern,
    RouteHandler handler)
{
    auto r = std::regex(pattern);

    _postRoutes.push_back(std::make_pair(std::move(r), handler));
}

bool Router::Route(
    const net::Request &request,
    net::Response &response)
{
    if (request._method == "GET")
    {
        for (auto &route : _getRoutes)
        {
            std::smatch matches;
            if (!std::regex_match(request._uri, matches, route.first))
            {
                continue;
            }

            route.second(request, response, matches);
        }
    }
    else if (request._method == "POST")
    {
        for (auto &route : _postRoutes)
        {
            std::smatch matches;
            if (!std::regex_match(request._uri, matches, route.first))
            {
                continue;
            }

            route.second(request, response, matches);
        }
    }
    else
    {
        return false;
    }

    _logger(fmt::format("{} {} {}", request._method, request._uri, response._responseCode));

    return true;
}
