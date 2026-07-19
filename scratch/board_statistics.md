## BOARD_STATISTICS
- **File**: `pcbnew/board_statistics.cpp`, `pcbnew/board_statistics.h`
- **Purpose**: Gathers statistics about the PCB, primarily regarding drill holes and quantities.
- **Functionality**: 
  - Iterates through the board's footprints/pads and tracks/vias to accumulate all drill items into a list of `DRILL_LINE_ITEM` structures.
  - Groups identical drills (by size, shape, plating, layer span, and whether they are a pad or via) to provide a count (`m_Qty`).
  - Provides a sorting utility `COMPARE` to organize the aggregated list for reporting or display.
- **Context**: Used for drill report generation and statistics dialogs that summarize manufacturing complexities for the board.
