# Sprint 15: DRC Unknown Net References

## Sprint Goal

Report physical primitives that reference non-empty net IDs missing from `project.nets`.

## Branch

`sprint-15-drc-unknown-nets`

## Progress

Progress: Phase 2/6, Sprint 15, `main`, merged and verified.

## Backlog

1. Done: add failing DRC tests for unknown pad, via, and track net IDs.
2. Done: add DRC project net lookup.
3. Done: emit typed unknown-net diagnostics.
4. Done: run focused DRC test.
5. In progress: update docs.
6. Done: run full native Qt build and CTest.
7. Done: merge to `main`.

## Verification

RED:

```powershell
ctest --test-dir build-qt -R drc --output-on-failure
```

Result: failed at `drc reports unknown pad net`.

GREEN:

```powershell
cmake --build build-qt --target ccad_drc_tests
ctest --test-dir build-qt -R drc --output-on-failure
```

Result: `drc` passed.

Focused CLI fixture correction:

```powershell
cmake --build build-qt --target ccad_cli_tests
ctest --test-dir build-qt -R cli --output-on-failure
```

Result: `cli` passed after clean-DRC fixture inserted logical `N1`.

Full gate before commit and after merge:

```powershell
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
```

Result: 10/10 tests passed.
