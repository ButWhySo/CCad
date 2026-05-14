# Sprint 13 Plan: Footprint Net Mapping

## Progress Line

Progress: Phase 2/6, Sprint 13, `sprint-13-footprint-net-mapping`, planning.

## Steps

1. Write failing black-box CLI test for `pcb place-footprint` net assignment from logical nets.
2. Implement helper to resolve component pin to net ID.
3. Use the helper during footprint pad placement.
4. Run focused CLI tests.
5. Update docs and progress counter.
6. Run full native Qt build and CTest.
7. Commit, merge to `main`, and close sprint docs.

## Verification Commands

```powershell
cmake --build build-qt --target ccad_cli_tests
ctest --test-dir build-qt -R cli --output-on-failure
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
```
