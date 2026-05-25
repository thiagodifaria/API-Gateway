#include "crypto/aes.hpp"

#include <iostream>
#include <vector>
#include <chrono>
#include <numeric>
#include <iomanip>
#include <openssl/aes.h>

int main() {
    const size_t block_size = 16;
    const size_t num_blocks = 20000000;
    const size_t total_size_bytes = block_size * num_blocks;

    std::vector<std::uint8_t> input(block_size, 0xAA);
    std::vector<std::uint8_t> output(block_size);
    std::vector<std::uint8_t> key(16, 0xBB);
    volatile std::uint8_t accumulator = 0;

    std::vector<std::uint8_t> round_keys(176);
    AES_set_encrypt_key(key.data(), 128, reinterpret_cast<AES_KEY*>(round_keys.data()));
    
    std::cout << "Starting Assembly AES-NI benchmark...\n";
    std::cout << "Processing " << num_blocks << " blocks (" << (total_size_bytes / (1024 * 1024)) << " MB).\n";

    const auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < num_blocks; ++i) {
        aes_encrypt_block_asm(input.data(), output.data(), round_keys.data());
        accumulator += output[0];
    }

    const auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> duration = end - start;
    const double throughput_gb_s = (static_cast<double>(total_size_bytes) / (1024 * 1024 * 1024)) / duration.count();

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Finished in " << duration.count() << " seconds.\n";
    std::cout << "Throughput: " << throughput_gb_s << " GB/s.\n";

    return 0;
}