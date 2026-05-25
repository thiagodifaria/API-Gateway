#ifndef HTTPS_SERVER_COMPRESSION_SUITE_HPP
#define HTTPS_SERVER_COMPRESSION_SUITE_HPP

#include <cstddef>
#include <cstdint>
#include <string>

namespace https_server {
namespace compression {

enum class CompressionType { 
    NONE, 
    DEFLATE, 
    LZ4, 
    BROTLI 
};

extern "C" size_t deflate_compress_small_asm(const uint8_t* input, size_t input_len,
                                             uint8_t* output, size_t output_len) noexcept;
extern "C" size_t lz4_compress_fast_asm(const uint8_t* input, size_t input_len,
                                        uint8_t* output, size_t output_len) noexcept;
extern "C" size_t brotli_compress_web_asm(const uint8_t* input, size_t input_len,
                                          uint8_t* output, size_t output_len) noexcept;

class CompressionOps {
public:
    static CompressionOps& instance() {
        static CompressionOps instance;
        return instance;
    }
    
    size_t deflate_compress_small(const uint8_t* input, size_t input_len,
                                  uint8_t* output, size_t output_len) const noexcept {
        if (input_len >= 32 && input_len < 65536 && has_avx2_) {
            return deflate_compress_small_asm(input, input_len, output, output_len);
        }
        return fallback_deflate(input, input_len, output, output_len);
    }
    
    size_t lz4_compress_fast(const uint8_t* input, size_t input_len,
                             uint8_t* output, size_t output_len) const noexcept {
        if (input_len >= 16 && has_avx2_) {
            return lz4_compress_fast_asm(input, input_len, output, output_len);
        }
        return fallback_lz4(input, input_len, output, output_len);
    }
    
    size_t brotli_compress_web(const uint8_t* input, size_t input_len,
                               uint8_t* output, size_t output_len) const noexcept {
        if (input_len >= 64 && has_avx2_) {
            return brotli_compress_web_asm(input, input_len, output, output_len);
        }
        return fallback_brotli(input, input_len, output, output_len);
    }
    
    CompressionType choose_best_compression(const std::string& content_type, 
                                            size_t content_length,
                                            const std::string& accept_encoding) const noexcept;
    
    bool should_compress(const std::string& content_type, size_t content_length) const noexcept;
    
    bool has_avx2() const noexcept { return has_avx2_; }

private:
    CompressionOps() : has_avx2_(detect_avx2()) {}
    
    static bool detect_avx2() noexcept;
    
    size_t fallback_deflate(const uint8_t* input, size_t input_len,
                            uint8_t* output, size_t output_len) const noexcept;
    size_t fallback_lz4(const uint8_t* input, size_t input_len,
                        uint8_t* output, size_t output_len) const noexcept;
    size_t fallback_brotli(const uint8_t* input, size_t input_len,
                           uint8_t* output, size_t output_len) const noexcept;
    
    bool has_avx2_;
};

} // namespace compression
} // namespace https_server

#endif