# KiCad courtyard-clearance source reading

Read whole `F:/kicad_src/pcbnew/drc/drc_test_provider_courtyard_clearance.cpp` on 2026-09-16. KiCad separates two contracts: courtyard definition validation detects malformed or missing front/back outlines, and pairwise clearance checks compare sorted footprint courtyard polygon sets on each copper side. It also checks PTH/NPTH holes against another footprint courtyard, while excluding heatsink pads and via holes.

CCad cannot port this provider yet without extending `BoardFootprint` with typed front/back courtyard geometry, malformed/missing policy, and serialization/import semantics. Current CCad `BoardFootprint` has identity, layer, position, rotation, BOM and lock state only. Implementing overlap from pad or footprint bounds would not represent KiCad courtyard polygons and risks false DRC. Next bounded design task: add courtyard polygon fields and round-trip tests before provider behavior.
