# Sprint 15 Design: DRC Unknown Physical Net References

## Goal

Report physical board primitives that reference non-empty net IDs that do not exist in the logical project net list.

## Why

CCad now maps placed footprint pads to logical nets and reports unconnected pads. The next consistency check is to catch stale or mistyped physical net IDs on pads, vias, and tracks. This keeps physical geometry tied to the logical kernel and gives LLM agents typed repair targets.

## Behavior

- Pad with unknown non-empty `net_id`: error `UNKNOWN_PAD_NET`.
- Via with unknown non-empty `net_id`: error `UNKNOWN_VIA_NET`.
- Track with unknown non-empty `net_id`: error `UNKNOWN_TRACK_NET`.
- Empty pad `net_id` remains warning `UNCONNECTED_PAD`.
- Empty via/track `net_id` is allowed for now.

## Non-Goals

- Do not require all vias/tracks to be connected yet.
- Do not add unrouted-net checks.
- Do not infer net names from geometry.

## Definition Of Done

- Focused DRC tests cover unknown pad, via, and track net IDs.
- Existing DRC checks remain green.
- Feature docs and codebase map describe the new diagnostics.
- Full native Qt build and CTest pass.
