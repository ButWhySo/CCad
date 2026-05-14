# Sprint 4: Native Board Canvas Prototype

## Sprint Goal

Render the physical board outline in the Qt GUI from kernel state. This creates the first CAD-style visual canvas while keeping all scene data deterministic and testable.

## Branch

`sprint-4-board-canvas`

## Backlog

1. Add tested canvas view-model in `ccad_core`.
2. Render board outline in Qt GUI using `QGraphicsView`.
3. Add empty-state canvas when no board exists.
4. Preserve review panels and diagnostics.
5. Document how to run the GUI demo.
6. Run full Qt build and CTest, then merge.

## Definition Of Done

- Canvas scene data is produced by `ccad_core`, not invented by GUI.
- A board created with `ccad init --width-mm --height-mm` appears as a scaled rectangle.
- GUI remains read-only.
- Tests cover canvas dimensions and empty-board behavior.
- Full Qt build and CTest pass.

