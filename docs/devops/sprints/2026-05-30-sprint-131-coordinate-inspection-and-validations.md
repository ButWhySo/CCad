# Sprint 131: Coordinate Inspection Helpers and Physical Unit Validations

## Goal

Implement coordinate inspection helpers in the `SelectionInspectorPanel` to display unit-converted physical properties (position, dimension, net, layer, rotation, drill, track length, etc.) for selected PCB objects (pads, vias, tracks, keepouts, placement regions) based on the kernel-mediated `Board` model. Display all physical lengths in both millimeters (mm) and mils (mil).

## Branch

`sprint-131-coordinate-inspection-and-validations`

## References Checked

KiCad's Properties Inspector displays selected board items with their physical properties: Pads show component pin, net, layer, coordinates, shape dimensions, and orientation/rotation; Vias show net, coordinates, size (diameter), and drill size; Tracks show layer, width, endpoints, and length. Both metric (mm) and imperial (mil) units are standard. CCad implements these inspectors by querying the kernel's board model and converting internal nanometer measurements at the UI boundary.

## Tasks

1. Create a helper method in `SelectionInspectorPanel` to format length values into `X.XX mm (Y.YY mil)` format.
2. Implement `clearExtraRows()` in `SelectionInspectorPanel` to clean up old type-specific rows when switching selection.
3. Add a `renderSelection` overload to `SelectionInspectorPanel` accepting `const std::optional<ccad::Board>& board, const QString& type, const QString& id`.
4. Implement type-specific property inspection for pads, vias, tracks, keepouts, and placement regions.
5. Update `ReviewWindow::updateSelectionStatus` to pass the board model cache.
6. Auto-select a default pad upon loading a project to ensure the selection inspector shows properties in screenshot outputs.
7. Implement unit tests in `tests/test_gui_inspector_panel.cpp` verifying correct formatting and display of all rows.
8. Execute a clean Qt build and ensure CTest suite passes.
9. Execute `scripts/run_sprint_demo.ps1` to produce a fresh GUI rendering and screenshot demonstrating coordinate inspection.
10. Update progress tracker `docs/devops/progress.md`.

## Bugs And Risks

- **Row Accumulation**: Dynamically adding rows in `SelectionInspectorPanel` can leave stale properties (like track endpoints on a pad selection). Mitigated by clearing extra rows before populating new ones.
- **Null Safety**: Board cache may be uninitialized or selected object may not be found in the board list. Mitigated by checking `std::optional::has_value` and falling back to basic ID info.

## Verification

- **Unit Tests**: Add tests verifying row rendering for Pad, Via, Track, Keepout, and Placement Region.
- **Visual Smoke Test**: Run the screenshot demo script to verify correct layout formatting in the inspector.
- **Test Gate**: Clean build and pass CTest tests.

## Closure

- **Status**: Closed
- **Next Sprint Target**: Physical DRC constraints editing and CLI clearances.
- **Expected Sprints to Complete Phase 4**: 1 sprint.
