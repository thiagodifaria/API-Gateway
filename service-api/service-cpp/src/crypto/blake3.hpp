#ifndef HTTPS_SERVER_CRYPTO_BLAKE3_HPP
#define HTTPS_SERVER_CRYPTO_BLAKE3_HPP

#include <cstdint>
#include <cstddef>

extern "C" void blake3_hash_chunk_asm(
    const std::uint8_t* input,
    std::size_t len,
    std::uint8_t* output
) noexcept;

#endif // HTTPS_SERVER_CRYPTO_BLAKE3_HPP