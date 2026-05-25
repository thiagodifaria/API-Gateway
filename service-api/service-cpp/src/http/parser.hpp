#ifndef HTTPS_SERVER_HTTP_PARSER_HPP
#define HTTPS_SERVER_HTTP_PARSER_HPP

#include "http/http.hpp"
#include <cstddef>
#include <string>

namespace https_server {

class Buffer;

namespace http {

struct ParserLimits {
    std::size_t max_header_bytes = 64 * 1024;
    std::size_t max_body_bytes = 2 * 1024 * 1024;
    std::size_t max_uri_bytes = 8192;
};

struct ParseResult {
    HttpRequest request;
    bool ok = false;
    int status_code = 400;
    std::string status_text = "Bad Request";
    std::string message;
};

class HttpParser {
public:
    explicit HttpParser(ParserLimits limits = {});

    ParseResult parse(const Buffer& buffer) const;

private:
    ParserLimits limits_;

    static std::string trim(std::string value);
    static std::string lower(std::string value);
    static void split_target(HttpRequest& request);
    static void parse_query(HttpRequest& request);
    static std::string url_decode(const std::string& value);
};

} // namespace http
} // namespace https_server

#endif
