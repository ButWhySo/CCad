# Sprint 19 Design: Keepout Authoring And Visibility

## Goal

Make Sprint 18 rectangular keepouts usable through the machine-callable CLI and visible in the native Qt review canvas.

## Requirements

- Add `ccad pcb add-keepout`.
- Keep the command deterministic and file-mutating like other `pcb` commands.
- Reject missing boards, duplicate keepout IDs, non-positive dimensions, and keepout rectangles outside the board outline.
- Add keepouts to `CanvasScene` so Qt rendering consumes tested core data.
- Render keepouts in the Qt canvas as visible orange dashed rectangular regions.
- Update command discovery JSON and user/agent documentation.

## Non-Goals

- No clearance solver.
- No interactive GUI keepout editing.
- No layer-specific keepouts yet.
- No full track segment intersection against keepouts beyond existing DRC endpoint checks.

## Test Strategy

- CLI black-box test for help metadata, successful keepout authoring, duplicate ID rejection, and out-of-board rejection.
- Canvas unit test for keepout scene projection and unit conversion.
- Focused gate: `ctest -R "cli|canvas"`.
- Full gate before commit and merge.
