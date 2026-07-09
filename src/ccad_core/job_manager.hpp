#ifndef CCAD_CORE_JOB_MANAGER_HPP
#define CCAD_CORE_JOB_MANAGER_HPP

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>

namespace ccad {

// Core background job manager for multi-threaded tasks (DRC, rendering, routing).
class JobManager {
public:
    JobManager();
    ~JobManager();

    void startJob(const std::string& name);
    void waitAll();

    // Submit a task to the background pool
    void enqueue(std::function<void()> task);

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    bool stop_;
};

} // namespace ccad

#endif // CCAD_CORE_JOB_MANAGER_HPP
