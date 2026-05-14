# Sprint 14 Plan: DRC Unconnected Pad Warning

## Progress Line

Progress: Phase 2/6, Sprint 14, `sprint-14-drc-unconnected-pads`, planning.

## Steps

1. Write failing DRC unit test for empty pad `net_id`.
2. Add warning diagnostic generation in `runDrc`.
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
