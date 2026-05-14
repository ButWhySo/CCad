# Board Canvas Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a tested board canvas scene model and render it in the Qt review GUI.

**Architecture:** Core produces canvas scene dimensions from board data. Qt GUI renders the scene with `QGraphicsView`.

**Tech Stack:** C++20, CMake, CTest, Qt 6 Widgets.

---

### Task 1: Canvas View-Model

**Files:**
- Create: `src/ccad_core/canvas.hpp`
- Create: `src/ccad_core/canvas.cpp`
- Create: `tests/test_canvas.cpp`
- Modify: `CMakeLists.txt`

- [ ] Write failing tests for project with no board and board rectangle scene.
- [ ] Implement `buildCanvasScene(const Project&)`.
- [ ] Run targeted canvas test.
- [ ] Commit.

### Task 2: Qt Canvas Rendering

**Files:**
- Modify: `src/ccad_gui/main.cpp`
- Modify: `docs/features/implemented-features.md`

- [ ] Add `QGraphicsView` and `QGraphicsScene`.
- [ ] Render board rectangle from core canvas scene.
- [ ] Preserve diagnostics and review summary.
- [ ] Build `ccad_gui`, manually launch demo.
- [ ] Commit.

### Task 3: Verify And Merge

**Files:**
- Modify sprint status docs.

- [ ] Run full Qt build and CTest.
- [ ] Merge to `main`.

