#ifdef _WIN32
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include "gateway/upstream_pool.hpp"
#include "metrics/metrics.hpp"
#include "nlohmann/json.hpp"
#include "utils/logger.hpp"
#include <algorithm>
#include <sstream>

using json = nlohmann::json;

namespace https_server::gateway {

namespace {
#ifdef _WIN32
void close_pool_socket(SOCKET socket) { closesocket(socket); }
#else
void close_pool_socket(int socket) { close(socket); }
#endif

struct TargetParts {
    std::string host;
    std::string port;
};

TargetParts parse_target_parts(const std::string& target) {
    const std::string prefix = "http://";
    if (target.find(prefix) != 0) {
        return {};
    }
    std::string rest = target.substr(prefix.size());
    const auto slash = rest.find('/');
    if (slash != std::string::npos) {
        rest = rest.substr(0, slash);
    }
    TargetParts parts{rest, "80"};
    const auto colon = rest.rfind(':');
    if (colon != std::string::npos) {
        parts.host = rest.substr(0, colon);
        parts.port = rest.substr(colon + 1);
    }
    return parts;
}

void set_timeout_for_socket(
#ifdef _WIN32
    SOCKET socket,
#else
    int socket,
#endif
    std::uint32_t timeout_ms) {
#ifdef _WIN32
    const DWORD timeout = static_cast<DWORD>(timeout_ms);
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
    setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
#else
    timeval timeout{};
    timeout.tv_sec = static_cast<long>(timeout_ms / 1000);
    timeout.tv_usec = static_cast<long>((timeout_ms % 1000) * 1000);
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
#endif
}
} // namespace

UpstreamPool::UpstreamPool(const ServerConfig& config) {
    for (const auto& upstream : config.upstreams) {
        Pool pool;
        pool.config = upstream;
        for (const auto& target : upstream.targets) {
            pool.endpoints.push_back({target});
            metrics::MetricsRegistry::instance().set_upstream_health(upstream.name, target, true);
        }
        pools_[upstream.name] = std::move(pool);
    }
}

UpstreamPool::~UpstreamPool() {
    stop_health_checks();
}

std::string UpstreamPool::select(const std::string& upstream_name, const std::string& algorithm) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = pools_.find(upstream_name);
    if (it == pools_.end() || it->second.endpoints.empty()) {
        throw std::runtime_error("upstream not found: " + upstream_name);
    }

    auto& pool = it->second;
    auto healthy = std::vector<std::size_t>{};
    for (std::size_t i = 0; i < pool.endpoints.size(); ++i) {
        if (pool.endpoints[i].healthy) {
            healthy.push_back(i);
        }
    }
    if (healthy.empty()) {
        throw std::runtime_error("no healthy endpoints for upstream: " + upstream_name);
    }

    std::size_t selected = healthy[0];
    if (algorithm == "least_connections") {
        selected = *std::min_element(healthy.begin(), healthy.end(), [&](std::size_t a, std::size_t b) {
            return pool.endpoints[a].active_connections < pool.endpoints[b].active_connections;
        });
    } else {
        selected = healthy[pool.cursor++ % healthy.size()];
    }
    pool.endpoints[selected].total_requests++;
    return pool.endpoints[selected].target;
}

void UpstreamPool::begin_request(const std::string& upstream_name, const std::string& target) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& endpoints = pools_[upstream_name].endpoints;
    for (auto& endpoint : endpoints) {
        if (endpoint.target == target) {
            endpoint.active_connections++;
            return;
        }
    }
}

void UpstreamPool::end_request(const std::string& upstream_name, const std::string& target) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& endpoints = pools_[upstream_name].endpoints;
    for (auto& endpoint : endpoints) {
        if (endpoint.target == target && endpoint.active_connections > 0) {
            endpoint.active_connections--;
            return;
        }
    }
}

