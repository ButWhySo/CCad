# Sprint 27 Design: CAD Editor Shell Foundation

## Purpose

The GUI must stop behaving like a dashboard with a tiny board preview. Sprint 27 reshapes it into the first version of the native CAD editor shell: central PCB canvas, dock panels, explicit fit, persistent viewport behavior, and editor status readouts.

## Scope

- Keep the GUI read-only.
- Keep `ccad_core::CanvasScene` as the render input.
- Make the PCB canvas the central widget inside editor tabs.
- Add left/right/bottom dock structure using `QMainWindow`.
- Move diagnostics into a bottom dock.
- Add project summary in a side dock.
- Add placeholder layers/objects dock.
- Add disabled schematic tab to establish the future multi-editor shell.
- Add wheel zoom, middle-button pan, explicit Fit, cursor coordinates, and zoom status.
- Stop renderer-controlled `fitInView`; viewport owns fit/zoom/pan.

## Non-Goals

- No GUI mutation of project files.
- No selection model yet.
- No layer visibility toggles yet.
- No schematic renderer yet.
- No transaction timeline yet.

## Visual QA

- Capture a native GUI screenshot after building.
- Board must occupy the central workspace, not appear as a tiny preview.
- Diagnostics must be docked below the canvas.
- Project summary and layer/object panels must not overlap the canvas.
- Fit must recover full-board view after resize.
