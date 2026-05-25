#include "utils/compression_suite.hpp"
#include <algorithm>

#ifdef _WIN32
#include <intrin.h>
#else
#include <cpuid.h>
#endif

namespace https_server {
namespace compression {

bool CompressionOps::detect_avx2() noexcept {
#ifdef _WIN32
    int cpuInfo[4];
    __cpuid(cpuInfo, 7);
    return (cpuInfo[1] & (1 << 5)) != 0;
#else
    uint32_t eax, ebx, ecx, edx;
    if (__get_cpuid_max(0, nullptr) >= 7) {
        __cpuid_count(7, 0, eax, ebx, ecx, edx);
        return (ebx & (1u << 5)) != 0;
    }
    return false;
#endif
}

size_t CompressionOps::fallback_deflate(const uint8_t* input, size_t input_len,
                                        uint8_t* output, size_t output_len) const noexcept {
    if (!input || !output || input_len == 0 || output_len < input_len + 6) {
        return 0;
    }
    
    output[0] = 0x78;
    output[1] = 0x9c;
    
    std::copy(input, input + input_len, output + 2);
    
    output[input_len + 2] = 0x00;
    output[input_len + 3] = 0x00;
    output[input_len + 4] = 0x00;
    output[input_len + 5] = 0x01;
    
    return input_len + 6;
}

size_t CompressionOps::fallback_lz4(const uint8_t* input, size_t input_len,
                                    uint8_t* output, size_t output_len) const noexcept {
    if (!input || !output || input_len == 0 || output_len < input_len) {
        return 0;
    }
    
    std::copy(input, input + input_len, output);
    return input_len;
}

size_t CompressionOps::fallback_brotli(const uint8_t* input, size_t input_len,
                                       uint8_t* output, size_t output_len) const noexcept {
    if (!input || !output || input_len == 0 || output_len < input_len + 3) {
        return 0;
    }
    
    output[0] = 0x8b;
    output[1] = 0x03;
    
    std::copy(input, input + input_len, output + 2);
    output[input_len + 2] = 0x03;
    
    return input_len + 3;
}

CompressionType CompressionOps::choose_best_compression(const std::string& content_type, 
                                                        size_t content_length,
                                                        const std::string& accept_encoding) const noexcept {
    if (!should_compress(content_type, content_length)) {
        return CompressionType::NONE;
    }
    
    if (accept_encoding.find("br") != std::string::npos) {
        if (content_type.find("text/html") != std::string::npos ||
            content_type.find("text/css") != std::string::npos ||
            content_type.find("application/javascript") != std::string::npos) {
            return CompressionType::BROTLI;
        }
    }
    
    if (accept_encoding.find("deflate") != std::string::npos) {
        if (content_length < 65536) {
            return CompressionType::DEFLATE;
        }
    }
    
    if (accept_encoding.find("gzip") != std::string::npos) {
        return CompressionType::LZ4;
    }
    
    return CompressionType::NONE;
}

bool CompressionOps::should_compress(const std::string& content_type, 
                                     size_t content_length) const noexcept {
    if (content_length < 1024) {
        return false;
    }
    
    if (content_type.find("image/") != std::string::npos ||
        content_type.find("video/") != std::string::npos ||
        content_type.find("audio/") != std::string::npos ||
        content_type.find("application/zip") != std::string::npos ||
        content_type.find("application/gzip") != std::string::npos) {
        return false;
    }
    
    return content_type.find("text/") != std::string::npos ||
           content_type.find("application/javascript") != std::string::npos ||
           content_type.find("application/json") != std::string::npos ||
           content_type.find("image/svg+xml") != std::string::npos;
}

} // namespace compression
} // namespace https_server