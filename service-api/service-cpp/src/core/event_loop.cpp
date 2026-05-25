#include "core/event_loop.hpp"
#include "utils/logger.hpp"
#include <stdexcept>
#include <cstring>

#ifdef _WIN32
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#endif

namespace https_server {

EventLoop::EventLoop() {
#ifdef _WIN32
    LOG_DEBUG("Windows event loop initialized (simplified)");
#else
    epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd_ == -1) {
        throw std::runtime_error("Failed to create epoll file descriptor");
    }
    LOG_DEBUG("Linux epoll event loop initialized");
#endif
}

EventLoop::~EventLoop() {
    cleanup();
}

void EventLoop::cleanup() {
#ifdef _WIN32
    callbacks_.clear();
#else
    if (epoll_fd_ != -1) {
        close(epoll_fd_);
        epoll_fd_ = -1;
    }
    callbacks_.clear();
#endif
}

void EventLoop::add_socket(SOCKET socket, EventCallback callback) {
#ifdef _WIN32
    // Simplified Windows implementation using select-style polling
    callbacks_[socket] = std::move(callback);
    LOG_DEBUG("Socket added to Windows event loop");
#else
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1) {
        throw std::runtime_error("Failed to get socket flags");
    }
    
    if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error("Failed to set socket non-blocking");
    }
    
    epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = socket;
    
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket, &event) == -1) {
        throw std::runtime_error("Failed to add socket to epoll");
    }
    
    callbacks_[socket] = std::move(callback);
    LOG_DEBUG("Socket added to epoll event loop");
#endif
}

void EventLoop::remove_socket(SOCKET socket) {
#ifdef _WIN32
    callbacks_.erase(socket);
    LOG_DEBUG("Socket removed from Windows event loop");
#else
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, socket, nullptr) == -1) {
        LOG_WARNING("Failed to remove socket from epoll");
    }
    
    callbacks_.erase(socket);
    LOG_DEBUG("Socket removed from epoll event loop");
#endif
}

void EventLoop::run_once(int timeout_ms) {
#ifdef _WIN32
    // Simplified Windows implementation - just call callbacks for now
    for (const auto& [socket, callback] : callbacks_) {
        if (callback) {
            callback(socket);
        }
    }
    Sleep(timeout_ms > 0 ? timeout_ms : 10);
#else
    epoll_event events[MAX_EVENTS];
    int event_count = epoll_wait(epoll_fd_, events, MAX_EVENTS, timeout_ms);
    
    if (event_count == -1) {
        if (errno != EINTR) {
            LOG_ERROR("epoll_wait failed: " + std::string(strerror(errno)));
        }
        return;
    }
    
    for (int i = 0; i < event_count; ++i) {
        SOCKET socket = events[i].data.fd;
        
        if (events[i].events & (EPOLLERR | EPOLLHUP)) {
            LOG_WARNING("Socket error/hangup detected");
            remove_socket(socket);
            continue;
        }
        
        if (events[i].events & EPOLLIN) {
            auto it = callbacks_.find(socket);
            if (it != callbacks_.end() && it->second) {
                it->second(socket);
            }
        }
    }
#endif
}

} // namespace https_server