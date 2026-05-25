#ifndef HTTPS_SERVER_CRYPTO_POLY1305_HPP
#define HTTPS_SERVER_CRYPTO_POLY1305_HPP

#include <cstdint>
#include <cstddef>

extern "C" void poly1305_mac_block_asm(
    const std::uint8_t* input,
    std::size_t len,
    const std::uint8_t* key,
    std::uint8_t* mac
) noexcept;

#endif // HTTPS_SERVER_CRYPTO_POLY1305_HPP