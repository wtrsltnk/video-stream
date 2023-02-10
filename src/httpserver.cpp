#include "httpserver.h"
#include "httprequest.h"
#include "httpresponse.h"
#include <fmt/format.h>
#include <iostream>
#include <sstream>
#include <thread>

using namespace net;

// Global Functions
std::string ToString(
    int data)
{
    std::stringstream buffer;
    buffer << data;
    return buffer.str();
}

// Constructor
HttpServer::HttpServer(
    int port)
    : _logger([](const std::string &) {})
{
    // Socket Settings
    ZeroMemory(&_hints, sizeof(_hints));
    _hints.ai_family = AF_INET;
    _hints.ai_flags = AI_PASSIVE;
    _hints.ai_socktype = SOCK_STREAM;
    _hints.ai_protocol = IPPROTO_TCP;

    // Initialize Data
    _maxConnections = 50;
    _listeningSocket = INVALID_SOCKET;
    _socketVersion = MAKEWORD(2, 2);
    _port = port;
    _result = nullptr;
}

void HttpServer::SetLogger(
    std::function<void(const std::string &)> logger)
{
    this->_logger = logger;
}

int HttpServer::Port() const
{
    return _port;
}
void HttpServer::SetPort(
    int port)
{
    _port = port;
}

std::string HttpServer::LocalUrl() const
{
    return fmt::format("http://localhost:{}/", _port);
}

bool HttpServer::Init()
{
    // Start Winsocket
    auto resultCode = WSAStartup(_socketVersion, &_wsaData);
    if (0 != resultCode)
    {
        this->_logger("Initialize Failed \nError Code : " + ToString(resultCode));
        return false;
    }

    return true;
}

bool HttpServer::Start()
{
    std::string port = ToString(_port);

    // Resolve Local Address And Port
    auto resultCode = getaddrinfo(nullptr, port.c_str(), &_hints, &_result);
    if (0 != resultCode)
    {
        this->_logger("Resolving Address And Port Failed");
        this->_logger(fmt::format("Error Code: {}", ToString(resultCode)));

        return false;
    }

    // Create Socket
    _listeningSocket = socket(_hints.ai_family, _hints.ai_socktype, _hints.ai_protocol);
    if (INVALID_SOCKET == _listeningSocket)
    {
        this->_logger("Could't Create Socket");

        return false;
    }

    // Bind
    resultCode = bind(_listeningSocket, _result->ai_addr, static_cast<int>(_result->ai_addrlen));
    if (SOCKET_ERROR == resultCode)
    {
        this->_logger("Bind Socket Failed");

        return false;
    }

    // Listen
    resultCode = listen(_listeningSocket, _maxConnections);
    if (SOCKET_ERROR == resultCode)
    {
        this->_logger(fmt::format("Listening On Port {} Failed", ToString(_port)));

        return false;
    }
    else
    {
        this->_logger("Server Is Up And Running.");
        this->_logger(fmt::format("Listening On Port {}", ToString(_port)));
    }

    return true;
}

void HttpServer::WaitForRequests(
    std::function<int(const Request &, Response &)> onConnection)
{
    sockaddr_in clientInfo;
    int clientInfoSize = sizeof(clientInfo);

    auto socket = accept(_listeningSocket, reinterpret_cast<sockaddr *>(&clientInfo), &clientInfoSize);
    if (INVALID_SOCKET == socket)
    {
        this->_logger("Accepting Connection Failed");
    }
    else
    {
        this->_logger("Spinning thread to handle request");

        std::thread t(Request::handleRequest, onConnection, Request(socket, clientInfo, _logger));

        t.detach();
    }
}

void HttpServer::Stop()
{
    closesocket(_listeningSocket);
    _listeningSocket = 0;
    WSACleanup();
}

bool HttpServer::IsStarted() const
{
    return _listeningSocket != 0;
}
