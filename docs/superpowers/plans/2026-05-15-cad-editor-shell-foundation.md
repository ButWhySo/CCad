# Sprint 27 Plan: CAD Editor Shell Foundation

Progress: Phase 2/6, Sprint 27, `sprint-27-cad-editor-shell`, implementation.

## Steps

1. Done: review current GUI architecture and parallel agent findings.
2. Done: remove renderer-owned auto-fit from board renderer.
3. Done: add viewport-owned fit, wheel zoom, middle-button pan, and cursor/zoom status.
4. Done: move GUI shell to central PCB tab plus docked project, layers, and diagnostics panels.
5. Done: capture demo screenshot.
6. In progress: update docs.
7. Pending: full native Qt build and CTest.
8. Pending: commit, merge to `main`, and rerun full integration gate.

## Verification So Far

```cmd
cmake --build build-qt --target ccad_gui
```

- Result: passed after CAD shell changes.

```cmd
powershell -ExecutionPolicy Bypass -File .\scripts\run_sprint_demo.ps1 -Name sprint27-cad-shell-fitfix
```

- Result: produced `artifacts/screenshots/sprint27-cad-shell-fitfix-20260515-160919.png`.

## Demo

- Screenshot: `artifacts/screenshots/sprint27-cad-shell-fitfix-20260515-160919.png`
