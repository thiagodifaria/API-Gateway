#include "crypto/sha256.hpp"
#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>

int main() {
    const size_t block_size = 64;
    const size_t num_blocks = 5000000;
    const size_t total_size_bytes = block_size * num_blocks;

    std::vector<std::uint8_t> input(block_size, 0xAA);
    std::vector<std::uint32_t> hash(8);
    volatile std::uint32_t accumulator = 0;

    hash[0] = 0x6a09e667;
    hash[1] = 0xbb67ae85;
    hash[2] = 0x3c6ef372;
    hash[3] = 0xa54ff53a;
    hash[4] = 0x510e527f;
    hash[5] = 0x9b05688c;
    hash[6] = 0x1f83d9ab;
    hash[7] = 0x5be0cd19;
    
    std::cout << "Starting Assembly SHA-256 benchmark...\n";
    std::cout << "Processing " << num_blocks << " blocks (" << (total_size_bytes / (1024 * 1024)) << " MB).\n";

    const auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < num_blocks; ++i) {
        sha256_block_asm(input.data(), hash.data());
        accumulator += hash[0];
    }

    const auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> duration = end - start;
    const double throughput_gb_s = (static_cast<double>(total_size_bytes) / (1024 * 1024 * 1024)) / duration.count();

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Finished in " << duration.count() << " seconds.\n";
    std::cout << "Throughput: " << throughput_gb_s << " GB/s.\n";

    return 0;
}