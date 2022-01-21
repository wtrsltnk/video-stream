#ifndef HTTPROUTER_H
#define HTTPROUTER_H

#include "httprequest.h"
#include "httpresponse.h"
#include <functional>
#include <regex>

typedef std::function<void(const net::Request &request, net::Response &response, const std::smatch &matches)> RouteHandler;
typedef std::vector<std::pair<std::regex, RouteHandler>> RouteCollection;

class Router
{
public:
    void Get(
        const std::string &pattern,
        RouteHandler handler);

    void Post(
        const std::string &pattern,
        RouteHandler handler);

    bool Route(
        const net::Request &request,
        net::Response &response);

private:
    RouteCollection _getRoutes;
    RouteCollection _postRoutes;
    RouteCollection _putRoutes;
};

#endif // HTTPROUTER_H
