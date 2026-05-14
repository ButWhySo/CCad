# Sprint 12: Machine-Readable CLI Help

## Sprint Goal

Add deterministic JSON command discovery for LLM/tool callers.

## Branch

`sprint-12-cli-help-json`

## Progress

Progress: Phase 2/6, Sprint 12, `main`, merged and verified.

## Backlog

1. Done: write failing black-box CLI test.
2. Done: implement `ccad help --format json`.
3. Done: update docs.
4. Done: run focused CLI test.
5. Done: run full native Qt build and CTest.
6. Done: merge to `main`.

## Verification

RED:

```powershell
ctest --test-dir build-qt -R cli --output-on-failure
```

Result: failed because `help` was an unknown command.

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

## Demo

Run:

```powershell
.\build-qt\ccad.exe help --format json
```

Expected result: deterministic JSON with a `commands` array, including `pcb place-footprint` and `--rotation-deg`.
