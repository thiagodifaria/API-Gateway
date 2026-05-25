#ifndef HTTPS_SERVER_ROUTER_HPP
#define HTTPS_SERVER_ROUTER_HPP

#include "http.hpp"
#include <functional>
#include <map>
#include <string>
#include <vector>
#include <sstream>

namespace https_server {

using HttpHandler = std::function<http::HttpResponse(const http::HttpRequest&)>;
using NextHandler = std::function<http::HttpResponse(const http::HttpRequest&)>;
using HttpMiddleware = std::function<http::HttpResponse(const http::HttpRequest&, NextHandler)>;

struct Route {
    std::string pattern;
    std::string method;
    HttpHandler handler;
    bool is_wildcard;
};

class Router {
public:
    void add_route(const std::string& method, const std::string& pattern, HttpHandler handler) {
        Route route;
        route.method = method;
        route.pattern = pattern;
        route.handler = std::move(handler);
        route.is_wildcard = pattern.find('*') != std::string::npos;
        
        routes_.push_back(route);
    }

    void use(HttpMiddleware middleware) {
        middlewares_.push_back(std::move(middleware));
    }

    http::HttpResponse route_request(const http::HttpRequest& request) const {
        HttpHandler selected_handler;
        http::HttpRequest routed_request = request;

        for (const auto& route : routes_) {
            const bool head_uses_get = request.method == "HEAD" && route.method == "GET";
            if (route.method != request.method && !head_uses_get) {
                continue;
            }
            
            std::map<std::string, std::string> path_params;
            if (matches_pattern(route.pattern, request.path.empty() ? request.uri : request.path, path_params)) {
                routed_request.path_params = std::move(path_params);
                selected_handler = route.handler;
                break;
            }
        }

        if (selected_handler) {
            NextHandler chain = [selected_handler](const http::HttpRequest& req) {
                return selected_handler(req);
            };

            for (auto it = middlewares_.rbegin(); it != middlewares_.rend(); ++it) {
                const auto middleware = *it;
                const NextHandler next = chain;
                chain = [middleware, next](const http::HttpRequest& req) {
                    return middleware(req, next);
                };
            }

            auto response = chain(routed_request);
            if (request.method == "HEAD") {
                response.body.clear();
            }
            return response;
        }

        if (path_exists_for_different_method(request)) {
            http::HttpResponse response;
            response.status_code = 405;
            response.status_text = "Method Not Allowed";
            response.body = "<h1>405 Method Not Allowed</h1>";
            response.headers["Content-Type"] = "text/html; charset=utf-8";
            return response;
        }

        http::HttpResponse response;
        response.status_code = 404;
        response.status_text = "Not Found";
        response.body = "<h1>404 Not Found</h1>";
        response.headers["Content-Type"] = "text/html; charset=utf-8";
        return response;
    }

private:
    std::vector<Route> routes_;
    std::vector<HttpMiddleware> middlewares_;
    
    bool path_exists_for_different_method(const http::HttpRequest& request) const {
        const std::string path = request.path.empty() ? request.uri : request.path;
        for (const auto& route : routes_) {
            if (route.method == request.method) {
                continue;
            }
            std::map<std::string, std::string> ignored;
            if (matches_pattern(route.pattern, path, ignored)) {
                return true;
            }
        }
        return false;
    }

    static std::vector<std::string> split_segments(const std::string& path) {
        std::vector<std::string> segments;
        std::stringstream ss(path);
        std::string segment;

        while (std::getline(ss, segment, '/')) {
            if (!segment.empty()) {
                segments.push_back(segment);
            }
        }

        return segments;
    }

    bool matches_pattern(const std::string& pattern,
                         const std::string& uri,
                         std::map<std::string, std::string>& path_params) const {
        if (pattern.find('*') == std::string::npos && pattern.find(':') == std::string::npos) {
            return pattern == uri;
        }
        
        if (pattern.back() == '*' && pattern.size() > 1 && pattern[pattern.size()-2] == '/') {
            const std::string prefix = pattern.substr(0, pattern.size() - 1);
            return uri.substr(0, prefix.size()) == prefix;
        }

        const auto pattern_segments = split_segments(pattern);
        const auto uri_segments = split_segments(uri);
        if (pattern_segments.size() != uri_segments.size()) {
            return false;
        }

        std::map<std::string, std::string> params;
        for (size_t i = 0; i < pattern_segments.size(); ++i) {
            const auto& pattern_segment = pattern_segments[i];
            const auto& uri_segment = uri_segments[i];

            if (!pattern_segment.empty() && pattern_segment.front() == ':') {
                params[pattern_segment.substr(1)] = uri_segment;
                continue;
            }

            if (pattern_segment != uri_segment) {
                return false;
            }
        }

        path_params = std::move(params);
        return true;
    }
};

}

#endif
