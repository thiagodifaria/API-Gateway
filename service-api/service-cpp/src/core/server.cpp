#ifdef _WIN32
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/server.hpp"
#include "utils/logger.hpp"
#include "utils/buffer.hpp"
#include "utils/fast_memory.hpp"
#include "utils/http_accelerated.hpp"
#include "utils/compression_suite.hpp"
#include "utils/network_operations.hpp"
#include "http/http.hpp"
#include "http/parser.hpp"
#include <iostream>
#include <stdexcept>
#include <system_error>
#include <thread>
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <vector>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/provider.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

#ifdef _WIN32
#include <ws2tcpip.h>
#include <signal.h>
#pragma comment(lib, "ws2_32.lib")
static BOOL WINAPI console_ctrl_handler(DWORD ctrl_type);
static https_server::Server* g_server_instance = nullptr;
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
static void signal_handler(int signum);
static https_server::Server* g_server_instance = nullptr;
#endif

namespace https_server {

void log_openssl_errors();
#ifdef _WIN32
void close_socket(SOCKET s);
#else
void close_socket(SOCKET s);
#endif
std::size_t expected_http_request_size(const Buffer& buffer, std::size_t header_end_pos);
void set_socket_timeouts(SOCKET socket, const ServerConfig& config);
bool is_websocket_upgrade(const http::HttpRequest& request);
bool handle_websocket_echo(SSL* ssl, const http::HttpRequest& request, const ServerConfig& config);

extern "C" int OSSL_provider_init(const OSSL_CORE_HANDLE *handle, 
                                  const OSSL_DISPATCH *in, 
                                  const OSSL_DISPATCH **out, 
                                  void **provctx);

Server::Server(const ServerConfig& config) 
    : config_(config),
      server_socket_(static_cast<SOCKET>(-1)), 
      pool_(config.threads == 0 ? std::thread::hardware_concurrency() : config.threads),
      ssl_ctx_(nullptr),
      default_provider_(nullptr),
      custom_provider_(nullptr),
      running_(true)
{
#ifdef _WIN32
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data_) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
#endif
    
    LOG_INFO("Starting API Gateway on port " + std::to_string(config_.port));
    
    if (fast_memory::MemoryOps::instance().has_avx2()) {
        LOG_INFO("AVX2 memory optimizations enabled");
    } else {
        LOG_INFO("Using standard memory operations (AVX2 not available)");
    }
    
    if (http_accelerated::HttpOps::instance().has_avx2()) {
        LOG_INFO("HTTP parsing optimizations enabled");
    } else {
        LOG_INFO("Using standard HTTP parsing");
    }
    
    if (compression::CompressionOps::instance().has_avx2()) {
        LOG_INFO("Compression optimizations enabled (Deflate/LZ4/Brotli)");
    } else {
        LOG_INFO("Using standard compression algorithms");
    }
    
    auto& net_ops = network_ops::NetworkOps::instance();
    if (net_ops.has_avx2() && net_ops.has_rdrand()) {
        LOG_INFO("Network operations optimized (Base64/UUID/Hex with RDRAND+AVX2)");
    } else if (net_ops.has_avx2()) {
        LOG_INFO("Network operations optimized (Base64/Hex with AVX2, UUID fallback)");
    } else {
        LOG_INFO("Using standard network operations");
    }
    
#ifdef HAS_CRYPTO_ADVANCED
    LOG_INFO("Advanced crypto algorithms available (ChaCha20, Blake3, X25519)");
#else
    LOG_INFO("Using standard crypto algorithms");
#endif
    
    init_openssl();
    setup_providers();
    load_openssl_config();
    create_ssl_context();
    setup_signal_handlers();
    
    event_loop_ = std::make_unique<EventLoop>();
}

