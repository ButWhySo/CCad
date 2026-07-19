## BOARD_ITEM_CONTAINER
- **File**: `pcbnew/board_item_container.h`
- **Purpose**: An abstract interface for `BOARD_ITEM`s that are capable of containing other `BOARD_ITEM`s.
- **Functionality**: 
  - Inherits from `BOARD_ITEM`.
  - Defines pure virtual `Add(BOARD_ITEM*, ADD_MODE, bool)` for inserting or appending items (with optional bulk mode or skipping connectivity rebuilds).
  - Defines pure virtual `Remove(BOARD_ITEM*, REMOVE_MODE)` for removing items from the container.
  - Provides a generic `Delete()` method that removes and immediately deletes an item.
- **Context**: The `BOARD` and `FOOTPRINT` classes implement this interface, serving as containers for their respective elements (e.g., `BOARD` holds tracks/footprints, `FOOTPRINT` holds pads/graphics).
