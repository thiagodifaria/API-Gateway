#include "crypto/aes.hpp"
#include <openssl/aes.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <iomanip>

int main() {
    // Vetor de teste padrão para AES-128
    const std::vector<std::uint8_t> key = {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
    };
    const std::vector<std::uint8_t> plaintext = {
        0x32, 0x43, 0xf6, 0xa8, 0x88, 0x5a, 0x30, 0x8d, 0x31, 0x31, 0x98, 0xa2, 0xe0, 0x37, 0x07, 0x34
    };
    const std::vector<std::uint8_t> expected_ciphertext = {
        0x39, 0x25, 0x84, 0x1d, 0x02, 0xdc, 0x09, 0xfb, 0xdc, 0x11, 0x85, 0x97, 0x19, 0x6a, 0x0b, 0x32
    };

    // AES requer chaves de rodada expandidas. Usamos o OpenSSL para gerá-las.
    std::vector<std::uint8_t> round_keys(176); // 11 rodadas * 16 bytes/rodada
    AES_set_encrypt_key(key.data(), 128, reinterpret_cast<AES_KEY*>(round_keys.data()));

    std::vector<std::uint8_t> actual_ciphertext(16);

    aes_encrypt_block_asm(plaintext.data(), actual_ciphertext.data(), round_keys.data());

    if (std::memcmp(expected_ciphertext.data(), actual_ciphertext.data(), 16) == 0) {
        std::cout << "SUCCESS: Assembly AES implementation is correct." << std::endl;
        return 0;
    } else {
        std::cout << "FAILURE: Assembly AES implementation is incorrect." << std::endl;
        std::cout << "Expected: ";
        for(auto b : expected_ciphertext) std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
        std::cout << std::endl;
        std::cout << "Actual:   ";
        for(auto b : actual_ciphertext) std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
        std::cout << std::endl;
        return 1;
    }
}