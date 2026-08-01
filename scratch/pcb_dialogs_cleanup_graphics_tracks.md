# KiCad Source Walk: Dialogs - Cleanup Graphics, Tracks, and Vias (Batch 14)

## Overview
This batch covers the UI and execution logic for two core "cleanup" utilities in the board editor: Graphics Cleanup and Tracks/Vias Cleanup.

- **Files**:
  - `pcbnew/dialogs/dialog_board_stats_job_base.h` (Leftover from Batch 13)
  - `pcbnew/dialogs/dialog_cleanup_graphics.cpp`, `.h`
  - `pcbnew/dialogs/dialog_cleanup_graphics_base.cpp`, `.h`
  - `pcbnew/dialogs/dialog_cleanup_tracks_and_vias.cpp`, `.h`
  - `pcbnew/dialogs/dialog_cleanup_tracks_and_vias_base.cpp`

## 1. Graphics Cleanup (`dialog_cleanup_graphics`)
- **Purpose**: A dialog that allows users to clean up redundant or overlapping graphical items on a board or footprint.
- **Mechanism**:
  - Defines `DIALOG_CLEANUP_GRAPHICS`.
  - Instantiates `GRAPHICS_CLEANER` and passes it either the `BOARD`'s drawings or the `FOOTPRINT`'s graphical items.
  - Offers a "dry run" mode (triggered initially and upon checkbox changes) which populates an `RC_TREE_MODEL` (DataViewCtrl) showing the proposed changes without committing them.
  - Options include: merging co-linear lines into rectangles, deleting redundant (superimposed) graphics, merging overlapping graphics into pads, and fixing board outline discontinuities, all bounded by a specified tolerance.
  - Clicking OK runs the cleaner with `aDryRun = false` and pushes the `BOARD_COMMIT`.

## 2. Tracks & Vias Cleanup (`dialog_cleanup_tracks_and_vias`)
- **Purpose**: A dialog to remove redundant, dangling, or erroneous routing elements.
- **Mechanism**:
  - Defines `DIALOG_CLEANUP_TRACKS_AND_VIAS`.
  - Uses `TRACKS_CLEANER` to do the heavy lifting.
  - It also uses the "dry run" preview paradigm, reporting issues to an `RC_TREE_MODEL` preview tree and a `WX_TEXT_CTRL_REPORTER` for a running log.
  - Extensive filtering capabilities are supported using a lambda function passed to `cleaner.SetFilter()`. You can filter by:
    - Selected items only
    - Specific Net
    - Specific Net Class
    - Specific Layer
  - Actions include: deleting tracks connecting different nets (shorts), deleting redundant vias, deleting dangling vias, merging co-linear tracks, deleting unconnected tracks, deleting tracks fully inside pads.
  - Contains options to automatically refill zones before and after cleanup, as routing changes can invalidate zone fills.
- **CCad Relevance**: The headless cleaner engines (`GRAPHICS_CLEANER`, `TRACKS_CLEANER`) and their use of filtering lambdas provide excellent examples of how automated LLM agents can perform layout sanitization. Exposing these cleaner tools via a CLI command (e.g., `ccad pcb cleanup-tracks --net GND`) will be very valuable.
