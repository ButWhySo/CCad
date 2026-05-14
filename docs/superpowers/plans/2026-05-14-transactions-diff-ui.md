# Transactions, Diff, And Review UI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add project diff, transaction journal entries, CLI inspect/diff commands, and modern Qt review styling.

**Architecture:** Keep diff/transaction/JSON rendering in `ccad_core`. CLI and GUI consume core outputs. Qt GUI remains read-only.

**Tech Stack:** C++20, CMake, CTest, Qt 6 Widgets.

---

### Task 1: Shared JSON Output Helpers

**Files:**
- Create: `src/ccad_core/json.hpp`
- Create: `src/ccad_core/json.cpp`
- Modify: `src/ccad_core/serialize.cpp`
- Modify: `src/ccad_cli/main.cpp`

- [ ] Write tests indirectly through existing serialization and CLI tests.
- [ ] Move reusable `escapeJson` to `ccad_core/json.hpp`.
- [ ] Update serialization and CLI code to use the shared helper.
- [ ] Run `ctest --test-dir build-qt --output-on-failure`.

### Task 2: Project Diff

**Files:**
- Create: `src/ccad_core/diff.hpp`
- Create: `src/ccad_core/diff.cpp`
- Create: `tests/test_diff.cpp`
- Modify: `CMakeLists.txt`

- [ ] Write failing tests for added, removed, changed, and clean project diff entries.
- [ ] Implement `diffProjects(const Project& before, const Project& after)`.
- [ ] Run targeted diff test.
- [ ] Commit.

### Task 3: Transaction Journal

**Files:**
- Create: `src/ccad_core/transaction.hpp`
- Create: `src/ccad_core/transaction.cpp`
- Create: `tests/test_transaction.cpp`
- Modify: `CMakeLists.txt`

- [ ] Write failing tests for transaction metadata and embedded diff.
- [ ] Implement `buildTransaction`.
- [ ] Run targeted transaction test.
- [ ] Commit.

### Task 4: CLI Inspect And Diff

**Files:**
- Modify: `src/ccad_cli/main.cpp`
- Modify: `tests/test_cli.cpp`

- [ ] Write failing CLI tests for `inspect` and `diff`.
- [ ] Implement commands and JSON output.
- [ ] Run CLI test.
- [ ] Commit.

### Task 5: Qt Review UI Polish

**Files:**
- Modify: `src/ccad_gui/main.cpp`
- Modify: `docs/features/implemented-features.md`

- [ ] Improve spacing, header, summary cards, status colors, and diagnostics table.
- [ ] Keep GUI read-only.
- [ ] Build `ccad_gui` with Qt and manually inspect.
- [ ] Commit.

### Task 6: Final Verification And Merge

**Files:**
- Modify docs only if verification exposes missing instructions.

- [ ] Run full Qt build and CTest.
- [ ] Update sprint status.
- [ ] Merge to `main`.

