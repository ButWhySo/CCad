## Tools (UI Actions)
- **File**: `pcbnew/tools/`
- **Purpose**: Defines `PCB_TOOL_BASE` and `TOOL_INTERACTIVE` derivations that handle all user-initiated UI actions in Pcbnew.
- **Key Modules**:
  - `board_editor_control.h/cpp`: Master control actions for opening, saving, plotting, tracking width adjustments, layer toggling, and launching high-level dialogs.
  - Sub-tools for specific domains: `align_distribute_tool`, `array_tool`, `board_inspection_tool`, `drawing_tool`, `edit_tool`, `global_edit_tool`, `group_tool`, `microwaves_tool`, `pad_tool`, `pcbnew_control`, `picker_tool`, `placement_tool`, `point_editor`, `position_relative_tool`, `selection_tool`, `zone_create_helper`, `zone_filler_tool`.
  - These tools interpret mouse and keyboard events from the GAL (Graphics Abstraction Layer) and convert them into geometry or data mutations on the board.