Server::~Server() {
    shutdown();
    
    if (custom_provider_) {
        OSSL_PROVIDER_unload(custom_provider_);
    }
    
    if (default_provider_) {
        OSSL_PROVIDER_unload(default_provider_);
    }
    
    if (ssl_ctx_) {
        SSL_CTX_free(ssl_ctx_);
    }
    
    cleanup_openssl();

    if (server_socket_ != static_cast<SOCKET>(-1)) {
        close_socket(server_socket_);
    }
    
#ifdef _WIN32
    WSACleanup();
#endif
    
    LOG_INFO("Server stopped");
}

void Server::init_openssl() {
    SSL_load_error_strings();	
    OpenSSL_add_ssl_algorithms();
    LOG_DEBUG("OpenSSL initialized");
}

void Server::setup_providers() {
    default_provider_ = OSSL_PROVIDER_load(NULL, "default");
    if (!default_provider_) {
        log_openssl_errors();
        throw std::runtime_error("Failed to load default OpenSSL provider");
    }
    LOG_DEBUG("Default OpenSSL provider loaded");
    
    if (OSSL_PROVIDER_add_builtin(NULL, "aes_provider", OSSL_provider_init) == 1) {
        custom_provider_ = OSSL_PROVIDER_load(NULL, "aes_provider");
        if (custom_provider_) {
            LOG_INFO("Custom crypto provider loaded (AES/SHA-256/ChaCha20/Blake3/X25519)");
        }
    }
    
    if (!custom_provider_) {
#ifdef _WIN32
        custom_provider_ = OSSL_PROVIDER_load(NULL, ".\\aes_provider.dll");
#else
        custom_provider_ = OSSL_PROVIDER_load(NULL, "./libaes_provider.so");
#endif
        if (custom_provider_) {
            LOG_INFO("External crypto provider loaded");
        }
    }
}

void Server::load_openssl_config() {
    const char* root_dir = getenv("OPENSSL_ROOT_DIR");
    std::string config_path = root_dir ? 
        std::string(root_dir) + "\\bin\\cnf\\openssl.cnf" :
        "C:\\Program Files\\OpenSSL-Win64\\bin\\cnf\\openssl.cnf";
    
    if (FILE* file = fopen(config_path.c_str(), "r")) {
        fclose(file);
        if (OSSL_LIB_CTX_load_config(NULL, config_path.c_str()) == 1) {
            LOG_DEBUG("OpenSSL config loaded from: " + config_path);
        }
    }
}

void Server::cleanup_openssl() {
    EVP_cleanup();
}

