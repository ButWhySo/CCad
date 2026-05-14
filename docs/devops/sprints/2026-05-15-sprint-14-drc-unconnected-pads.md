# Sprint 14: DRC Unconnected Pad Warning

## Sprint Goal

Report pads with empty `net_id` as DRC warnings.

## Branch

`sprint-14-drc-unconnected-pads`

## Progress

Progress: Phase 2/6, Sprint 14, `main`, merged and verified.

## Backlog

1. Done: write failing DRC unit test.
2. Done: implement `UNCONNECTED_PAD` warning.
3. Done: run focused DRC test.
4. In progress: update docs.
5. Done: run full native Qt build and CTest.
6. Done: merge to `main`.

## Verification

RED:

```powershell
ctest --test-dir build-qt -R drc --output-on-failure
```

Result: failed at `drc reports unconnected pad as warning`.

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
