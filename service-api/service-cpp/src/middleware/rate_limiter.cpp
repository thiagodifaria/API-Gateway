#include "middleware/rate_limiter.hpp"
#include "metrics/metrics.hpp"
#include <algorithm>
#include <cmath>

namespace https_server::middleware {

TokenBucketRateLimiter::TokenBucketRateLimiter(ServerConfig::RateLimit config)
    : config_(std::move(config)) {}

std::string TokenBucketRateLimiter::key_for(const http::HttpRequest& request) const {
    if (config_.key == "route") {
        return request.method + ":" + (request.path.empty() ? request.uri : request.path);
    }
    if (config_.key == "api_key") {
        const auto it = request.headers.find("x-api-key");
        if (it != request.headers.end()) {
            return "key:" + it->second;
        }
    }
    return request.client_ip.empty() ? "unknown" : request.client_ip;
}

http::HttpResponse TokenBucketRateLimiter::check(const http::HttpRequest& request) const {
    http::HttpResponse allowed;
    if (!config_.enabled || config_.capacity == 0 || config_.refill_per_second == 0) {
        return allowed;
    }

    const auto now = std::chrono::steady_clock::now();
    const std::string key = key_for(request);
    std::lock_guard<std::mutex> lock(mutex_);
    auto& bucket = buckets_[key];
    if (!bucket.initialized) {
        bucket.tokens = static_cast<double>(config_.capacity);
        bucket.updated = now;
        bucket.initialized = true;
    }

    const std::chrono::duration<double> elapsed = now - bucket.updated;
    bucket.tokens = std::min<double>(
        static_cast<double>(config_.capacity),
        bucket.tokens + elapsed.count() * static_cast<double>(config_.refill_per_second));
    bucket.updated = now;

    if (bucket.tokens >= 1.0) {
        bucket.tokens -= 1.0;
        allowed.headers["RateLimit-Limit"] = std::to_string(config_.capacity);
        allowed.headers["RateLimit-Remaining"] = std::to_string(static_cast<unsigned long long>(std::floor(bucket.tokens)));
        allowed.headers["RateLimit-Reset"] = "1";
        return allowed;
    }

    metrics::MetricsRegistry::instance().record_rate_limit_hit();
    http::HttpResponse rejected;
    rejected.status_code = 429;
    rejected.status_text = "Too Many Requests";
    rejected.headers["Content-Type"] = "application/json; charset=utf-8";
    rejected.headers["RateLimit-Limit"] = std::to_string(config_.capacity);
    rejected.headers["RateLimit-Remaining"] = "0";
    rejected.headers["RateLimit-Reset"] = "1";
    rejected.headers["Retry-After"] = "1";
    rejected.body = "{\"error\":\"rate_limited\",\"message\":\"Too many requests\"}";
    return rejected;
}

} // namespace https_server::middleware
