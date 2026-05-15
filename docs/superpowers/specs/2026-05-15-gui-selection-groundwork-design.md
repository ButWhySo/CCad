# Sprint 29 Design: GUI Selection Groundwork

## Purpose

The GUI needs stable object selection before it can support inspectors, layer/object browsers, diagnostic linking, or editing. Sprint 29 adds read-only selection metadata to rendered canvas primitives.

## Scope

- Mark rendered pads, vias, tracks, and keepouts as selectable `QGraphicsItem`s.
- Store stable object type and object ID on each selectable item.
- Show the selected object in the status bar.
- Show the selected object in the visible right-side Layers / Objects dock.
- Add an optional demo-script click for screenshot proof.

## Non-Goals

- No project mutation.
- No inspector panel yet.
- No multi-selection behavior beyond Qt defaults.
- No diagnostic-to-object linking yet.
- No layer visibility toggles yet.

## Visual QA

Use the demo script with `-ClickSelection` and confirm the screenshot shows a selected object ID in the right dock.
