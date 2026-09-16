#include "ccad_core/job_manager.hpp"

#include <atomic>
#include <chrono>
#include <thread>
#include <stdexcept>

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
  return 0;
}
