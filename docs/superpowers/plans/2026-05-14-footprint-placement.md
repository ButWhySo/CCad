# Footprint Placement Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add `ccad pcb place-footprint` to place imported footprint pads onto a board.

**Architecture:** Reuse the existing `Footprint` model and board `Pad` model. Add footprint JSON loading, then add a CLI mutation that translates footprint-local pads into board-absolute pads.

**Tech Stack:** C++20, CMake, CTest, optional Qt 6 build for full gate.

---

## File Map

- Modify `src/ccad_core/kicad_footprint_import.hpp`: expose `loadFootprintJson`.
- Modify `src/ccad_core/kicad_footprint_import.cpp`: add footprint JSON parser.
- Modify `tests/test_kicad_footprint_import.cpp`: footprint JSON load tests.
- Modify `src/ccad_cli/main.cpp`: add `pcb place-footprint`.
- Modify `tests/test_cli.cpp`: CLI placement tests.
- Modify docs and demo script.

## Task 1: Footprint JSON Load Tests

- [ ] Add test that imports sample KiCad footprint, dumps JSON, loads JSON back, and checks footprint name/pad geometry.
- [ ] Run importer tests and confirm failure before implementation.

## Task 2: Footprint JSON Loader

- [ ] Implement `Footprint loadFootprintJson(std::string_view source)`.
- [ ] Keep parser strict for known output shape.
- [ ] Run importer tests and full Qt gate.
- [ ] Commit:

```powershell
git add src\ccad_core\kicad_footprint_import.hpp src\ccad_core\kicad_footprint_import.cpp tests\test_kicad_footprint_import.cpp
git commit -m "feat: load ccad footprint json"
```

## Task 3: CLI Place Footprint

- [ ] Add failing CLI test for `pcb place-footprint`.
- [ ] Implement command in `src/ccad_cli/main.cpp`.
- [ ] Validate board, layer, duplicate pad IDs, footprint pads, and inside-board positions.
- [ ] Run CLI tests and full Qt gate.
- [ ] Commit:

```powershell
git add src\ccad_cli\main.cpp tests\test_cli.cpp
git commit -m "feat: add cli footprint placement"
```

## Task 4: Docs, Demo, Merge

- [ ] Update README and feature docs.
- [ ] Update progress counter to Sprint 9.
- [ ] Extend demo script to place imported footprint pads on the board.
- [ ] Run demo and capture screenshot.
- [ ] Full Qt gate.
- [ ] Commit docs.
- [ ] Merge to `main` and verify again.