void Server::create_ssl_context() {
    const SSL_METHOD *method = TLS_server_method();
    ssl_ctx_ = SSL_CTX_new(method);
    if (!ssl_ctx_) {
        log_openssl_errors();
        throw std::runtime_error("Unable to create SSL context");
    }

    if (SSL_CTX_use_certificate_file(ssl_ctx_, config_.cert_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
        log_openssl_errors();
        throw std::runtime_error("Failed to load certificate file: " + config_.cert_file);
    }

    if (SSL_CTX_use_PrivateKey_file(ssl_ctx_, config_.key_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
        log_openssl_errors();
        throw std::runtime_error("Failed to load private key file: " + config_.key_file);
    }
    
    if (!config_.client_ca_file.empty()) {
        LOG_INFO("Configuring mutual TLS authentication");
        
        if (SSL_CTX_load_verify_locations(ssl_ctx_, config_.client_ca_file.c_str(), nullptr) != 1) {
            log_openssl_errors();
            throw std::runtime_error("Failed to load client CA file: " + config_.client_ca_file);
        }
        
        SSL_CTX_set_verify(ssl_ctx_, 
                          SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, 
                          nullptr);
        
        LOG_INFO("Mutual TLS authentication enabled");
    }
    
    LOG_INFO("SSL context created with cert: " + config_.cert_file + ", key: " + config_.key_file);
}

void Server::setup_signal_handlers() {
    g_server_instance = this;
    
#ifdef _WIN32
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
#else
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGHUP, &sa, nullptr);
#endif
    
    LOG_DEBUG("Signal handlers configured");
}

void Server::handle_shutdown_signal() {
    LOG_INFO("Received shutdown signal, initiating graceful shutdown");
    running_ = false;
}

void Server::handle_reload_signal() {
    LOG_INFO("Received reload signal, reloading SSL certificates");
    try {
        reload_ssl_context();
        LOG_INFO("SSL certificates reloaded successfully");
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to reload SSL certificates: " + std::string(e.what()));
    }
}

void Server::reload_ssl_context() {
    std::lock_guard<std::mutex> lock(ssl_context_mutex_);
    
    SSL_CTX* new_ctx = SSL_CTX_new(TLS_server_method());
    if (!new_ctx) {
        throw std::runtime_error("Failed to create new SSL context");
    }
    
    if (SSL_CTX_use_certificate_file(new_ctx, config_.cert_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
        SSL_CTX_free(new_ctx);
        throw std::runtime_error("Failed to load new certificate file");
    }
    
    if (SSL_CTX_use_PrivateKey_file(new_ctx, config_.key_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
        SSL_CTX_free(new_ctx);
        throw std::runtime_error("Failed to load new private key file");
    }
    
    if (!config_.client_ca_file.empty()) {
        if (SSL_CTX_load_verify_locations(new_ctx, config_.client_ca_file.c_str(), nullptr) != 1) {
            SSL_CTX_free(new_ctx);
            throw std::runtime_error("Failed to load new client CA file");
        }
        SSL_CTX_set_verify(new_ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
    }
    
    SSL_CTX* old_ctx = ssl_ctx_;
    ssl_ctx_ = new_ctx;
    SSL_CTX_free(old_ctx);
}

void Server::setup_socket() {
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
    if (server_socket_ == INVALID_SOCKET) {
        const int error = WSAGetLastError();
        throw std::runtime_error("Failed to create socket. Error: " + std::to_string(error));
    }
#else
    if (server_socket_ < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to create socket");
    }
#endif

    const int opt = 1;
#ifdef _WIN32
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt)) == SOCKET_ERROR) {
        const int error = WSAGetLastError();
        close_socket(server_socket_);
        throw std::runtime_error("Failed to set socket options. Error: " + std::to_string(error));
    }
#else
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt)) < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to set socket options");
    }
#endif

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(config_.port);

