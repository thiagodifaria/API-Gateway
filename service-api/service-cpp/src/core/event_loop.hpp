#ifndef HTTPS_SERVER_EVENT_LOOP_HPP
#define HTTPS_SERVER_EVENT_LOOP_HPP

#include <functional>
#include <map>
#include <vector>
#include <memory>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
using SOCKET = ::SOCKET;
#else
#include <sys/epoll.h>
using SOCKET = int;
#endif

namespace https_server {

class EventLoop {
public:
    using EventCallback = std::function<void(SOCKET)>;

    EventLoop();
    ~EventLoop();

    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;

    void add_socket(SOCKET socket, EventCallback callback);
    void remove_socket(SOCKET socket);
    void run_once(int timeout_ms = -1);

private:
    void cleanup();

#ifdef _WIN32
    // Simplified Windows implementation
    std::map<SOCKET, EventCallback> callbacks_;
#else
    int epoll_fd_;
    std::map<SOCKET, EventCallback> callbacks_;
    static constexpr int MAX_EVENTS = 64;
#endif
};

} // namespace https_server

#endif // HTTPS_SERVER_EVENT_LOOP_HPP