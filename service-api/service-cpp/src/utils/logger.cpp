#include "utils/logger.hpp"
#include "utils/network_operations.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstdlib>

namespace https_server {

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

void Logger::set_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

void Logger::set_json_output(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    json_output_ = enabled;
}

namespace {

std::string json_escape(const std::string& input) {
    std::ostringstream out;
    for (const unsigned char ch : input) {
        switch (ch) {
            case '\\': out << "\\\\"; break;
            case '"': out << "\\\""; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (ch < 0x20) {
                    out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(ch);
                } else {
                    out << static_cast<char>(ch);
                }
        }
    }
    return out.str();
}

} // namespace

void Logger::log(LogLevel level, const std::string& message) {
    if (level < level_) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    const auto now = std::chrono::system_clock::now();
    const auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
#ifdef _WIN32
    struct tm timeinfo;
    localtime_s(&timeinfo, &time_t);
    ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
#else
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
#endif
    
    const char* level_str[] = {"Debug", "Info", "Warning", "Error"};
    
    std::string log_id = network_ops::NetworkOps::instance().generate_uuid_string();

    if (json_output_) {
        std::cout << "{"
                  << "\"timestamp\":\"" << ss.str() << "\","
                  << "\"level\":\"" << level_str[static_cast<int>(level)] << "\","
                  << "\"log_id\":\"" << log_id << "\","
                  << "\"message\":\"" << json_escape(message) << "\""
                  << "}" << std::endl;
    } else {
        std::cout << "[" << ss.str() << "] [" << level_str[static_cast<int>(level)]
                  << "] [" << log_id.substr(0, 8) << "] " << message << std::endl;
    }
}

void Logger::log_with_binary_data(LogLevel level, const std::string& message, 
                                  const void* data, size_t size) {
    if (level < level_) return;
    
    std::string hex_data;
    if (data && size > 0) {
        hex_data = " [HEX: ";
        if (size > 64) {
            std::string partial_data = network_ops::NetworkOps::instance().encode_hex(
                std::string(static_cast<const char*>(data), 32));
            hex_data += partial_data + "..." + std::to_string(size) + " bytes total]";
        } else {
            hex_data += network_ops::NetworkOps::instance().encode_hex(
                std::string(static_cast<const char*>(data), size)) + "]";
        }
    }
    
    log(level, message + hex_data);
}

std::string Logger::generate_request_id() {
    return network_ops::NetworkOps::instance().generate_uuid_string();
}

}
