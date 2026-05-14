# Sprint 9: Footprint Placement

## Sprint Goal

Place imported footprint pads onto a board using one machine-callable command.

## Branch

`sprint-9-footprint-placement`

## Progress

Progress: Phase 2/6, Sprint 9, `sprint-9-footprint-placement`, planning.

## Backlog

1. Add failing footprint JSON load tests.
2. Implement CCad footprint JSON loader.
3. Add failing CLI placement tests.
4. Implement `ccad pcb place-footprint`.
5. Update docs, progress, and demo script.
6. Run full Qt build and CTest before every commit and before merge.
7. Merge to `main`.

## Definition Of Done

- Imported footprint JSON can be loaded back into `Footprint`.
- CLI can place a footprint onto a board.
- Placed pads render through existing GUI canvas.
- Command rejects unknown layer, duplicate pad IDs, missing board, and out-of-board placement.
- Full Qt build and CTest pass before merge.

