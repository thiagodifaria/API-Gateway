#ifndef HTTPS_SERVER_CRYPTO_AES_HPP
#define HTTPS_SERVER_CRYPTO_AES_HPP

#include <cstdint>

extern "C" void aes_encrypt_block_asm(
    const std::uint8_t* input,
    std::uint8_t* output,
    const std::uint8_t* round_keys
) noexcept;

#endif // HTTPS_SERVER_CRYPTO_AES_HPP