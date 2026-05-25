#ifndef HTTPS_SERVER_NETWORK_OPERATIONS_HPP
#define HTTPS_SERVER_NETWORK_OPERATIONS_HPP

#include <cstddef>
#include <cstdint>
#include <string>

namespace https_server {
namespace network_ops {

extern "C" size_t base64_encode_simd_asm(const uint8_t* input, size_t len, char* output) noexcept;
extern "C" size_t base64_decode_simd_asm(const char* input, size_t len, uint8_t* output) noexcept;
extern "C" void uuid_generate_v4_asm(uint8_t uuid[16]) noexcept;
extern "C" void hex_encode_fast_asm(const uint8_t* input, size_t len, char* output) noexcept;

class NetworkOps {
public:
    static NetworkOps& instance() {
        static NetworkOps instance;
        return instance;
    }
    
    size_t base64_encode_simd(const uint8_t* input, size_t len, char* output) const noexcept {
        if (len >= 16 && has_avx2_) {
            return base64_encode_simd_asm(input, len, output);
        }
        return fallback_base64_encode(input, len, output);
    }
    
    size_t base64_decode_simd(const char* input, size_t len, uint8_t* output) const noexcept {
        if (len >= 16 && has_avx2_) {
            return base64_decode_simd_asm(input, len, output);
        }
        return fallback_base64_decode(input, len, output);
    }
    
    void uuid_generate_v4(uint8_t uuid[16]) const noexcept {
        if (has_rdrand_) {
            uuid_generate_v4_asm(uuid);
        } else {
            fallback_uuid_generate(uuid);
        }
    }
    
    void hex_encode_fast(const uint8_t* input, size_t len, char* output) const noexcept {
        if (len >= 8 && has_avx2_) {
            hex_encode_fast_asm(input, len, output);
        } else {
            fallback_hex_encode(input, len, output);
        }
    }
    
    std::string generate_uuid_string() const noexcept;
    std::string encode_base64(const std::string& input) const noexcept;
    std::string decode_base64(const std::string& input) const noexcept;
    std::string encode_hex(const std::string& input) const noexcept;
    
    bool has_avx2() const noexcept { return has_avx2_; }
    bool has_rdrand() const noexcept { return has_rdrand_; }

private:
    NetworkOps() : has_avx2_(detect_avx2()), has_rdrand_(detect_rdrand()) {}
    
    static bool detect_avx2() noexcept;
    static bool detect_rdrand() noexcept;
    
    size_t fallback_base64_encode(const uint8_t* input, size_t len, char* output) const noexcept;
    size_t fallback_base64_decode(const char* input, size_t len, uint8_t* output) const noexcept;
    void fallback_uuid_generate(uint8_t uuid[16]) const noexcept;
    void fallback_hex_encode(const uint8_t* input, size_t len, char* output) const noexcept;
    
    bool has_avx2_;
    bool has_rdrand_;
};

} // namespace network_ops
} // namespace https_server

#endif