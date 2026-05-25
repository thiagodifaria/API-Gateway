#ifndef HTTPS_SERVER_AUTH_HPP
#define HTTPS_SERVER_AUTH_HPP

#include "core/config.hpp"
#include "http/http.hpp"
#include "nlohmann/json.hpp"
#include <string>
#include <vector>

namespace https_server::middleware {

class Authenticator {
public:
    explicit Authenticator(ServerConfig::Auth config);

    http::HttpResponse authorize(const http::HttpRequest& request,
                                 bool required,
                                 const std::vector<std::string>& required_scopes = {}) const;

private:
    struct JwtValidation {
        bool valid = false;
        std::vector<std::string> scopes;
    };

    static std::string base64url_decode(std::string input);
    static std::string base64url_encode(const unsigned char* data, std::size_t size);
    static bool constant_time_equals(const std::string& a, const std::string& b);
    static std::vector<std::string> extract_scopes(const nlohmann::json& payload);
    bool validate_api_key(const http::HttpRequest& request) const;
    JwtValidation validate_jwt(const std::string& token) const;
    bool verify_asymmetric_jwt(const std::string& signed_part, const std::string& signature) const;
    bool has_required_scopes(const std::vector<std::string>& granted,
                             const std::vector<std::string>& required) const;

    ServerConfig::Auth config_;
};

} // namespace https_server::middleware

#endif
