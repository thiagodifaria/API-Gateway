#ifndef HTTPS_SERVER_CONFIG_HPP
#define HTTPS_SERVER_CONFIG_HPP

#include "utils/logger.hpp"
#include <string>
#include <cstdint>
#include <vector>
#include <map>

namespace https_server {

struct SecurityConfig {
    bool enable_hsts = true;
    bool enable_csp = true;
    bool enable_xcto = true;
    bool enable_xfo = true;
    std::string hsts_max_age = "31536000";
    bool hsts_include_subdomains = true;
    bool hsts_preload = false;
    std::string csp_policy = "default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'; img-src 'self' data:; font-src 'self'";
};

struct HttpLimitsConfig {
    std::uint32_t max_header_bytes = 65536;
    std::uint32_t max_body_bytes = 2097152;
    std::uint32_t read_timeout_ms = 5000;
    std::uint32_t write_timeout_ms = 5000;
    std::uint32_t max_uri_bytes = 8192;
    bool keep_alive = false;
};

struct ServerConfig {
    std::uint16_t port = 8443;
    std::uint32_t threads = 0;
    std::string cert_file = "service-api/service-cpp/certs/cert.pem";
    std::string key_file = "service-api/service-cpp/certs/key.pem";
    std::string web_root = "client-web";
    LogLevel log_level = LogLevel::Info;
    
    std::string client_ca_file = "";
    
    SecurityConfig security;
    HttpLimitsConfig http;

    struct ProxyRoute {
        std::string method = "GET";
        std::string path;
        std::string upstream;
        std::string strip_prefix;
        std::string load_balancer = "round_robin";
        bool require_auth = false;
        std::vector<std::string> required_scopes;
        bool rate_limit = true;
        bool websocket = false;
    };

    struct Upstream {
        std::string name;
        std::vector<std::string> targets;
        std::string health_path = "/gateway/health";
        std::uint32_t health_interval_ms = 5000;
        std::uint32_t health_timeout_ms = 1000;
        std::uint32_t unhealthy_threshold = 2;
        std::uint32_t healthy_threshold = 1;
    };

    struct RateLimit {
        bool enabled = true;
        std::uint32_t capacity = 120;
        std::uint32_t refill_per_second = 20;
        std::string key = "ip";
    };

    struct Auth {
        bool enabled = false;
        std::string api_key_header = "X-API-Key";
        std::vector<std::string> api_keys;
        std::string jwt_algorithm = "HS256";
        std::string jwt_hmac_secret;
        std::string jwt_public_key_file;
        std::string issuer;
        std::string audience;
    };

    struct Observability {
        bool metrics_enabled = true;
        bool json_logs = false;
    };

    struct WebSocket {
        bool enabled = true;
        std::uint32_t max_frame_bytes = 1048576;
        std::uint32_t idle_timeout_ms = 60000;
    };

    std::vector<ProxyRoute> proxy_routes;
    std::vector<Upstream> upstreams;
    RateLimit rate_limit;
    Auth auth;
    Observability observability;
    WebSocket websocket;
};

class Config {
public:
    static ServerConfig load(const std::string& filename = "config.json");
    
private:
    Config() = delete;
};

} // namespace https_server

#endif // HTTPS_SERVER_CONFIG_HPP
