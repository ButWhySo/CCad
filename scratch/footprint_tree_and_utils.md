## footprint_tree_pane
- **File**: `pcbnew/footprint_tree_pane.cpp`, `pcbnew/footprint_tree_pane.h`
- **Purpose**: A panel containing the `LIB_TREE` widget used for browsing and selecting footprints in the Footprint Editor.
- **Functionality**: 
  - Wraps a `LIB_TREE` widget.
  - Interacts with `FP_TREE_SYNCHRONIZING_ADAPTER`.
  - Responds to `EVT_LIBITEM_CHOSEN` by calling `m_frame->LoadFootprintFromLibrary(...)` on the parent `FOOTPRINT_EDIT_FRAME`.
  - Blocks preview updates while menus are open to avoid UI glitches.
- **Context**: The left-hand side panel in the Footprint Editor allowing navigation of the configured libraries.

## footprint_utils
- **File**: `pcbnew/footprint_utils.cpp`, `pcbnew/footprint_utils.h`
- **Purpose**: General utility functions for footprints.
- **Functionality**: 
  - `ComputeFootprintShift(const FOOTPRINT& aExisting, const FOOTPRINT& aNew, ...)` matches uniquely numbered pads between an existing footprint on a board and a new footprint from a library to calculate the relative translation and rotation shift. 
  - Uses `ORTHO_ITEM_REALIGNER`.
- **Context**: Useful when updating a footprint on the board from a library if the footprint origin or orientation has changed in the library, to keep the footprint placed correctly relative to its routed tracks.
