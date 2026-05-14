# Sprint 13 Design: Footprint Net Mapping

## Goal

When `pcb place-footprint` places imported footprint pads for a component that already exists in the logical project, assign each placed pad's `net_id` from the logical net membership for that component pin.

## Why

Sprint 9 and Sprint 10 placed footprint geometry correctly, but placed pads always had empty `net_id` values. That is acceptable for pure geometry demos, but it blocks schematic-layout parity and makes later DRC/routing less useful. The next conservative step is to preserve existing logical connectivity when physical pads are generated.

## Behavior

For each footprint pad:

1. Generated pad ID remains `<component>.<pad-number>`.
2. `component_id` remains the requested placement component.
3. `pin_name` remains the footprint pad number.
4. `net_id` is assigned by scanning `project.nets[*].members` for a member whose `component_id` equals the placed component and whose `pin_name` equals the footprint pad number.
5. If no matching logical net member exists, `net_id` remains empty for backward compatibility.

## Non-Goals

- Do not add symbol-to-footprint assignment yet.
- Do not require the component to exist.
- Do not reject unconnected pads yet.
- Do not infer pin swaps or aliases.

## Definition Of Done

- Black-box CLI test proves a placed footprint pad inherits `net_id` from project logical nets.
- Existing placement, rotation, import, and mutation tests still pass.
- README/features/codebase map document the behavior.
- Full native Qt build and CTest pass before commit and merge.
