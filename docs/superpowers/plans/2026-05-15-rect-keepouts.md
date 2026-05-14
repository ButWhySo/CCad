# Sprint 18 Plan: Rectangular Keepouts

## Progress Line

Progress: Phase 2/6, Sprint 18, `sprint-18-rect-keepouts`, planning.

## Steps

1. Add failing serialization and DRC tests for rectangular keepouts.
2. Add `Keepout` model and `Board::keepouts`.
3. Implement keepout JSON read/write.
4. Implement DRC checks for pad, via, and track endpoints inside keepouts.
5. Run focused serialize/DRC tests.
6. Update docs and progress counter.
7. Run full native Qt build and CTest.
8. Commit, merge to `main`, and close sprint docs.

## Verification Commands

```powershell
cmake --build build-qt --target ccad_tests ccad_drc_tests
ctest --test-dir build-qt -R "serialize|drc" --output-on-failure
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
```
