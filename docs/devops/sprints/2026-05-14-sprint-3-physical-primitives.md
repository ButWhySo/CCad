# Sprint 3: Physical Units And Board Primitives

## Sprint Goal

Create the first physical PCB data model: exact units, geometry primitives, board outline, and layer definitions. This enables later DRC, canvas rendering, placement, routing, and manufacturing exports without inventing geometry ad hoc.

## Branch

`sprint-3-physical-primitives`

## Backlog

1. Add exact unit conversion helpers.
   - Status: done in `b72ac69`.
2. Add geometry primitives: point, size, rectangle.
   - Status: done in `b72ac69`.
3. Add board/layer model.
   - Status: done in `7e18214`.
4. Add board model to `Project`.
   - Status: done in `7e18214`.
5. Extend deterministic JSON serialization.
   - Status: done in `7e18214`.
6. Add tests for units, geometry, board model, and JSON round trip.
   - Status: done.
7. Update docs/features.
   - Status: in progress.
8. Run full Qt build and CTest, then merge to `main`.
   - Status: pending.

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
