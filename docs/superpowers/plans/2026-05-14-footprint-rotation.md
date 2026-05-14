# Footprint Rotation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add rotation-aware footprint placement and rotated pad rendering.

**Architecture:** Store pad rotation in the core board model, carry it into the canvas model, render it in Qt, and apply a placement transform in the CLI.

**Tech Stack:** C++20, CMake, CTest, Qt 6 Widgets.

---

## File Map

- Modify `src/ccad_core/model.hpp`: add `Pad::rotation_degrees`.
- Modify `src/ccad_core/serialize.cpp`: round-trip pad rotation.
- Modify `src/ccad_core/canvas.hpp/.cpp`: propagate pad rotation.
- Modify `src/ccad_gui/main.cpp`: draw rotated pad paths.
- Modify `src/ccad_cli/main.cpp`: add `--rotation-deg` placement transform.
- Modify tests: `test_serialize.cpp`, `test_canvas.cpp`, `test_cli.cpp`.
- Modify docs and demo script.

## Task 1: Model/Canvas Tests

- [ ] Add failing serialization assertion for `Pad::rotation_degrees`.
- [ ] Add failing canvas assertion for `CanvasPad::rotation_degrees`.
- [ ] Run focused tests and confirm compile/failure before implementation.

## Task 2: Model/Canvas Implementation

- [ ] Add `rotation_degrees` to `Pad` and `CanvasPad`.
- [ ] Update JSON read/write.
- [ ] Update `buildCanvasScene`.
- [ ] Run focused tests and full Qt gate.
- [ ] Commit:

```powershell
git add src\ccad_core\model.hpp src\ccad_core\serialize.cpp src\ccad_core\canvas.hpp src\ccad_core\canvas.cpp tests\test_serialize.cpp tests\test_canvas.cpp
git commit -m "feat: add pad rotation model"
```

## Task 3: CLI Rotation

- [ ] Add failing CLI test using `pcb place-footprint --rotation-deg 90`.
- [ ] Implement rotation transform in `src/ccad_cli/main.cpp`.
- [ ] Run CLI tests and full Qt gate.
- [ ] Commit:

```powershell
git add src\ccad_cli\main.cpp tests\test_cli.cpp
git commit -m "feat: rotate placed footprints"
```

## Task 4: GUI Rendering, Docs, Demo, Merge

- [ ] Render rotated pads in Qt canvas.
- [ ] Update README, feature docs, progress, and sprint log.
- [ ] Update demo script with `--rotation-deg 90`.
- [ ] Run demo and capture screenshot.
- [ ] Full Qt gate.
- [ ] Commit docs/rendering.
- [ ] Merge to `main` and verify.

