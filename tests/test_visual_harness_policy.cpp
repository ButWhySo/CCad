#include "test_support.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#ifndef CCAD_SOURCE_DIR
#error "CCAD_SOURCE_DIR must be defined for visual harness policy tests"
#endif

namespace {

std::string readFile(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  require(bool(input), "policy fixture file opens: " + path.string());
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

bool contains(const std::string& haystack, const std::string& needle) {
  return haystack.find(needle) != std::string::npos;
}

void requireContains(const std::string& haystack, const std::string& needle,
                     const std::string& message) {
  require(contains(haystack, needle), message + " contains " + needle);
}

void requireNotContains(const std::string& haystack, const std::string& needle,
                        const std::string& message) {
  require(!contains(haystack, needle), message + " excludes " + needle);
}

}  // namespace

int main() {
  const std::filesystem::path root = CCAD_SOURCE_DIR;
  const std::string single_harness =
      readFile(root / "scripts" / "run_sprint_demo.ps1");
  const std::string target_harness =
      readFile(root / "scripts" / "run_ui_map_mouse_target_demo.ps1");
  const std::string interaction_harness =
      readFile(root / "scripts" / "run_gui_interaction_demo.ps1");
  const std::string workflow =
      readFile(root / ".agents" / "workflows" / "visual-validation.md");

  requireContains(single_harness, "[int]$GuiWaitSeconds = 7",
                  "single screenshot harness default wait");
  requireNotContains(single_harness, "20 seconds", "single screenshot harness wording");
  requireNotContains(single_harness, "Start-Sleep -Seconds 20",
                     "single screenshot harness sleeps");
  requireContains(single_harness, "pcb add-graphic-line",
                  "single screenshot harness demonstrates board graphics");
  requireContains(single_harness, "pcb add-text",
                  "single screenshot harness demonstrates board text");
  requireContains(single_harness, "pcb add-zone",
                  "single screenshot harness demonstrates board zones");

  requireContains(target_harness, "[int]$InitialLoadMilliseconds = 5000",
                  "multi-target harness initial wait");
  requireContains(target_harness, "[int]$PerTargetMilliseconds = 800",
                  "multi-target harness per-target wait");

  requireContains(interaction_harness, "[int]$GuiWaitSeconds = 7",
                  "live interaction harness preview wait");
  requireContains(interaction_harness, "[int]$InitialLoadMilliseconds = 5000",
                  "live interaction harness initial wait");
  requireContains(interaction_harness, "[int]$PerActionMilliseconds = 800",
                  "live interaction harness action wait");
  requireContains(interaction_harness, "GetForegroundWindow",
                  "live interaction harness targets active chooser dialog");
  requireNotContains(interaction_harness, "AddSeconds(20)",
                     "live interaction harness window deadline");
  requireNotContains(interaction_harness, "Start-Sleep -Seconds 3",
                     "live interaction harness fixed action sleep");
  requireNotContains(interaction_harness, "$rect.Left + 315",
                     "live interaction harness hard-coded chooser x");
  requireNotContains(interaction_harness, "$rect.Top + 165",
                     "live interaction harness hard-coded chooser y");

  requireContains(workflow, "7 seconds", "visual workflow single-preview policy");
  requireContains(workflow, "5-second initial load wait",
                  "visual workflow multi-target initial wait policy");
  requireContains(workflow, "800 ms", "visual workflow per-action policy");
  requireNotContains(workflow, "20 seconds", "visual workflow obsolete wait policy");
}
