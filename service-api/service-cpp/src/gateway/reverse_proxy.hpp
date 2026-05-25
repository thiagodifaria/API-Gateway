#ifndef HTTPS_SERVER_REVERSE_PROXY_HPP
#define HTTPS_SERVER_REVERSE_PROXY_HPP

#include "core/config.hpp"
#include "gateway/upstream_pool.hpp"
#include "http/http.hpp"
#include <string>

namespace https_server::gateway {

class ReverseProxy {
public:
    ReverseProxy(const ServerConfig& config, UpstreamPool& upstream_pool);

    http::HttpResponse handle(const http::HttpRequest& request,
                              const ServerConfig::ProxyRoute& route);

private:
    struct ParsedTarget {
        std::string host;
        std::string port;
        std::string base_path;
    };

    UpstreamPool& upstream_pool_;
    std::uint32_t upstream_timeout_ms_;

    static ParsedTarget parse_target(const std::string& target);
    static std::string rewrite_path(const http::HttpRequest& request,
                                    const ServerConfig::ProxyRoute& route,
                                    const ParsedTarget& target);
    static std::string build_forwarded_request(const http::HttpRequest& request,
                                               const ServerConfig::ProxyRoute& route,
                                               const ParsedTarget& target,
                                               const std::string& path);
    static http::HttpResponse parse_upstream_response(const std::string& raw);
};

} // namespace https_server::gateway

#endif
