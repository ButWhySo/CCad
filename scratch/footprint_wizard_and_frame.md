## footprint_wizard
- **File**: `pcbnew/footprint_wizard.cpp`, `pcbnew/footprint_wizard.h`
- **Purpose**: Core logic and structures for generating footprints programmatically using Python (IPC API).
- **Functionality**: 
  - `FOOTPRINT_WIZARD_MANAGER` manages loading wizard plugins via `API_PLUGIN_MANAGER`.
  - Defines `WIZARD_PARAMETER` variants (int, real, bool, string) representing parameters passed to the wizard.
  - `Generate( FOOTPRINT_WIZARD* aWizard )` invokes the IPC action `--generate` with parameters, and deserializes the resulting protobuf response into a `FOOTPRINT*`.
- **Context**: Used to construct standardized footprints (e.g., QFP, BGA, SOIC) dynamically from parameters without needing them stored in a library.

## footprint_wizard_frame
- **File**: `pcbnew/footprint_wizard_frame.cpp`, `pcbnew/footprint_wizard_frame.h`, `pcbnew/footprint_wizard_frame_functions.cpp`
- **Purpose**: The UI dialog (window) for interacting with footprint wizards.
- **Functionality**: 
  - Modal frame inheriting `PCB_BASE_EDIT_FRAME`.
  - Uses AUI to layout a parameter panel (`FOOTPRINT_WIZARD_PROPERTIES_PANEL`), a message box, and a main GAL canvas.
  - User can edit parameters, triggering `RegenerateFootprint()` which asks the `FOOTPRINT_WIZARD_MANAGER` to rebuild the footprint, then updates the canvas.
  - `ExportSelectedFootprint()` finalizes the wizard output to be consumed by the board editor.
- **Context**: When a user selects "Create Footprint from Wizard" in the layout or footprint editor.
