# Sprint 18: Rectangular Keepouts

## Sprint Goal

Add rectangular board keepouts to model, JSON, and DRC.

## Branch

`sprint-18-rect-keepouts`

## Progress

Progress: Phase 2/6, Sprint 18, `main`, merged and verified.

## Backlog

1. Done: add failing serialization and DRC tests.
2. Done: add `Keepout` model and `Board::keepouts`.
3. Done: implement keepout JSON read/write.
4. Done: implement DRC checks for keepout violations.
5. Done: run focused serialize/DRC tests.
6. Done: update docs.
7. Done: run full native Qt build and CTest.
8. Done: merge to `main`.

## Verification

RED:

- Focused build failed before implementation because `Keepout` and `Board::keepouts` did not exist.

GREEN:

```powershell
cmake --build build-qt --target ccad_tests ccad_drc_tests
ctest --test-dir build-qt -R "serialize|drc" --output-on-failure
```

- Result: focused serialize/DRC tests passed after implementation.

Full feature-branch gate:

```powershell
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
```

- Result: 10 / 10 tests passed before feature/docs commits.

Main integration gate:

```powershell
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
```

- Result: 10 / 10 tests passed after merging Sprint 18 to `main`.

## Demo

- No GUI screenshot was captured for this sprint because rectangular keepouts are kernel/JSON/DRC behavior only in Sprint 18.
- GUI rendering and CLI authoring for keepouts are intentionally deferred to a later sprint.
