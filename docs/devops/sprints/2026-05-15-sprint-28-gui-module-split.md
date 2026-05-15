# Sprint 28: GUI Module Split

## Sprint Goal

Split stable GUI panel responsibilities out of `ReviewWindow` so the CAD shell can grow without becoming a monolith.

## Branch

`sprint-28-gui-module-split`

## Progress

Progress: Phase 2/6, Sprint 28, `main`, merged and verified.

## Backlog

1. Done: close completed subagents.
2. Done: inspect current GUI shell files.
3. Done: extract project summary widget.
4. Done: extract diagnostics table widget.
5. Done: update CMake.
6. Done: run focused GUI build.
7. In progress: update docs.
8. Done: full native Qt build and CTest.
9. Done: screenshot demo.
10. Done: merge to `main`.

## Verification So Far

Focused GUI build:

```cmd
cmake --build build-qt --target ccad_gui
```

- Result: passed after fixing the extracted card factory return type.

Full branch gate:

```cmd
cmake --build build-qt --clean-first && ctest --test-dir build-qt --output-on-failure
```

- Result: passed, 11/11 tests.

Demo:

```cmd
powershell -ExecutionPolicy Bypass -File .\scripts\run_sprint_demo.ps1 -Name sprint28-gui-module-split
```

- Result: screenshot captured at `artifacts/screenshots/sprint28-gui-module-split-20260515-162646.png`.

Main integration gate:

```cmd
cmake --build build-qt --clean-first && ctest --test-dir build-qt --output-on-failure
```

- Result: passed, 11/11 tests after merge to `main`.
