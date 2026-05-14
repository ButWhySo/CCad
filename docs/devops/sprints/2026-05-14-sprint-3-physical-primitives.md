# Sprint 3: Physical Units And Board Primitives

## Sprint Goal

Create the first physical PCB data model: exact units, geometry primitives, board outline, and layer definitions. This enables later DRC, canvas rendering, placement, routing, and manufacturing exports without inventing geometry ad hoc.

## Branch

`sprint-3-physical-primitives`

## Backlog

1. Add exact unit conversion helpers.
2. Add geometry primitives: point, size, rectangle.
3. Add board/layer model.
4. Add board model to `Project`.
5. Extend deterministic JSON serialization.
6. Add tests for units, geometry, board model, and JSON round trip.
7. Update docs/features.
8. Run full Qt build and CTest, then merge to `main`.

## Definition Of Done

- Units are integer nanometers internally.
- mm and mil conversions are deterministic and tested.
- Board outline is represented without GUI state.
- Layers have IDs, names, kinds, and visibility.
- Project JSON round-trips board data.
- No GUI-only or CLI-only physical state exists.

## Local CI

```powershell
cmake -S . -B build-qt -DCCAD_WARNINGS_AS_ERRORS=ON -DCCAD_BUILD_GUI=ON -DCMAKE_PREFIX_PATH=C:\Qt\6.11.1\mingw_64
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
```

