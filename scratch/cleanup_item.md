## CLEANUP_ITEM
- **File**: `pcbnew/cleanup_item.cpp`, `pcbnew/cleanup_item.h`
- **Purpose**: Defines error codes and descriptors for automated board cleanup tasks.
- **Functionality**: 
  - Extends `RC_ITEM` (Rule Check Item, base for DRC).
  - Translates `CLEANUP_RC_CODE` values into human-readable strings (e.g. "Remove duplicate track", "Remove redundant via", "Merge co-linear tracks").
  - Provides `VECTOR_CLEANUP_ITEMS_PROVIDER` class to pass a list of cleanup findings to UI dialogs/models.
- **Context**: Used by the PCB "Cleanup Tracks and Vias" or "Cleanup Graphics" dialogs to present actionable anomalies found on the board.
