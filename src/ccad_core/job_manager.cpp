#include "job_manager.hpp"

#include <iostream>

namespace ccad {

JobManager::JobManager() : stop_(false) {
    size_t num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 4;
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex_);
                    this->condition_.wait(lock, [this] {
                        return this->stop_ || !this->tasks_.empty();
                    });
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

JobManager::~JobManager() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }
    condition_.notify_all();
    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void JobManager::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (stop_) {
            throw std::runtime_error("enqueue on stopped JobManager");
        }
        tasks_.push(std::move(task));
    }
    condition_.notify_one();
}

void JobManager::startJob(const std::string& name) {
    enqueue([name]() {
        // Placeholder for actual job logic
        std::cout << "[JobManager] Running job: " << name << std::endl;
    });
}

void JobManager::waitAll() {
    // Basic spin wait for tasks to clear (naive approach for stub)
    while (true) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (tasks_.empty()) {
                break;
            }
        }
        std::this_thread::yield();
    }
}

} // namespace ccad
