#include "utils/buffer.hpp"
#include <iostream>
#include <cassert>

int main() {
    https_server::Buffer buffer;
    
    buffer.append("Hello", 5);
    assert(buffer.readable_bytes() == 5);
    assert(buffer.readable_view() == "Hello");
    
    buffer.consume(2);
    assert(buffer.readable_bytes() == 3);
    assert(buffer.readable_view() == "llo");
    
    buffer.clear();
    buffer.append("GET /\r\n\r\nBody");
    assert(buffer.find_crlf_crlf() == 5);
    
    std::cout << "SUCCESS: All buffer tests passed." << std::endl;
    return 0;
}