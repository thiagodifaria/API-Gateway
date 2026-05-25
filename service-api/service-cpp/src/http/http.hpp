#ifndef HTTPS_SERVER_HTTP_HPP
#define HTTPS_SERVER_HTTP_HPP

#include <string>
#include <vector>
#include <map>
#include <sstream>

namespace https_server {
    struct SecurityConfig;

namespace http {

struct HttpRequest {
    std::string method;
    std::string uri;
    std::string path;
    std::string query_string;
    std::string http_version;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> query_params;
    std::map<std::string, std::string> path_params;
    std::string body;
    std::string client_ip;
};

struct HttpResponse {
    int status_code = 200;
    std::string status_text = "OK";
    std::map<std::string, std::string> headers;
    std::string body;
    const SecurityConfig* security_config = nullptr;

    void apply_security_headers();

    std::string to_string() const {
        std::stringstream ss;
        ss << "HTTP/1.1 " << status_code << " " << status_text << "\r\n";
        
        const_cast<HttpResponse*>(this)->apply_security_headers();
        
        if (headers.find("Content-Length") == headers.end()) {
             ss << "Content-Length: " << body.length() << "\r\n";
        }
        if (headers.find("Content-Type") == headers.end()) {
             ss << "Content-Type: text/html; charset=utf-8\r\n";
        }
        if (headers.find("Connection") == headers.end()) {
             ss << "Connection: close\r\n";
        }

        for (const auto& [key, value] : headers) {
            ss << key << ": " << value << "\r\n";
        }

        ss << "\r\n";
        ss << body;
        return ss.str();
    }
};

} // namespace http
} // namespace https_server

#endif // HTTPS_SERVER_HTTP_HPP
