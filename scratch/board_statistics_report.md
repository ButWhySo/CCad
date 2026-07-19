## BOARD_STATISTICS_REPORT
- **File**: `pcbnew/board_statistics_report.cpp`, `pcbnew/board_statistics_report.h`
- **Purpose**: Computes comprehensive board statistics (dimensions, areas, clearances) and formats them into text or JSON reports.
- **Functionality**: 
  - `BOARD_STATISTICS_DATA`: Structure to hold computed statistics like board width/height/area, copper area, courtyard area, component density, and drill/pad/via counts.
  - `InitializeBoardStatisticsData()`: Sets up the default categories to track.
  - `ComputeBoardStatistics()`: Iterates through footprints, tracks, and the board outline to calculate polygon areas, find minimum clearances, and bucket pad/via types.
  - `FormatBoardStatisticsReport()`: Generates a human-readable text report with ASCII tables and unit-converted values.
  - `FormatBoardStatisticsJson()`: Generates a structured JSON object containing all computed statistics for integration with external tools or plugins.
- **Context**: Drives the UI statistics dialog and provides programmatic access to board-wide physical metrics.
