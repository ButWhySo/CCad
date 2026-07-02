#pragma once
// Sprint 250: Durable Agent Runner
// Provides background execution loop and queue persistence for agent goals/tasks.

#include "agent_orchestrator.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <functional>
#include <string>

namespace ccad {

class AgentRunner {
public:
    AgentRunner();
    ~AgentRunner();

    // Start background thread
    void start();
    
    // Stop background thread cleanly
    void stop();

    // Add a goal to the background execution queue
    void enqueue_goal(const AgentGoal& goal);

    // Callbacks for UI updates
    using ProgressCallback = std::function<void(const AgentGoal&)>;
    void set_progress_callback(ProgressCallback cb);

    // Save and Load Queue to/from disk (.ccad-agent-queue.json)
    void save_queue(const std::string& filepath) const;
    void load_queue(const std::string& filepath);

private:
    void execution_loop();
    
    std::thread worker_thread_;
    mutable std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::atomic<bool> running_{false};
    
    std::queue<AgentGoal> pending_goals_;
    ProgressCallback on_progress_;
};

} // namespace ccad
