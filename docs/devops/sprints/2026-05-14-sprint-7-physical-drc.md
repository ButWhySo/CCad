# Sprint 7: Physical DRC

## Sprint Goal

Add kernel-level physical DRC diagnostics and a `ccad drc` CLI gate for first board primitives.

## Branch

`sprint-7-physical-drc`

## Progress

Progress: Phase 2/6, Sprint 7, `sprint-7-physical-drc`, planning.

## Backlog

1. Add failing kernel DRC tests.
2. Implement `ccad_core::runDrc`.
3. Add `ccad drc <path>` CLI command.
4. Update README, feature docs, and progress counter.
5. Run full Qt build and CTest before every commit and before merge.
6. Merge to `main`.

## Definition Of Done

- DRC is independent of GUI state.
- DRC returns typed diagnostics.
- CLI prints DRC diagnostics as JSON.
- DRC catches structural and basic geometric primitive errors.
- Full Qt build and CTest pass before merge.

