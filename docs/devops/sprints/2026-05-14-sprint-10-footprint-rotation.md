# Sprint 10: Footprint Rotation

## Sprint Goal

Add rotation-aware footprint placement and rotated pad rendering.

## Branch

`sprint-10-footprint-rotation`

## Progress

Progress: Phase 2/6, Sprint 10, `main`, merged and verified.

## Backlog

1. Done: add failing pad rotation serialization/canvas tests.
2. Done: implement pad rotation in model, JSON, and canvas.
3. Done: add failing CLI placement rotation test.
4. Done: implement `--rotation-deg` for `pcb place-footprint`.
5. Done: render rotated pads in Qt.
6. Done: update docs, progress, and demo script.
7. Done: run full Qt build and CTest before every implementation commit.
8. Done: merge to `main`.

## Demo Artifacts

Run:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_sprint_demo.ps1 -Name sprint10-footprint-rotation-demo
```

Outputs include:

- `artifacts/demos/sprint10-footprint-rotation-demo.ccad.json`
- `artifacts/screenshots/sprint10-footprint-rotation-demo-20260515-004031.png`

Observed screenshot behavior:

- Native Qt review window launches from the demo script.
- The imported 0805 footprint is placed with `--rotation-deg 90`.
- Pad centers and pad bodies are visibly rotated on the board canvas.
- The current `EMPTY_PROJECT` warning remains expected because Sprint 10 still exercises physical placement directly, before schematic/component parity is implemented.

Latest verified checkpoints:

- `b2c1807 refactor: split qt review gui modules`
- Command: `cmake --build build-qt --clean-first`
- Command: `ctest --test-dir build-qt --output-on-failure`
- Result: 10/10 tests passed before checkpoint commit.
- `main` merge gate after `merge: sprint 10 footprint rotation`
- Command: `cmake --build build-qt --clean-first`
- Command: `ctest --test-dir build-qt --output-on-failure`
- Result: 10/10 tests passed after merge.

## Definition Of Done

- Board pads persist rotation.
- Canvas pads carry rotation.
- GUI renders rotated pads.
- Footprint placement can rotate pad centers and pad orientation.
- Full Qt build and CTest pass before merge.
