#ifndef HTTPS_SERVER_RATE_LIMITER_HPP
#define HTTPS_SERVER_RATE_LIMITER_HPP

#include "core/config.hpp"
#include "http/http.hpp"
#include <chrono>
#include <map>
#include <mutex>
#include <string>

namespace https_server::middleware {

class TokenBucketRateLimiter {
public:
    explicit TokenBucketRateLimiter(ServerConfig::RateLimit config);

    http::HttpResponse check(const http::HttpRequest& request) const;
    bool enabled() const { return config_.enabled; }

private:
    struct Bucket {
        double tokens = 0.0;
        bool initialized = false;
        std::chrono::steady_clock::time_point updated = std::chrono::steady_clock::now();
    };

    std::string key_for(const http::HttpRequest& request) const;

    ServerConfig::RateLimit config_;
    mutable std::mutex mutex_;
    mutable std::map<std::string, Bucket> buckets_;
};

} // namespace https_server::middleware

#endif
