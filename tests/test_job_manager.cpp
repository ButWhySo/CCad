#include "ccad_core/job_manager.hpp"
#include "ccad_core/agent_runner.hpp"

#include <atomic>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <future>
#include <filesystem>
#include <fstream>

#include "test_support.hpp"

int main() {
  ccad::JobManager manager;
  std::atomic<bool> started{false};
  std::atomic<bool> finished{false};

  manager.enqueue([&] {
    started.store(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    finished.store(true);
  });

  while (!started.load()) {
    std::this_thread::yield();
  }
  manager.waitAll();
  require(finished.load(), "waitAll waits for running task completion");

  std::atomic<bool> after_throw{false};
  manager.enqueue([] { throw std::runtime_error("expected worker task failure"); });
  manager.enqueue([&] { after_throw.store(true); });
  manager.waitAll();
  require(after_throw.load(), "worker survives task exception");

  const auto queue_file = std::filesystem::temp_directory_path() / "ccad-agent-queue.json";
  ccad::AgentRunner source;
  ccad::AgentGoal queued;
  queued.id = "resume-goal";
  queued.description = "Resume after restart";
  queued.context_json = "{\"project_id\":\"p1\"}";
  queued.tasks.push_back(ccad::AgentTask{.id = "resume-task", .tool_name = "local.tool", .tool_args_json = "{\"value\":7}"});
  source.enqueue_goal(queued);
  source.save_queue(queue_file.string());

  ccad::AgentRunner restored;
  std::promise<ccad::AgentGoal> restored_goal;
  auto restored_future = restored_goal.get_future();
  restored.set_task_executor([](ccad::AgentGoal& goal, const std::string& task_id) {
    for (auto& task : goal.tasks) {
      if (task.id == task_id) {
        task.status = ccad::TaskStatus::Completed;
        task.result_json = "{\"status\":\"executed_by_test_executor\"}";
        return task;
      }
    }
    ccad::AgentTask missing;
    missing.id = task_id;
    missing.status = ccad::TaskStatus::Failed;
    missing.error_message = "test_task_not_found";
    return missing;
  });
  restored.set_progress_callback([&](const ccad::AgentGoal& goal) {
    if (goal.status == ccad::GoalStatus::Completed) restored_goal.set_value(goal);
  });
  restored.load_queue(queue_file.string());
  restored.start();
  const auto result = restored_future.get();
  require(result.id == "resume-goal", "load_queue restores queued goal");
  require(result.tasks.front().id == "resume-task", "load_queue restores queued task");
  require(result.context_json == "{\"project_id\":\"p1\"}", "load_queue restores goal context");
  require(result.tasks.front().tool_args_json == "{\"value\":7}", "load_queue restores task arguments");
  std::filesystem::remove(queue_file);

  const auto invalid_file = std::filesystem::temp_directory_path() / "ccad-agent-queue-invalid.json";
  { std::ofstream invalid(invalid_file); invalid << "{\"wrong\":true}"; }
  bool rejected = false;
  try { restored.load_queue(invalid_file.string()); } catch (const std::runtime_error&) { rejected = true; }
  require(rejected, "load_queue rejects malformed root");
  std::filesystem::remove(invalid_file);
  return 0;
}
