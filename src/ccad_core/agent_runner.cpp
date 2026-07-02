#include "agent_runner.hpp"
#include <iostream>
#include <fstream>
// Include json serialization helper (assumed similar to other core files)

namespace ccad {

AgentRunner::AgentRunner() {}

AgentRunner::~AgentRunner() {
    stop();
}

void AgentRunner::start() {
    if (!running_) {
        running_ = true;
        worker_thread_ = std::thread(&AgentRunner::execution_loop, this);
    }
}

void AgentRunner::stop() {
    if (running_) {
        running_ = false;
        cv_.notify_all();
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
    }
}

void AgentRunner::enqueue_goal(const AgentGoal& goal) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        pending_goals_.push(goal);
    }
    cv_.notify_one();
}

void AgentRunner::set_progress_callback(ProgressCallback cb) {
    on_progress_ = cb;
}

void AgentRunner::set_task_executor(TaskExecutor ex) {
    task_executor_ = ex;
}

void AgentRunner::save_queue(const std::string& filepath) const {
    std::ofstream out(filepath);
    if (!out) return;
    out << "{\"pending_goals\":[";
    
    // Create a copy of the queue to iterate over without popping from the original
    std::queue<AgentGoal> temp_queue;
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        temp_queue = pending_goals_;
    }
    
    bool first = true;
    while (!temp_queue.empty()) {
        if (!first) out << ",";
        out << temp_queue.front().to_json();
        temp_queue.pop();
        first = false;
    }
    
    out << "]}";
    out.close();
}

void AgentRunner::load_queue(const std::string& filepath) {
    (void)filepath;
    // Stub for JSON loading
}

void AgentRunner::execution_loop() {
    while (running_) {
        AgentGoal current_goal;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            cv_.wait(lock, [this] { return !pending_goals_.empty() || !running_; });
            
            if (!running_) break;
            
            current_goal = pending_goals_.front();
            pending_goals_.pop();
        }
        
        // Execute tasks inside the goal
        for (auto& task : current_goal.tasks) {
            if (!running_) break;
            
            if (task_executor_) {
                task = task_executor_(current_goal, task.id);
                if (task.status == TaskStatus::Failed) {
                    current_goal.failed_count++;
                } else if (task.status == TaskStatus::Completed) {
                    current_goal.completed_count++;
                }
            } else {
                // Check for approvals (Sprint 250 feature)
                if (task.risk != TaskRisk::ReadOnly) {
                    // If an approval is required, the task should block. 
                    // For now, we simulate an approval gate.
                    task.status = TaskStatus::Blocked;
                    task.error_message = "approval_required";
                    if (on_progress_) on_progress_(current_goal);
                    
                    // Break or pause depending on whether we want to wait for user interaction
                    // For this MVP execution thread, we will skip it if it's blocked.
                    continue; 
                }

                int retries = 0;
                int max_retries = 2;
                bool success = false;
                
                while (retries <= max_retries && !success && running_) {
                    // Mock execution delay
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    
                    // In a real execution, we'd invoke the ToolBroker here.
                    success = true; // Assume success for mock
                    
                    if (success) {
                        task.status = TaskStatus::Completed;
                    } else {
                        task.status = TaskStatus::Failed;
                        task.error_message = "Execution failed, retry " + std::to_string(retries);
                        retries++;
                    }
                }

                if (!success) {
                    // Write rollback record here
                    task.error_message += " | Rollback required.";
                    current_goal.failed_count++;
                } else {
                    current_goal.completed_count++;
                }
            }
            
            if (on_progress_) {
                on_progress_(current_goal);
            }
        }
        if (current_goal.failed_count > 0 || current_goal.tasks.empty()) {
            current_goal.status = GoalStatus::Failed;
        } else {
            current_goal.status = GoalStatus::Completed;
        }
        if (on_progress_) {
            on_progress_(current_goal);
        }
    }
}

} // namespace ccad
