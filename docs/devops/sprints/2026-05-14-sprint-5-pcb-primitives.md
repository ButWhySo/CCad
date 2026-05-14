# Sprint 5: PCB Drawable Primitives

## Sprint Goal

Add the first PCB drawable objects to the kernel: pads, vias, and track segments. Render them in the Qt canvas from deterministic project state.

## Branch

`sprint-5-pcb-primitives`

## Backlog

1. Done: add `Pad`, `Via`, and `TrackSegment` model types.
2. Done: add deterministic JSON round-trip for primitives.
3. Done: extend canvas scene with drawable primitives.
4. Done: render primitives in Qt canvas.
5. Done: update docs/features and README usage notes.
6. In progress: final branch verification, then merge.

## Implementation Notes

- Primitive data belongs to `ccad_core::Board`; the GUI does not own PCB state.
- JSON round-trip tests cover primitive persistence.
- Canvas tests cover conversion from nanometer geometry to millimeter view units.
- Qt rendering currently shows red track segments, pink pads, and yellow vias on the dark board canvas.
- The current GUI remains a review canvas, not an interactive editor. Authoring commands for PCB primitives are planned for a later sprint.

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

- Pads, vias, and tracks are kernel data, not GUI state.
- JSON round-trip preserves primitive IDs, geometry, net IDs, and layer IDs.
- Canvas scene exposes primitives in millimeter view units.
- Qt canvas renders board outline plus primitives.
- Full tests pass.
