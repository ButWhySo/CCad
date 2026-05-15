# Sprint 32 Design: Diagnostic Selection Link

## Purpose

Diagnostics should become part of the CAD review loop. When a diagnostic names an object, the GUI should be able to select that object on the PCB canvas by stable ID rather than relying on pixel hunting.

## Scope

The diagnostics panel should expose the stable object ID stored in each row. The canvas renderer should provide a small selection helper that clears the old selection, finds a selectable item by stable object ID, and selects it. The review window should connect diagnostic row clicks to that helper. This is groundwork for richer diagnostic centering and overlay markers.

## Non-Goals

This sprint does not add DRC diagnostics to the review model, diagnostic marker overlays, auto-centering, schematic diagnostic links, or edit/fix commands.

## Verification

Use focused Qt widget tests for diagnostics row object IDs and canvas select-by-ID behavior. Run the full native Qt build and CTest gate before committing. Do not capture screenshots unless the user explicitly frees the desktop.
