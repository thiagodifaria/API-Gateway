#include "http/parser.hpp"
#include "utils/buffer.hpp"
#include <cassert>
#include <iostream>
#include <string>

using https_server::Buffer;
using https_server::http::HttpParser;
using https_server::http::ParserLimits;

int main() {
    {
        Buffer buffer;
        const std::string raw =
            "POST /api/users?id=42&name=Thiago HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Content-Length: 11\r\n"
            "\r\n"
            "{\"ok\":true}";
        buffer.append(raw);

        const auto result = HttpParser().parse(buffer);
        assert(result.ok);
        assert(result.request.method == "POST");
        assert(result.request.path == "/api/users");
        assert(result.request.query_params.at("id") == "42");
        assert(result.request.query_params.at("name") == "Thiago");
        assert(result.request.headers.at("host") == "localhost");
        assert(result.request.body == "{\"ok\":true}");
    }

    {
        Buffer buffer;
        const std::string raw =
            "GET /too-large HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "\r\n";
        buffer.append(raw);

        ParserLimits limits;
        limits.max_uri_bytes = 4;
        const auto result = HttpParser(limits).parse(buffer);
        assert(!result.ok);
        assert(result.status_code == 414);
    }

    {
        Buffer buffer;
        const std::string raw =
            "POST /api HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Content-Length: 12\r\n"
            "\r\n"
            "short";
        buffer.append(raw);

        const auto result = HttpParser().parse(buffer);
        assert(!result.ok);
        assert(result.status_code == 400);
    }

    std::cout << "test_http_parser passed\n";
    return 0;
}
