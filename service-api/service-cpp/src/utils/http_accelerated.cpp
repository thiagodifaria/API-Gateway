#include "utils/http_accelerated.hpp"

#ifdef _WIN32
#include <intrin.h>
#else
#include <cpuid.h>
#endif

namespace https_server {
namespace http_accelerated {

bool HttpOps::detect_avx2() noexcept {
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

} // namespace http_accelerated
} // namespace https_server