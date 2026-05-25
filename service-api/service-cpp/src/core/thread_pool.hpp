#ifndef HTTPS_SERVER_THREAD_POOL_HPP
#define HTTPS_SERVER_THREAD_POOL_HPP

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <chrono>
#include <stdexcept>

namespace https_server {

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads) : stop_(false) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex_);
                        this->condition_.wait(lock, [this] { return this->stop_ || !this->tasks_.empty(); });
                        if (this->stop_ && this->tasks_.empty()) {
                            return;
                        }
                        task = std::move(this->tasks_.front());
                        this->tasks_.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        shutdown();
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void enqueue(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (stop_) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            tasks_.emplace(std::move(task));
        }
        condition_.notify_one();
    }

    void shutdown(const std::chrono::milliseconds& timeout = std::chrono::milliseconds(5000)) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }
        condition_.notify_all();
        
        const auto start_time = std::chrono::steady_clock::now();
        for (std::thread &worker : workers_) {
            if (worker.joinable()) {
                const auto elapsed = std::chrono::steady_clock::now() - start_time;
                if (elapsed < timeout) {
                    worker.join();
                } else {
                    worker.detach();
                }
            }
        }
        workers_.clear();
    }

    size_t pending_tasks() const {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        return tasks_.size();
    }

    bool is_stopped() const {
        return stop_;
    }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;
};

} // namespace https_server

#endif // HTTPS_SERVER_THREAD_POOL_HPP
