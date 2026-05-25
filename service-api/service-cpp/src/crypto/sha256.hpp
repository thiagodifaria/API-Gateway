#ifndef HTTPS_SERVER_CRYPTO_SHA256_HPP
#define HTTPS_SERVER_CRYPTO_SHA256_HPP

#include <cstdint>

extern "C" void sha256_block_asm(
    const std::uint8_t* input,
    std::uint32_t* hash
) noexcept;

#endif // HTTPS_SERVER_CRYPTO_SHA256_HPP