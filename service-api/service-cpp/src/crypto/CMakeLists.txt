add_library(aes_provider SHARED aes_provider.cpp)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    set(SHA256_IMPL sha256_neon_impl)
else()
    set(SHA256_IMPL sha256_asm_impl)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64")
    set(HAS_CRYPTO_ADVANCED TRUE)
else()
    set(HAS_CRYPTO_ADVANCED FALSE)
endif()

target_link_libraries(aes_provider PRIVATE aes_asm_impl ${SHA256_IMPL} p256_asm_impl)

if(HAS_CRYPTO_ADVANCED)
    target_link_libraries(aes_provider PRIVATE crypto_advanced_asm_impl)
    target_compile_definitions(aes_provider PRIVATE HAS_CRYPTO_ADVANCED=1)
endif()

target_link_libraries(aes_provider PRIVATE OpenSSL::SSL OpenSSL::Crypto)
target_include_directories(aes_provider PRIVATE .. ${OPENSSL_INCLUDE_DIR})

if(MSVC)
    target_link_options(aes_provider PRIVATE /LARGEADDRESSAWARE)
endif()

add_custom_command(TARGET aes_provider POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:aes_provider>
    $<TARGET_FILE_DIR:api_gateway>
)

if(MSVC)
    target_compile_options(aes_provider PRIVATE /W4 /permissive-)
else()
    target_compile_options(aes_provider PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

message(STATUS "  Crypto Advanced Assembly: ${HAS_CRYPTO_ADVANCED}")
