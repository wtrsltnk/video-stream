#include "httprequest.h"
#include "httpresponse.h"
#include "httprouter.h"
#include "httpserver.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <nlohmann/json.hpp>
#include <regex>
#include <sstream>
#include <string>
#include <thread>

#define MAX_RANGE_SIZE (5000000)

const char DirectorySeparatorChar = '\\';
const char AltDirectorySeparatorChar = '/';

static bool _isRunning = true;

int onRecieveRequest(const net::Request &request, net::Response &response);

void LogMessageAtLevel(
    const std::string &level,
    const std::string &message)
{
    std::time_t t = std::time(nullptr);
    fmt::print("{:%Y-%m-%d %H:%M:%S} [{}] {}\n", fmt::localtime(t), level, message);
}

void LogInfo(
    const std::string &message)
{
    LogMessageAtLevel("INF", message);
}

void LogError(
    const std::string &message)
{
    LogMessageAtLevel("ERR", message);
}

void GetFiles(
    std::vector<std::string> &files,
    std::filesystem::path const &root,
    std::string const &relativePath = "");

nlohmann::json GetFilesAsJson(
    std::filesystem::path const &root);

std::string urlDecode(
    const std::string &SRC);

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
    long long rangeStart = 0;
    long long rangeEnd = 0;

    for (auto &header : request._headers)
    {
        if (header.first.compare("Range") != 0)
        {
            continue;
        }

        std::regex pattern("bytes=([0-9]+)\\-([0-9]*)");
        auto words_begin = std::sregex_iterator(header.second.begin(), header.second.end(), pattern);

        if ((*words_begin).size() < 3)
        {
            continue;
        }

        rangeRequested = true;

        if ((*words_begin)[1].str().size() > 0)
        {
            rangeStart = std::stoll((*words_begin)[1].str());
        }
        if ((*words_begin)[2].str().size() > 0)
        {
            rangeEnd = std::stoll((*words_begin)[2].str());
        }
    }

    std::ifstream stream(file, std::ios::binary | std::ios::ate);

    long long size = stream.tellg();

    if (rangeStart > 0 && rangeEnd == 0)
    {
        rangeEnd = std::min<long long>(size, rangeStart + MAX_RANGE_SIZE);
    }

    if (rangeEnd > size)
    {
        response._responseCode = 416;

        return;
    }

    if (!rangeRequested || size < MAX_RANGE_SIZE)
    {
        response._responseCode = 200;

        stream.seekg(0, stream.beg);
        response._response = std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    }
    else
    {
        response._responseCode = 206;

        response.addHeader("Accept-Ranges", "bytes");

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

        std::string buffer;
        buffer.resize(MAX_RANGE_SIZE);
        stream.read(&buffer[0], static_cast<std::streamsize>(MAX_RANGE_SIZE));

        auto contentRange = fmt::format("bytes {}-{}/{}", rangeStart, rangeEnd - 1, size);
        response.addHeader("Content-Range", contentRange);

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

    if (argc > 2)
    {
        videoRoot = argv[1];
        contentRoot = argv[2];
    }
    else if (argc > 1)
    {
        videoRoot = argv[1];
        contentRoot = argv[1];
    }

    if (!std::filesystem::exists(videoRoot))
    {
        LogError(fmt::format("{} does not exist", videoRoot.string()));

        return 0;
    }

    if (!std::filesystem::exists(contentRoot))
    {
        LogError(fmt::format("{} does not exist", contentRoot.string()));

        return 0;
    }

    LogInfo(fmt::format("{} is our video root", videoRoot.string()));
    LogInfo(fmt::format("{} is our content root", contentRoot.string()));

    net::HttpServer server(80);
    Router router;

    server.SetLogger(LogInfo);
    router.SetLogger(LogInfo);

    server.Init();

    if (!server.Start())
    {
        return 0;
    }

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
        server.WaitForRequests([&](const net::Request &request, net::Response &response) -> int {
            router.Route(request, response);

            return response._responseCode;
        });
    }

    return 0;
}

std::string FileExtension(
    std::string const &fullpath)
{
    auto i = fullpath.length();
    while (--i > 0)
    {
        if (fullpath[i] == DirectorySeparatorChar)
        {
            return "";
        }

        if (fullpath[i] == AltDirectorySeparatorChar)
        {
            return "";
        }

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
            sscanf_s(SRC.substr(i + 1, 2).c_str(), "%x", &ii);
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
