## Generators, Delay Calcs, Misc
- **length_delay_calculation**: (`length_delay_calculation.h/cpp`) Calculates time-domain (propagation delay) and space-domain (physical length) properties of routed nets. Accounts for trace lengths, via span thicknesses, and pad-to-die package delays.
- **generators (pcb_tuning_pattern)**: (`pcb_tuning_pattern.h/cpp`) The interactive `PCB_GENERATOR` for length-tuning meanders (single, diff pair, diff pair skew). Interacts with `PNS::MEANDER_PLACER` and `PNS::MEANDER_SETTINGS`.
- **microwave**: Microwave/RF geometry generators (`microwave_inductor`, `microwave_polygon`). Provides specific footprint construction routines for RF components.
- **import_gfx**: Importers for external graphics (DXF, SVG) into board geometries or footprints.
- **git**: Visual diff/merge tool implementation for KiCad PCB layouts.
