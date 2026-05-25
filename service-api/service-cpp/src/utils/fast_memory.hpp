#ifndef HTTPS_SERVER_FAST_MEMORY_HPP
#define HTTPS_SERVER_FAST_MEMORY_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>

#ifdef _WIN32
#include <intrin.h>
#else
#include <cpuid.h>
#endif

namespace https_server {
namespace fast_memory {

extern "C" void* fast_memcpy_avx2(void* dst, const void* src, size_t size) noexcept;
extern "C" void* fast_memchr_avx2(const void* ptr, int value, size_t size) noexcept;
extern "C" void* fast_memmove_avx2(void* dst, const void* src, size_t size) noexcept;

class MemoryOps {
public:
    static MemoryOps& instance() {
        static MemoryOps instance;
        return instance;
    }
    
    void* memcpy(void* dst, const void* src, size_t size) const noexcept {
        if (size >= 32 && has_avx2_) {
            return fast_memcpy_avx2(dst, src, size);
        }
        return std::memcpy(dst, src, size);
    }
    
    void* memchr(const void* ptr, int value, size_t size) const noexcept {
        if (size >= 32 && has_avx2_) {
            return fast_memchr_avx2(ptr, value, size);
        }
        return const_cast<void*>(std::memchr(ptr, value, size));
    }
    
    void* memmove(void* dst, const void* src, size_t size) const noexcept {
        if (size >= 32 && has_avx2_) {
            return fast_memmove_avx2(dst, src, size);
        }
        return std::memmove(dst, src, size);
    }
    
    bool has_avx2() const noexcept { return has_avx2_; }

private:
    MemoryOps() : has_avx2_(detect_avx2()) {}
    
    static bool detect_avx2() noexcept {
#ifdef _WIN32
        int cpuInfo[4];
        __cpuid(cpuInfo, 7);
        return (cpuInfo[1] & (1 << 5)) != 0; // AVX2 bit
#else
        uint32_t eax, ebx, ecx, edx;
        if (__get_cpuid_max(0, nullptr) >= 7) {
            __cpuid_count(7, 0, eax, ebx, ecx, edx);
            return (ebx & (1u << 5)) != 0; // AVX2 bit
        }
        return false;
#endif
    }
    
    bool has_avx2_;
};

inline void* memcpy(void* dst, const void* src, size_t size) noexcept {
    return MemoryOps::instance().memcpy(dst, src, size);
}

inline void* memchr(const void* ptr, int value, size_t size) noexcept {
    return MemoryOps::instance().memchr(ptr, value, size);
}

inline void* memmove(void* dst, const void* src, size_t size) noexcept {
    return MemoryOps::instance().memmove(dst, src, size);
}

} // namespace fast_memory
} // namespace https_server

#endif // HTTPS_SERVER_FAST_MEMORY_HPP