# Sprint 6: CLI PCB Authoring

## Sprint Goal

Add machine-callable CLI commands to create pads, vias, and track segments in existing board projects.

## Branch

`sprint-6-cli-pcb-authoring`

## Backlog

1. Add failing CLI tests for `pcb add-pad`, `pcb add-via`, and `pcb add-track`.
2. Implement CLI mutation guards and deterministic project writeback.
3. Document command usage in README and feature docs.
4. Run full Qt build and CTest.
5. Merge to `main`.

## Definition Of Done

- Agents can create first PCB primitives without manual JSON editing.
- Commands reject missing board, duplicate IDs, invalid dimensions, unknown layers, out-of-board points, and invalid via drill/diameter.
- CLI-created primitives load through existing serialization and render through the existing Qt canvas.
- Full test suite passes before merge.

