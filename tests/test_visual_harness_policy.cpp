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
  const std::string evidence_verifier =
      readFile(root / "scripts" / "verify_sprint.ps1");
  const std::string evidence_hook =
      readFile(root / "scripts" / "hooks" / "commit-msg");
  const std::string evidence_ci =
      readFile(root / ".github" / "workflows" / "ci.yml");
  const std::string evidence_ignore = readFile(root / ".gitignore");
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
  requireContains(single_harness, "$useInternalScreenshot = $true",
                  "single screenshot harness prefers native rendering");
  requireContains(single_harness, "fallback is used only when native rendering returns a failure",
                  "single screenshot harness fallback is explicit");

  requireContains(target_harness, "[int]$InitialLoadMilliseconds = 5000",
                  "multi-target harness initial wait");
  requireContains(target_harness, "[int]$PerTargetMilliseconds = 800",
                  "multi-target harness per-target wait");
  requireContains(target_harness,
                  "} elseif ($Name.Contains(\"sprint997\")) {\n      @(\"conversation_turn_visible\", \"pcb_geometry_relationships_visible\")",
                  "Sprint 997 geometry validation uses its own conversation assertions");
  requireContains(target_harness,
                  "} elseif ($Name.Contains(\"sprint997\")) {\n      $requiredScreenshots = @(\"before\", \"turn-persisted\",",
                  "Sprint 997 geometry validation checks its four feature-specific checkpoints");

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
  requireContains(workflow, "push the verified branch to trigger its independent CI run",
                  "visual workflow triggers CI after local verification");
  requireContains(workflow, "merge only after the actual required",
                  "visual workflow gates merge on real CI status");
  requireContains(workflow, "CI checks pass.",
                  "visual workflow requires passing CI checks before merge");
  requireContains(workflow, "Do not commit screenshots or logs",
                  "visual workflow preserves workspace-only evidence policy");
  requireContains(workflow, "scripts/verify_sprint.ps1",
                  "visual workflow identifies the canonical gate");
  requireContains(workflow, "scripts/run_ui_map_mouse_target_demo.ps1",
                  "visual workflow requires the app-owned mapped harness");
  requireContains(workflow, "-ReusePassedBuildAndTestsFrom",
                  "visual workflow permits only verifier-checked gate reuse");
  requireContains(workflow, "-WorkspaceOnlyEvidence",
                  "visual workflow preserves local-only screenshot/log handling");
  requireContains(evidence_verifier, "[switch]$WorkspaceOnlyEvidence",
                  "evidence verifier supports local-only payloads");
  requireContains(evidence_verifier, "[string]$ReusePassedBuildAndTestsFrom",
                  "evidence verifier gates prior build/test reuse");
  requireContains(evidence_hook, "scripts/check_evidence_manifest.py",
                  "commit hook uses the canonical evidence checker");
  requireContains(evidence_ci, "evidence-manifest:",
                  "hosted CI validates evidence manifests");
  requireContains(evidence_ignore, "artifacts/",
                  "generated evidence remains excluded by default");
  requireContains(evidence_verifier, "[switch]$NonVisual",
                  "evidence verifier supports explicit non-visual changes");
  requireContains(evidence_verifier, "not_applicable_no_gui_behavior_changed",
                  "non-visual manifest states why GUI proof is not applicable");
  requireContains(evidence_verifier, "-InteractionPlan",
                  "GUI evidence still requires an interaction plan");
}
