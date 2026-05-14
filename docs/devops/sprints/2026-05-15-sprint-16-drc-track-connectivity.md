# Sprint 16: DRC Track Endpoint Connectivity

## Sprint Goal

Warn when a track endpoint is not attached to same-net geometry.

## Branch

`sprint-16-drc-track-connectivity`

## Progress

Progress: Phase 2/6, Sprint 16, `main`, merged and verified.

## Backlog

1. Done: add failing DRC test for dangling endpoint.
2. Done: implement same-net endpoint connectivity helper.
3. Done: run focused DRC test.
4. In progress: update docs.
5. Done: run full native Qt build and CTest.
6. Done: merge to `main`.

## Verification

RED:

```powershell
ctest --test-dir build-qt -R drc --output-on-failure
```

Result: failed at `drc reports dangling track endpoint as warning`.

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
