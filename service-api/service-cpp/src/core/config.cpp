#include "core/config.hpp"
#include "nlohmann/json.hpp"
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <array>

using json = nlohmann::json;

namespace https_server {

ServerConfig Config::load(const std::string& filename) {
    ServerConfig config;
    
    const char* env_config = std::getenv("HTTPS_SERVER_CONFIG");
    const std::string requested_file = env_config && *env_config ? env_config : filename;

    std::ifstream file(requested_file);
    if (!file.is_open()) {
        const std::array<std::string, 3> fallback_paths = {
            "service-api/service-cpp/config/config.json",
            "../service-api/service-cpp/config/config.json",
            "../../service-api/service-cpp/config/config.json"
        };

        for (const auto& path : fallback_paths) {
            file.clear();
            file.open(path);
            if (file.is_open()) {
                break;
            }
        }
    }

    if (!file.is_open()) {
        throw std::runtime_error("Cannot open config file: " + requested_file);
    }
    
    json j;
    file >> j;
    
    if (j.contains("port")) config.port = j["port"];
    if (j.contains("threads")) config.threads = j["threads"];
    if (j.contains("cert_file")) config.cert_file = j["cert_file"];
    if (j.contains("key_file")) config.key_file = j["key_file"];
    if (j.contains("web_root")) config.web_root = j["web_root"];
    
    if (j.contains("client_ca_file")) config.client_ca_file = j["client_ca_file"];
    
    if (j.contains("log_level")) {
        const std::string level = j["log_level"];
        if (level == "Debug") config.log_level = LogLevel::Debug;
        else if (level == "Info") config.log_level = LogLevel::Info;
        else if (level == "Warning") config.log_level = LogLevel::Warning;
        else if (level == "Error") config.log_level = LogLevel::Error;
    }
    
    if (j.contains("security")) {
        const auto& security = j["security"];
        
        if (security.contains("enable_hsts")) {
            config.security.enable_hsts = security["enable_hsts"];
        }
        if (security.contains("enable_csp")) {
            config.security.enable_csp = security["enable_csp"];
        }
        if (security.contains("enable_xcto")) {
            config.security.enable_xcto = security["enable_xcto"];
        }
        if (security.contains("enable_xfo")) {
            config.security.enable_xfo = security["enable_xfo"];
        }
        if (security.contains("hsts_max_age")) {
            config.security.hsts_max_age = security["hsts_max_age"];
        }
        if (security.contains("hsts_include_subdomains")) {
            config.security.hsts_include_subdomains = security["hsts_include_subdomains"];
        }
        if (security.contains("hsts_preload")) {
            config.security.hsts_preload = security["hsts_preload"];
        }
        if (security.contains("csp_policy")) {
            config.security.csp_policy = security["csp_policy"];
        }
    }

    if (j.contains("http")) {
        const auto& http = j["http"];
        if (http.contains("max_header_bytes")) {
            config.http.max_header_bytes = http["max_header_bytes"];
        }
        if (http.contains("max_body_bytes")) {
            config.http.max_body_bytes = http["max_body_bytes"];
        }
        if (http.contains("read_timeout_ms")) {
            config.http.read_timeout_ms = http["read_timeout_ms"];
        }
        if (http.contains("write_timeout_ms")) {
            config.http.write_timeout_ms = http["write_timeout_ms"];
        }
        if (http.contains("max_uri_bytes")) {
            config.http.max_uri_bytes = http["max_uri_bytes"];
        }
        if (http.contains("keep_alive")) {
            config.http.keep_alive = http["keep_alive"];
        }
    }

    if (j.contains("upstreams") && j["upstreams"].is_object()) {
        for (const auto& item : j["upstreams"].items()) {
            ServerConfig::Upstream upstream;
            upstream.name = item.key();

            if (item.value().is_array()) {
                for (const auto& target : item.value()) {
                    upstream.targets.push_back(target.get<std::string>());
                }
            } else if (item.value().is_object()) {
                const auto& upstream_json = item.value();
                if (upstream_json.contains("targets") && upstream_json["targets"].is_array()) {
                    for (const auto& target : upstream_json["targets"]) {
                        upstream.targets.push_back(target.get<std::string>());
                    }
                }
                if (upstream_json.contains("health_path")) upstream.health_path = upstream_json["health_path"];
                if (upstream_json.contains("health_interval_ms")) upstream.health_interval_ms = upstream_json["health_interval_ms"];
                if (upstream_json.contains("health_timeout_ms")) upstream.health_timeout_ms = upstream_json["health_timeout_ms"];
                if (upstream_json.contains("unhealthy_threshold")) upstream.unhealthy_threshold = upstream_json["unhealthy_threshold"];
                if (upstream_json.contains("healthy_threshold")) upstream.healthy_threshold = upstream_json["healthy_threshold"];
            }

            if (!upstream.name.empty() && !upstream.targets.empty()) {
                config.upstreams.push_back(std::move(upstream));
            }
        }
    }

    if (j.contains("proxy_routes") && j["proxy_routes"].is_array()) {
        for (const auto& route_json : j["proxy_routes"]) {
            ServerConfig::ProxyRoute route;
            if (route_json.contains("method")) route.method = route_json["method"];
            if (route_json.contains("path")) route.path = route_json["path"];
            if (route_json.contains("upstream")) route.upstream = route_json["upstream"];
            if (route_json.contains("strip_prefix")) route.strip_prefix = route_json["strip_prefix"];
            if (route_json.contains("load_balancer")) route.load_balancer = route_json["load_balancer"];
            if (route_json.contains("require_auth")) route.require_auth = route_json["require_auth"];
            if (route_json.contains("rate_limit")) route.rate_limit = route_json["rate_limit"];
            if (route_json.contains("websocket")) route.websocket = route_json["websocket"];
            if (route_json.contains("required_scopes") && route_json["required_scopes"].is_array()) {
                for (const auto& scope : route_json["required_scopes"]) {
                    route.required_scopes.push_back(scope.get<std::string>());
                }
            }

            if (!route.path.empty() && !route.upstream.empty()) {
                config.proxy_routes.push_back(std::move(route));
            }
        }
    }

    if (j.contains("rate_limit")) {
        const auto& rate = j["rate_limit"];
        if (rate.contains("enabled")) config.rate_limit.enabled = rate["enabled"];
        if (rate.contains("capacity")) config.rate_limit.capacity = rate["capacity"];
        if (rate.contains("refill_per_second")) config.rate_limit.refill_per_second = rate["refill_per_second"];
        if (rate.contains("key")) config.rate_limit.key = rate["key"];
    }

    if (j.contains("auth")) {
        const auto& auth = j["auth"];
        if (auth.contains("enabled")) config.auth.enabled = auth["enabled"];
        if (auth.contains("api_key_header")) config.auth.api_key_header = auth["api_key_header"];
        if (auth.contains("jwt_hmac_secret")) config.auth.jwt_hmac_secret = auth["jwt_hmac_secret"];
        if (auth.contains("issuer")) config.auth.issuer = auth["issuer"];
        if (auth.contains("audience")) config.auth.audience = auth["audience"];
        if (auth.contains("api_keys") && auth["api_keys"].is_array()) {
            for (const auto& key : auth["api_keys"]) {
                config.auth.api_keys.push_back(key.get<std::string>());
            }
        }
    }

    if (j.contains("observability")) {
        const auto& obs = j["observability"];
        if (obs.contains("metrics_enabled")) config.observability.metrics_enabled = obs["metrics_enabled"];
        if (obs.contains("json_logs")) config.observability.json_logs = obs["json_logs"];
    }

    if (j.contains("websocket")) {
        const auto& ws = j["websocket"];
        if (ws.contains("enabled")) config.websocket.enabled = ws["enabled"];
        if (ws.contains("max_frame_bytes")) config.websocket.max_frame_bytes = ws["max_frame_bytes"];
        if (ws.contains("idle_timeout_ms")) config.websocket.idle_timeout_ms = ws["idle_timeout_ms"];
    }
    
    return config;
}

} // namespace https_server
