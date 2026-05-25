#include "utils/network_operations.hpp"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <intrin.h>
#else
#include <cpuid.h>
#endif

namespace https_server {
namespace network_ops {

bool NetworkOps::detect_avx2() noexcept {
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

bool NetworkOps::detect_rdrand() noexcept {
#ifdef _WIN32
    int cpuInfo[4];
    __cpuid(cpuInfo, 1);
    return (cpuInfo[2] & (1 << 30)) != 0;
#else
    uint32_t eax, ebx, ecx, edx;
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        return (ecx & (1u << 30)) != 0;
    }
    return false;
#endif
}

size_t NetworkOps::fallback_base64_encode(const uint8_t* input, size_t len, char* output) const noexcept {
    static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    size_t output_len = 0;
    size_t i = 0;
    
    while (i < len) {
        uint32_t a = input[i++];
        uint32_t b = (i < len) ? input[i++] : 0;
        uint32_t c = (i < len) ? input[i++] : 0;
        
        uint32_t combined = (a << 16) | (b << 8) | c;
        
        output[output_len++] = base64_chars[(combined >> 18) & 63];
        output[output_len++] = base64_chars[(combined >> 12) & 63];
        output[output_len++] = (i - 1 < len) ? base64_chars[(combined >> 6) & 63] : '=';
        output[output_len++] = (i < len) ? base64_chars[combined & 63] : '=';
    }
    
    return output_len;
}

size_t NetworkOps::fallback_base64_decode(const char* input, size_t len, uint8_t* output) const noexcept {
    auto decode_char = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    };
    
    size_t output_len = 0;
    size_t i = 0;
    
    while (i < len) {
        int a = (i < len) ? decode_char(input[i++]) : 0;
        int b = (i < len) ? decode_char(input[i++]) : 0;
        int c = (i < len && input[i] != '=') ? decode_char(input[i++]) : -1;
        int d = (i < len && input[i] != '=') ? decode_char(input[i++]) : -1;
        
        if (a < 0 || b < 0) break;
        
        uint32_t combined = (a << 18) | (b << 12);
        if (c >= 0) combined |= (c << 6);
        if (d >= 0) combined |= d;
        
        output[output_len++] = (combined >> 16) & 0xFF;
        if (c >= 0) output[output_len++] = (combined >> 8) & 0xFF;
        if (d >= 0) output[output_len++] = combined & 0xFF;
    }
    
    return output_len;
}

void NetworkOps::fallback_uuid_generate(uint8_t uuid[16]) const noexcept {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<int> dis(0, 255);
    
    for (int i = 0; i < 16; ++i) {
        uuid[i] = static_cast<uint8_t>(dis(gen));
    }
    
    uuid[6] = (uuid[6] & 0x0F) | 0x40;
    uuid[8] = (uuid[8] & 0x3F) | 0x80;
}

void NetworkOps::fallback_hex_encode(const uint8_t* input, size_t len, char* output) const noexcept {
    static const char hex_chars[] = "0123456789ABCDEF";
    
    for (size_t i = 0; i < len; ++i) {
        output[i * 2] = hex_chars[input[i] >> 4];
        output[i * 2 + 1] = hex_chars[input[i] & 0x0F];
    }
}

std::string NetworkOps::generate_uuid_string() const noexcept {
    uint8_t uuid[16];
    uuid_generate_v4(uuid);
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    
    for (int i = 0; i < 4; ++i) ss << std::setw(2) << static_cast<int>(uuid[i]);
    ss << "-";
    for (int i = 4; i < 6; ++i) ss << std::setw(2) << static_cast<int>(uuid[i]);
    ss << "-";
    for (int i = 6; i < 8; ++i) ss << std::setw(2) << static_cast<int>(uuid[i]);
    ss << "-";
    for (int i = 8; i < 10; ++i) ss << std::setw(2) << static_cast<int>(uuid[i]);
    ss << "-";
    for (int i = 10; i < 16; ++i) ss << std::setw(2) << static_cast<int>(uuid[i]);
    
    return ss.str();
}

std::string NetworkOps::encode_base64(const std::string& input) const noexcept {
    if (input.empty()) return "";
    
    size_t output_size = ((input.size() + 2) / 3) * 4;
    std::string output(output_size, '\0');
    
    size_t actual_size = base64_encode_simd(
        reinterpret_cast<const uint8_t*>(input.data()),
        input.size(),
        &output[0]
    );
    
    output.resize(actual_size);
    return output;
}

std::string NetworkOps::decode_base64(const std::string& input) const noexcept {
    if (input.empty()) return "";
    
    size_t output_size = (input.size() / 4) * 3;
    std::string output(output_size, '\0');
    
    size_t actual_size = base64_decode_simd(
        input.data(),
        input.size(),
        reinterpret_cast<uint8_t*>(&output[0])
    );
    
    output.resize(actual_size);
    return output;
}

std::string NetworkOps::encode_hex(const std::string& input) const noexcept {
    if (input.empty()) return "";
    
    std::string output(input.size() * 2, '\0');
    
    hex_encode_fast(
        reinterpret_cast<const uint8_t*>(input.data()),
        input.size(),
        &output[0]
    );
    
    return output;
}

} // namespace network_ops
} // namespace https_server