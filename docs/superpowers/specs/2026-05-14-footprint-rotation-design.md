# Footprint Rotation Design

## Goal

Add rotation-aware footprint placement so agents can place imported footprints at an angle and the GUI can render rotated pads.

## Scope

Extend board pads and canvas pads with `rotation_degrees`. Add optional CLI flag:

```powershell
.\build-qt\ccad.exe pcb place-footprint --file board.ccad.json --footprint R_0805_2012Metric.ccad-footprint.json --component R1 --at-x-mm 16 --at-y-mm 14 --layer F.Cu --rotation-deg 90
```

Placement uses a standard 2D transform around the footprint origin:

```text
x' = origin_x + x*cos(theta) - y*sin(theta)
y' = origin_y + x*sin(theta) + y*cos(theta)
pad_rotation = footprint_pad_rotation + placement_rotation
```

Existing commands keep default rotation `0`.

## Architecture

- Add `rotation_degrees` to `ccad::Pad`.
- Serialize/deserialize pad rotation in project JSON.
- Add rotation to `CanvasPad`.
- Render pads in Qt with a transformed painter path.
- Add `--rotation-deg` to `pcb place-footprint`.

Do not add component-instance objects yet. This sprint keeps board pads as the placement output to stay compatible with current renderer and DRC.

## Validation

- `--rotation-deg` must parse as a number.
- Rotated pad centers must remain inside board outline.
- Existing duplicate pad and layer checks still apply.

## Test Strategy

- Serialization test covers pad rotation round-trip.
- Canvas test covers pad rotation propagation.
- CLI test places a footprint with `--rotation-deg 90` and checks transformed pad coordinates.
- Full Qt build and CTest before every commit.

