# KiCad Exploration - Batch 26: Drill File Generation and Generators Dialog

## Overview
This batch covers the dialog used for generating drill files (`dialog_gendrill`) and the dialog for managing/rebuilding Generator Objects (`dialog_generators`).

## Key Findings

### Drill File Generation Dialog
- **`dialog_gendrill.cpp/.h`**: 
  - Central interface for exporting EXCELLON and GERBER X2 drill files.
  - Can be instantiated either directly by the user via the UI (`BOARD_EDITOR_CONTROL::GenerateDrillFiles`) or as part of a job configuration (`JOB_EXPORT_PCB_DRILL`).
  - **Formats**: Supports `EXCELLON` and `GERBER_X2`.
  - **Options**:
    - Output directory.
    - Excellon-specific: Units (mm/inches), Zero formats (Decimal, Suppress Leading/Trailing, Keep), Mirror Y axis, Minimal header, Merge PTH/NPTH, alternate drill mode for oval holes.
    - Drill Origin (Absolute vs Drill/place file origin).
    - Precision control based on units.
    - Drill map generation in various formats (Postscript, Gerber X2, DXF, SVG, PDF).
  - Uses `EXCELLON_WRITER` and `GERBER_WRITER` subclasses to actually emit the files based on the dialog's options.
  - Generates Drill Report files.

### Generator Objects Dialog
- **`dialog_generators.cpp/.h`**: 
  - Dialog to list, select, and rebuild PCB generator objects (`PCB_GENERATOR_T`), like parametric footprints or script-generated board elements.
  - Uses a `wxNotebook` to organize generators into pages by type. Each page contains a `wxDataViewCtrl` mapping the generator properties into a sortable table.
  - **Data binding**: Queries `PCB_GENERATOR::GetRowData()` which returns key-value pairs representing properties. Dynamically builds the grid columns from these properties.
  - **Rebuild Actions**:
    - Users can select items in the grid and execute `ACTIONS::selectItems` followed by `PCB_ACTIONS::regenerateSelected` to invoke the underlying Python/C++ generator.
    - Can also "Rebuild All" (`PCB_ACTIONS::regenerateAll`).
  - Implements `BOARD_LISTENER` to auto-refresh when the board model changes (e.g., items added/removed).

## Architectural Notes
- The drill generator is tightly integrated with `PCB_PLOT_PARAMS` and KiCad's generic job execution system (`JOB_EXPORT_PCB_DRILL`), showcasing how KiCad isolates UI from the actual data/export models.
- The `DIALOG_GENERATORS` is a great example of dynamic UI construction in KiCad, dealing with varied row data formats returned by abstract `PCB_GENERATOR` objects, rather than hardcoding columns.
