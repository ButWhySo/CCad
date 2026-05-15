# Sprint 29: GUI Selection Groundwork

## Sprint Goal

Make rendered PCB canvas primitives selectable by stable object type and ID.

## Branch

`sprint-29-gui-selection-groundwork`

## Progress

Progress: Phase 2/6, Sprint 29, `sprint-29-gui-selection-groundwork`, implementation.

## Backlog

1. Done: ingest methodology file in one full read.
2. Done: update methodology with subagent ingestion rule.
3. Done: add canvas item metadata roles.
4. Done: mark pads, vias, tracks, and keepouts selectable.
5. Done: show selection in status bar and right dock.
6. Done: add opt-in selection click to demo script.
7. Done: capture screenshot.
8. Pending: full native Qt build and CTest.
9. Pending: merge to `main`.

## Verification So Far

Focused GUI build:

```cmd
cmake --build build-qt --target ccad_gui
```

- Result: passed.

Demo:

```cmd
powershell -ExecutionPolicy Bypass -File .\scripts\run_sprint_demo.ps1 -Name sprint29-gui-selection-visible -ClickSelection
```

- Result: screenshot captured at `artifacts/screenshots/sprint29-gui-selection-visible-20260515-173628.png`.
