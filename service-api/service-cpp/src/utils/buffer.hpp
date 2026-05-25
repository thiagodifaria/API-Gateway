#ifndef HTTPS_SERVER_BUFFER_HPP
#define HTTPS_SERVER_BUFFER_HPP

#ifdef _WIN32
#define NOMINMAX
#endif

#include <vector>
#include <cstring>
#include <string_view>
#include <algorithm>

namespace https_server {

class Buffer {
public:
    explicit Buffer(size_t initial_capacity = 8192) 
        : data_(initial_capacity), read_pos_(0), write_pos_(0) {}

    void clear() noexcept {
        read_pos_ = 0;
        write_pos_ = 0;
    }

    void append(const void* data, size_t size) {
        ensure_capacity(size);
        std::memcpy(data_.data() + write_pos_, data, size);
        write_pos_ += size;
    }

    void append(const std::string& str) {
        append(str.data(), str.size());
    }

    char* write_ptr() noexcept {
        return data_.data() + write_pos_;
    }

    size_t writable_bytes() const noexcept {
        return data_.size() - write_pos_;
    }

    void has_written(size_t size) noexcept {
        write_pos_ += size;
    }

    std::string_view readable_view() const noexcept {
        return std::string_view(data_.data() + read_pos_, readable_bytes());
    }

    size_t readable_bytes() const noexcept {
        return write_pos_ - read_pos_;
    }

    void consume(size_t size) noexcept {
        read_pos_ = (std::min)(read_pos_ + size, write_pos_);
        
        if (read_pos_ > data_.size() / 2) {
            const size_t remaining = readable_bytes();
            if (remaining > 0) {
                std::memmove(data_.data(), data_.data() + read_pos_, remaining);
            }
            read_pos_ = 0;
            write_pos_ = remaining;
        }
    }

    size_t find_crlf_crlf() const noexcept {
        const auto view = readable_view();
        const auto pos = view.find("\r\n\r\n");
        return pos != std::string_view::npos ? pos : SIZE_MAX;
    }

    void ensure_capacity(size_t additional_size) {
        if (write_pos_ + additional_size > data_.size()) {
            const size_t new_size = (std::max)(data_.size() * 2, write_pos_ + additional_size);
            data_.resize(new_size);
        }
    }

private:
    std::vector<char> data_;
    size_t read_pos_;
    size_t write_pos_;
};

} // namespace https_server

#endif // HTTPS_SERVER_BUFFER_HPP