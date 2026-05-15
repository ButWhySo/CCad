# Sprint 38: Net Highlight Groundwork

## Scope

Sprint 38 adds stable net and layer metadata to selectable Qt canvas primitives and introduces a helper for selecting every canvas object on a requested net ID.

## Rationale

The editor shell target calls for net highlight in the right-dock browser workflow. This sprint keeps the change at the renderer boundary: the kernel-derived `CanvasScene` remains the source of net and layer identity, while the GUI stores that identity as item metadata for future highlight and browser interactions.

## Definition of Done

Pads, tracks, and vias expose net metadata on their `QGraphicsItem`s, pads and tracks expose layer metadata, and selecting by net ID clears the previous selection before selecting all matching objects. Focused Qt tests cover the behavior, and the full CMake and CTest gate passes before merge.
