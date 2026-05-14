# Sprint 8: KiCad Footprint Import

## Sprint Goal

Start KiCad library reuse by importing a narrow `.kicad_mod` footprint subset into CCad's own footprint model.

## Branch

`sprint-8-kicad-footprint-import`

## Progress

Progress: Phase 2/6, Sprint 8, `sprint-8-kicad-footprint-import`, planning.

## Backlog

1. Done: add failing importer unit tests.
2. Done: implement core footprint model and KiCad importer.
3. Done: add `ccad lib import-footprint`.
4. In progress: document KiCad reuse policy and command usage.
5. In progress: generate demo footprint-import artifact.
6. In progress: run full Qt build and CTest before every commit and before merge.
7. Pending: merge to `main`.

## KiCad Reuse Policy

- Use KiCad symbols, footprints, and 3D models as external library data.
- Keep CCad's source of truth in its own typed kernel.
- Preserve provenance in docs and future catalog metadata.
- Do not execute library file contents.
- Do not vendor large external library packs into this repo without an explicit license/provenance decision.

## Demo Artifacts

Run:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_sprint_demo.ps1 -Name sprint8-kicad-footprint-demo
```

Outputs include:

- `artifacts/demos/sprint8-kicad-footprint-demo-R_0805_2012Metric.kicad_mod`
- `artifacts/demos/sprint8-kicad-footprint-demo-R_0805_2012Metric.ccad-footprint.json`

## Definition Of Done

- CCad can parse a basic KiCad `.kicad_mod` footprint.
- Imported pads preserve number, type, shape, position, size, simple drill, and layers.
- CLI exports deterministic CCad footprint JSON.
- Unsupported KiCad constructs are ignored safely, not executed.
- Full Qt build and CTest pass before merge.
