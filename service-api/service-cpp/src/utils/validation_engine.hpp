#ifndef HTTPS_SERVER_VALIDATION_ENGINE_HPP
#define HTTPS_SERVER_VALIDATION_ENGINE_HPP

#include <cstddef>
#include <cstdint>

namespace https_server {
namespace validation {

enum class ValidationResult { 
    VALID, 
    INVALID_JSON, 
    INVALID_UTF8, 
    UNSAFE_CHARS 
};

extern "C" int json_validate_fast_avx2(const char* data, size_t len) noexcept;
extern "C" bool input_sanitize_basic_avx2(char* data, size_t len) noexcept;
extern "C" bool utf8_validate_simd_avx2(const char* data, size_t len) noexcept;

class ValidationOps {
public:
    static ValidationOps& instance() {
        static ValidationOps instance;
        return instance;
    }
    
    ValidationResult json_validate_fast(const char* data, size_t len) const noexcept {
        if (len >= 16 && has_avx2_) {
            if (!utf8_validate_simd_avx2(data, len)) {
                return ValidationResult::INVALID_UTF8;
            }
            
            int result = json_validate_fast_avx2(data, len);
            return result == 0 ? ValidationResult::VALID : ValidationResult::INVALID_JSON;
        }
        
        return fallback_json_validate(data, len);
    }
    
    bool input_sanitize_basic(char* data, size_t len) const noexcept {
        if (len >= 8 && has_avx2_) {
            return input_sanitize_basic_avx2(data, len);
        }
        
        return fallback_sanitize(data, len);
    }
    
    bool utf8_validate_simd(const char* data, size_t len) const noexcept {
        if (len >= 16 && has_avx2_) {
            return utf8_validate_simd_avx2(data, len);
        }
        
        return fallback_utf8_validate(data, len);
    }
    
    bool has_avx2() const noexcept { return has_avx2_; }

private:
    ValidationOps() : has_avx2_(detect_avx2()) {}
    
    static bool detect_avx2() noexcept;
    
    ValidationResult fallback_json_validate(const char* data, size_t len) const noexcept;
    bool fallback_sanitize(char* data, size_t len) const noexcept;
    bool fallback_utf8_validate(const char* data, size_t len) const noexcept;
    
    bool has_avx2_;
};

} // namespace validation
} // namespace https_server

#endif