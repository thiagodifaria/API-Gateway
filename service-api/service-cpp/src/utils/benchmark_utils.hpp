#ifndef HTTPS_SERVER_BENCHMARK_UTILS_HPP
#define HTTPS_SERVER_BENCHMARK_UTILS_HPP

#include <string>
#include <cstddef>

namespace https_server {
namespace benchmark {

struct BenchmarkResult {
    double duration_seconds;
    double throughput_gb_s;
    size_t total_bytes;
    size_t operations;
};

struct P256Result {
    double field_ops_per_sec;
    double point_ops_per_sec;
    double estimated_ecdh_per_sec;
};

BenchmarkResult run_aes_benchmark();
BenchmarkResult run_sha256_benchmark();
P256Result run_p256_benchmark();

}
}

#endif