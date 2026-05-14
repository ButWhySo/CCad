# Sprint 16 Plan: DRC Track Endpoint Connectivity

## Progress Line

Progress: Phase 2/6, Sprint 16, `sprint-16-drc-track-connectivity`, planning.

## Steps

1. Add failing focused DRC test for dangling track endpoint.
2. Implement endpoint connectivity helper.
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
