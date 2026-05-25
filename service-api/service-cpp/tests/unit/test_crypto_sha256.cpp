#include "crypto/sha256.hpp"
#include <openssl/sha.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <iomanip>

int main() {
    const std::vector<std::uint8_t> input = {
        0x61, 0x62, 0x63, 0x80, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18
    };
    
    const std::vector<std::uint32_t> expected_hash = {
        0xba7816bf, 0x8f01cfea, 0x414140de, 0x5dae2223,
        0xb00361a3, 0x96177a9c, 0xb410ff61, 0xf20015ad
    };

    std::vector<std::uint32_t> hash = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    sha256_block_asm(input.data(), hash.data());

    bool success = true;
    for (size_t i = 0; i < 8; ++i) {
        if (hash[i] != expected_hash[i]) {
            success = false;
            break;
        }
    }

    if (success) {
        std::cout << "SUCCESS: Assembly SHA-256 implementation is correct." << std::endl;
        return 0;
    } else {
        std::cout << "FAILURE: Assembly SHA-256 implementation is incorrect." << std::endl;
        std::cout << "Expected: ";
        for (auto h : expected_hash) {
            std::cout << std::hex << std::setw(8) << std::setfill('0') << h;
        }
        std::cout << std::endl;
        std::cout << "Actual:   ";
        for (auto h : hash) {
            std::cout << std::hex << std::setw(8) << std::setfill('0') << h;
        }
        std::cout << std::endl;
        return 1;
    }
}