#ifdef _WIN32
    if (bind(server_socket_, reinterpret_cast<const sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
        const int error = WSAGetLastError();
        close_socket(server_socket_);
        
        std::string error_msg = "Failed to bind socket. Error: " + std::to_string(error);
        if (error == WSAEADDRINUSE) {
            error_msg += " (Port " + std::to_string(config_.port) + " already in use)";
        }
        
        throw std::runtime_error(error_msg);
    }
#else
    if (bind(server_socket_, reinterpret_cast<const sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to bind socket");
    }
#endif

#ifdef _WIN32
    if (listen(server_socket_, 32) == SOCKET_ERROR) {
        const int error = WSAGetLastError();
        close_socket(server_socket_);
        throw std::runtime_error("Failed to listen on socket. Error: " + std::to_string(error));
    }
#else
    if (listen(server_socket_, 32) < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to listen on socket");
    }
#endif
}

void Server::handle_new_connection(SOCKET server_socket) {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    const SOCKET client_socket = accept(server_socket, reinterpret_cast<sockaddr*>(&client_addr), &client_len);

#ifdef _WIN32
    if (client_socket == INVALID_SOCKET) {
#else
    if (client_socket < 0) {
#endif
        return;
    }

    set_socket_timeouts(client_socket, config_);

    char client_ip[INET_ADDRSTRLEN] = "unknown";
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, sizeof(client_ip));
    const std::string client_ip_string = client_ip;

    event_loop_->add_socket(client_socket, [this, client_ip_string](SOCKET sock) {
        handle_client_data(sock, client_ip_string);
    });
}

void Server::handle_client_data(SOCKET client_socket, const std::string& client_ip) {
    pool_.enqueue([this, client_socket, client_ip] {
        handle_connection(client_socket, client_ip);
    });
}

void Server::handle_connection(SOCKET client_socket, const std::string& client_ip) {
    std::lock_guard<std::mutex> lock(ssl_context_mutex_);
    
    SSL* ssl = SSL_new(ssl_ctx_);
    if (!ssl) {
        close_socket(client_socket);
        return;
    }
    
    SSL_set_fd(ssl, static_cast<int>(client_socket));

    if (SSL_accept(ssl) <= 0) {
        LOG_WARNING("SSL handshake failed");
        log_openssl_errors();
    } else {
        Buffer buffer;
        
        while (true) {
            const int bytes_read = SSL_read(ssl, buffer.write_ptr(), 
                                          static_cast<int>(buffer.writable_bytes()));
            if (bytes_read <= 0) break;
            
            buffer.has_written(static_cast<size_t>(bytes_read));

            if (buffer.readable_bytes() > config_.http.max_header_bytes + config_.http.max_body_bytes) {
                break;
            }
            
            size_t header_end_pos;
            if (http_accelerated::HttpOps::instance().find_header_end(
                buffer.readable_view().data(), buffer.readable_view().size(), &header_end_pos)) {
                const std::size_t expected_size = expected_http_request_size(buffer, header_end_pos);
                if (buffer.readable_bytes() >= expected_size) {
                    break;
                }
            } else if (buffer.readable_bytes() > config_.http.max_header_bytes) {
                break;
            }
            
            buffer.ensure_capacity(4096);
        }

        if (buffer.readable_bytes() > 0) {
            http::ParserLimits limits;
            limits.max_header_bytes = config_.http.max_header_bytes;
            limits.max_body_bytes = config_.http.max_body_bytes;
            limits.max_uri_bytes = config_.http.max_uri_bytes;
            const auto parse_result = http::HttpParser(limits).parse(buffer);
            if (!parse_result.ok) {
                http::HttpResponse error_response;
                error_response.status_code = parse_result.status_code;
                error_response.status_text = parse_result.status_text;
                error_response.headers["Content-Type"] = "text/plain; charset=utf-8";
                error_response.body = parse_result.message.empty()
                    ? parse_result.status_text
                    : parse_result.message;
                const std::string error_str = error_response.to_string();
                SSL_write(ssl, error_str.c_str(), static_cast<int>(error_str.length()));
                SSL_shutdown(ssl);
                SSL_free(ssl);
                close_socket(client_socket);
                return;
            }

            http::HttpRequest request = parse_result.request;
            request.client_ip = client_ip;
            LOG_DEBUG("Request: " + request.method + " " + request.uri);

            if (config_.websocket.enabled && is_websocket_upgrade(request)) {
                handle_websocket_echo(ssl, request, config_);
            } else {
                const http::HttpResponse response = router_.route_request(request);
                const std::string response_str = response.to_string();
                SSL_write(ssl, response_str.c_str(), static_cast<int>(response_str.length()));
            }
        }
    }
    
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close_socket(client_socket);
}

void Server::run() {
    setup_socket();
    LOG_INFO("Server listening on port " + std::to_string(config_.port) + " with " + std::to_string(config_.threads == 0 ? std::thread::hardware_concurrency() : config_.threads) + " threads");

    event_loop_->add_socket(server_socket_, [this](SOCKET sock) {
        handle_new_connection(sock);
    });
    
    while (running_) {
        event_loop_->run_once(100);
    }
    
    LOG_INFO("Main event loop exited");
}

void Server::shutdown() {
    if (running_) {
        running_ = false;
        pool_.shutdown();
        LOG_INFO("Server shutdown completed");
    }
}

void log_openssl_errors() {
    unsigned long err_code;
    while ((err_code = ERR_get_error())) {
        char err_msg[256];
        ERR_error_string_n(err_code, err_msg, sizeof(err_msg));
        LOG_ERROR("OpenSSL: " + std::string(err_msg));
    }
}

#ifdef _WIN32
void close_socket(const SOCKET s) { closesocket(s); }
#else
void close_socket(const SOCKET s) { close(s); }
#endif

std::size_t expected_http_request_size(const Buffer& buffer, std::size_t header_end_pos) {
    const auto view = buffer.readable_view();
    std::string raw_request(view);
    std::istringstream request_stream(raw_request);
    std::string line;

    std::getline(request_stream, line);

    size_t content_length = 0;
    while (std::getline(request_stream, line) && !line.empty() && line != "\r") {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            const std::string value = line.substr(colon_pos + 1);
            std::transform(key.begin(), key.end(), key.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
            if (key == "content-length") {
                try {
                    content_length = static_cast<size_t>(std::stoull(value));
                } catch (const std::exception&) {
                    return header_end_pos;
                }
            }
        }
    }

    return header_end_pos + content_length;
}

void set_socket_timeouts(SOCKET socket, const ServerConfig& config) {
#ifdef _WIN32
    const DWORD read_timeout = static_cast<DWORD>(config.http.read_timeout_ms);
    const DWORD write_timeout = static_cast<DWORD>(config.http.write_timeout_ms);
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&read_timeout), sizeof(read_timeout));
    setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&write_timeout), sizeof(write_timeout));
