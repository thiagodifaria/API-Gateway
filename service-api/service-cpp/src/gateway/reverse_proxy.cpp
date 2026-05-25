#ifdef _WIN32
#define NOMINMAX
#endif

#include "gateway/reverse_proxy.hpp"
#include "utils/logger.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace https_server::gateway {

namespace {

#ifdef _WIN32
void close_proxy_socket(SOCKET socket) { closesocket(socket); }
#else
void close_proxy_socket(int socket) { close(socket); }
#endif

bool is_hop_by_hop_header(std::string key) {
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    return key == "connection" ||
           key == "keep-alive" ||
           key == "proxy-authenticate" ||
           key == "proxy-authorization" ||
           key == "te" ||
           key == "trailer" ||
           key == "transfer-encoding" ||
           key == "upgrade";
}

std::string lower_header_name(std::string key) {
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return key;
}

} // namespace

ReverseProxy::ReverseProxy(const ServerConfig& config, UpstreamPool& upstream_pool)
    : upstream_pool_(upstream_pool),
      upstream_timeout_ms_(config.http.read_timeout_ms) {}

http::HttpResponse ReverseProxy::handle(const http::HttpRequest& request,
                                        const ServerConfig::ProxyRoute& route) {
    try {
        const std::string selected = upstream_pool_.select(route.upstream, route.load_balancer);
        upstream_pool_.begin_request(route.upstream, selected);
        const ParsedTarget target = parse_target(selected);
        const std::string path = rewrite_path(request, route, target);
        const std::string payload = build_forwarded_request(request, route, target, path);

        addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        addrinfo* result = nullptr;
        if (getaddrinfo(target.host.c_str(), target.port.c_str(), &hints, &result) != 0 || !result) {
            throw std::runtime_error("failed to resolve upstream");
        }

        const auto sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
#ifdef _WIN32
        if (sock == INVALID_SOCKET) {
#else
        if (sock < 0) {
#endif
            freeaddrinfo(result);
            throw std::runtime_error("failed to create upstream socket");
        }

#ifdef _WIN32
        const DWORD timeout = static_cast<DWORD>(upstream_timeout_ms_);
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
#else
        timeval timeout{};
        timeout.tv_sec = static_cast<long>(upstream_timeout_ms_ / 1000);
        timeout.tv_usec = static_cast<long>((upstream_timeout_ms_ % 1000) * 1000);
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
#endif

        if (connect(sock, result->ai_addr, static_cast<int>(result->ai_addrlen)) != 0) {
            freeaddrinfo(result);
            close_proxy_socket(sock);
            upstream_pool_.end_request(route.upstream, selected);
            upstream_pool_.record_result(route.upstream, selected, false);
            throw std::runtime_error("failed to connect upstream");
        }
        freeaddrinfo(result);

        const char* data = payload.data();
        std::size_t remaining = payload.size();
        while (remaining > 0) {
            const int sent = send(sock, data, static_cast<int>(remaining), 0);
            if (sent <= 0) {
                close_proxy_socket(sock);
                upstream_pool_.end_request(route.upstream, selected);
                upstream_pool_.record_result(route.upstream, selected, false);
                throw std::runtime_error("failed to write upstream request");
            }
            data += sent;
            remaining -= static_cast<std::size_t>(sent);
        }

        std::string raw_response;
        char buffer[8192];
        while (true) {
            const int received = recv(sock, buffer, sizeof(buffer), 0);
            if (received <= 0) {
                break;
            }
            raw_response.append(buffer, static_cast<std::size_t>(received));
        }
        close_proxy_socket(sock);
        upstream_pool_.end_request(route.upstream, selected);

        if (raw_response.empty()) {
            upstream_pool_.record_result(route.upstream, selected, false);
            throw std::runtime_error("empty upstream response");
        }

        auto response = parse_upstream_response(raw_response);
        upstream_pool_.record_result(route.upstream, selected, response.status_code < 500);
        return response;
    } catch (const std::exception& e) {
        LOG_WARNING("Reverse proxy failed: " + std::string(e.what()));
        http::HttpResponse response;
        response.status_code = 502;
        response.status_text = "Bad Gateway";
        response.headers["Content-Type"] = "application/json; charset=utf-8";
        response.body = "{\"error\":\"bad_gateway\",\"message\":\"Upstream request failed\"}";
        return response;
    }
}

ReverseProxy::ParsedTarget ReverseProxy::parse_target(const std::string& target) {
    const std::string prefix = "http://";
    if (target.substr(0, prefix.size()) != prefix) {
        throw std::runtime_error("only http upstream targets are supported initially");
    }

    std::string rest = target.substr(prefix.size());
    std::string path;
    const auto slash = rest.find('/');
    if (slash != std::string::npos) {
        path = rest.substr(slash);
        rest = rest.substr(0, slash);
    }

    std::string host = rest;
    std::string port = "80";
    const auto colon = rest.rfind(':');
    if (colon != std::string::npos) {
        host = rest.substr(0, colon);
        port = rest.substr(colon + 1);
    }

    return {host, port, path};
}

std::string ReverseProxy::rewrite_path(const http::HttpRequest& request,
                                       const ServerConfig::ProxyRoute& route,
                                       const ParsedTarget& target) {
    std::string path = request.path.empty() ? request.uri : request.path;
    if (!route.strip_prefix.empty() && path.find(route.strip_prefix) == 0) {
        path = path.substr(route.strip_prefix.size());
        if (path.empty() || path.front() != '/') {
            path = "/" + path;
        }
    }

    std::string rewritten = target.base_path + path;
    if (!request.query_string.empty()) {
        rewritten += "?" + request.query_string;
    }
    return rewritten;
}

std::string ReverseProxy::build_forwarded_request(const http::HttpRequest& request,
                                                  const ServerConfig::ProxyRoute&,
                                                  const ParsedTarget& target,
                                                  const std::string& path) {
    std::ostringstream ss;
    ss << request.method << " " << path << " HTTP/1.1\r\n";
    ss << "Host: " << target.host << "\r\n";
    ss << "X-Forwarded-Proto: https\r\n";
    ss << "X-Forwarded-For: " << (request.client_ip.empty() ? "unknown" : request.client_ip) << "\r\n";
    ss << "X-Real-IP: " << (request.client_ip.empty() ? "unknown" : request.client_ip) << "\r\n";
    ss << "Via: 1.1 api-gateway\r\n";

    for (const auto& [key, value] : request.headers) {
        const std::string normalized_key = lower_header_name(key);
        if (is_hop_by_hop_header(normalized_key) ||
            normalized_key == "host" ||
            normalized_key == "content-length") {
            continue;
        }
        ss << key << ": " << value << "\r\n";
    }

    ss << "Content-Length: " << request.body.size() << "\r\n";
    ss << "Connection: close\r\n\r\n";
    ss << request.body;
    return ss.str();
}

http::HttpResponse ReverseProxy::parse_upstream_response(const std::string& raw) {
    http::HttpResponse response;
    const auto header_end = raw.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        response.status_code = 502;
        response.status_text = "Bad Gateway";
        response.body = raw;
        return response;
    }

    std::istringstream stream(raw.substr(0, header_end));
    std::string line;
    if (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        std::istringstream status_line(line);
        std::string version;
        status_line >> version >> response.status_code;
        std::getline(status_line, response.status_text);
        if (!response.status_text.empty() && response.status_text.front() == ' ') {
            response.status_text.erase(response.status_text.begin());
        }
        if (response.status_text.empty()) {
            response.status_text = "OK";
        }
    }

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const auto colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        const std::string key = line.substr(0, colon);
        if (is_hop_by_hop_header(key)) {
            continue;
        }
        std::string value = line.substr(colon + 1);
        if (!value.empty() && value.front() == ' ') {
            value.erase(value.begin());
        }
        if (key != "Content-Length") {
            response.headers[key] = value;
        }
    }

    response.body = raw.substr(header_end + 4);
    return response;
}

} // namespace https_server::gateway
