# PCB Drawable Primitives Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add pads, vias, and track segments to the kernel, JSON, canvas scene, and Qt rendering.

**Architecture:** Physical primitive data lives in `ccad_core::Board`. Canvas rendering consumes `ccad_core::CanvasScene`.

**Tech Stack:** C++20, CMake, CTest, Qt 6 Widgets.

---

### Task 1: Model And Serialization

**Files:**
- Modify: `src/ccad_core/model.hpp`
- Modify: `src/ccad_core/serialize.cpp`
- Modify: `tests/test_serialize.cpp`

- [ ] Write failing serialization round-trip tests for pad, via, and track.
- [ ] Implement model structs.
- [ ] Extend JSON load/dump.
- [ ] Run serialization tests.
- [ ] Commit.

### Task 2: Canvas Scene And Rendering

**Files:**
- Modify: `src/ccad_core/canvas.hpp`
- Modify: `src/ccad_core/canvas.cpp`
- Modify: `tests/test_canvas.cpp`
- Modify: `src/ccad_gui/main.cpp`

- [ ] Write failing canvas tests for primitive conversion.
- [ ] Extend canvas scene.
- [ ] Render primitives in Qt.
- [ ] Build GUI.
- [ ] Commit.

### Task 3: Docs And Merge

**Files:**
- Modify: `docs/features/implemented-features.md`
- Modify: `docs/devops/sprints/2026-05-14-sprint-5-pcb-primitives.md`

- [ ] Document primitive support.
- [ ] Run full Qt build and CTest.
- [ ] Merge to `main`.

