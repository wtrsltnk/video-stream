#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <functional>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#define DEFAULT_PORT 8080

namespace net {

class HttpServer
{
public:
    HttpServer(int port = DEFAULT_PORT);

    void SetLogger(
        std::function<void(const std::string &)> logger);

    int Port() const;

    void SetPort(
        int port);

    std::string LocalUrl() const;

    bool Init(); // Initialize Server

    bool Start(); // Start server

    void WaitForRequests(
        std::function<int(const class Request &, class Response &)> onConnection);

    void Stop(); // Close Server

    bool IsStarted() const;

    static void handleRequest(
        std::function<int(const class Request &, class Response &)> onConnection,
        class Request request);

private:
    std::function<void(const std::string &)> _logger;
    WORD _socketVersion;
    WSADATA _wsaData;
    SOCKET _listeningSocket;
    addrinfo _hints, *_result;
    int _maxConnections = 100;

    //  Server Info
    int _port = DEFAULT_PORT;
};

} // namespace net

#endif // HTTPSERVER_H
