# Sprint 17 Plan: DRC Empty Via/Track Net Warnings

## Progress Line

Progress: Phase 2/6, Sprint 17, `sprint-17-drc-empty-net-warnings`, planning.

## Steps

1. Add failing focused DRC tests for empty via and track `net_id`.
2. Emit `UNCONNECTED_VIA` and `UNCONNECTED_TRACK` warnings.
3. Run focused DRC test.
4. Update docs and progress counter.
5. Run full native Qt build and CTest.
6. Commit, merge to `main`, and close sprint docs.

## Verification Commands

```powershell
cmake --build build-qt --target ccad_drc_tests
ctest --test-dir build-qt -R drc --output-on-failure
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
```
