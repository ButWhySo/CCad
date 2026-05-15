# Sprint 31 Design: Layer And Object Browser

## Purpose

Sprint 31 starts the right-dock browser promised by the CAD editor shell target. The GUI should show the board layers and object inventory from the kernel canvas scene instead of a fixed placeholder list.

## Scope

The PCB canvas scene should carry enough read-only metadata for browsing layers and primitive objects. The right dock should render a grouped list with board layers followed by pads, vias, tracks, and keepouts. Pads and tracks should show their layer and net, vias should show their net, and keepouts should show their kind. This panel remains a browser, not an editor.

Selection highlighting should also stop using Qt's default bounding rectangle. Selected objects should draw a shape-level highlight over the actual pad, via, track, or keepout, matching the CAD expectation that selection emphasizes the selected geometry rather than a loose bounding box.

## Non-Goals

This sprint does not add layer visibility toggles, net highlighting, click-to-select from the browser, schematic browsing, GUI mutation, or transaction-backed editing.

## Verification

Use TDD with focused tests for canvas metadata, object browser rows, and shape-level selection highlighting. Run the full native Qt build and CTest gate before committing. Do not capture screenshots during this sprint unless the user explicitly frees the desktop for visual QA.
