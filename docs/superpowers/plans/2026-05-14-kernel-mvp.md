# Kernel MVP Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the tested native base of CCad: repo hygiene, docs, CI, C++ logical project model, deterministic JSON, ERC, and CLI.

**Architecture:** C++20 static library `ccad_core` with focused modules for model, serialization, and ERC. Native executable `ccad` is the first machine-callable surface. JSON is deterministic and schema-versioned.

**Tech Stack:** C++20, CMake, CTest, GitHub Actions. Python is not required for core build.

---

### Task 1: Repo And Tooling Base

**Files:**
- Create: `CMakeLists.txt`
- Create: `.gitignore`
- Create: `.github/workflows/ci.yml`
- Create: `README.md`
- Create: `AGENTS.md`
- Create: `docs/technical-handover.md`

- [ ] **Step 1: Add project metadata and tools**

Create root `CMakeLists.txt` with C++20, `ccad_core`, `ccad` executable, and CTest integration.

- [ ] **Step 2: Add CI**

Create GitHub Actions workflow that configures CMake, builds, and runs CTest.

- [ ] **Step 3: Add documentation**

Write README, AGENTS handover, and technical handover docs describing phase roadmap, commands, branch rules, testing, and security constraints.

- [ ] **Step 4: Commit**

Run `git add` for docs/tooling and commit `chore: establish project base`.

### Task 2: Model And Serialization

**Files:**
- Create: `src/ccad_core/model.hpp`
- Create: `src/ccad_core/model.cpp`
- Create: `src/ccad_core/serialize.hpp`
- Create: `src/ccad_core/serialize.cpp`
- Create: `tests/test_serialize.cpp`

- [ ] **Step 1: Write failing serialization tests**

Tests require deterministic JSON, schema version, project ID/name, component pins, nets, and constraints.

- [ ] **Step 2: Run targeted tests and confirm failure**

Run `cmake --build build --target ccad_tests && ctest --test-dir build -R serialize --output-on-failure`. Expected: compile failure before implementation.

- [ ] **Step 3: Implement dataclasses and JSON functions**

Implement `Pin`, `Component`, `NetMember`, `Net`, `Constraint`, `Project`, `dumpProjectJson`, and `loadProjectJson`.

- [ ] **Step 4: Run tests and commit**

Run targeted CTest, then commit `feat: add deterministic project model`.

### Task 3: ERC Rules

**Files:**
- Create: `src/ccad_core/erc.hpp`
- Create: `src/ccad_core/erc.cpp`
- Create: `tests/test_erc.cpp`

- [ ] **Step 1: Write failing ERC tests**

Tests cover clean designs, unknown component refs, unknown pin refs, duplicate net membership, and warning for empty projects.

- [ ] **Step 2: Run targeted tests and confirm failure**

Run targeted CTest. Expected: compile failure before implementation.

- [ ] **Step 3: Implement typed diagnostics and ERC**

Implement `Diagnostic` and `runErc(const Project&) -> std::vector<Diagnostic>`.

- [ ] **Step 4: Run tests and commit**

Run targeted CTest, then commit `feat: add logical erc checks`.

### Task 4: CLI

**Files:**
- Create: `src/ccad_cli/main.cpp`
- Create: `tests/test_cli.cpp`

- [ ] **Step 1: Write failing CLI tests**

Tests cover `ccad init --name demo --out board.json`, `ccad validate board.json`, JSON diagnostics, and nonzero exit for errors.

- [ ] **Step 2: Run targeted tests and confirm failure**

Run targeted CTest. Expected: compile failure before implementation.

- [ ] **Step 3: Implement CLI**

Implement `init` and `validate` subcommands with simple native argument parsing. `validate` prints JSON diagnostics and exits `1` if any error exists.

- [ ] **Step 4: Run tests and commit**

Run targeted CTest, then commit `feat: add machine-callable cli`.

### Task 5: Full Verification And Merge

**Files:**
- Modify as needed only for fixes discovered by verification.

- [ ] **Step 1: Run full local verification**

Run `cmake -S . -B build -DCCAD_WARNINGS_AS_ERRORS=ON`, `cmake --build build`, and `ctest --test-dir build --output-on-failure`.

- [ ] **Step 2: Fix failures with tests first**

For behavior failures, add or adjust tests before production fixes.

- [ ] **Step 3: Commit final docs/fixes**

Commit with `chore: verify kernel mvp`.

- [ ] **Step 4: Merge locally**

Checkout `main`, merge `phase-0-kernel-base`, rerun full verification, and keep history intact.
