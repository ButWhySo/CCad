# Sprint 17: DRC Empty Via/Track Net Warnings

## Sprint Goal

Warn when vias or tracks have empty `net_id`.

## Branch

`sprint-17-drc-empty-net-warnings`

## Progress

Progress: Phase 2/6, Sprint 17, `main`, merged and verified.

## Backlog

1. Done: add failing DRC tests for empty via and track net IDs.
2. Done: implement `UNCONNECTED_VIA` and `UNCONNECTED_TRACK` warnings.
3. Done: run focused DRC test.
4. In progress: update docs.
5. Done: run full native Qt build and CTest.
6. Done: merge to `main`.

## Verification

RED:

```powershell
ctest --test-dir build-qt -R drc --output-on-failure
```

Result: failed at `drc reports unconnected via as warning`.

GREEN:

```powershell
cmake --build build-qt --target ccad_drc_tests
ctest --test-dir build-qt -R drc --output-on-failure
```

Result: `drc` passed.

Full gate before commit and after merge:

```powershell
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
```

Result: 10/10 tests passed.
