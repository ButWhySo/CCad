# Sprint 20 Design: Track Keepout Crossing DRC

## Goal

Detect straight track segments that cross a rectangular keepout even when both endpoints are outside the keepout.

## Requirements

- Add typed DRC diagnostic `TRACK_CROSSES_KEEPOUT`.
- Preserve existing `TRACK_ENDPOINT_IN_KEEPOUT` behavior for endpoints inside keepouts.
- Keep the geometry helper deterministic, local, and independent of Qt.
- Use portable C++20 accepted by the current MinGW `-Wpedantic -Werror` build.

## Non-Goals

- No clearance expansion around keepout boundaries.
- No rounded/polygon keepouts.
- No track width versus keepout clearance calculation yet.
- No GUI change required; this is a DRC rule improvement.

## Test Strategy

- Add a failing `drc` test with a horizontal track crossing a rectangular keepout while both endpoints remain outside.
- Run focused DRC test.
- Run full native gate before commit and merge.
