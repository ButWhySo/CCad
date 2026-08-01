## edit
- **File**: `pcbnew/edit.cpp`
- **Purpose**: High-level dispatch for PCB editor actions.
- **Functionality**: 
  - `Process_Special_Functions()`: Handles exporting footprints to libraries.
  - `SwitchLayer()`: Validates and changes the active layer (ensuring copper layer bounds).
  - `OnEditItemRequest()`: Triggers the appropriate properties dialog (or tool action) depending on the type of `BOARD_ITEM` provided (e.g. `ShowTextPropertiesDialog`, `ShowPadPropertiesDialog`, `Edit_Zone_Params`, or running `ACTIONS::groupProperties`).
- **Context**: A legacy switchboard tying UI events or double-clicks directly to dialog launchers.
