#include "utils/benchmark_utils.hpp"
#include <iostream>
#include <vector>
#include <chrono>
#include <numeric>
#include <iomanip>
#include <random>

extern "C" {
    void aes_encrypt_block_asm(const std::uint8_t* input, std::uint8_t* output, const std::uint8_t* round_keys);
    void sha256_block_asm(const std::uint8_t* input, std::uint32_t* hash);
    void p256_mul_mont(std::uint64_t res[4], const std::uint64_t a[4], const std::uint64_t b[4]);
    void p256_sqr_mont(std::uint64_t res[4], const std::uint64_t a[4]);
    void p256_point_add(std::uint64_t res[12], const std::uint64_t p1[12], const std::uint64_t p2[12]);
    void p256_point_double(std::uint64_t res[12], const std::uint64_t point[12]);
}

#ifdef _WIN32
#include <openssl/aes.h>
#endif

namespace https_server {
namespace benchmark {

BenchmarkResult run_aes_benchmark() {
    const size_t block_size = 16;
    const size_t num_blocks = 1000000;
    const size_t total_size_bytes = block_size * num_blocks;

    std::vector<std::uint8_t> input(block_size, 0xAA);
    std::vector<std::uint8_t> output(block_size);
    std::vector<std::uint8_t> key(16, 0xBB);
    volatile std::uint8_t accumulator = 0;

    std::vector<std::uint8_t> round_keys(176);

#ifdef _WIN32
    AES_set_encrypt_key(key.data(), 128, reinterpret_cast<AES_KEY*>(round_keys.data()));
#else
    for (size_t i = 0; i < 176; ++i) {
        round_keys[i] = static_cast<std::uint8_t>(i ^ 0xBB);
    }
#endif

    const auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < num_blocks; ++i) {
        aes_encrypt_block_asm(input.data(), output.data(), round_keys.data());
        accumulator += output[0];
    }

    const auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> duration = end - start;
    const double throughput_gb_s = (static_cast<double>(total_size_bytes) / (1024 * 1024 * 1024)) / duration.count();

    return {duration.count(), throughput_gb_s, total_size_bytes, num_blocks};
}

BenchmarkResult run_sha256_benchmark() {
    const size_t block_size = 64;
    const size_t num_blocks = 250000;
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

    const auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < num_blocks; ++i) {
        sha256_block_asm(input.data(), hash.data());
        accumulator += hash[0];
    }

    const auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> duration = end - start;
    const double throughput_gb_s = (static_cast<double>(total_size_bytes) / (1024 * 1024 * 1024)) / duration.count();

    return {duration.count(), throughput_gb_s, total_size_bytes, num_blocks};
}

P256Result run_p256_benchmark() {
    std::mt19937_64 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    
    const int field_iterations = 50000;
    const int point_iterations = 5000;
    
    std::vector<std::uint64_t> a(4), b(4), result(4);
    std::vector<std::uint64_t> point1(12), point2(12), point_result(12);
    
    for (int i = 0; i < 4; ++i) {
        a[i] = rng();
        b[i] = rng();
    }
    a[3] &= 0x7FFFFFFFFFFFFFFF;
    b[3] &= 0x7FFFFFFFFFFFFFFF;

    for (int i = 0; i < 12; ++i) {
        point1[i] = rng();
        point2[i] = rng();
    }
    point1[8] = 1;
    point1[9] = 0;
    point1[10] = 0;
    point1[11] = 0;
    point2[8] = 1;
    point2[9] = 0;
    point2[10] = 0;
    point2[11] = 0;

    const auto field_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < field_iterations; ++i) {
        p256_mul_mont(result.data(), a.data(), b.data());
    }
    const auto field_end = std::chrono::high_resolution_clock::now();
    
    const auto point_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < point_iterations; ++i) {
        p256_point_double(point_result.data(), point1.data());
    }
    const auto point_end = std::chrono::high_resolution_clock::now();

    const double field_duration = std::chrono::duration<double>(field_end - field_start).count();
    const double point_duration = std::chrono::duration<double>(point_end - point_start).count();
    
    const double field_ops_per_sec = static_cast<double>(field_iterations) / field_duration;
    const double point_ops_per_sec = static_cast<double>(point_iterations) / point_duration;
    const double estimated_ecdh_per_sec = point_ops_per_sec / 256.0;

    return {field_ops_per_sec, point_ops_per_sec, estimated_ecdh_per_sec};
}

}
}