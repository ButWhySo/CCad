# Sprint 19: Keepout Authoring And Visibility

## Sprint Goal

Make rectangular keepouts authorable through the CLI and visible in the native Qt board canvas.

## Branch

`sprint-19-keepout-authoring-visibility`

## Progress

Progress: Phase 2/6, Sprint 19, `main`, merged and verified.

## Backlog

1. Done: add failing CLI and canvas tests.
2. Done: add keepout data to `CanvasScene`.
3. Done: add `pcb add-keepout`.
4. Done: add command help metadata.
5. Done: render keepouts in Qt canvas.
6. Done: run focused CLI/canvas tests.
7. Done: update docs and demo script.
8. Done: run full native Qt build and CTest.
9. Done: capture GUI screenshot demo.
10. Done: merge to `main`.

## Verification So Far

RED:

```powershell
cmake --build build-qt --target ccad_canvas_tests ccad_cli_tests
```

- Result: failed because `CanvasScene` had no `keepouts` member.

GREEN:

```powershell
cmake --build build-qt --target ccad_canvas_tests ccad_cli_tests ccad_gui
ctest --test-dir build-qt -R "canvas|cli" --output-on-failure
```

- Result: focused CLI and canvas tests passed.

Full feature-branch gate:

```powershell
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
```

- Result: 10 / 10 tests passed before commit.

Main integration gate:

```cmd
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
```

- Result: 10 / 10 tests passed after merging Sprint 19 to `main`.

## Demo

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_sprint_demo.ps1 -Name sprint19-keepout-demo
```

- Screenshot: `artifacts/screenshots/sprint19-keepout-demo-20260515-105958.png`
- Result: GUI screenshot shows the keepout as an orange dashed rectangle on the board canvas.
