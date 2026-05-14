# Sprint 15 Plan: DRC Unknown Net References

## Progress Line

Progress: Phase 2/6, Sprint 15, `sprint-15-drc-unknown-nets`, planning.

## Steps

1. Add failing focused DRC tests for unknown pad/via/track net IDs.
2. Add project net lookup in DRC.
3. Emit typed diagnostics for unknown physical net references.
4. Run focused DRC test.
5. Update docs and progress counter.
6. Run full native Qt build and CTest.
7. Commit, merge to `main`, and close sprint docs.

## Verification Commands

```powershell
cmake --build build-qt --target ccad_drc_tests
ctest --test-dir build-qt -R drc --output-on-failure
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
```
