# Sprint 9: Footprint Placement

## Sprint Goal

Place imported footprint pads onto a board using one machine-callable command.

## Branch

`sprint-9-footprint-placement`

## Progress

Progress: Phase 2/6, Sprint 9, `sprint-9-footprint-placement`, planning.

## Backlog

1. Done: add failing footprint JSON load tests.
2. Done: implement CCad footprint JSON loader.
3. Done: add failing CLI placement tests.
4. Done: implement `ccad pcb place-footprint`.
5. Done: update docs, progress, and demo script.
6. Done: run full Qt build and CTest before every commit and before merge.
7. Pending: merge to `main`.

## Demo Artifacts

Run:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_sprint_demo.ps1 -Name sprint9-footprint-placement-demo
```

Outputs include:

- `artifacts/demos/sprint9-footprint-placement-demo.ccad.json`
- `artifacts/demos/sprint9-footprint-placement-demo-R_0805_2012Metric.ccad-footprint.json`
- `artifacts/screenshots/sprint9-footprint-placement-demo-<timestamp>.png`

Latest local demo screenshot:

- `artifacts/screenshots/sprint9-footprint-placement-demo-20260514-180624.png`

## Verification Log

2026-05-14:

```powershell
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
powershell -ExecutionPolicy Bypass -File .\scripts\run_sprint_demo.ps1 -Name sprint9-footprint-placement-demo
```

Result:

- Clean Qt build passed.
- 10/10 CTest tests passed.
- Demo project, imported footprint JSON, placed board pads, and GUI screenshot were generated under `artifacts/`.

## Definition Of Done

- Imported footprint JSON can be loaded back into `Footprint`.
- CLI can place a footprint onto a board.
- Placed pads render through existing GUI canvas.
- Command rejects unknown layer, duplicate pad IDs, missing board, and out-of-board placement.
- Full Qt build and CTest pass before merge.
