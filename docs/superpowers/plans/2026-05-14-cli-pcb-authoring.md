# CLI PCB Authoring Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add `ccad pcb add-pad`, `ccad pcb add-via`, and `ccad pcb add-track` so agents can author first PCB primitives through the CLI.

**Architecture:** The CLI loads a `.ccad.json` project into `ccad_core::Project`, validates arguments against `Project::board`, appends one primitive, then rewrites the deterministic JSON. The GUI and kernel rendering consume the same model, so new CLI-created primitives appear in the review canvas without GUI-owned state.

**Tech Stack:** C++20, CMake, CTest, Qt 6 optional build.

---

## File Map

- Modify `src/ccad_cli/main.cpp`: add `pcb` command dispatch, argument parsing helpers, mutation validation, project writeback.
- Modify `tests/test_cli.cpp`: add black-box CLI tests for successful and rejected PCB primitive authoring.
- Modify `README.md`: document `pcb add-*` commands and when to use them.
- Modify `docs/features/implemented-features.md`: document CLI primitive authoring.
- Create `docs/devops/sprints/2026-05-14-sprint-6-cli-pcb-authoring.md`: sprint log.

## Task 1: Failing CLI Tests

**Files:**
- Modify: `tests/test_cli.cpp`

- [ ] Add a test sequence after board init that runs:

```cpp
const std::string add_pad_command =
    quote(CCAD_BINARY) + " pcb add-pad --file " + quote(board_project_path) +
    " --id P1 --component U1 --pin 1 --net N1 --layer F.Cu"
    " --x-mm 5 --y-mm 6 --width-mm 1.5 --height-mm 1.0";
require(run(add_pad_command) == 0, "pcb add-pad exits zero");
const std::string pad_json = readFile(board_project_path);
require(pad_json.find("\"pads\"") != std::string::npos, "pcb add-pad writes pads");
require(pad_json.find("\"id\": \"P1\"") != std::string::npos, "pcb add-pad writes id");
require(pad_json.find("\"width_nm\": 1500000") != std::string::npos, "pcb add-pad writes width");
```

- [ ] Add success checks for `pcb add-via`:

```cpp
const std::string add_via_command =
    quote(CCAD_BINARY) + " pcb add-via --file " + quote(board_project_path) +
    " --id V1 --net N1 --x-mm 8 --y-mm 9 --diameter-mm 0.8 --drill-mm 0.4";
require(run(add_via_command) == 0, "pcb add-via exits zero");
const std::string via_json = readFile(board_project_path);
require(via_json.find("\"vias\"") != std::string::npos, "pcb add-via writes vias");
require(via_json.find("\"drill_nm\": 400000") != std::string::npos, "pcb add-via writes drill");
```

- [ ] Add success checks for `pcb add-track`:

```cpp
const std::string add_track_command =
    quote(CCAD_BINARY) + " pcb add-track --file " + quote(board_project_path) +
    " --id T1 --net N1 --layer F.Cu"
    " --start-x-mm 5 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9 --width-mm 0.25";
require(run(add_track_command) == 0, "pcb add-track exits zero");
const std::string track_json = readFile(board_project_path);
require(track_json.find("\"tracks\"") != std::string::npos, "pcb add-track writes tracks");
require(track_json.find("\"width_nm\": 250000") != std::string::npos, "pcb add-track writes width");
```

- [ ] Add negative checks:

```cpp
require(run(add_pad_command) != 0, "pcb add-pad rejects duplicate id");
const std::string bad_layer_command =
    quote(CCAD_BINARY) + " pcb add-track --file " + quote(board_project_path) +
    " --id T_BAD --net N1 --layer Inner.Cu"
    " --start-x-mm 5 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9 --width-mm 0.25";
require(run(bad_layer_command) != 0, "pcb add-track rejects unknown layer");
const std::string bad_via_command =
    quote(CCAD_BINARY) + " pcb add-via --file " + quote(board_project_path) +
    " --id V_BAD --net N1 --x-mm 8 --y-mm 9 --diameter-mm 0.4 --drill-mm 0.8";
require(run(bad_via_command) != 0, "pcb add-via rejects drill larger than diameter");
const std::string outside_track_command =
    quote(CCAD_BINARY) + " pcb add-track --file " + quote(board_project_path) +
    " --id T_OUT --net N1 --layer F.Cu"
    " --start-x-mm 5 --start-y-mm 6 --end-x-mm 99 --end-y-mm 9 --width-mm 0.25";
require(run(outside_track_command) != 0, "pcb add-track rejects endpoint outside board");
```

- [ ] Run:

```powershell
cmake --build build-qt --target ccad_cli_tests
ctest --test-dir build-qt -R cli --output-on-failure
```

Expected: test fails because `pcb` command is not implemented.

## Task 2: CLI Mutation Implementation

**Files:**
- Modify: `src/ccad_cli/main.cpp`

- [ ] Update usage text to include:

```text
  ccad pcb add-pad --file <path> --id <id> --component <id> --pin <name> --net <id> --layer <id> --x-mm <n> --y-mm <n> --width-mm <n> --height-mm <n>
  ccad pcb add-via --file <path> --id <id> --net <id> --x-mm <n> --y-mm <n> --diameter-mm <n> --drill-mm <n>
  ccad pcb add-track --file <path> --id <id> --net <id> --layer <id> --start-x-mm <n> --start-y-mm <n> --end-x-mm <n> --end-y-mm <n> --width-mm <n>
```

- [ ] Add helpers:

```cpp
bool hasLayer(const ccad::Board& board, const std::string& layer_id);
bool containsPoint(const ccad::Board& board, ccad::Point point);
bool writeProjectFile(const std::string& path, const ccad::Project& project);
```

- [ ] Add `pcbCommand(const std::vector<std::string>& args)` that dispatches `add-pad`, `add-via`, and `add-track`.
- [ ] For each subcommand, parse only documented flags; unknown or missing flags return exit code `2`.
- [ ] Load project from `--file`, require `project.board.has_value()`, validate primitive-specific constraints, append primitive, write deterministic JSON.
- [ ] Add `if (command == "pcb") return pcbCommand(args);` in `main`.
- [ ] Run:

```powershell
cmake --build build-qt --target ccad_cli_tests
ctest --test-dir build-qt -R cli --output-on-failure
```

Expected: `cli` test passes.

- [ ] Commit:

```powershell
git add src\ccad_cli\main.cpp tests\test_cli.cpp
git commit -m "feat: add cli pcb primitive authoring"
```

## Task 3: Docs And Verification

**Files:**
- Modify: `README.md`
- Modify: `docs/features/implemented-features.md`
- Create: `docs/devops/sprints/2026-05-14-sprint-6-cli-pcb-authoring.md`

- [ ] Document each `pcb add-*` command, what it does, and when to run it.
- [ ] Document limitations: no DRC beyond command guards, no schematic parity, no route solving.
- [ ] Run full gate:

```powershell
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
```

Expected: clean build and 8/8 or more CTest tests pass.

- [ ] Commit docs:

```powershell
git add README.md docs\features\implemented-features.md docs\devops\sprints\2026-05-14-sprint-6-cli-pcb-authoring.md
git commit -m "docs: document cli pcb authoring"
```

- [ ] Merge after successful verification.
