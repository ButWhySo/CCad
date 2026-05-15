# Sprint 36: Canvas Render Theme Groundwork

## Scope

Sprint 36 introduces an explicit `CanvasRenderTheme` for the Qt board canvas renderer. The default theme preserves the current visual appearance, while tests prove that custom track and pad colors also drive the lighter shape-selection highlight.

## Rationale

The GUI needs to stay compatible with future themes, plugins, custom component creators, and schematic/PCB phase-specific renderers. Presentation choices should be passed as presentation data, not buried as hard-coded meaning in the renderer. The kernel still owns geometry, object identity, and validation.

## Definition of Done

The board renderer keeps the existing `renderBoardCanvas(scene)` API for current callers and adds a themed overload for future customization. Diagnostic marker rendering also accepts the same theme type. Focused Qt tests cover custom theme colors for selection highlights, and the full CMake and CTest gate passes before merge.
