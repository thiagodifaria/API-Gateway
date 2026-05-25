#include "utils/fast_memory.hpp"
#include <iostream>
#include <vector>
#include <cstring>
#include <random>

using namespace std;

int main() {
    cout << "Fast Memory Operations Test" << endl;
    
    const auto& mem_ops = https_server::fast_memory::MemoryOps::instance();
    
    cout << "AVX2 Support: " << (mem_ops.has_avx2() ? "YES" : "NO") << endl;
    
    constexpr size_t test_size = 1024;
    vector<char> src(test_size);
    vector<char> dst(test_size);
    vector<char> reference(test_size);
    
    mt19937 rng(42);
    uniform_int_distribution<int> dist(0, 255);
    for (size_t i = 0; i < test_size; ++i) {
        src[i] = static_cast<char>(dist(rng));
    }
    
    cout << "\nTesting memcpy..." << endl;
    memcpy(reference.data(), src.data(), test_size);
    https_server::fast_memory::memcpy(dst.data(), src.data(), test_size);
    
    if (memcmp(dst.data(), reference.data(), test_size) == 0) {
        cout << "PASS: memcpy produces correct results" << endl;
    } else {
        cout << "FAIL: memcpy produces incorrect results" << endl;
        return 1;
    }
    
    cout << "Testing memchr..." << endl;
    const char search_val = src[test_size / 2];
    
    void* std_result = memchr(src.data(), search_val, test_size);
    void* fast_result = https_server::fast_memory::memchr(src.data(), search_val, test_size);
    
    if (std_result == fast_result) {
        cout << "PASS: memchr produces correct results" << endl;
    } else {
        cout << "FAIL: memchr produces incorrect results" << endl;
        return 1;
    }
    
    cout << "Testing memmove..." << endl;
    vector<char> overlap_test(test_size);
    vector<char> overlap_ref(test_size);
    
    memcpy(overlap_test.data(), src.data(), test_size);
    memcpy(overlap_ref.data(), src.data(), test_size);
    
    memmove(overlap_ref.data() + 10, overlap_ref.data(), test_size - 10);
    https_server::fast_memory::memmove(overlap_test.data() + 10, overlap_test.data(), test_size - 10);
    
    if (memcmp(overlap_test.data(), overlap_ref.data(), test_size) == 0) {
        cout << "PASS: memmove produces correct results" << endl;
    } else {
        cout << "FAIL: memmove produces incorrect results" << endl;
        return 1;
    }
    
    cout << "\nSUCCESS: All fast memory tests passed." << endl;
    return 0;
}