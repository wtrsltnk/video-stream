#include "httprequest.h"
#include "httpresponse.h"
#include "httprouter.h"
#include "httpserver.h"
#include <chrono>
#include <filesystem>
#include <fmt/chrono.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <nlohmann/json.hpp>
#include <regex>
#include <sstream>
#include <string>
#include <thread>

#define MAX_RANGE_SIZE (1024 * 1024)

int onRecieveRequest(const net::Request &request, net::Response &response);

void GetFiles(
    std::vector<std::string> &files,
    std::filesystem::path const &root,
    std::string const &relativePath = "");

nlohmann::json GetFilesAsJson(
    std::filesystem::path const &root);

std::string urlDecode(
    const std::string &SRC);

static bool _isRunning = true;

bool DoesFileExist(
    net::Response &response,
    const std::filesystem::path &file)
{
    if (!std::filesystem::exists(file))
    {
        response._responseCode = 404;
        response._response = "video file not found";

        return false;
    }

    return true;
}

void FileResponse(
    net::Response &response,
    const std::filesystem::path &file,
    const std::string &contentType)
{
    if (!DoesFileExist(response, file))
    {
        return;
    }

    std::ifstream stream(file);

    response.addHeader("Content-Type", contentType);
    response._response = std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

void FileRangeResponse(
    const net::Request &request,
    net::Response &response,
    const std::filesystem::path &file)
{
    if (!DoesFileExist(response, file))
    {
        return;
    }

    bool rangeRequested = false;
    unsigned long rangeStart = 0;
    unsigned long rangeEnd = 0;

    for (auto &header : request._headers)
    {
        if (header.first.compare("Range") == 0)
        {
            fmt::print("Range: {}\n", header.second);

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

    std::ifstream stream(file, std::ios::binary | std::ios::ate);

    long long size = stream.tellg();

    if (rangeEnd >= size)
    {
        response._responseCode = 416;

        return;
    }

    fmt::print("rangeStart {}\n", rangeStart);
    fmt::print("rangeEnd   {}\n", rangeEnd);
    fmt::print("size       {}\n", size);
    if (!rangeRequested && size < MAX_RANGE_SIZE)
    {
        response._responseCode = 200;

        stream.seekg(0, stream.beg);
        response._response = std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    }
    else
    {
        response.addHeader("Accept-Ranges", "bytes");

        response._responseCode = 206;

        if (rangeEnd == 0)
        {
            if (size > MAX_RANGE_SIZE)
            {
                rangeEnd = rangeStart + MAX_RANGE_SIZE;
            }
            else
            {
                rangeEnd = static_cast<unsigned long>(size);
            }
        }

        stream.seekg(rangeStart, stream.beg);

        auto rangeSize = rangeEnd - rangeStart + 1;

        std::string buffer;
        buffer.resize(rangeSize);
        stream.read(&buffer[0], static_cast<std::streamsize>(rangeSize));

        response.addHeader("Content-Range", fmt::format("bytes {}-{}/{}", rangeStart, rangeEnd, size));

        response._response = buffer;
    }
}

void JsonResponse(
    net::Response &response,
    const nlohmann::json &obj)
{
    response.addHeader("Content-Type", "application/json");
    response._response = obj.dump();
}

int main(
    int argc,
    char *argv[])
{
    static std::filesystem::path videoRoot = std::filesystem::current_path() / ".." / "video-stream";
    static std::filesystem::path contentRoot = std::filesystem::current_path() / ".." / "video-stream";

    if (argc > 1)
    {
        videoRoot = argv[1];
    }

    if (!std::filesystem::exists(videoRoot))
    {
        std::cout << videoRoot << " does not exist" << std::endl;

        return 0;
    }

    if (!std::filesystem::exists(contentRoot))
    {
        std::cout << contentRoot << " does not exist" << std::endl;

        return 0;
    }

    std::cout << videoRoot << " is our video root" << std::endl;
    std::cout << contentRoot << " is our content root" << std::endl;

    net::HttpServer _server(80);

    _server.Init();

    if (!_server.Start())
    {
        return 0;
    }

    Router router;

    router.Get("/", [&](const net::Request &, net::Response &response, const std::smatch &) {
        FileResponse(response, contentRoot / "htdocs" / "index.html", "text/html");
    });

    router.Get("/css", [&](const net::Request &, net::Response &response, const std::smatch &) {
        FileResponse(response, contentRoot / "htdocs" / "styles.css", "text/css");
    });

    router.Get("/js", [&](const net::Request &, net::Response &response, const std::smatch &) {
        FileResponse(response, contentRoot / "htdocs" / "scripts.js", "application/javascript");
    });

    router.Get("/data", [&](const net::Request &, net::Response &response, const std::smatch &) {
        JsonResponse(response, GetFilesAsJson(videoRoot));
    });

    router.Get("/video/.*", [&](const net::Request &request, net::Response &response, const std::smatch &) {
        auto fileName = std::filesystem::path(urlDecode(request._uri).substr(1));

        auto file = videoRoot / fileName;

        if (fileName.extension() == ".ogg")
        {
            response.addHeader("Content-Type", "video/ogg");
        }
        else if (fileName.extension() == ".mp4")
        {
            response.addHeader("Content-Type", "video/mp4");
        }
        else if (fileName.extension() == ".mkv")
        {
            response.addHeader("Content-Type", "video/mkv");
        }

        FileRangeResponse(request, response, file);
    });

    while (_isRunning)
    {
        _server.WaitForRequests([&](const net::Request &request, net::Response &response) -> int {
            router.Route(request, response);

            return response._responseCode;
        });
    }

    return 0;
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

nlohmann::json GetFilesAsJson(
    std::filesystem::path const &root)
{
    std::vector<std::string> files;

    GetFiles(files, root);

    nlohmann::json filesJson = {
        {"files", files},
    };

    return filesJson;
}

void GetFiles(
    std::vector<std::string> &files,
    std::filesystem::path const &root,
    std::string const &relativePath)
{
#ifdef _WIN32
    HANDLE hFind;
    WIN32_FIND_DATA data;

    std::filesystem::path lookFor = root / relativePath / "*";

    std::cout << lookFor << std::endl;
    hFind = FindFirstFile(lookFor.string().c_str(), &data);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                auto ext = FileExtension(data.cFileName);
                if (ext != ".mp4" && ext != ".ogg" && ext != ".mkv")
                {
                    continue;
                }
                files.push_back(relativePath + "/" + data.cFileName);
            }
            else
            {
                if (data.cFileName[0] == '.')
                {
                    continue;
                }
                GetFiles(files, root, relativePath + data.cFileName);
            }
        } while (FindNextFile(hFind, &data));

        FindClose(hFind);
    }
#endif // _WIN32
}

std::string urlDecode(
    const std::string &SRC)
{
    std::string ret;
    char ch;
    int ii;
    for (unsigned int i = 0; i < SRC.length(); i++)
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
