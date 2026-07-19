## footprint_chooser_frame
- **File**: `pcbnew/footprint_chooser_frame.cpp`, `pcbnew/footprint_chooser_frame.h`
- **Purpose**: A dialog frame allowing the user to select footprints from loaded libraries, view their 2D and 3D preview, and assign them.
- **Functionality**: 
  - Hosts `PANEL_FOOTPRINT_CHOOSER` which implements the actual footprint list/search filtering.
  - Hosts `FOOTPRINT_PREVIEW_PANEL` (2D graphics) and `EDA_3D_CANVAS` (3D preview).
  - Can receive IPC mail (`MAIL_SYMBOL_NETLIST`) to automatically constrain footprint choices by the pin count requested from the schematic symbol.
  - Maintains `FOOTPRINT_CHOOSER_SELECTION_TOOL` and manages viewer lifecycle.
- **Context**: Instantiated modal when assigning footprints to schematic symbols (CvPcb), or when manually placing new footprints in the board editor.
