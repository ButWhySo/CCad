# Sprint 6: CLI PCB Authoring

## Sprint Goal

Add machine-callable CLI commands to create pads, vias, and track segments in existing board projects.

## Branch

`sprint-6-cli-pcb-authoring`

## Backlog

1. Done: add failing CLI tests for `pcb add-pad`, `pcb add-via`, and `pcb add-track`.
2. Done: implement CLI mutation guards and deterministic project writeback.
3. Done: document command usage in README and feature docs.
4. Done: run full Qt build and CTest.
5. Pending: merge to `main`.

## Implementation Notes

- `ccad pcb add-pad` appends one `Pad` to `board.pads`.
- `ccad pcb add-via` appends one `Via` to `board.vias`.
- `ccad pcb add-track` appends one `TrackSegment` to `board.tracks`.
- All commands use deterministic project JSON writeback.
- Guardrails are command-level safety checks, not full physical DRC.

## Verification Log

2026-05-14:

```powershell
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
```

Result:

- Clean Qt build passed.
- 8/8 CTest tests passed.

## Definition Of Done

- Agents can create first PCB primitives without manual JSON editing.
- Commands reject missing board, duplicate IDs, invalid dimensions, unknown layers, out-of-board points, and invalid via drill/diameter.
- CLI-created primitives load through existing serialization and render through the existing Qt canvas.
- Full test suite passes before merge.
