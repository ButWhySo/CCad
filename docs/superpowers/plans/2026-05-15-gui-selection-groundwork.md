# Sprint 29 Plan: GUI Selection Groundwork

Progress: Phase 2/6, Sprint 29, `sprint-29-gui-selection-groundwork`, implementation.

## Steps

1. Done: ingest `docs/agent-methodology.md` in one full read.
2. Done: record the rule that subagents must receive the whole methodology file before task instructions.
3. Done: add canvas item type/id metadata.
4. Done: mark board primitives selectable.
5. Done: display selected object in the status bar and right dock.
6. Done: add opt-in `-ClickSelection` to the demo script.
7. Done: capture visual selection screenshot.
8. Pending: full native Qt build and CTest.
9. Pending: commit, merge to `main`, and rerun full integration gate.

## Verification So Far

```cmd
cmake --build build-qt --target ccad_gui
```

- Result: passed.

```cmd
powershell -ExecutionPolicy Bypass -File .\scripts\run_sprint_demo.ps1 -Name sprint29-gui-selection-visible -ClickSelection
```

- Result: screenshot captured at `artifacts/screenshots/sprint29-gui-selection-visible-20260515-173628.png`.

## Demo

- Screenshot: `artifacts/screenshots/sprint29-gui-selection-visible-20260515-173628.png`
