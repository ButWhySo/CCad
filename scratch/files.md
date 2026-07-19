## files
- **File**: `pcbnew/files.cpp`
- **Purpose**: High-level board file input/output routines, project loading, and import management.
- **Functionality**: 
  - `OpenProjectFiles()`: Main entry point for loading `.kicad_pcb` or importing foreign formats. Handles file locking, project switching, legacy design settings migration, and board initialization (`BOARD_LOADER::Load`).
  - Provides native GUI prompts like `AskLoadBoardFileName()` and `AskSaveBoardFileName()` using the `PCB_IO_MGR` plugin registry.
  - Controls save workflows `SaveBoard()` and autosave recovery sequences.
  - Implements `inferLegacyEdgeClearance()` to bridge differences between KiCad 5 (boundary outline width controlled zone clearance) and KiCad 6+ (explicit edge clearance rule).
- **Context**: Anchors the standard desktop File menu functions and KiCad cross-module project loading workflows.
