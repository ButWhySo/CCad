## pcbnew_jobs_handler
- **File**: `pcbnew/pcbnew_jobs_handler.cpp`, `pcbnew/pcbnew_jobs_handler.h`
- **Purpose**: A command dispatcher for various PCB export and batch operations.
- **Functionality**:
  - Registers handlers for jobs such as `render`, `step`, `svg`, `dxf`, `pdf`, `png`, `ps`, `stats`, `gerber`, `drill`, `pos`, `drc`, `ipc2581`, `odb`, `ipcd356`.
  - Serves as the backend for the `kicad-cli` tool for headless operations, maintaining a cached `BOARD` pointer to avoid reloading the file for multiple operations on the same board in a single CLI run.
  - Can spawn the GUI modal dialogs for these export operations when running inside `pcbnew`.
- **Context**: Centralized execution hub for all batch outputs (fabrication data, renders, reports) separating the CLI arguments/dialog states from the actual exporter logic.

## pcbnew_printout
- **File**: `pcbnew/pcbnew_printout.cpp`, `pcbnew/pcbnew_printout.h`
- **Purpose**: Encapsulates logic for printing PCBs (e.g. to physical printers or PDF via the print dialog).
- **Functionality**:
  - `PCBNEW_PRINTOUT` manages layer visibility, pagination (all layers on one page vs. layer-per-page), and drill mark drawing during the print cycle.
  - `KIGFX::PCB_PRINT_PAINTER` overrides painter behaviors specifically for printing (e.g. replacing actual via holes with crosshairs/small markers based on the `DRILL_MARKS` setting).
- **Context**: Extends the base `BOARD_PRINTOUT` to handle Pcbnew-specific print requirements, which differ slightly from standard screen rendering or Gerber plotting.
