#include "httprequest.h"

#include "httpresponse.h"
#include <algorithm>
#include <cctype>
#include <fmt/printf.h>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace net;

static std::map<int, std::string> responseCodes = std::map<int, std::string>(
    {{100, "Continue"},
     {101, "Switching Protocols"},
     {200, "OK"},
     {201, "Created"},
     {202, "Accepted"},
     {203, "Non-Authoritative Information"},
     {204, "No Content"},
     {205, "Reset Content"},
     {206, "Partial Content"},
     {300, "Multiple Choices"},
     {301, "Moved Permanently"},
     {302, "Found"},
     {303, "See Other"},
     {304, "Not Modified"},
     {305, "Use Proxy"},
     {306, "(Unused)"},
     {307, "Temporary Redirect"},
     {400, "Bad Request"},
     {401, "Unauthorized"},
     {402, "Payment Required"},
     {403, "Forbidden"},
     {404, "Not Found"},
     {405, "Method Not Allowed"},
     {406, "Not Acceptable"},
     {407, "Proxy Authentication Required"},
     {408, "Request Timeout"},
     {409, "Conflict"},
     {410, "Gone"},
     {411, "Length Required"},
     {412, "Precondition Failed"},
     {413, "Request Entity Too Large"},
     {414, "Request-URI Too Long"},
     {415, "Unsupported Media Type"},
     {416, "Requested Range Not Satisfiable"},
     {417, "Expectation Failed"},
     {500, "Internal Server Error"},
     {501, "Not Implemented"},
     {502, "Bad Gateway"},
     {503, "Service Unavailable"},
     {504, "Gateway Timeout"},
     {505, "HTTP Version Not Supported"}});

// trim from start
static inline std::string &ltrim(
    std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int c) { return !std::isspace(c); }));
    return s;
}

// trim from end
static inline std::string &rtrim(
    std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         [](int c) { return !std::isspace(c); })
                .base(),
            s.end());
    return s;
}

// trim from both ends
static inline std::string &trim(
    std::string &s)
{
    return ltrim(rtrim(s));
}

Request::Request(
    SOCKET socket,
    sockaddr_in clientInfo)
    : _socket(socket),
      _clientInfo(clientInfo)
{
    std::string allData = getMessage();
    auto pos = allData.find("\r\n\r\n");
    if (pos != std::string::npos)
    {
        std::stringstream ss;
        ss.str(allData.substr(0, pos));
        std::string line;

        // Determine methode, uri and HTTP version
        if (std::getline(ss, line))
        {
            trim(line);
            auto first = line.find_first_of(' ');
            auto last = line.find_last_of(' ');
            if (first != std::string::npos && last != std::string::npos)
            {
                auto version = line.substr(last);
                if (trim(version) != "HTTP/1.1")
                    throw new std::runtime_error("invalid HTTP version");

                auto method = line.substr(0, first);
                this->_method = trim(method);

                auto uri = line.substr(first, last - first);
                this->_uri = trim(uri);
            }
        }

        // Determine headers
        while (std::getline(ss, line))
        {
            auto found = line.find_first_of(':');
            if (found != std::string::npos)
            {
                auto key = line.substr(0, found);
                auto value = line.substr(found + 1);
                this->_headers.insert(std::make_pair(trim(key), trim(value)));
            }
        }

        // And finally determine payload(if any)
        this->_payload = allData.substr(pos + 4);
    }
}

std::string Request::getMessage()
{
    char buffer[BUFFER_SIZE + 1];
    int bytes;

    bytes = recv(this->_socket, buffer, BUFFER_SIZE, 0);
    if (bytes < 0)
    {
        return "";
    }

    buffer[bytes] = '\0';

    std::stringstream browserData;
    browserData << buffer;

    while (BUFFER_SIZE == bytes)
    {
        bytes = recv(this->_socket, buffer, BUFFER_SIZE, 0);
        buffer[bytes] = '\0';
        browserData << buffer;
    }

    return browserData.str();
}

#define STREAMING_TRESHOLD 1024

bool KeepAlive(
    const std::map<std::string, std::string> &headers)
{
    for (const auto &header : headers)
    {
        if (header.first == "Connection" && header.second == "keep-alive")
        {
            return true;
        }
    }

    return false;
}

// The function we want to execute on the new thread.
void Request::handleRequest(
    std::function<int(const Request &, Response &)> onConnection,
    Request request)
{
    if (request._method == "")
    {
        // This is no HTTP request
        shutdown(request._socket, SD_BOTH);
        closesocket(request._socket);

        return;
    }

    while (true)
    {
        fmt::print("\n============================================================\nRequest Headers\n");
        for (const auto &header : request._headers)
        {
            fmt::print("{}: {}\n", header.first, header.second);
        }

        Response response;

        int responseCode = onConnection(request, response);

        std::stringstream headerData;

        headerData << "HTTP/1.1 " << responseCode << " " << responseCodes[responseCode] << "\r\n";

        for (auto &pair : response._headers)
        {
            headerData << pair.first << ": " << pair.second << "\r\n";
        }

        long long contentSize = response._contentSize;

        if (contentSize == 0 && !response._response.empty())
        {
            contentSize = response._response.size();
        }

        headerData << "Content-Length: " << contentSize << "\r\n"
                << "\r\n";

        fmt::print("============================================================\nResponse Headers\n");
        for (const auto &header : response._headers)
        {
            fmt::print("{}: {}\n", header.first, header.second);
        }

        auto hdrs = headerData.str();

        send(request._socket, headerData.str().c_str(), headerData.str().size(), 0);

        send(request._socket, response._response.c_str(), response._response.size(), 0);

        if (!KeepAlive(request._headers))
        {
            fmt::print("####### Not keeping this connection alive ####### \n\n");
            break;
        }

        request = Request(request._socket, request._clientInfo);
    }

    shutdown(request._socket, SD_BOTH);
    closesocket(request._socket);
}

std::string Request::ipAddress() const
{
    return std::string(inet_ntoa(this->_clientInfo.sin_addr));
}
