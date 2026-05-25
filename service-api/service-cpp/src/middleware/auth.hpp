#ifndef HTTPS_SERVER_AUTH_HPP
#define HTTPS_SERVER_AUTH_HPP

#include "core/config.hpp"
#include "http/http.hpp"
#include <string>

namespace https_server::middleware {

class Authenticator {
public:
    explicit Authenticator(ServerConfig::Auth config);

    http::HttpResponse authorize(const http::HttpRequest& request, bool required) const;

private:
    static std::string base64url_decode(std::string input);
    static bool constant_time_equals(const std::string& a, const std::string& b);
    bool validate_api_key(const http::HttpRequest& request) const;
    bool validate_jwt(const std::string& token) const;

    ServerConfig::Auth config_;
};

} // namespace https_server::middleware

#endif
