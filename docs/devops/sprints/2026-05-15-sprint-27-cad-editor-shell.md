# Sprint 27: CAD Editor Shell Foundation

## Sprint Goal

Replace the review-dashboard layout with the first native CAD editor shell: central PCB canvas, docked panels, editor tabs, and viewport-owned navigation.

## Branch

`sprint-27-cad-editor-shell`

## Progress

Progress: Phase 2/6, Sprint 27, `main`, merged and verified.

## Backlog

1. Done: inspect current GUI architecture and parallel agent output.
2. Done: add CAD editor shell target spec.
3. Done: make PCB canvas central in editor tabs.
4. Done: add project, layers/objects, and diagnostics docks.
5. Done: add explicit Fit, wheel zoom, middle-button pan, cursor coordinates, and zoom status.
6. Done: capture demo screenshot.
7. Done: full native Qt build and CTest.
8. Done: merge to `main`.

## Verification So Far

Build:

```cmd
cmake --build build-qt --target ccad_gui
```

- Result: passed.

Demo:

```cmd
powershell -ExecutionPolicy Bypass -File .\scripts\run_sprint_demo.ps1 -Name sprint27-cad-shell-fitfix
```

- Result: screenshot captured at `artifacts/screenshots/sprint27-cad-shell-fitfix-20260515-160919.png`.

Full branch gate:

```cmd
cmake --build build-qt --clean-first && ctest --test-dir build-qt --output-on-failure
```

- Result: passed, 11/11 tests.

Main integration gate:

```cmd
cmake --build build-qt --clean-first && ctest --test-dir build-qt --output-on-failure
```

- Result: passed, 11/11 tests after merge to `main`.
