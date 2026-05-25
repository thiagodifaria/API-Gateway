#ifndef HTTPS_SERVER_UPSTREAM_POOL_HPP
#define HTTPS_SERVER_UPSTREAM_POOL_HPP

#include "core/config.hpp"
#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace https_server::gateway {

class UpstreamPool {
public:
    explicit UpstreamPool(const ServerConfig& config);
    ~UpstreamPool();

    UpstreamPool(const UpstreamPool&) = delete;
    UpstreamPool& operator=(const UpstreamPool&) = delete;

    std::string select(const std::string& upstream_name, const std::string& algorithm);
    void record_result(const std::string& upstream_name, const std::string& target, bool success);
    void begin_request(const std::string& upstream_name, const std::string& target);
    void end_request(const std::string& upstream_name, const std::string& target);
    void start_health_checks();
    void stop_health_checks();
    std::string health_json() const;

private:
    struct Endpoint {
        std::string target;
        bool healthy = true;
        std::uint32_t active_connections = 0;
        std::uint64_t total_requests = 0;
        std::uint32_t consecutive_failures = 0;
        std::uint32_t consecutive_successes = 0;
        std::chrono::steady_clock::time_point last_health_check{};
    };

    struct Pool {
        ServerConfig::Upstream config;
        std::vector<Endpoint> endpoints;
        std::size_t cursor = 0;
    };

    static bool probe_http(const std::string& target, const std::string& path, std::uint32_t timeout_ms);
    void health_loop();

    mutable std::mutex mutex_;
    std::map<std::string, Pool> pools_;
    std::atomic<bool> running_{false};
    std::thread health_thread_;
};

} // namespace https_server::gateway

#endif
