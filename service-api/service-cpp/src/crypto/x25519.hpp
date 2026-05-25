#ifndef HTTPS_SERVER_CRYPTO_X25519_HPP
#define HTTPS_SERVER_CRYPTO_X25519_HPP

#include <cstdint>

extern "C" void x25519_scalar_mult_asm(
    const std::uint8_t* scalar,
    const std::uint8_t* point,
    std::uint8_t* result
) noexcept;

#endif // HTTPS_SERVER_CRYPTO_X25519_HPP