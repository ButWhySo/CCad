# Sprint 11 Plan: CLI Module Split

## Progress Line

Progress: Phase 2/6, Sprint 11, `sprint-11-cli-module-split`, planning.

## Steps

1. Baseline focused CLI characterization test.
2. Add Sprint 11 spec, plan, and sprint log.
3. Split `src/ccad_cli/main.cpp` into app/common/project/pcb/lib modules.
4. Update `CMakeLists.txt` for new CLI sources.
5. Run focused CLI test.
6. Update `docs/codebase-map.md`, `docs/features/implemented-features.md`, and progress docs.
7. Run full native Qt build and CTest.
8. Commit with detailed body.
9. Merge to `main` after full gate passes and update sprint counter.

## Verification Commands

```powershell
cmake --build build-qt --target ccad_cli_tests
ctest --test-dir build-qt -R cli --output-on-failure
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
```

## Commit Plan

1. `docs: plan sprint 11 cli module split`
2. `refactor: split cli command modules`
3. `docs: close sprint 11 on main`
