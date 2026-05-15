# Sprint 34 Design: Diagnostic Overlay Markers

## Purpose

Object-linked diagnostics should be visible on the PCB canvas, not only in the table. Sprint 34 adds small read-only markers over canvas objects that have diagnostics, completing the first diagnostic review loop: review model diagnostics, table rows, canvas selection, and visual markers.

## Scope

The board canvas renderer should add marker items for diagnostics whose `object_id` matches a selectable canvas object. Marker items should store the target object ID and diagnostic severity as metadata for tests and future interactions. Error markers and warning markers should use distinct colors. The review window should render markers after rendering the board canvas.

Selection highlights should use a lighter version of each object's display color, not a fixed unrelated highlight color. This keeps pad, via, track, and keepout selection readable now and leaves room for future theme/plugin palettes.

## Non-Goals

This sprint does not add auto-centering, hover cards beyond the marker tooltip, schematic markers, marker click behavior, custom theme loading, plugin loading, or repair commands.

## Verification

Use a focused Qt renderer test that renders a canvas object, adds diagnostics, and verifies that one matching marker is created with the expected object ID and severity. Run the full native Qt build and CTest gate before committing. No screenshot should be captured while the user is using the desktop.
