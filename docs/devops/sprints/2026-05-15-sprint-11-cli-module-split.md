# Sprint 11: CLI Module Split

## Sprint Goal

Split the LLM-facing native CLI into maintainable modules without changing command behavior.

## Branch

`sprint-11-cli-module-split`

## Progress

Progress: Phase 2/6, Sprint 11, `sprint-11-cli-module-split`, planning.

## Backlog

1. Done: run focused CLI characterization test before refactor.
2. Done: document Sprint 11 design and plan.
3. Done: split CLI entry, dispatcher, common helpers, project commands, PCB commands, and library commands.
4. Done: update CMake.
5. Done: run focused CLI test.
6. In progress: update codebase map and feature docs.
7. Pending: run full native Qt build and CTest.
8. Pending: merge to `main`.

## Definition Of Done

- `main.cpp` is entrypoint-only.
- CLI modules have clear ownership.
- Existing command syntax and exit behavior remain stable.
- Focused CLI test passes.
- Full native Qt build and CTest pass.
