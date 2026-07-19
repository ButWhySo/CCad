# KiCad Exploration - Batch 28: Global Deletion, Edit Teardrops, Edit Text and Graphics

## Overview
This batch covers dialogs responsible for executing global operations on the board: Global Deletion (`dialog_global_deletion`), Global Teardrop Edit (`dialog_global_edit_teardrops`), and the start of Global Edit Text and Graphics (`dialog_global_edit_text_and_graphics`).

## Key Findings

### Global Deletion Dialog
- **`dialog_global_deletion.cpp/.h`**: 
  - Exposes a UI to delete items globally across the entire board or on a specific layer.
  - **Targets**: Zones, Text, Board Outlines, Graphics, Footprints, Tracks & Vias, Teardrops, Markers.
  - **Filters**: Can target locked vs unlocked items separately.
  - **Implementation**: Uses `BOARD_COMMIT` to push removals to the undo stack. For example, deleting all tracks clears `board->Tracks()`. Recompiles ratsnest after deletions.

### Global Edit Teardrops Dialog
- **`dialog_global_edit_teardrops.cpp/.h`**: 
  - Dialog for managing teardrops globally (adding, removing, setting to specific values).
  - **Targets**: PTH pads, SMD pads, Vias, Track-to-Track teardrops.
  - **Filters**: Net, Netclass, Layer, Round pads only, Existing teardrops only, Selected items only.
  - **Actions**: Remove teardrops, Remove all, Add teardrops with default values, Add teardrops with specified values.
  - Uses `TEARDROP_MANAGER` for `TARGET_TRACK` operations, and updates `TEARDROP_PARAMETERS` for `PAD` and `PCB_VIA` directly before committing with `BOARD_COMMIT`.

### Global Edit Text and Graphics Dialog (Part 1)
- **`dialog_global_edit_text_and_graphics.cpp`**: 
  - Used in both PCB editor and Footprint editor to change properties of texts, graphics, and dimensions in bulk.
  - Modifies properties such as: Line thickness, Text width/height, Text thickness, Italic, Upright, Bold, Layer, and visibility.
  - **Scope**: Footprint Reference, Value, Other footprint fields, Footprint text, Footprint graphics, Footprint dimensions, Board text, Board graphics, Board dimensions.
  - **Filters**: Selected items, Layer, Footprint Reference, Footprint ID.
  - Uses `BOARD_COMMIT` and acts on `BOARD_ITEM` properties directly.
