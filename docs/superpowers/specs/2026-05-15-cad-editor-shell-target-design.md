# CAD Editor Shell Target

## Why This Exists

The current GUI began as a review dashboard. That was useful for early validation, but it is not the product target. CCad needs a native desktop CAD editor shell: a large persistent canvas, editor-grade navigation, docked panels, diagnostics, object inspection, and eventually transaction-backed editing.

## Reference UI Direction

Use KiCad and Altium as layout references, not as implementation dependencies. The key shared pattern is a central design space surrounded by toolbars, docked panels, object/layer controls, and status readouts. CCad must keep the kernel and CLI as source of truth, so the GUI is a renderer/editor client over the transaction bus.

Reference sources gathered for visual/UI study:

- KiCad PCB Editor documentation: https://docs.kicad.org/
- KiCad Schematic Editor documentation: https://docs.kicad.org/
- Altium Designer design environment documentation: https://www.altium.com/documentation/altium-designer
- Altium PCB panel documentation: https://www.altium.com/documentation/altium-designer/pcb-panel

## Target Layout

- `QMainWindow` shell.
- Central `QTabWidget` with PCB first and schematic later.
- PCB tab is dominated by a `QGraphicsView` canvas.
- Left rail: select, pan, measure, place, route, inspect tools.
- Right docks: layers, objects, nets, properties.
- Bottom dock: diagnostics and transaction/command log.
- Status bar: cursor mm, grid, zoom percent, active layer, active tool, selected object ID, verification state.

## Canvas Expectations

- Large scene rect around the board, not a tiny auto-fit preview.
- Wheel zoom centered near cursor.
- Middle-button or hand-mode pan.
- Explicit fit-to-board action.
- Zoom and pan persist across resize and reload unless the user requests fit.
- Grid and scale remain legible.
- Board primitives remain rendered from `ccad_core::CanvasScene`; Qt item positions never become source truth.

## Next GUI Sprints

1. CAD shell foundation: split `ReviewWindow`, make PCB canvas central, add real zoom/pan/fit/status readouts.
2. Selection and inspector: stable object IDs on `QGraphicsItem`s and a read-only properties dock.
3. Layers/nets/object browser: layer visibility, net highlight, grouped object tree.
4. Diagnostics overlay: click diagnostic to select/center object, draw warning/error markers.
5. Transaction timeline: read-only command/diff log before enabling GUI mutation.

## Visual QA

Every GUI sprint must produce screenshots under ignored `artifacts/screenshots/`:

- Desktop viewport around 1280x800 or larger.
- Smaller viewport around 900x600.
- At least one interaction state when the sprint adds interaction, such as zoomed pad, selected object, or highlighted net.
