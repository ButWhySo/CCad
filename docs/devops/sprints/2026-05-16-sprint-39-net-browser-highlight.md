# Sprint 39: Net Browser Highlight

## Scope

Sprint 39 is a moderate-size GUI branch. It builds on Sprint 38 net metadata by adding net rows to the right-dock object browser and wiring those rows to canvas net selection.

## Tasks

- Add a `Nets` section in `ObjectBrowserPanel` derived from `CanvasScene` pad, via, and track net IDs.
- Keep object row activation separate from net row activation.
- Wire net row activation in `ReviewWindow` to `selectCanvasObjectsByNetId`.
- Record the user's sprint-sizing preference so future work batches related GUI affordances before expensive full compile gates.

## Definition of Done

Focused Qt tests cover net rows, net IDs, object row activation, and net row activation. The branch uses focused builds during implementation and one full clean CMake/CTest gate before merge, then one post-merge gate on `main`.
