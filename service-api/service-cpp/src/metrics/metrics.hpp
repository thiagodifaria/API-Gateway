#ifndef HTTPS_SERVER_METRICS_HPP
#define HTTPS_SERVER_METRICS_HPP

#include "http/http.hpp"
#include <atomic>
#include <map>
#include <mutex>
#include <string>

namespace https_server::metrics {

class MetricsRegistry {
public:
    static MetricsRegistry& instance();

    void record_request(const std::string& route, int status_code, long long latency_us);
    void record_rate_limit_hit();
    void record_cache_hit();
    void record_cache_miss();
    void set_upstream_health(const std::string& upstream, const std::string& target, bool healthy);
    std::string render_prometheus() const;
    http::HttpResponse render_json() const;

private:
    MetricsRegistry() = default;

    mutable std::mutex mutex_;
    std::atomic<unsigned long long> total_requests_{0};
    std::atomic<unsigned long long> rate_limit_hits_{0};
    std::atomic<unsigned long long> cache_hits_{0};
    std::atomic<unsigned long long> cache_misses_{0};
    std::map<std::string, unsigned long long> requests_by_route_;
    std::map<int, unsigned long long> responses_by_status_;
    std::map<std::string, long long> latency_total_us_by_route_;
    std::map<std::string, bool> upstream_health_;
};

} // namespace https_server::metrics

#endif
