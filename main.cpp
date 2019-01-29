#include "httprequest.h"
#include "httpresponse.h"
#include "httpserver.h"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <regex>
#include <string>
#include <thread>

#define MAX_RANGE_SIZE (1024 * 1024)

int onRecieveRequest(const net::Request &request, net::Response &response);

static std::string root = "../video-stream";

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        root = argv[1];
    }

    net::HttpServer _server;
    std::thread _thread;
    bool _isRunning = true;

    _server.Init();

    if (!_server.Start())
    {
        return 0;
    }

    while (_isRunning)
    {
        _server.WaitForRequests(onRecieveRequest);
    }

    return 0;
}

void GetFiles(std::vector<std::string> &files, std::string const &root, std::string const &relativePath = "/")
{
#ifdef _WIN32
    HANDLE hFind;
    WIN32_FIND_DATA data;

    std::string lookFor = root + relativePath + "*";

    std::cout << lookFor << std::endl;
    hFind = FindFirstFile(lookFor.c_str(), &data);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                files.push_back(relativePath + data.cFileName);
            }
            else
            {
                if (data.cFileName[0] == '.')
                {
                    continue;
                }
                GetFiles(files, root, relativePath + data.cFileName + "/");
            }
        } while (FindNextFile(hFind, &data));
        FindClose(hFind);
    }
#endif // _WIN32
}

const char DirectorySeparatorChar = '\\';
const char AltDirectorySeparatorChar = '/';
#include <algorithm>

std::string FileExtension(std::string const &fullpath)
{
    auto i = fullpath.length();
    while (--i > 0)
    {
        if (fullpath[i] == DirectorySeparatorChar)
            return "";
        if (fullpath[i] == AltDirectorySeparatorChar)
            return "";
        if (fullpath[i] == '.')
        {
            auto data = fullpath.substr(i);
            std::transform(data.begin(), data.end(), data.begin(), ::tolower);
            return data;
        }
    }
    return "";
}
std::string urlDecode(std::string &SRC)
{
    std::string ret;
    char ch;
    int i, ii;
    for (i = 0; i < SRC.length(); i++)
    {
        if (int(SRC[i]) == 37)
        {
            sscanf(SRC.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            ret += ch;
            i = i + 2;
        }
        else
        {
            ret += SRC[i];
        }
    }
    return (ret);
}
int onRecieveRequest(const net::Request &request, net::Response &response)
{
    std::string contentRoot = "../video-stream";
    int result = 200;

    if (request._uri == "/")
    {

        std::ifstream stream(contentRoot + "/index.html");

        response.addHeader("Content-Type", "text/html");
        response._response = std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());

        return result;
    }

    if (request._uri == "/css")
    {

        std::ifstream stream(contentRoot + "/styles.css");

        response.addHeader("Content-Type", "text/css");
        response._response = std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());

        return result;
    }

    if (request._uri == "/js")
    {

        std::ifstream stream(contentRoot + "/scripts.js");

        response.addHeader("Content-Type", "application/javascript");
        response._response = std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());

        return result;
    }

    if (request._uri == "/data")
    {

        response.addHeader("Content-Type", "application/json");

        std::vector<std::string> files;
        GetFiles(files, root);

        const char *separator = "";
        std::stringstream json;
        json << "{\"files\":[";
        for (auto &f : files)
        {
            auto ext = FileExtension(f);
            // if (ext != ".avi" && ext != ".mp4" && ext != ".mov" && ext != ".ogg") {
            if (ext != ".mp4" && ext != ".ogg")
            {
                continue;
            }
            json << separator << "\"" << f << "\"";
            separator = ",";
        }
        json << "]}";

        response._response = json.str();

        return result;
    }

    bool rangeRequested = false;
    unsigned long rangeStart = 0;
    unsigned long rangeEnd = 0;

    for (auto &header : request._headers)
    {

        if (header.first.compare("Range") == 0)
        {
            std::regex pattern("bytes=([0-9]+)\\-([0-9]*)");
            auto words_begin = std::sregex_iterator(header.second.begin(), header.second.end(), pattern);

            if ((*words_begin).size() >= 3)
            {
                rangeRequested = true;

                if ((*words_begin)[1].str().size() > 0)
                {
                    rangeStart = std::stoul((*words_begin)[1].str());
                }
                if ((*words_begin)[2].str().size() > 0)
                {
                    rangeEnd = std::stoul((*words_begin)[2].str());
                }
            }
        }
    }

    response.addHeader("Accept-Ranges", "bytes");

    std::string r = request._uri;
    r = urlDecode(r);
    std::cout << r << std::endl;

    std::stringstream ss;
    ss << root << r;

    std::ifstream stream(ss.str(), std::ios::binary | std::ios::ate);

    long long size = stream.tellg();

    if (!rangeRequested && size < MAX_RANGE_SIZE)
    {
        stream.seekg(0, stream.beg);
        response._response = std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    }
    else
    {
        if (rangeEnd == 0)
        {
            if (size > MAX_RANGE_SIZE)
            {
                rangeEnd = rangeStart + MAX_RANGE_SIZE;
                result = 206;
            }
            else
            {
                rangeEnd = static_cast<unsigned long>(size);
            }
        }
        if (rangeStart > 0)
        {
            result = 206;
        }

        stream.seekg(rangeStart, stream.beg);
        auto rangeSize = rangeEnd - rangeStart;

        std::string buffer;
        buffer.resize(rangeSize);
        stream.read(&buffer[0], static_cast<std::streamsize>(rangeSize));

        std::stringstream contentRange;
        contentRange << "bytes " << rangeStart << "-" << rangeEnd << "/" << size;
        response.addHeader("Content-Range", contentRange.str());

        response._response = buffer;
    }

    if (request._uri.substr(request._uri.size() - 4) == ".ogg")
    {
        response.addHeader("Content-Type", "video/ogg");
    }
    else if (request._uri.substr(request._uri.size() - 4) == ".mp4")
    {
        response.addHeader("Content-Type", "video/mp4");
    }

    return result;
}
