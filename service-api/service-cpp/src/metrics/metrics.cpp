#include "metrics/metrics.hpp"
#include "nlohmann/json.hpp"
#include <sstream>

using json = nlohmann::json;

namespace https_server::metrics {

MetricsRegistry& MetricsRegistry::instance() {
    static MetricsRegistry registry;
    return registry;
}

void MetricsRegistry::record_request(const std::string& route, int status_code, long long latency_us) {
    total_requests_.fetch_add(1);
    std::lock_guard<std::mutex> lock(mutex_);
    requests_by_route_[route]++;
    responses_by_status_[status_code]++;
    latency_total_us_by_route_[route] += latency_us;
}

void MetricsRegistry::record_rate_limit_hit() {
    rate_limit_hits_.fetch_add(1);
}

void MetricsRegistry::record_cache_hit() {
    cache_hits_.fetch_add(1);
}

void MetricsRegistry::record_cache_miss() {
    cache_misses_.fetch_add(1);
}

void MetricsRegistry::set_upstream_health(const std::string& upstream, const std::string& target, bool healthy) {
    std::lock_guard<std::mutex> lock(mutex_);
    upstream_health_[upstream + "{" + target + "}"] = healthy;
}

std::string MetricsRegistry::render_prometheus() const {
    std::ostringstream out;
    out << "# TYPE gateway_requests_total counter\n";
    out << "gateway_requests_total " << total_requests_.load() << "\n";
    out << "# TYPE gateway_rate_limit_hits_total counter\n";
    out << "gateway_rate_limit_hits_total " << rate_limit_hits_.load() << "\n";
    out << "# TYPE gateway_cache_hits_total counter\n";
    out << "gateway_cache_hits_total " << cache_hits_.load() << "\n";
    out << "# TYPE gateway_cache_misses_total counter\n";
    out << "gateway_cache_misses_total " << cache_misses_.load() << "\n";

    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [route, count] : requests_by_route_) {
        out << "gateway_route_requests_total{route=\"" << route << "\"} " << count << "\n";
    }
    for (const auto& [status, count] : responses_by_status_) {
        out << "gateway_responses_total{status=\"" << status << "\"} " << count << "\n";
    }
    for (const auto& [route, total_us] : latency_total_us_by_route_) {
        const auto count = requests_by_route_.count(route) ? requests_by_route_.at(route) : 1;
        out << "gateway_route_latency_average_us{route=\"" << route << "\"} " << (total_us / static_cast<long long>(count)) << "\n";
    }
    for (const auto& [endpoint, healthy] : upstream_health_) {
        out << "gateway_upstream_healthy{endpoint=\"" << endpoint << "\"} " << (healthy ? 1 : 0) << "\n";
    }
    return out.str();
}

http::HttpResponse MetricsRegistry::render_json() const {
    json body;
    body["requests_total"] = total_requests_.load();
    body["rate_limit_hits_total"] = rate_limit_hits_.load();
    body["cache_hits_total"] = cache_hits_.load();
    body["cache_misses_total"] = cache_misses_.load();

    std::lock_guard<std::mutex> lock(mutex_);
    body["requests_by_route"] = requests_by_route_;
    body["responses_by_status"] = responses_by_status_;
    body["upstream_health"] = upstream_health_;

    http::HttpResponse response;
    response.headers["Content-Type"] = "application/json; charset=utf-8";
    response.body = body.dump(2);
    return response;
}

} // namespace https_server::metrics
