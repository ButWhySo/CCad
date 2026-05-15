# Sprint 35: Object Browser Selection Link

## Scope

Sprint 35 makes the right-dock object browser participate in the same read-only selection loop as the diagnostics table. Browser object rows carry stable PCB object IDs, and activating an object row asks the canvas renderer to select the matching graphics item.

## Rationale

The GUI is still a kernel-backed review surface, so the object browser should not own project state or infer geometry. It should expose semantic object IDs derived from `CanvasScene`, then let the existing canvas selection helper perform the actual selection. This keeps the behavior compatible with future themes, plugins, and custom component creators because the browser depends on stable object identity rather than current colors, shapes, or editor implementation details.

## Definition of Done

The browser panel exposes object IDs only for selectable PCB object rows. Section, layer, status, and out-of-range rows return an empty ID. The main review window wires browser activation to `selectCanvasObjectById`, matching the diagnostics-selection path. Focused Qt tests cover the browser row metadata, and the full CMake and CTest gate passes before merge.
