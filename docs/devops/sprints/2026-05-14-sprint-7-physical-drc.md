# Sprint 7: Physical DRC

## Sprint Goal

Add kernel-level physical DRC diagnostics and a `ccad drc` CLI gate for first board primitives.

## Branch

`sprint-7-physical-drc`

## Progress

Progress: Phase 2/6, Sprint 7, `sprint-7-physical-drc`, planning.

## Backlog

1. Done: add failing kernel DRC tests.
2. Done: implement `ccad_core::runDrc`.
3. Done: add `ccad drc <path>` CLI command.
4. In progress: update README, feature docs, progress counter, and demo script.
5. In progress: run full Qt build and CTest before every commit and before merge.
6. Pending: merge to `main`.

## Definition Of Done

- DRC is independent of GUI state.
- DRC returns typed diagnostics.
- CLI prints DRC diagnostics as JSON.
- DRC catches structural and basic geometric primitive errors.
- Full Qt build and CTest pass before merge.

## Demo Artifacts

Run:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_sprint_demo.ps1 -Name sprint7-drc-demo
```

Outputs:

- `artifacts/demos/sprint7-drc-demo.ccad.json`
- `artifacts/demos/sprint7-drc-demo.inspect.json`
- `artifacts/demos/sprint7-drc-demo.validate.json`
- `artifacts/demos/sprint7-drc-demo.drc.json`
- `artifacts/screenshots/sprint7-drc-demo-<timestamp>.png`
