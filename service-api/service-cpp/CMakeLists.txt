cmake_minimum_required(VERSION 3.16)
project(api_gateway LANGUAGES CXX ASM_NASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenSSL REQUIRED)

set(ASSEMBLY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../service-assembly")

add_library(aes_asm_impl ${ASSEMBLY_DIR}/crypto/asm/x86_64/aes_ni.asm)
add_library(sha256_asm_impl ${ASSEMBLY_DIR}/crypto/asm/x86_64/sha256_avx.asm)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    enable_language(ASM)
    add_library(sha256_neon_impl ${ASSEMBLY_DIR}/crypto/asm/arm64/sha256_neon.S)
    set(SHA256_IMPL sha256_neon_impl)
else()
    set(SHA256_IMPL sha256_asm_impl)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64")
    add_library(fast_memory_impl ${ASSEMBLY_DIR}/utils/asm/x86_64/memory_avx2.asm)
    set(HAS_FAST_MEMORY TRUE)
    add_library(http_asm_impl ${ASSEMBLY_DIR}/utils/asm/x86_64/http_accelerated.asm)
    set(HAS_HTTP_ASM TRUE)
    add_library(validation_asm_impl ${ASSEMBLY_DIR}/utils/asm/x86_64/validation_engine.asm)
    set(HAS_VALIDATION_ASM TRUE)
    add_library(crypto_advanced_asm_impl ${ASSEMBLY_DIR}/crypto/asm/x86_64/crypto_advanced.asm)
    set(HAS_CRYPTO_ADVANCED TRUE)
    add_library(compression_asm_impl ${ASSEMBLY_DIR}/utils/asm/x86_64/compression_suite.asm)
    set(HAS_COMPRESSION_ASM TRUE)
    add_library(network_asm_impl ${ASSEMBLY_DIR}/utils/asm/x86_64/network_operations.asm)
    set(HAS_NETWORK_ASM TRUE)
else()
    set(HAS_FAST_MEMORY FALSE)
    set(HAS_HTTP_ASM FALSE)
    set(HAS_VALIDATION_ASM FALSE)
    set(HAS_CRYPTO_ADVANCED FALSE)
    set(HAS_COMPRESSION_ASM FALSE)
    set(HAS_NETWORK_ASM FALSE)
endif()

add_library(p256_asm_impl ${ASSEMBLY_DIR}/crypto/asm/x86_64/p256_avx2.asm)

add_executable(api_gateway 
    src/main.cpp
    src/core/server.cpp
    src/core/config.cpp
    src/core/event_loop.cpp
    src/utils/logger.cpp
    src/utils/http_accelerated.cpp
    src/utils/validation_engine.cpp
    src/utils/compression_suite.cpp
    src/utils/network_operations.cpp
    src/utils/benchmark_utils.cpp
    src/http/http.cpp
    src/http/parser.cpp
    src/http/static_handler.cpp
    src/gateway/upstream_pool.cpp
    src/gateway/reverse_proxy.cpp
    src/middleware/rate_limiter.cpp
    src/middleware/auth.cpp
    src/metrics/metrics.cpp
    src/crypto/aes_provider.cpp
)

add_subdirectory(src/crypto)

target_compile_definitions(api_gateway PRIVATE OPENSSL_ROOT_DIR="${OPENSSL_ROOT_DIR}")
target_include_directories(api_gateway PRIVATE src third_party ${OPENSSL_INCLUDE_DIR})

target_link_libraries(api_gateway PRIVATE 
    aes_asm_impl 
    ${SHA256_IMPL}
    p256_asm_impl
    OpenSSL::SSL 
    OpenSSL::Crypto
)

if(HAS_FAST_MEMORY)
    target_link_libraries(api_gateway PRIVATE fast_memory_impl)
    target_compile_definitions(api_gateway PRIVATE HAS_FAST_MEMORY=1)
endif()

if(HAS_HTTP_ASM)
    target_link_libraries(api_gateway PRIVATE http_asm_impl)
    target_compile_definitions(api_gateway PRIVATE HAS_HTTP_ASM=1)
endif()

if(HAS_VALIDATION_ASM)
    target_link_libraries(api_gateway PRIVATE validation_asm_impl)
    target_compile_definitions(api_gateway PRIVATE HAS_VALIDATION_ASM=1)
endif()

if(HAS_CRYPTO_ADVANCED)
    target_link_libraries(api_gateway PRIVATE crypto_advanced_asm_impl)
    target_compile_definitions(api_gateway PRIVATE HAS_CRYPTO_ADVANCED=1)
endif()

if(HAS_COMPRESSION_ASM)
    target_link_libraries(api_gateway PRIVATE compression_asm_impl)
    target_compile_definitions(api_gateway PRIVATE HAS_COMPRESSION_ASM=1)
endif()

if(HAS_NETWORK_ASM)
    target_link_libraries(api_gateway PRIVATE network_asm_impl)
    target_compile_definitions(api_gateway PRIVATE HAS_NETWORK_ASM=1)
endif()

if(WIN32)
    target_link_libraries(api_gateway PRIVATE ws2_32)
    if(MSVC)
        target_link_options(api_gateway PRIVATE /LARGEADDRESSAWARE)
    endif()
endif()

add_executable(unit_test_aes tests/unit/test_crypto_aes.cpp)
target_include_directories(unit_test_aes PRIVATE src ${OPENSSL_INCLUDE_DIR})
target_link_libraries(unit_test_aes PRIVATE aes_asm_impl OpenSSL::SSL OpenSSL::Crypto)

add_executable(unit_test_sha256 tests/unit/test_crypto_sha256.cpp)
target_include_directories(unit_test_sha256 PRIVATE src ${OPENSSL_INCLUDE_DIR})
target_link_libraries(unit_test_sha256 PRIVATE ${SHA256_IMPL} OpenSSL::SSL OpenSSL::Crypto)
if(MSVC)
    target_link_options(unit_test_sha256 PRIVATE /LARGEADDRESSAWARE)
endif()

add_executable(unit_test_p256 tests/unit/test_crypto_p256.cpp)
target_include_directories(unit_test_p256 PRIVATE src ${OPENSSL_INCLUDE_DIR})
target_link_libraries(unit_test_p256 PRIVATE p256_asm_impl OpenSSL::SSL OpenSSL::Crypto)

add_executable(unit_test_router tests/unit/test_router.cpp)
target_include_directories(unit_test_router PRIVATE src)

add_executable(unit_test_http_parser
    tests/unit/test_http_parser.cpp
    src/http/parser.cpp
    src/utils/http_accelerated.cpp
)
target_include_directories(unit_test_http_parser PRIVATE src)
if(HAS_HTTP_ASM)
    target_link_libraries(unit_test_http_parser PRIVATE http_asm_impl)
    target_compile_definitions(unit_test_http_parser PRIVATE HAS_HTTP_ASM=1)
endif()

add_executable(unit_test_auth
    tests/unit/test_auth.cpp
    src/middleware/auth.cpp
)
target_include_directories(unit_test_auth PRIVATE src third_party ${OPENSSL_INCLUDE_DIR})
target_link_libraries(unit_test_auth PRIVATE OpenSSL::SSL OpenSSL::Crypto)

add_executable(benchmark_aes tests/perf/benchmark_aes.cpp)
target_include_directories(benchmark_aes PRIVATE src ${OPENSSL_INCLUDE_DIR})
target_link_libraries(benchmark_aes PRIVATE aes_asm_impl OpenSSL::SSL OpenSSL::Crypto)

add_executable(benchmark_sha256 tests/perf/benchmark_sha256.cpp)
target_include_directories(benchmark_sha256 PRIVATE src ${OPENSSL_INCLUDE_DIR})
target_link_libraries(benchmark_sha256 PRIVATE ${SHA256_IMPL} OpenSSL::SSL OpenSSL::Crypto)
if(MSVC)
    target_link_options(benchmark_sha256 PRIVATE /LARGEADDRESSAWARE)
endif()

add_executable(benchmark_p256 tests/perf/benchmark_p256.cpp)
target_include_directories(benchmark_p256 PRIVATE src ${OPENSSL_INCLUDE_DIR})
target_link_libraries(benchmark_p256 PRIVATE p256_asm_impl OpenSSL::SSL OpenSSL::Crypto)

if(HAS_FAST_MEMORY)
    add_executable(test_fast_memory tests/unit/test_fast_memory.cpp)
    target_include_directories(test_fast_memory PRIVATE src)
    target_link_libraries(test_fast_memory PRIVATE fast_memory_impl)
    target_compile_definitions(test_fast_memory PRIVATE HAS_FAST_MEMORY=1)
endif()

if(MSVC)
    target_compile_options(api_gateway PRIVATE /W4 /permissive-)
    target_compile_options(unit_test_aes PRIVATE /W4 /permissive-)
    target_compile_options(unit_test_sha256 PRIVATE /W4 /permissive-)
    target_compile_options(unit_test_p256 PRIVATE /W4 /permissive-)
    target_compile_options(unit_test_router PRIVATE /W4 /permissive-)
    target_compile_options(unit_test_http_parser PRIVATE /W4 /permissive-)
    target_compile_options(unit_test_auth PRIVATE /W4 /permissive-)
    target_compile_options(benchmark_aes PRIVATE /W4 /permissive-)
    target_compile_options(benchmark_sha256 PRIVATE /W4 /permissive-)
    target_compile_options(benchmark_p256 PRIVATE /W4 /permissive-)
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(api_gateway PRIVATE /O2 /DNDEBUG)
        target_compile_options(unit_test_aes PRIVATE /O2 /DNDEBUG)
        target_compile_options(unit_test_sha256 PRIVATE /O2 /DNDEBUG)
        target_compile_options(unit_test_p256 PRIVATE /O2 /DNDEBUG)
        target_compile_options(unit_test_router PRIVATE /O2 /DNDEBUG)
        target_compile_options(unit_test_http_parser PRIVATE /O2 /DNDEBUG)
        target_compile_options(unit_test_auth PRIVATE /O2 /DNDEBUG)
        target_compile_options(benchmark_aes PRIVATE /O2 /DNDEBUG)
        target_compile_options(benchmark_sha256 PRIVATE /O2 /DNDEBUG)
        target_compile_options(benchmark_p256 PRIVATE /O2 /DNDEBUG)
    endif()
else()
    set(COMMON_FLAGS -Wall -Wextra -Wpedantic -Wconversion)
    
    target_compile_options(api_gateway PRIVATE ${COMMON_FLAGS})
    target_compile_options(unit_test_aes PRIVATE ${COMMON_FLAGS})
    target_compile_options(unit_test_sha256 PRIVATE ${COMMON_FLAGS})
    target_compile_options(unit_test_p256 PRIVATE ${COMMON_FLAGS})
    target_compile_options(unit_test_router PRIVATE ${COMMON_FLAGS})
    target_compile_options(unit_test_http_parser PRIVATE ${COMMON_FLAGS})
    target_compile_options(unit_test_auth PRIVATE ${COMMON_FLAGS})
    target_compile_options(benchmark_aes PRIVATE ${COMMON_FLAGS})
    target_compile_options(benchmark_sha256 PRIVATE ${COMMON_FLAGS})
    target_compile_options(benchmark_p256 PRIVATE ${COMMON_FLAGS})
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(DEBUG_FLAGS -g)
        target_compile_options(api_gateway PRIVATE ${DEBUG_FLAGS})
        target_compile_options(benchmark_aes PRIVATE ${DEBUG_FLAGS})
        target_compile_options(unit_test_aes PRIVATE ${DEBUG_FLAGS})
        target_compile_options(unit_test_sha256 PRIVATE ${DEBUG_FLAGS})
        target_compile_options(unit_test_p256 PRIVATE ${DEBUG_FLAGS})
        target_compile_options(unit_test_router PRIVATE ${DEBUG_FLAGS})
        target_compile_options(unit_test_http_parser PRIVATE ${DEBUG_FLAGS})
        target_compile_options(unit_test_auth PRIVATE ${DEBUG_FLAGS})
        target_compile_options(benchmark_sha256 PRIVATE ${DEBUG_FLAGS})
        target_compile_options(benchmark_p256 PRIVATE ${DEBUG_FLAGS})
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(RELEASE_FLAGS -O3 -DNDEBUG)
        target_compile_options(api_gateway PRIVATE ${RELEASE_FLAGS})
        target_compile_options(benchmark_aes PRIVATE ${RELEASE_FLAGS})
        target_compile_options(unit_test_aes PRIVATE ${RELEASE_FLAGS})
        target_compile_options(unit_test_sha256 PRIVATE ${RELEASE_FLAGS})
        target_compile_options(unit_test_p256 PRIVATE ${RELEASE_FLAGS})
        target_compile_options(unit_test_router PRIVATE ${RELEASE_FLAGS})
        target_compile_options(unit_test_http_parser PRIVATE ${RELEASE_FLAGS})
        target_compile_options(unit_test_auth PRIVATE ${RELEASE_FLAGS})
        target_compile_options(benchmark_sha256 PRIVATE ${RELEASE_FLAGS})
        target_compile_options(benchmark_p256 PRIVATE ${RELEASE_FLAGS})
    endif()
endif()

if(HAS_FAST_MEMORY)
    if(MSVC)
        target_compile_options(test_fast_memory PRIVATE /W4 /permissive-)
    else()
        target_compile_options(test_fast_memory PRIVATE ${COMMON_FLAGS})
    endif()
endif()

message(STATUS "API Gateway Build Configuration:")
message(STATUS "  Processor: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  Build Type: Release")
message(STATUS "  OpenSSL Version: ${OPENSSL_VERSION}")
message(STATUS "  SHA-256 Implementation: ${SHA256_IMPL}")
message(STATUS "  Fast Memory Operations: ${HAS_FAST_MEMORY}")
message(STATUS "  HTTP Assembly Optimizations: ${HAS_HTTP_ASM}")
message(STATUS "  Validation Assembly Optimizations: ${HAS_VALIDATION_ASM}")
message(STATUS "  Crypto Advanced Assembly: ${HAS_CRYPTO_ADVANCED}")
message(STATUS "  Compression Assembly Optimizations: ${HAS_COMPRESSION_ASM}")
message(STATUS "  Network Assembly Optimizations: ${HAS_NETWORK_ASM}")

if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    message(STATUS "  ARM64 NEON optimizations enabled")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64")
    message(STATUS "  x86_64 AVX2/AES-NI optimizations enabled")
endif()
