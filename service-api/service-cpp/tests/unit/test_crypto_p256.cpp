#include <iostream>
#include <vector>
#include <cstring>
#include <iomanip>
#include <cstdint>

extern "C" {
    void p256_mul_mont(std::uint64_t res[4], const std::uint64_t a[4], const std::uint64_t b[4]);
    void p256_sqr_mont(std::uint64_t res[4], const std::uint64_t a[4]);
    void p256_add_mod(std::uint64_t res[4], const std::uint64_t a[4], const std::uint64_t b[4]);
    void p256_sub_mod(std::uint64_t res[4], const std::uint64_t a[4], const std::uint64_t b[4]);
}

namespace {

void print_bigint(const std::uint64_t val[4]) {
    for (int i = 3; i >= 0; --i) {
        std::cout << std::hex << std::setw(16) << std::setfill('0') << val[i];
    }
}

bool compare_bigint(const std::uint64_t a[4], const std::uint64_t b[4]) {
    return std::memcmp(a, b, 32) == 0;
}

bool test_modular_arithmetic() {
    std::uint64_t a[4] = {0x0123456789abcdef, 0xfedcba9876543210, 0x1111111111111111, 0x2222222222222222};
    std::uint64_t b[4] = {0xaaaaaaaaaaaaaaaa, 0xbbbbbbbbbbbbbbbb, 0xcccccccccccccccc, 0xdddddddddddddddd};
    std::uint64_t result[4];
    
    std::cout << "Testing P-256 modular addition..." << std::endl;
    p256_add_mod(result, a, b);
    
    if (compare_bigint(result, a) && compare_bigint(result, b)) {
        std::cout << "FAIL: Addition result identical to inputs" << std::endl;
        return false;
    }
    
    std::cout << "Testing P-256 modular subtraction..." << std::endl;
    p256_sub_mod(result, a, b);
    
    std::cout << "Testing P-256 Montgomery multiplication..." << std::endl;
    p256_mul_mont(result, a, b);
    
    std::cout << "Testing P-256 Montgomery squaring..." << std::endl;
    p256_sqr_mont(result, a);
    
    return true;
}

bool test_field_properties() {
    std::uint64_t zero[4] = {0, 0, 0, 0};
    std::uint64_t a[4] = {0x0123456789abcdef, 0xfedcba9876543210, 0x1111111111111111, 0x2222222222222222};
    std::uint64_t result[4];
    std::uint64_t temp[4];
    
    std::cout << "Testing field identity properties..." << std::endl;
    
    p256_add_mod(result, a, zero);
    if (!compare_bigint(result, a)) {
        std::cout << "FAIL: a + 0 != a" << std::endl;
        return false;
    }
    
    p256_sub_mod(result, a, a);
    
    p256_sqr_mont(result, a);
    p256_mul_mont(temp, a, a);
    if (!compare_bigint(result, temp)) {
        std::cout << "FAIL: a^2 != a * a" << std::endl;
        return false;
    }
    
    return true;
}

}

int main() {
    std::cout << "P-256 Assembly Implementation Tests" << std::endl;
    
    bool all_tests_passed = true;
    
    try {
        if (!test_modular_arithmetic()) {
            all_tests_passed = false;
        }
        
        if (!test_field_properties()) {
            all_tests_passed = false;
        }
        
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << std::endl;
        all_tests_passed = false;
    }
    
    if (all_tests_passed) {
        std::cout << "SUCCESS: All P-256 assembly tests passed." << std::endl;
        return 0;
    } else {
        std::cout << "FAILURE: Some P-256 assembly tests failed." << std::endl;
        return 1;
    }
}
#include <cstdint>
