#include "ccad_core/job_manager.hpp"

#include <atomic>
#include <chrono>
#include <thread>

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
  return 0;
}
