# Physical Primitives Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add exact units, geometry, board outline, layers, and JSON round-trip support.

**Architecture:** Extend `ccad_core` model with physical primitives. Keep all state in the kernel and serialize deterministically.

**Tech Stack:** C++20, CMake, CTest.

---

### Task 1: Units And Geometry

**Files:**
- Create: `src/ccad_core/geometry.hpp`
- Create: `src/ccad_core/geometry.cpp`
- Create: `tests/test_geometry.cpp`
- Modify: `CMakeLists.txt`

- [ ] Write failing tests for mm/mil conversion and rectangle max point.
- [ ] Implement integer nanometer `Length`, `Point`, `Size`, and `Rect`.
- [ ] Run targeted geometry tests.
- [ ] Commit.

### Task 2: Board Model And Serialization

**Files:**
- Modify: `src/ccad_core/model.hpp`
- Modify: `src/ccad_core/serialize.cpp`
- Modify: `tests/test_serialize.cpp`

- [ ] Write failing tests for board/layer JSON round trip.
- [ ] Add `Layer`, `Board`, and optional `Project::board`.
- [ ] Extend JSON dump/load.
- [ ] Run serialization tests.
- [ ] Commit.

### Task 3: Docs And Verification

**Files:**
- Modify: `docs/features/implemented-features.md`
- Modify: `docs/technical-handover.md`
- Modify: `docs/devops/sprints/2026-05-14-sprint-3-physical-primitives.md`

- [ ] Document physical primitives and tests.
- [ ] Run full Qt build and CTest.
- [ ] Merge to `main`.

