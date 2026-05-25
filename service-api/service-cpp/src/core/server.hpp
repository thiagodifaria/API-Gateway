#ifndef HTTPS_SERVER_SERVER_HPP
#define HTTPS_SERVER_SERVER_HPP

#include "core/thread_pool.hpp"
#include "core/config.hpp"
#include "core/event_loop.hpp"
#include "http/router.hpp"
#include <cstdint>
#include <atomic>
#include <mutex>
#include <memory>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#else
using SOCKET = int;
#endif

struct ssl_ctx_st;
using SSL_CTX = struct ssl_ctx_st;

struct ossl_provider_st;
using OSSL_PROVIDER = struct ossl_provider_st;

namespace https_server {

class Server {
public:
    explicit Server(const ServerConfig& config);
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&) = delete;
    Server& operator=(Server&&) = delete;

    void run();
    void shutdown();
    Router& get_router() { return router_; }
    
    void handle_shutdown_signal();
    void handle_reload_signal();

private:
    void init_openssl();
    void cleanup_openssl();
    void setup_providers();
    void load_openssl_config();
    void create_ssl_context();
    void setup_socket();
    void handle_connection(SOCKET client_socket, const std::string& client_ip = "unknown");
    
    void setup_signal_handlers();
    void reload_ssl_context();
    
    void handle_new_connection(SOCKET server_socket);
    void handle_client_data(SOCKET client_socket, const std::string& client_ip = "unknown");

    const ServerConfig config_;
    SOCKET server_socket_;
    ThreadPool pool_;
    SSL_CTX* ssl_ctx_;
    Router router_;
    
    OSSL_PROVIDER* default_provider_;
    OSSL_PROVIDER* custom_provider_;
    
    std::atomic<bool> running_;
    std::mutex ssl_context_mutex_;
    
    std::unique_ptr<EventLoop> event_loop_;

#ifdef _WIN32
    WSADATA wsa_data_;
#endif
};

} // namespace https_server

#endif // HTTPS_SERVER_SERVER_HPP
