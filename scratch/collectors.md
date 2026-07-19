## COLLECTORS
- **File**: `pcbnew/collectors.cpp`, `pcbnew/collectors.h`
- **Purpose**: Provides spatial querying and filtering mechanisms for identifying and collecting `BOARD_ITEM`s under the cursor or within a region.
- **Functionality**: 
  - `COLLECTORS_GUIDE`: An interface defining filtering preferences (e.g., ignore locked items, ignore specific layers, ignore vias). `GENERAL_COLLECTORS_GUIDE` implements this by pulling from global user preferences.
  - `PCB_COLLECTOR`: Base collection container that casts results to `BOARD_ITEM*`.
  - `GENERAL_COLLECTOR`: The primary spatial query tool used by the canvas. It iterates over elements, performing `HitTest()` against a reference position `m_refPos`, sorting results into a primary and secondary list based on the visibility/locking rules in the `COLLECTORS_GUIDE`.
  - `PCB_TYPE_COLLECTOR`: Collects all items of specific types (ignoring geometry).
  - `PCB_LAYER_COLLECTOR`: Collects all items on a specific layer.
- **Context**: Critical for the interactive UI selection engine. Used when the user clicks or drags a box to determine exactly what physical objects they are attempting to select, factoring in visibility toggles and selection filters.
