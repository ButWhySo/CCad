# CLI PCB Authoring Design

## Goal

Add the first machine-callable PCB authoring commands so agents can create pads, vias, and track segments without editing raw JSON by hand.

## Scope

This sprint adds deterministic CLI mutations for existing `.ccad.json` project files:

- `ccad pcb add-pad`
- `ccad pcb add-via`
- `ccad pcb add-track`

Each command loads a project, validates arguments, mutates the kernel model, and writes the updated project back to disk. The command surface is intentionally narrow and explicit. It is not an interactive editor and does not attempt placement, routing, DRC, or schematic-layout parity yet.

## Command Shape

```powershell
.\build-qt\ccad.exe pcb add-pad --file board.ccad.json --id P1 --component U1 --pin 1 --net N1 --layer F.Cu --x-mm 5 --y-mm 6 --width-mm 1.5 --height-mm 1.0
.\build-qt\ccad.exe pcb add-via --file board.ccad.json --id V1 --net N1 --x-mm 8 --y-mm 9 --diameter-mm 0.8 --drill-mm 0.4
.\build-qt\ccad.exe pcb add-track --file board.ccad.json --id T1 --net N1 --layer F.Cu --start-x-mm 5 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9 --width-mm 0.25
```

## Validation Rules

- `--file` must point to a readable and writable project file.
- The project must already contain a board.
- IDs must be non-empty and unique within their primitive type.
- Numeric dimensions must be positive.
- Positions and track endpoints must be inside the board outline.
- `--layer` must exist on the board for pads and tracks.
- Via drill must be smaller than or equal to via diameter.

These checks are CLI mutation guards, not full DRC. Full physical DRC remains a later sprint.

## Architecture

Keep parsing and mutation simple inside `src/ccad_cli/main.cpp` for now because the command set is still small. Do not put GUI logic in the CLI. Do not shell out. Project files remain data only.

If CLI authoring grows beyond a few verbs, move reusable mutation helpers into a dedicated `ccad_core` module.

## Test Strategy

Extend `tests/test_cli.cpp` first. Tests should create a board with `init`, run each `pcb add-*` command, then inspect the resulting JSON text for the expected primitive fields. Negative tests should cover missing board, duplicate primitive ID, invalid dimensions, unknown layer, endpoint outside board, and via drill larger than diameter.

Full verification remains:

```powershell
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
```
