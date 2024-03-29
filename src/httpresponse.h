#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <map>
#include <string>

#include "httprequest.h"

namespace net {

class Response
{
public:
    Response();
    virtual ~Response();

    void addHeader(std::string const &key, std::string const &value);

    int _responseCode;
    std::map<std::string, std::string> _headers;
    std::string _response;
    long long _contentSize = 0;
};

} // namespace net

#endif // HTTPRESPONSE_H
