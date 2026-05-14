# Sprint 10: Footprint Rotation

## Sprint Goal

Add rotation-aware footprint placement and rotated pad rendering.

## Branch

`sprint-10-footprint-rotation`

## Progress

Progress: Phase 2/6, Sprint 10, `sprint-10-footprint-rotation`, planning.

## Backlog

1. Done: add failing pad rotation serialization/canvas tests.
2. Done: implement pad rotation in model, JSON, and canvas.
3. Done: add failing CLI placement rotation test.
4. Done: implement `--rotation-deg` for `pcb place-footprint`.
5. Done: render rotated pads in Qt.
6. In progress: update docs, progress, and demo script.
7. In progress: run full Qt build and CTest before every commit and before merge.
8. Pending: merge to `main`.

## Demo Artifacts

Run:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_sprint_demo.ps1 -Name sprint10-footprint-rotation-demo
```

Outputs include:

- `artifacts/demos/sprint10-footprint-rotation-demo.ccad.json`
- `artifacts/screenshots/sprint10-footprint-rotation-demo-<timestamp>.png`

## Definition Of Done

- Board pads persist rotation.
- Canvas pads carry rotation.
- GUI renders rotated pads.
- Footprint placement can rotate pad centers and pad orientation.
- Full Qt build and CTest pass before merge.
