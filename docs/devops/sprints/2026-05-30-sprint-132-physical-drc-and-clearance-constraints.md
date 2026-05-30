# Sprint 132: Physical DRC and Clearance Constraints Editing

## Goal

Provide interactive UI controls for editing design rules (copper clearance, track width, via annular ring) and physical object properties (pad dimensions/rotation, via diameter/drill, track width, keepout/region area). Wire callback handlers in `ReviewWindow` to mutate the kernel-mediated `Board` model, save updates back to the project file, and trigger canvas/diagnostics re-rendering.

## Branch

`sprint-132-physical-drc-and-clearance-constraints`

## References Checked

KiCad's Properties dialogs allow modifying physical dimensions of tracks, vias, pads, keepouts, and board rules in real-time, instantly recalculating DRC violations. CCad implements this by attaching modification callbacks to the Qt Selection Inspector Panel, committing mutated properties back to the project JSON file, and reloading the review diagnostics and canvas renderer to reflect visual changes and DRC errors immediately.

## Tasks

1. Implement input fields (QLineEdits) and Apply buttons in `SelectionInspectorPanel` for board rules, pads, vias, tracks, keepouts, and placement regions.
2. Implement callback setters on `SelectionInspectorPanel` for the various object type mutations.
3. Plumb the lambda callbacks in `ReviewWindow` constructor to:
   - Mutate the `project_cache_.board` with user-entered values.
   - Write updated project JSON to disk using the file I/O layer.
   - Trigger `renderReview()` to update DRC diagnostics, layers/objects browser, and canvas scene.
   - Re-select the updated object in the scene to preserve focus/inspector display.
4. Upgrade `SelectionInspectorPanel::rowText` helper to search form inputs (QLineEdits), enabling unit tests to verify updated values.
5. Update unit tests in `tests/test_gui_inspector_panel.cpp` to expect QLineEdit values and assert design rules fields.
6. Build and run CTest suite to verify 100% test pass rate.
7. Run `scripts/run_sprint_demo.ps1` with the Powershell execution bypass to verify visual layout and screenshot generation.
8. Update progress tracker `docs/devops/progress.md`.

## Bugs And Risks

- **Focus Loss**: Clearing and rebuilding the canvas scene deletes current QGraphicsItems, losing selection state. Mitigated by calling `selectCanvasObjectById` with the updated ID right after `renderReview`.
- **Invalid Dimensions**: Users might enter zero, negative numbers, or invalid formats (like non-numeric strings or via drills larger than diameters). Mitigated by client-side verification and friendly warning dialogs (`QMessageBox::warning`) before invoking callbacks.

## Verification

- **Unit Tests**: Executed `ctest` with 22/22 passing tests.
- **Visual Smoke Test**: Executed `scripts/run_sprint_demo.ps1` to produce fresh screenshot `artifacts/screenshots/sprint-demo-20260530-235551.png`.

## Closure

- **Status**: Closed
- **Next Sprint Target**: Interoperability, export valid KiCad files.
- **Expected Sprints to Complete Phase 4**: 0 sprints (Phase 4 complete).