void UpstreamPool::record_result(const std::string& upstream_name, const std::string& target, bool success) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto pool_it = pools_.find(upstream_name);
    if (pool_it == pools_.end()) {
        return;
    }
    for (auto& endpoint : pool_it->second.endpoints) {
        if (endpoint.target != target) {
            continue;
        }
        if (success) {
            endpoint.consecutive_failures = 0;
            endpoint.consecutive_successes++;
            if (endpoint.consecutive_successes >= pool_it->second.config.healthy_threshold) {
                endpoint.healthy = true;
            }
        } else {
            endpoint.consecutive_successes = 0;
            endpoint.consecutive_failures++;
            if (endpoint.consecutive_failures >= pool_it->second.config.unhealthy_threshold) {
                endpoint.healthy = false;
            }
        }
        metrics::MetricsRegistry::instance().set_upstream_health(upstream_name, target, endpoint.healthy);
        return;
    }
}

void UpstreamPool::start_health_checks() {
    if (running_.exchange(true)) {
        return;
    }
    health_thread_ = std::thread([this] { health_loop(); });
}

void UpstreamPool::stop_health_checks() {
    running_ = false;
    if (health_thread_.joinable()) {
        health_thread_.join();
    }
}

void UpstreamPool::health_loop() {
    while (running_) {
        std::vector<std::pair<std::string, ServerConfig::Upstream>> checks;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto now = std::chrono::steady_clock::now();
            for (const auto& [name, pool] : pools_) {
                auto& mutable_pool = pools_[name];
                for (auto& endpoint : mutable_pool.endpoints) {
                    if (endpoint.last_health_check.time_since_epoch().count() != 0) {
                        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                            now - endpoint.last_health_check).count();
                        if (elapsed < mutable_pool.config.health_interval_ms) {
                            continue;
                        }
                    }
                    endpoint.last_health_check = now;
                    ServerConfig::Upstream config = pool.config;
                    config.targets = {endpoint.target};
                    checks.push_back({name, config});
                }
            }
        }

        for (const auto& [name, upstream] : checks) {
            const bool healthy = probe_http(upstream.targets.front(), upstream.health_path, upstream.health_timeout_ms);
            record_result(name, upstream.targets.front(), healthy);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

bool UpstreamPool::probe_http(const std::string& target, const std::string& path, std::uint32_t timeout_ms) {
    const auto parts = parse_target_parts(target);
    if (parts.host.empty()) {
        return false;
    }

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* result = nullptr;
    if (getaddrinfo(parts.host.c_str(), parts.port.c_str(), &hints, &result) != 0 || !result) {
        return false;
    }

    const auto sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
#ifdef _WIN32
    if (sock == INVALID_SOCKET) {
#else
    if (sock < 0) {
#endif
        freeaddrinfo(result);
        return false;
    }
    set_timeout_for_socket(sock, timeout_ms);
    if (connect(sock, result->ai_addr, static_cast<int>(result->ai_addrlen)) != 0) {
        freeaddrinfo(result);
        close_pool_socket(sock);
        return false;
    }
    freeaddrinfo(result);

    const std::string request = "GET " + path + " HTTP/1.1\r\nHost: " + parts.host + "\r\nConnection: close\r\n\r\n";
    send(sock, request.data(), static_cast<int>(request.size()), 0);
    char buffer[256];
    const int received = recv(sock, buffer, sizeof(buffer), 0);
    close_pool_socket(sock);
    if (received <= 0) {
        return false;
    }
    const std::string response(buffer, static_cast<std::size_t>(received));
    return response.find(" 2") != std::string::npos || response.find(" 3") != std::string::npos;
}

std::string UpstreamPool::health_json() const {
    json body;
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [name, pool] : pools_) {
        for (const auto& endpoint : pool.endpoints) {
            body[name].push_back({
                {"target", endpoint.target},
                {"healthy", endpoint.healthy},
                {"active_connections", endpoint.active_connections},
                {"total_requests", endpoint.total_requests},
                {"consecutive_failures", endpoint.consecutive_failures}
            });
        }
    }
    return body.dump(2);
}

} // namespace https_server::gateway
