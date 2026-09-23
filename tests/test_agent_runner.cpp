#include <cassert>
#include <iostream>
#include "ccad_core/agent_runner.hpp"
#include <chrono>
#include <future>
#include <stdexcept>
#include <thread>

using namespace ccad;

static void test_starts_and_stops() {
    std::cout << "  test_starts_and_stops... ";
    for (int attempt = 0; attempt < 64; ++attempt) {
        AgentRunner runner;
        runner.start();
        if ((attempt % 8) == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        runner.stop();
    }
    std::cout << "PASS\n";
}

static void test_processes_goal() {
    std::cout << "  test_processes_goal... ";
    AgentRunner runner;
    std::promise<AgentGoal> completed_goal;
    auto completed = completed_goal.get_future();
    
    runner.set_progress_callback([&](const AgentGoal& goal) {
        if (goal.status == GoalStatus::Completed) {
            try { completed_goal.set_value(goal); } catch (...) {}
        }
    });
    runner.set_task_executor([](AgentGoal&, const std::string& task_id) {
        AgentTask task;
        task.id = task_id;
        task.status = TaskStatus::Completed;
        task.result_json = "{\"status\":\"executed_by_test_executor\"}";
        return task;
    });

    runner.start();

    AgentGoal goal;
    goal.id = "test_goal_1";
    AgentTask task;
    task.id = "t1";
    task.risk = TaskRisk::ReadOnly;
    goal.tasks.push_back(task);
    goal.total_count = 1;

    runner.enqueue_goal(goal);

    // A production runner must never leave the caller waiting forever when a
    // worker, callback, or scheduler regression prevents terminal progress.
    // Keep the assertion tied to the real callback, but bound the test so the
    // full CTest gate reports a failure instead of hanging indefinitely.
    if (completed.wait_for(std::chrono::seconds(5)) != std::future_status::ready) {
        runner.stop();
        throw std::runtime_error("agent runner did not report completed goal within five seconds");
    }
    const AgentGoal completed_result = completed.get();
    runner.stop();
    assert(completed_result.completed_count == 1);
    assert(completed_result.tasks.front().result_json == "{\"status\":\"executed_by_test_executor\"}");
    std::cout << "PASS\n";
}

static void test_rejects_goal_without_executor() {
    std::cout << "  test_rejects_goal_without_executor... ";
    AgentRunner runner;
    std::promise<AgentGoal> failed_goal;
    auto failed = failed_goal.get_future();
    runner.set_progress_callback([&](const AgentGoal& goal) {
        if (goal.status == GoalStatus::Failed) {
            try { failed_goal.set_value(goal); } catch (...) {}
        }
    });
    runner.start();
    AgentGoal goal;
    goal.id = "missing-executor";
    goal.tasks.push_back(AgentTask{.id = "read-only-task", .risk = TaskRisk::ReadOnly});
    goal.total_count = 1;
    runner.enqueue_goal(goal);
    if (failed.wait_for(std::chrono::seconds(5)) != std::future_status::ready) {
        runner.stop();
        throw std::runtime_error("agent runner did not report failed goal within five seconds");
    }
    const AgentGoal result = failed.get();
    runner.stop();
    assert(result.failed_count == 1);
    assert(result.tasks.front().status == TaskStatus::Failed);
    assert(result.tasks.front().error_message == "task_executor_unavailable");
    std::cout << "PASS\n";
}

int main() {
    std::cout << "Agent Runner Tests\n========================\n";
    test_starts_and_stops();
    test_processes_goal();
    test_rejects_goal_without_executor();
    std::cout << "\nAll runner tests passed!\n";
    return 0;
}
