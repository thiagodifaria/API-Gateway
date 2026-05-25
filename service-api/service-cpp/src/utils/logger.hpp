#ifndef HTTPS_SERVER_LOGGER_HPP
#define HTTPS_SERVER_LOGGER_HPP

#include <string>
#include <mutex>
#include <fstream>

namespace https_server {

enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3
};

class Logger {
public:
    static Logger& instance();
    
    void set_level(LogLevel level);
    void log(LogLevel level, const std::string& message);
    void log_with_binary_data(LogLevel level, const std::string& message, 
                              const void* data, size_t size);
    std::string generate_request_id();
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger() = default;
    
    std::mutex mutex_;
    LogLevel level_ = LogLevel::Info;
    std::ofstream file_;
};

#define LOG_DEBUG(msg) https_server::Logger::instance().log(https_server::LogLevel::Debug, msg)
#define LOG_INFO(msg) https_server::Logger::instance().log(https_server::LogLevel::Info, msg)
#define LOG_WARNING(msg) https_server::Logger::instance().log(https_server::LogLevel::Warning, msg)
#define LOG_ERROR(msg) https_server::Logger::instance().log(https_server::LogLevel::Error, msg)

#define LOG_BINARY(level, msg, data, size) \
    https_server::Logger::instance().log_with_binary_data(level, msg, data, size)

} // namespace https_server

#endif // HTTPS_SERVER_LOGGER_HPP