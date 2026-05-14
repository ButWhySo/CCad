# Sprint 11: CLI Module Split

## Sprint Goal

Split the LLM-facing native CLI into maintainable modules without changing command behavior.

## Branch

`sprint-11-cli-module-split`

## Progress

Progress: Phase 2/6, Sprint 11, `main`, merged and verified.

## Backlog

1. Done: run focused CLI characterization test before refactor.
2. Done: document Sprint 11 design and plan.
3. Done: split CLI entry, dispatcher, common helpers, project commands, PCB commands, and library commands.
4. Done: update CMake.
5. Done: run focused CLI test.
6. In progress: update codebase map and feature docs.
7. Done: run full native Qt build and CTest.
8. Done: merge to `main`.

## Verification

Focused characterization:

```powershell
cmake --build build-qt --target ccad_cli_tests
ctest --test-dir build-qt -R cli --output-on-failure
```

Result: `cli` passed.

Full gate before implementation commit and after merge:

```powershell
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
```

Result: 10/10 tests passed.

## Demo Artifacts

No screenshot was required for Sprint 11 because the sprint is an internal native CLI refactor. The verification artifact is the black-box CLI test suite plus the full native CTest gate.

## Definition Of Done

- `main.cpp` is entrypoint-only.
- CLI modules have clear ownership.
- Existing command syntax and exit behavior remain stable.
- Focused CLI test passes.
- Full native Qt build and CTest pass.
