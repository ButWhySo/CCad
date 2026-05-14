# Footprint Placement Design

## Goal

Let agents place imported footprint pads onto a board through one semantic CLI command instead of manually adding each pad.

## Scope

Add a narrow placement flow:

```powershell
.\build-qt\ccad.exe pcb place-footprint --file board.ccad.json --footprint R_0805_2012Metric.ccad-footprint.json --component R1 --at-x-mm 10 --at-y-mm 12 --layer F.Cu
```

The command loads a board project and a CCad footprint JSON file, then creates placed board pads from footprint pads. It generates stable placed pad IDs:

```text
R1.1
R1.2
```

The initial placement transform is translation only. Rotation, side flipping, net mapping, courtyard checks, and schematic parity are later work.

## Architecture

Add footprint JSON loading to the existing footprint module. Keep library footprint pads separate from placed board pads:

- `FootprintPad`: reusable package geometry around local origin.
- `Pad`: placed board geometry with component ID, pin name, net ID, layer ID, absolute position, and size.

Add a CLI command that maps footprint pads into board pads:

- `component_id = --component`
- `pin_name = footprint pad number`
- `net_id = ""` for now
- `layer_id = --layer`
- `position = placement origin + footprint pad local position`
- `size = footprint pad size`

## Validation

- Project must contain a board.
- Footprint JSON must be readable and parseable.
- Board layer must exist.
- Generated pad IDs must not collide with existing board pads.
- All generated pad positions must be inside the board outline.
- Footprint must contain at least one pad.

## Test Strategy

- Unit test footprint JSON round-trip loading.
- CLI test imports a sample KiCad footprint, creates a board, runs `place-footprint`, and verifies placed pads appear in project JSON.
- Negative CLI checks cover unknown layer and duplicate placement.

