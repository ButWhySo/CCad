# Sprint 10: Footprint Rotation

## Sprint Goal

Add rotation-aware footprint placement and rotated pad rendering.

## Branch

`sprint-10-footprint-rotation`

## Progress

Progress: Phase 2/6, Sprint 10, `sprint-10-footprint-rotation`, planning.

## Backlog

1. Add failing pad rotation serialization/canvas tests.
2. Implement pad rotation in model, JSON, and canvas.
3. Add failing CLI placement rotation test.
4. Implement `--rotation-deg` for `pcb place-footprint`.
5. Render rotated pads in Qt.
6. Update docs, progress, and demo script.
7. Run full Qt build and CTest before every commit and before merge.
8. Merge to `main`.

## Definition Of Done

- Board pads persist rotation.
- Canvas pads carry rotation.
- GUI renders rotated pads.
- Footprint placement can rotate pad centers and pad orientation.
- Full Qt build and CTest pass before merge.

