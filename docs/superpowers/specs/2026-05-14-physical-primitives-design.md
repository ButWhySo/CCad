# Physical Primitives Design

## Purpose

Add the first physical PCB model to CCad. This sprint establishes exact units, geometry primitives, board outline, and layer definitions as kernel data. Later GUI canvas, DRC, placement, routing, and fabrication export must build on these primitives.

## Scope

In scope:

- Integer nanometer unit type.
- Conversion helpers for mm and mil.
- `Point`, `Size`, and `Rect`.
- `Layer` and `Board`.
- Optional `Board` field on `Project`.
- Deterministic JSON round-trip for board data.

Out of scope:

- Footprints.
- Pads, vias, tracks, zones.
- DRC.
- Canvas rendering.
- Editing commands.

## Design

All physical distances are stored as integer nanometers. This mirrors the need for exact CAD state and avoids floating-point drift in source-of-truth files. User-facing commands can accept mm/mil later, but core state remains integer.

Initial model:

- `Length`: `std::int64_t nanometers`.
- `Point`: `x`, `y`.
- `Size`: `width`, `height`.
- `Rect`: `origin`, `size`.
- `Layer`: `id`, `name`, `kind`, `visible`.
- `Board`: `outline`, `layers`.

The board is optional in `Project` because logical-only projects are still valid.

## Testing

Tests must cover:

- mm to nm conversion.
- mil to nm conversion.
- rectangle max point.
- default board layers.
- project JSON round trip with board.

## Debt Boundaries

- No floating-point storage in core geometry.
- No hidden GUI geometry.
- No DRC until board primitives are stable.
- No physical object type without tests and JSON round trip.

