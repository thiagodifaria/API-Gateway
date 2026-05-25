#ifndef HTTPS_SERVER_CRYPTO_CHACHA20_HPP
#define HTTPS_SERVER_CRYPTO_CHACHA20_HPP

#include <cstdint>

extern "C" void chacha20_encrypt_block_asm(
    const std::uint8_t* input,
    std::uint8_t* output,
    const std::uint8_t* key,
    const std::uint8_t* nonce,
    std::uint32_t counter
) noexcept;

#endif // HTTPS_SERVER_CRYPTO_CHACHA20_HPP