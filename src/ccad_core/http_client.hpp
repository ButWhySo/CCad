#ifndef CCAD_CORE_HTTP_CLIENT_HPP
#define CCAD_CORE_HTTP_CLIENT_HPP

#include <string>
#include <map>

namespace ccad {

// Core HTTP client wrapper for fetching libraries and remote assets.
class HttpClient {
public:
    HttpClient() = default;

    std::string get(const std::string& url, const std::map<std::string, std::string>& headers = {});
};

} // namespace ccad

#endif // CCAD_CORE_HTTP_CLIENT_HPP
