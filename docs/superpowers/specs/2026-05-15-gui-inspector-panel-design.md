# Sprint 30 Design: GUI Inspector Panel

## Purpose

Sprint 29 made canvas primitives selectable by stable object type and ID. Sprint 30 turns that selection feedback into a dedicated read-only inspector panel so the GUI begins to look and behave like a CAD editor instead of a status-only viewer.

## Scope

The right dock should show a selection inspector above the existing layers/object list. When nothing is selected, the inspector explains that a board object can be selected. When a stable CCad object is selected, the inspector shows a title, the object type, and the object ID. When a selectable Qt item has no stable CCad identity, the inspector reports that it is a generic canvas item.

The inspector must live in its own GUI module rather than growing `ReviewWindow`. Selection state remains display-only and continues to come from `QGraphicsItem` metadata created by the board canvas renderer.

## Non-Goals

This sprint does not add editing, multi-selection property merging, net highlighting, layer visibility toggles, diagnostic-to-object linking, or transaction-backed GUI mutation.

## Verification

Use a focused Qt widget test for the inspector model/display text, build the native GUI target, run the normal full CMake and CTest gate, and capture a selected-object screenshot through `scripts/run_sprint_demo.ps1 -ClickSelection`.
