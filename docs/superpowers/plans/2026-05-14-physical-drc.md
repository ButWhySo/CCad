# Physical DRC Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add kernel-level physical DRC diagnostics and a `ccad drc` CLI command for board primitives.

**Architecture:** Implement DRC in `ccad_core` as a pure function over `Project`, returning existing `Diagnostic` objects. Wire CLI output through the existing diagnostics JSON formatter and CTest coverage.

**Tech Stack:** C++20, CMake, CTest, optional Qt 6 build for full gate.

---

## File Map

- Create `src/ccad_core/drc.hpp`: public DRC declaration.
- Create `src/ccad_core/drc.cpp`: DRC rules.
- Modify `CMakeLists.txt`: add DRC source and `ccad_drc_tests`.
- Create `tests/test_drc.cpp`: kernel DRC behavior tests.
- Modify `src/ccad_cli/main.cpp`: add `ccad drc <path>`.
- Modify `tests/test_cli.cpp`: CLI DRC command tests.
- Modify docs: README, features, sprint log, progress counter.

## Task 1: Kernel DRC Tests

- [ ] Create `tests/test_drc.cpp`.
- [ ] Build valid board project helper with one pad, one via, and one track.
- [ ] Assert `runDrc(valid_project).empty()`.
- [ ] Add invalid project cases for:
  - pad outside board -> `PAD_OUTSIDE_BOARD`
  - unknown track layer -> `UNKNOWN_TRACK_LAYER`
  - via drill larger than diameter -> `VIA_DRILL_TOO_LARGE`
  - zero-length track -> `ZERO_LENGTH_TRACK`
  - duplicate pad ID -> `DUPLICATE_PAD_ID`
- [ ] Run:

```powershell
cmake --build build-qt --target ccad_drc_tests
ctest --test-dir build-qt -R drc --output-on-failure
```

Expected: configure/build fails before implementation because target/header does not exist.

## Task 2: Kernel DRC Implementation

- [ ] Add `src/ccad_core/drc.hpp` with:

```cpp
#pragma once

#include "ccad_core/erc.hpp"
#include "ccad_core/model.hpp"

#include <vector>

namespace ccad {

std::vector<Diagnostic> runDrc(const Project& project);

}  // namespace ccad
```

- [ ] Add `src/ccad_core/drc.cpp` implementing structural/geometry checks from the spec.
- [ ] Add `src/ccad_core/drc.cpp` to `ccad_core` sources in `CMakeLists.txt`.
- [ ] Add executable `ccad_drc_tests` in `CMakeLists.txt`.
- [ ] Run DRC tests until passing.
- [ ] Run full Qt gate before commit:

```powershell
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
```

- [ ] Commit:

```powershell
git add CMakeLists.txt src\ccad_core\drc.hpp src\ccad_core\drc.cpp tests\test_drc.cpp
git commit -m "feat: add physical drc diagnostics"
```

## Task 3: CLI DRC Command

- [ ] Add failing CLI test for:

```powershell
.\build-qt\ccad.exe drc board.ccad.json
```

- [ ] Implement `drcCommand` in `src/ccad_cli/main.cpp`.
- [ ] Update usage text.
- [ ] Run CLI tests and full Qt gate.
- [ ] Commit:

```powershell
git add src\ccad_cli\main.cpp tests\test_cli.cpp
git commit -m "feat: add cli drc command"
```

## Task 4: Docs And Merge

- [ ] Update README with `ccad drc`.
- [ ] Update `docs/features/implemented-features.md`.
- [ ] Update `docs/devops/progress.md` to Sprint 7.
- [ ] Update sprint log verification.
- [ ] Run full Qt gate.
- [ ] Commit docs.
- [ ] Merge to `main`.

