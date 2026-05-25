#ifndef HTTPS_SERVER_STATIC_HANDLER_HPP
#define HTTPS_SERVER_STATIC_HANDLER_HPP

#include "http/http.hpp"
#include <string>
#include <map>
#include <mutex>

namespace https_server {

struct CompressedCache {
    std::string data;
    std::string encoding;
    size_t original_size;
};

class StaticHandler {
public:
    explicit StaticHandler(const std::string& web_root);
    
    http::HttpResponse handle(const http::HttpRequest& request);

private:
    std::string web_root_;
    std::map<std::string, std::string> mime_types_;
    std::map<std::string, CompressedCache> compression_cache_;
    std::mutex cache_mutex_;
    
    void init_mime_types();
    std::string get_content_type(const std::string& file_path) const;
    bool is_safe_path(const std::string& requested_path) const;
    std::string normalize_path(const std::string& path) const;
    http::HttpResponse load_error_page(int code, const std::string& status_text);
    std::string make_etag(const std::string& full_path) const;
    std::string cache_control_for(const std::string& file_path) const;
    std::string last_modified_for(const std::string& full_path) const;
    std::string get_header(const http::HttpRequest& request, const std::string& name) const;
    static bool etag_matches(const std::string& header_value, const std::string& etag);
    
    std::string get_accept_encoding(const http::HttpRequest& request) const;
    bool try_serve_compressed(const std::string& file_path, 
                              const std::string& content,
                              const std::string& content_type,
                              const std::string& accept_encoding,
                              http::HttpResponse& response);
    std::string compress_content(const std::string& content,
                                 const std::string& content_type,
                                 const std::string& accept_encoding,
                                 std::string& encoding_used);
};

}

#endif
