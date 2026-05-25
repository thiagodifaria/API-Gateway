#include "http/parser.hpp"
#include "utils/buffer.hpp"
#include "utils/http_accelerated.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

namespace https_server::http {

HttpParser::HttpParser(ParserLimits limits) : limits_(limits) {}

ParseResult HttpParser::parse(const Buffer& buffer) const {
    ParseResult result;
    const auto view = buffer.readable_view();

    if (view.size() > limits_.max_header_bytes + limits_.max_body_bytes) {
        result.status_code = 413;
        result.status_text = "Payload Too Large";
        result.message = "request exceeds configured limits";
        return result;
    }

    size_t header_end_pos = 0;
    if (!http_accelerated::HttpOps::instance().find_header_end(
            view.data(), view.size(), &header_end_pos)) {
        result.status_code = view.size() > limits_.max_header_bytes ? 431 : 400;
        result.status_text = view.size() > limits_.max_header_bytes
            ? "Request Header Fields Too Large"
            : "Bad Request";
        result.message = "incomplete HTTP headers";
        return result;
    }

    if (header_end_pos > limits_.max_header_bytes) {
        result.status_code = 431;
        result.status_text = "Request Header Fields Too Large";
        result.message = "headers exceed configured limit";
        return result;
    }

    std::string raw_request(view);
    std::istringstream request_stream(raw_request);
    std::string line;

    if (!std::getline(request_stream, line)) {
        result.message = "missing request line";
        return result;
    }
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }

    std::istringstream request_line_stream(line);
    request_line_stream >> result.request.method >> result.request.uri >> result.request.http_version;
    if (result.request.method.empty() || result.request.uri.empty() || result.request.http_version.empty()) {
        result.message = "malformed request line";
        return result;
    }
    if (result.request.uri.size() > limits_.max_uri_bytes) {
        result.status_code = 414;
        result.status_text = "URI Too Long";
        result.message = "URI exceeds configured limit";
        return result;
    }

    std::size_t content_length = 0;
    while (std::getline(request_stream, line) && !line.empty() && line != "\r") {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        const std::size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos) {
            result.message = "malformed header";
            return result;
        }

        const std::string key = lower(trim(line.substr(0, colon_pos)));
        const std::string value = trim(line.substr(colon_pos + 1));
        result.request.headers[key] = value;

        if (key == "content-length") {
            try {
                content_length = static_cast<std::size_t>(std::stoull(value));
            } catch (const std::exception&) {
                result.message = "invalid content-length";
                return result;
            }
        }
    }

    if (content_length > limits_.max_body_bytes) {
        result.status_code = 413;
        result.status_text = "Payload Too Large";
        result.message = "body exceeds configured limit";
        return result;
    }

    const std::size_t body_start = header_end_pos;
    const std::size_t body_available = body_start < raw_request.size()
        ? raw_request.size() - body_start
        : 0;
    if (content_length > body_available) {
        result.status_code = 400;
        result.status_text = "Bad Request";
        result.message = "incomplete request body";
        return result;
    }
    if (content_length > 0) {
        result.request.body = raw_request.substr(body_start, content_length);
    }

    split_target(result.request);
    parse_query(result.request);

    result.ok = true;
    result.status_code = 200;
    result.status_text = "OK";
    return result;
}

std::string HttpParser::trim(std::string value) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::string HttpParser::lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

void HttpParser::split_target(HttpRequest& request) {
    const auto query_pos = request.uri.find('?');
    if (query_pos == std::string::npos) {
        request.path = request.uri.empty() ? "/" : request.uri;
        request.query_string.clear();
        return;
    }

    request.path = request.uri.substr(0, query_pos);
    request.query_string = request.uri.substr(query_pos + 1);
    if (request.path.empty()) {
        request.path = "/";
    }
}

void HttpParser::parse_query(HttpRequest& request) {
    std::size_t start = 0;
    while (start < request.query_string.size()) {
        const std::size_t amp = request.query_string.find('&', start);
        const std::size_t end = amp == std::string::npos ? request.query_string.size() : amp;
        const std::string part = request.query_string.substr(start, end - start);
        const std::size_t eq = part.find('=');

        if (!part.empty()) {
            const std::string key = url_decode(eq == std::string::npos ? part : part.substr(0, eq));
            const std::string value = eq == std::string::npos ? "" : url_decode(part.substr(eq + 1));
            request.query_params[key] = value;
        }

        if (amp == std::string::npos) {
            break;
        }
        start = amp + 1;
    }
}

std::string HttpParser::url_decode(const std::string& value) {
    std::string decoded;
    decoded.reserve(value.size());

    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '+' ) {
            decoded.push_back(' ');
        } else if (value[i] == '%' && i + 2 < value.size()) {
            const std::string hex = value.substr(i + 1, 2);
            char* end = nullptr;
            const long code = std::strtol(hex.c_str(), &end, 16);
            if (end && *end == '\0') {
                decoded.push_back(static_cast<char>(code));
                i += 2;
            } else {
                decoded.push_back(value[i]);
            }
        } else {
            decoded.push_back(value[i]);
        }
    }

    return decoded;
}

} // namespace https_server::http
