# Sprint 8: KiCad Footprint Import

## Sprint Goal

Start KiCad library reuse by importing a narrow `.kicad_mod` footprint subset into CCad's own footprint model.

## Branch

`sprint-8-kicad-footprint-import`

## Progress

Progress: Phase 2/6, Sprint 8, `sprint-8-kicad-footprint-import`, planning.

## Backlog

1. Add failing importer unit tests.
2. Implement core footprint model and KiCad importer.
3. Add `ccad lib import-footprint`.
4. Document KiCad reuse policy and command usage.
5. Generate demo footprint-import artifact.
6. Run full Qt build and CTest before every commit and before merge.
7. Merge to `main`.

## Definition Of Done

- CCad can parse a basic KiCad `.kicad_mod` footprint.
- Imported pads preserve number, type, shape, position, size, simple drill, and layers.
- CLI exports deterministic CCad footprint JSON.
- Unsupported KiCad constructs are ignored safely, not executed.
- Full Qt build and CTest pass before merge.

