# Qt Review GUI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a native Qt 6 review GUI that lets humans open CCad project files and inspect summary plus ERC diagnostics while keeping all logic in `ccad_core`.

**Architecture:** Implement a tested `ccad_core` review view-model first. Add optional Qt 6 Widgets executable `ccad_gui` that renders the view-model. CMake must continue to build core/CLI/tests without Qt.

**Tech Stack:** C++20, CMake, CTest, optional Qt 6 Widgets.

---

### Task 1: Core Review View-Model

**Files:**
- Create: `src/ccad_core/review.hpp`
- Create: `src/ccad_core/review.cpp`
- Create: `tests/test_review.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write failing review tests**

Create `tests/test_review.cpp` that includes `ccad_core/review.hpp`, builds valid and invalid `Project` objects, and checks `buildReview(Project)` returns counts, diagnostics, and status text.

- [ ] **Step 2: Run targeted test and confirm failure**

Run `cmake -S . -B build -DCCAD_WARNINGS_AS_ERRORS=ON && cmake --build build --target ccad_review_tests`.

Expected: compile failure because `ccad_core/review.hpp` does not exist.

- [ ] **Step 3: Implement review model**

Add `ProjectReview` with project ID/name, component/net/constraint counts, diagnostics, error count, warning count, and status text. Implement `buildReview(const Project&)`.

- [ ] **Step 4: Run targeted test and commit**

Run `cmake --build build --target ccad_review_tests && ctest --test-dir build -R review --output-on-failure`.

Commit `feat: add project review model`.

### Task 2: Optional Qt GUI Shell

**Files:**
- Create: `src/ccad_gui/main.cpp`
- Modify: `CMakeLists.txt`
- Modify: `README.md`
- Modify: `docs/technical-handover.md`
- Modify: `AGENTS.md`

- [ ] **Step 1: Add optional CMake target**

Use `option(CCAD_BUILD_GUI "Build Qt review GUI" ON)` and `find_package(Qt6 QUIET COMPONENTS Widgets)`. Build `ccad_gui` only when Qt 6 Widgets is found.

- [ ] **Step 2: Implement Qt shell**

Create a `QMainWindow` with Open, Reload, summary labels, diagnostics table, and status label. Loading a file uses `loadProjectJson`, `runErc`, and `buildReview`.

- [ ] **Step 3: Update docs**

Document that Qt is optional and that `ccad_gui` appears only when Qt 6 Widgets is available.

- [ ] **Step 4: Verify and commit**

Run full CMake build and CTest. If Qt is available, confirm `ccad_gui` builds. Commit `feat: add optional qt review gui`.

### Task 3: Final Verification And Merge

**Files:**
- Modify only files needed for fixes.

- [ ] **Step 1: Run full verification**

Run `cmake -S . -B build -DCCAD_WARNINGS_AS_ERRORS=ON`, `cmake --build build --clean-first`, and `ctest --test-dir build --output-on-failure`.

- [ ] **Step 2: Review git history and status**

Run `git status --short --branch` and `git log --oneline --decorate --graph --all -10`.

- [ ] **Step 3: Merge**

Checkout `main`, merge `feature-qt-review-gui` with `--no-ff`, and rerun full verification.

