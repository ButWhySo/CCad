#include <cassert>
#include <iostream>
#include "ccad_core/agent_runner.hpp"
#include <chrono>
#include <thread>

using namespace ccad;

static void test_starts_and_stops() {
    std::cout << "  test_starts_and_stops... ";
    AgentRunner runner;
    runner.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    runner.stop();
    std::cout << "PASS\n";
}

static void test_processes_goal() {
    std::cout << "  test_processes_goal... ";
    AgentRunner runner;
    bool progress_called = false;
    
    runner.set_progress_callback([&](const AgentGoal&) {
        progress_called = true;
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

    std::this_thread::sleep_for(std::chrono::milliseconds(800));
    
    runner.stop();
    assert(progress_called == true);
    std::cout << "PASS\n";
}

int main() {
    std::cout << "Agent Runner Tests\n========================\n";
    test_starts_and_stops();
    test_processes_goal();
    std::cout << "\nAll runner tests passed!\n";
    return 0;
}
