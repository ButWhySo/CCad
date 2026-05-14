# Sprint 14 Design: DRC Unconnected Pad Warning

## Goal

Report placed pads with empty `net_id` as DRC warnings.

## Why

CCad now supports footprint placement, rotation, and logical net mapping. Any remaining empty pad net is useful information for an LLM or human reviewer, but it should not be an error yet because early physical experiments and imported footprints may intentionally be incomplete.

## Behavior

- A pad with an empty `net_id` produces:
  - `severity`: `warning`
  - `code`: `UNCONNECTED_PAD`
  - `object_id`: pad ID
- `ccad drc` still exits `0` when only warnings are present because exit code `1` is reserved for error diagnostics.

## Non-Goals

- Do not require every physical pad to be represented by a logical component pin yet.
- Do not add unrouted-net analysis.
- Do not add clearance checks in this sprint.

## Definition Of Done

- Unit test proves `UNCONNECTED_PAD` has warning severity.
- Existing DRC errors remain errors.
- Feature docs explain the warning and exit-code behavior.
- Full native Qt build and CTest pass.
