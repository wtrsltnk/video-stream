#include "httprouter.h"

#include <fmt/chrono.h>

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
    std::time_t t = std::time(nullptr);

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

    fmt::print("{:%Y-%m-%d %H:%M:%S} {} {} {}\n", fmt::localtime(t), request._method, request._uri, response._responseCode);

    return true;
}
