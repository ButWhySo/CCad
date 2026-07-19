## BOARD_LOADER
- **File**: `pcbnew/board_loader.cpp`, `pcbnew/board_loader.h`
- **Purpose**: Provides a centralized entry point to load or save a `BOARD` using various file formats via `PCB_IO_MGR`.
- **Functionality**: 
  - `Load()`: Invokes the appropriate `PCB_IO` plugin to parse the `.kicad_pcb` file.
  - Initializes board properties post-load (`initializeLoadedBoard()`): loads drawing sheets, builds connectivity/ratsnest, synchronizes netclasses, component classes, and tuning profile properties, and initializes the DRC engine (`DRC_ENGINE`).
  - `SaveBoard()`: Ensures the board's nets and connectivity are updated before writing to disk using the appropriate format.
  - Handles parsing errors and delegates progress reporting and plugin configurations.
- **Context**: The orchestrator for importing/exporting board structures to/from disk.
