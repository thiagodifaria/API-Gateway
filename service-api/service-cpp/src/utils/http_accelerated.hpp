#ifndef HTTPS_SERVER_HTTP_ACCELERATED_HPP
#define HTTPS_SERVER_HTTP_ACCELERATED_HPP

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace https_server {
namespace http_accelerated {

struct HttpParseResult {
    size_t method_len;
    size_t uri_start; 
    size_t uri_len;
    bool valid;
};

extern "C" bool http_find_header_end_avx2(const char* data, size_t len, size_t* pos) noexcept;
extern "C" bool http_parse_method_uri_avx2(const char* data, size_t len, HttpParseResult* result) noexcept;

class HttpOps {
public:
    static HttpOps& instance() {
        static HttpOps instance;
        return instance;
    }
    
    bool find_header_end(const char* data, size_t len, size_t* pos) const noexcept {
        if (len >= 32 && has_avx2_) {
            return http_find_header_end_avx2(data, len, pos);
        }
        
        std::string_view view(data, len);
        auto found = view.find("\r\n\r\n");
        if (found != std::string_view::npos) {
            *pos = found + 4;
            return true;
        }
        return false;
    }
    
    HttpParseResult parse_method_uri(const char* data, size_t len) const noexcept {
        if (len >= 16 && has_avx2_) {
            HttpParseResult result;
            if (http_parse_method_uri_avx2(data, len, &result)) {
                return result;
            }
        }
        return {0, 0, 0, false};
    }
    
    bool has_avx2() const noexcept { return has_avx2_; }

private:
    HttpOps() : has_avx2_(detect_avx2()) {}
    
    static bool detect_avx2() noexcept;
    bool has_avx2_;
};

} // namespace http_accelerated
} // namespace https_server

#endif