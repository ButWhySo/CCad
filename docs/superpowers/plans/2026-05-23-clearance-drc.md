# Clearance DRC Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a first kernel-level copper clearance DRC check for different-net pads, vias, and tracks.

**Architecture:** Keep clearance checking inside `ccad_core::runDrc` so CLI, GUI review, and future RPC surfaces receive the same typed diagnostics. Use the current fixed default clearance of 0.20 mm until the rule/constraint engine owns per-netclass values.

**Tech Stack:** C++20 kernel code, CTest DRC regression tests, existing Qt demo screenshot harness.

---

### Task 1: DRC Test Coverage

**Files:**
- Modify: `tests/test_drc.cpp`

- [x] **Step 1: Add a second logical net to the valid board fixture**

The test fixture must include `N1` and `N2` so different-net copper can be tested without triggering unknown-net diagnostics as the only failure.

- [x] **Step 2: Add same-net non-error coverage**

Add a same-net touching track case and assert that `COPPER_CLEARANCE` is not emitted.

- [x] **Step 3: Add different-net pad and track clearance coverage**

Add one different-net pad too close to an existing pad and one different-net track crossing another track. Assert that `COPPER_CLEARANCE` is emitted on the newly added object ID.

- [x] **Step 4: Run the focused RED test**

Run:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && cmake --build build-qt --target ccad_drc_tests && build-qt\ccad_drc_tests.exe"
```

Expected RED result before implementation:

```text
test failure: drc reports different-net pads closer than default clearance
```

### Task 2: Clearance Implementation

**Files:**
- Modify: `src/ccad_core/drc.cpp`

- [x] **Step 1: Add geometry helpers**

Add point-to-segment distance, segment-to-segment distance, rotated pad corner extraction, point-in-polygon, segment-to-polygon distance, and polygon-to-polygon distance helpers.

- [x] **Step 2: Add pairwise copper checks**

Check pad-pad, track-track, pad-track, via-via, via-pad, and via-track pairs. Skip same-net copper and empty-net objects. Compare only shared copper exposure, where pads and tracks must share a copper layer and vias are treated as exposed across copper layers.

- [x] **Step 3: Emit typed diagnostics**

Emit `COPPER_CLEARANCE` as an error on the later checked object ID, with a message naming the nearby object.

- [x] **Step 4: Run the focused GREEN test**

Run:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && cmake --build build-qt --target ccad_drc_tests && build-qt\ccad_drc_tests.exe"
```

Expected GREEN result:

```text
exit code 0
```

### Task 3: Demo And Documentation

**Files:**
- Modify: `scripts/run_sprint_demo.ps1`
- Modify: `README.md`
- Modify: `docs/codebase-map.md`
- Modify: `docs/features/implemented-features.md`
- Modify: `docs/technical-handover.md`
- Modify: `docs/devops/progress.md`
- Create: `docs/devops/sprints/2026-05-23-sprint-43-clearance-drc.md`

- [x] **Step 1: Update the demo**

Add a second different-net track to the generated demo and add logical `N1` and `N2` nets using PowerShell's JSON parser. This lets the demo DRC report show a real clearance error without hand-writing the whole project file.

- [x] **Step 2: Update docs**

Document the fixed default 0.20 mm copper clearance behavior, the diagnostic code, and the Sprint 43 state.

- [x] **Step 3: Run full verification**

Run:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && cmake --build build-qt --clean-first && ctest --test-dir build-qt --output-on-failure"
```

- [x] **Step 4: Run visual demo**

Run:

```cmd
cmd /c "powershell -ExecutionPolicy Bypass -File .\scripts\run_sprint_demo.ps1 -Name sprint43-clearance-drc"
```
