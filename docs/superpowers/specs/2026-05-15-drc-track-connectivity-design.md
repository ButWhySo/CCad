# Sprint 16 Design: DRC Track Endpoint Connectivity

## Goal

Warn when a track endpoint does not touch any same-net pad center, via center, or other track endpoint.

## Why

CCad now validates logical net references on physical primitives. The next useful agent-facing signal is whether a routed segment is actually attached to same-net geometry. This is an early connectivity check, not a full copper solver.

## Behavior

- Diagnostic code: `UNCONNECTED_TRACK_ENDPOINT`.
- Severity: `warning`.
- Object ID: track ID.
- A track endpoint is considered connected if it exactly matches:
  - a same-net pad center,
  - a same-net via center,
  - a same-net start or end point of another track.
- Tracks with empty `net_id` are skipped for this check.

## Non-Goals

- Do not implement full trace-shape intersection.
- Do not detect pad area overlap yet.
- Do not solve connectivity through copper zones.
- Do not make dangling endpoints errors yet.

## Definition Of Done

- Focused DRC test proves a dangling endpoint is reported as warning.
- Existing valid board fixture remains clean.
- Full native Qt build and CTest pass.
