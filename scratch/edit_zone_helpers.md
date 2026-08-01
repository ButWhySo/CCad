## edit_zone_helpers
- **File**: `pcbnew/edit_zone_helpers.cpp`
- **Purpose**: Helper functions for modifying zones/rule areas.
- **Functionality**: 
  - `Edit_Zone_Params()`: Launches the appropriate zone property dialog depending on if the zone is a copper zone, a non-copper zone, or a rule area. Handles the commit undo record (`BOARD_COMMIT`).
  - `BOARD::TestZoneIntersection()`: Analyzes if two zones overlap (same layer) using bounding box and precise segment intersection tests. Useful when trying to merge two zones with the same netcode.
- **Context**: Connects the zone structures with their respective UI configuration dialogs and connectivity rebuilds.
