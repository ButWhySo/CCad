# Sprint 37: Transaction Timeline Panel

## Scope

Sprint 37 adds a read-only transaction timeline panel to the native Qt GUI. The panel is placed beside diagnostics in the bottom dock and displays transaction ID, command, summary, and compact diff counts.

## Rationale

The editor shell target calls for a transaction or command log before enabling GUI mutation. This sprint adds the UI surface without changing project state, replaying transactions, or making the GUI the source of truth.

## Definition of Done

The timeline panel renders an empty state, renders transaction metadata from `ccad_core::Transaction`, exposes stable transaction IDs for future row linking, and is wired into the main review window. Focused Qt tests cover the panel behavior, a GUI screenshot is captured when the user allows it, and the full CMake and CTest gate passes before merge.
