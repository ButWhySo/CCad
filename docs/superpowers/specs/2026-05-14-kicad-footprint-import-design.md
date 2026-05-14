# KiCad Footprint Import Design

## Goal

Start reusing KiCad's official footprint ecosystem by importing a narrow, deterministic subset of `.kicad_mod` footprint files into CCad's own footprint model.

## Research Basis

KiCad documents footprint libraries as s-expression files with the `.kicad_mod` extension. A footprint library file defines a single footprint. KiCad also publishes separate library sets for schematic symbols, PCB footprints, 3D models, 3D model sources, and templates. This sprint uses KiCad footprint files as data inputs only; CCad remains the runtime kernel.

Sources:

- https://dev-docs.kicad.org/en/file-formats/sexpr-footprint/index.html
- https://dev-docs.kicad.org/en/file-formats/sexpr-intro/
- https://kicad.io/libraries/download/
- https://klc.kicad.org/

## Scope

Add a core footprint data model and a narrow KiCad footprint importer for common pad geometry:

- footprint name
- pad number/name
- pad type token, such as `smd` or `thru_hole`
- pad shape token, such as `rect`, `roundrect`, or `circle`
- `(at x y [rotation])`
- `(size width height)`
- `(drill diameter)` for simple through-hole pads
- `(layers ...)`

Unsupported KiCad constructs are skipped safely for now:

- graphics, text, zones, groups, properties
- custom pad polygons
- multiple drill forms
- model paths
- advanced pad options

Skipping unsupported constructs is acceptable only because this sprint is an importer foundation, not a production library signoff path. The importer must reject malformed s-expressions and files whose root is not `footprint`.

## Command Shape

```powershell
.\build-qt\ccad.exe lib import-footprint --in resistor.kicad_mod --out resistor.ccad-footprint.json
```

The command emits a deterministic JSON representation of the imported footprint.

## Architecture

Create a new `ccad_core::Footprint` model separate from placed board primitives. A footprint is library/package data, while `Board::pads` are placed physical geometry in a project.

Add a small s-expression reader scoped to this importer. Keep it independent from project JSON parsing. Do not execute file contents, resolve external paths, or shell out.

## Test Strategy

- Unit test the importer with a small inline `.kicad_mod` sample.
- Unit test rejection of malformed/root-mismatched files.
- CLI test imports a sample file and checks deterministic JSON output.
- Full Qt build and CTest before every commit.

