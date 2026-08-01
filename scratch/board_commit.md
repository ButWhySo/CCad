## BOARD_COMMIT
- **File**: `pcbnew/board_commit.cpp`, `pcbnew/board_commit.h`
- **Purpose**: Extends the `COMMIT` class to provide transaction, undo/redo, and modification propagation features tailored to the `BOARD`.
- **Functionality**: 
  - Inherits from `COMMIT`.
  - Provides `Stage()` overrides to capture item changes (`CHT_ADD`, `CHT_REMOVE`, `CHT_MODIFY`).
  - Overrides `Push()` to apply changes: it recalculates connectivity (`CONNECTIVITY_DATA`), rebuilds teardrops (`TEARDROP_MANAGER`), updates `BOARD_DESIGN_SETTINGS` (DRC rules, zones, bounding boxes, solder masks) and refreshes views via `TOOL_MANAGER`.
  - Calculates "damage" to zones/ratsnest (`propagateDamage`) so only intersecting polygons are rebuilt.
  - Provides `Revert()` logic to swap items back with their cached snapshots and clear caches.
- **Context**: Used heavily in interactive tools and operations to atomically mutate the board while preserving undo history and derived topological data.
