# Sprint 28 Design: GUI Module Split

## Purpose

Sprint 27 made the GUI look more like a CAD editor shell, but `ReviewWindow` still owned too many responsibilities. Sprint 28 splits stable panel responsibilities into dedicated widgets before adding selection, inspectors, or layer controls.

## Scope

- Extract project summary/status cards into `ProjectSummaryPanel`.
- Extract diagnostics table rendering into `DiagnosticsPanel`.
- Keep `ReviewWindow` as shell orchestration: docks, tabs, menus, toolbar, project loading, and canvas wiring.
- Preserve existing GUI behavior and screenshot shape.
- Do not add selection or editing in this sprint.

## Module Ownership

- `ReviewWindow`: top-level shell, file actions, dock placement, status bar, canvas wiring.
- `ProjectSummaryPanel`: project title, subtitle, status chip, component/net/layer/diagnostic counts.
- `DiagnosticsPanel`: diagnostics table setup and row rendering.
- `BoardCanvasView`: viewport navigation.
- `BoardCanvasRenderer`: scene item construction only.

## Verification

- Build `ccad_gui`.
- Run full native Qt CTest gate.
- Capture a screenshot because this touches GUI layout code.
