# Sprint 13: Footprint Net Mapping

## Sprint Goal

Make placed footprint pads inherit `net_id` from existing logical project nets.

## Branch

`sprint-13-footprint-net-mapping`

## Progress

Progress: Phase 2/6, Sprint 13, `main`, merged and verified.

## Backlog

1. Done: write failing black-box CLI test.
2. Done: implement logical net lookup for footprint pads.
3. Done: run focused CLI test.
4. Done: update docs.
5. Done: run full native Qt build and CTest.
6. Done: merge to `main`.

## Verification

RED:

```powershell
ctest --test-dir build-qt -R cli --output-on-failure
```

Result: failed at `mapped placement assigns logical net id`.

GREEN:

```powershell
cmake --build build-qt --target ccad_cli_tests
ctest --test-dir build-qt -R cli --output-on-failure
```

Result: `cli` passed.

Full gate before commit and after merge:

```powershell
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
```

Result: 10/10 tests passed.
