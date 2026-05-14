# Sprint 5: PCB Drawable Primitives

## Sprint Goal

Add the first PCB drawable objects to the kernel: pads, vias, and track segments. Render them in the Qt canvas from deterministic project state.

## Branch

`sprint-5-pcb-primitives`

## Backlog

1. Add `Pad`, `Via`, and `TrackSegment` model types.
2. Add deterministic JSON round-trip for primitives.
3. Extend canvas scene with drawable primitives.
4. Render primitives in Qt canvas.
5. Update docs/features.
6. Full Qt build and CTest, then merge.

## Definition Of Done

- Pads, vias, and tracks are kernel data, not GUI state.
- JSON round-trip preserves primitive IDs, geometry, net IDs, and layer IDs.
- Canvas scene exposes primitives in millimeter view units.
- Qt canvas renders board outline plus primitives.
- Full tests pass.

