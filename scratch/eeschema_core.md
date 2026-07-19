## Connectivity & BOM
- **connection_graph.h/cpp**: The core engine in `eeschema` that calculates electrical connectivity across the entire schematic hierarchy.
  - Generates `CONNECTION_SUBGRAPH` instances representing unbroken electrical nodes on a single sheet.
  - Connects subgraphs across sheets via hierarchical labels and pins.
  - Handles "drivers" (strong vs weak, e.g., labels vs pins) to resolve canonical net names.
- **annotate.cpp**: Assigns reference designators (e.g. R?, C?) to unannotated symbols using gap-filling and sequential placement algorithms.
- **bom_plugins.h/cpp**: A wrapper class `BOM_GENERATOR_HANDLER` that invokes external scripts/executables to process the intermediate XML netlist into user-friendly BOMs.
