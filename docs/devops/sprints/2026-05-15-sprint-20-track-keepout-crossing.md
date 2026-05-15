# Sprint 20: Track Keepout Crossing DRC

## Sprint Goal

Report DRC errors when a straight track segment crosses a rectangular keepout even if neither endpoint is inside the keepout.

## Branch

`sprint-20-track-keepout-crossing`

## Progress

Progress: Phase 2/6, Sprint 20, `main`, merged and verified.

## Backlog

1. Done: add failing DRC test.
2. Done: implement track segment versus keepout rectangle intersection.
3. Done: run focused DRC test.
4. In progress: update docs.
5. Pending: run full native Qt build and CTest.
6. Done: merge to `main`.

## Verification So Far

RED:

```cmd
cmake --build build-qt --target ccad_drc_tests
ctest --test-dir build-qt -R drc --output-on-failure
```

- Result: failed with `test failure: drc reports track crossing keepout with endpoints outside`.

GREEN:

```cmd
cmake --build build-qt --target ccad_drc_tests
ctest --test-dir build-qt -R drc --output-on-failure
```

- Result: focused DRC test passed after implementation.

Full feature-branch gate:

```cmd
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
```

- Result: 10 / 10 tests passed before commit.

Main integration gate:

```cmd
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
```

- Result: 10 / 10 tests passed after merging Sprint 20 to `main`.

## Demo

- No screenshot required. This sprint changes DRC diagnostics only; there is no GUI rendering change.