#else
    timeval read_timeout{};
    read_timeout.tv_sec = static_cast<long>(config.http.read_timeout_ms / 1000);
    read_timeout.tv_usec = static_cast<long>((config.http.read_timeout_ms % 1000) * 1000);
    timeval write_timeout{};
    write_timeout.tv_sec = static_cast<long>(config.http.write_timeout_ms / 1000);
    write_timeout.tv_usec = static_cast<long>((config.http.write_timeout_ms % 1000) * 1000);
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout));
    setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &write_timeout, sizeof(write_timeout));
#endif
}

bool is_websocket_upgrade(const http::HttpRequest& request) {
    const auto upgrade = request.headers.find("upgrade");
    const auto connection = request.headers.find("connection");
    if (upgrade == request.headers.end() || connection == request.headers.end()) {
        return false;
    }
    std::string upgrade_value = upgrade->second;
    std::string connection_value = connection->second;
    std::transform(upgrade_value.begin(), upgrade_value.end(), upgrade_value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    std::transform(connection_value.begin(), connection_value.end(), connection_value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return request.method == "GET" &&
           upgrade_value == "websocket" &&
           connection_value.find("upgrade") != std::string::npos;
}

std::string websocket_accept_value(const std::string& key) {
    const std::string magic = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    unsigned char digest[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(magic.data()), magic.size(), digest);
    std::string encoded(32, '\0');
    const int size = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(&encoded[0]), digest, SHA_DIGEST_LENGTH);
    encoded.resize(static_cast<std::size_t>(size));
    return encoded;
}

bool read_exact_ssl(SSL* ssl, unsigned char* data, std::size_t size) {
    std::size_t offset = 0;
    while (offset < size) {
        const int read = SSL_read(ssl, data + offset, static_cast<int>(size - offset));
        if (read <= 0) {
            return false;
        }
        offset += static_cast<std::size_t>(read);
    }
    return true;
}

bool write_websocket_frame(SSL* ssl, unsigned char opcode, const std::vector<unsigned char>& payload) {
    std::vector<unsigned char> frame;
    frame.push_back(static_cast<unsigned char>(0x80 | opcode));
    if (payload.size() < 126) {
        frame.push_back(static_cast<unsigned char>(payload.size()));
    } else if (payload.size() <= 0xffff) {
        frame.push_back(126);
        frame.push_back(static_cast<unsigned char>((payload.size() >> 8) & 0xff));
        frame.push_back(static_cast<unsigned char>(payload.size() & 0xff));
    } else {
        frame.push_back(127);
        for (int shift = 56; shift >= 0; shift -= 8) {
            frame.push_back(static_cast<unsigned char>((payload.size() >> shift) & 0xff));
        }
    }
    frame.insert(frame.end(), payload.begin(), payload.end());
    return SSL_write(ssl, frame.data(), static_cast<int>(frame.size())) == static_cast<int>(frame.size());
}

bool handle_websocket_echo(SSL* ssl, const http::HttpRequest& request, const ServerConfig& config) {
    const auto key_it = request.headers.find("sec-websocket-key");
    if (key_it == request.headers.end()) {
        http::HttpResponse response;
        response.status_code = 400;
        response.status_text = "Bad Request";
        response.body = "Missing Sec-WebSocket-Key";
        const auto raw = response.to_string();
        SSL_write(ssl, raw.data(), static_cast<int>(raw.size()));
        return false;
    }

    std::ostringstream handshake;
    handshake << "HTTP/1.1 101 Switching Protocols\r\n"
              << "Upgrade: websocket\r\n"
              << "Connection: Upgrade\r\n"
              << "Sec-WebSocket-Accept: " << websocket_accept_value(key_it->second) << "\r\n\r\n";
    const auto raw_handshake = handshake.str();
    if (SSL_write(ssl, raw_handshake.data(), static_cast<int>(raw_handshake.size())) <= 0) {
        return false;
    }

    while (true) {
        unsigned char header[2];
        if (!read_exact_ssl(ssl, header, sizeof(header))) {
            return false;
        }
        const unsigned char opcode = header[0] & 0x0f;
        const bool masked = (header[1] & 0x80) != 0;
        std::uint64_t payload_len = header[1] & 0x7f;
        if (payload_len == 126) {
            unsigned char ext[2];
            if (!read_exact_ssl(ssl, ext, sizeof(ext))) return false;
            payload_len = (static_cast<std::uint64_t>(ext[0]) << 8) | ext[1];
        } else if (payload_len == 127) {
            unsigned char ext[8];
            if (!read_exact_ssl(ssl, ext, sizeof(ext))) return false;
            payload_len = 0;
            for (unsigned char byte : ext) {
                payload_len = (payload_len << 8) | byte;
            }
        }
        if (payload_len > config.websocket.max_frame_bytes) {
            const std::vector<unsigned char> close_payload = {0x03, 0xf1};
            write_websocket_frame(ssl, 0x8, close_payload);
            return false;
        }

        unsigned char mask[4] = {0, 0, 0, 0};
        if (masked && !read_exact_ssl(ssl, mask, sizeof(mask))) {
            return false;
        }
        std::vector<unsigned char> payload(static_cast<std::size_t>(payload_len));
        if (payload_len > 0 && !read_exact_ssl(ssl, payload.data(), payload.size())) {
            return false;
        }
        if (masked) {
            for (std::size_t i = 0; i < payload.size(); ++i) {
                payload[i] ^= mask[i % 4];
            }
        }

        if (opcode == 0x8) {
            write_websocket_frame(ssl, 0x8, payload);
            return true;
        }
        if (opcode == 0x9) {
            write_websocket_frame(ssl, 0xA, payload);
            continue;
        }
        if (opcode == 0x1 || opcode == 0x2) {
            if (!write_websocket_frame(ssl, opcode, payload)) {
                return false;
            }
        }
    }
}

} // namespace https_server

#ifdef _WIN32
BOOL WINAPI console_ctrl_handler(DWORD ctrl_type) {
    if (g_server_instance) {
        switch (ctrl_type) {
            case CTRL_C_EVENT:
            case CTRL_BREAK_EVENT:
            case CTRL_CLOSE_EVENT:
                g_server_instance->handle_shutdown_signal();
                return TRUE;
        }
    }
    return FALSE;
}
#else
void signal_handler(int signum) {
    if (g_server_instance) {
        switch (signum) {
            case SIGINT:
            case SIGTERM:
                g_server_instance->handle_shutdown_signal();
                break;
            case SIGHUP:
                g_server_instance->handle_reload_signal();
                break;
        }
    }
}
#endif
