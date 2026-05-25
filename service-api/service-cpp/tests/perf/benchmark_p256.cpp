#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <random>

extern "C" {
    void p256_mul_mont(std::uint64_t res[4], const std::uint64_t a[4], const std::uint64_t b[4]);
    void p256_sqr_mont(std::uint64_t res[4], const std::uint64_t a[4]);
    void p256_add_mod(std::uint64_t res[4], const std::uint64_t a[4], const std::uint64_t b[4]);
    void p256_sub_mod(std::uint64_t res[4], const std::uint64_t a[4], const std::uint64_t b[4]);
    void p256_point_add(std::uint64_t res[12], const std::uint64_t p1[12], const std::uint64_t p2[12]);
    void p256_point_double(std::uint64_t res[12], const std::uint64_t point[12]);
}

namespace {

void generate_random_field_element(std::uint64_t elem[4], std::mt19937_64& rng) {
    for (int i = 0; i < 4; ++i) {
        elem[i] = rng();
    }
    elem[3] &= 0x7FFFFFFFFFFFFFFF;
}

void generate_random_point(std::uint64_t point[12], std::mt19937_64& rng) {
    for (int i = 0; i < 12; ++i) {
        point[i] = rng();
    }
    point[8] = 1;
    point[9] = 0;
    point[10] = 0;
    point[11] = 0;
}

template<typename Func>
double benchmark_operation(const std::string& name, Func operation, int iterations) {
    const auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        operation();
    }
    
    const auto end = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration<double, std::milli>(end - start);
    
    const double ops_per_second = static_cast<double>(iterations) / (duration.count() / 1000.0);
    
    std::cout << std::left << std::setw(25) << name << ": "
              << std::right << std::setw(12) << std::fixed << std::setprecision(2)
              << ops_per_second << " ops/sec ("
              << std::setprecision(6) << (duration.count() / iterations) << " ms/op)"
              << std::endl;
    
    return ops_per_second;
}

} // anonymous namespace

int main() {
    std::cout << "P-256 Assembly Performance Benchmark" << std::endl;
    
    std::mt19937_64 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    
    const int field_iterations = 1000000;
    const int point_iterations = 100000;
    
    std::vector<std::uint64_t> a(4), b(4), result(4);
    std::vector<std::uint64_t> point1(12), point2(12), point_result(12);
    
    generate_random_field_element(a.data(), rng);
    generate_random_field_element(b.data(), rng);
    generate_random_point(point1.data(), rng);
    generate_random_point(point2.data(), rng);
    
    std::cout << "\nField Arithmetic Benchmarks:" << std::endl;
    
    benchmark_operation("Addition", [&]() {
        p256_add_mod(result.data(), a.data(), b.data());
    }, field_iterations);
    
    benchmark_operation("Subtraction", [&]() {
        p256_sub_mod(result.data(), a.data(), b.data());
    }, field_iterations);
    
    benchmark_operation("Multiplication", [&]() {
        p256_mul_mont(result.data(), a.data(), b.data());
    }, field_iterations);
    
    benchmark_operation("Squaring", [&]() {
        p256_sqr_mont(result.data(), a.data());
    }, field_iterations);
    
    std::cout << "\nPoint Arithmetic Benchmarks:" << std::endl;
    
    benchmark_operation("Point Addition", [&]() {
        p256_point_add(point_result.data(), point1.data(), point2.data());
    }, point_iterations);
    
    benchmark_operation("Point Doubling", [&]() {
        p256_point_double(point_result.data(), point1.data());
    }, point_iterations);
    
    std::cout << "\nEstimated ECDH Performance:" << std::endl;
    
    const double avg_point_ops_per_sec = benchmark_operation("Point Doubling", [&]() {
        p256_point_double(point_result.data(), point1.data());
    }, point_iterations / 10);
    
    const double estimated_scalar_mults_per_sec = avg_point_ops_per_sec / 256.0;
    
    std::cout << "Estimated scalar mult    : "
              << std::setprecision(2) << estimated_scalar_mults_per_sec << " ops/sec" << std::endl;
    
    std::cout << "Estimated ECDH handshakes: "
              << std::setprecision(2) << (estimated_scalar_mults_per_sec / 2.0) << " /sec" << std::endl;
    
    return 0;
}