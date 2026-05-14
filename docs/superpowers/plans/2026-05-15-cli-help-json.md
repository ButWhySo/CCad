# Sprint 12 Plan: Machine-Readable CLI Help

## Progress Line

Progress: Phase 2/6, Sprint 12, `sprint-12-cli-help-json`, planning.

## Steps

1. Write failing black-box CLI test for `ccad help --format json`.
2. Add deterministic command metadata in the CLI dispatcher.
3. Implement `helpCommand`.
4. Run focused CLI test.
5. Update README, feature docs, codebase map, sprint log, and progress counter.
6. Run full native Qt build and CTest.
7. Commit and merge to `main`.

## Verification Commands

```powershell
cmake --build build-qt --target ccad_cli_tests
ctest --test-dir build-qt -R cli --output-on-failure
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
```
