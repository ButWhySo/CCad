# KiCad Source Walk: Dialogs - Footprint Checker & Properties (Batch 23)

## Overview
This batch explores the Footprint Associations base UI (leftover from previous), the Footprint Checker (for running footprint-specific DRC checks within the footprint editor), and the Footprint Properties dialog.

- **Files**:
  - `pcbnew/dialogs/dialog_footprint_associations_base.cpp`, `.h` (Base UI)
  - `pcbnew/dialogs/dialog_footprint_checker.cpp`, `.h`, `_base.cpp`, `_base.h`
  - `pcbnew/dialogs/dialog_footprint_properties.cpp`, `.h`

## 1. Footprint Checker (`dialog_footprint_checker`)
- **Purpose**: Runs a suite of footprint-level Design Rule Checks (DRC) directly from the Footprint Editor without needing a full board context.
- **Mechanism**:
  - Iterates through checks like `CheckFootprintAttributes`, `CheckPads`, `CheckShortingPads`, `CheckNetTies`, and `CheckClippedSilk`.
  - When an error is found, it instantiates a `DRC_ITEM` and a corresponding `PCB_MARKER`, which is added to the footprint editor canvas to highlight the issue.
  - The dialog UI displays these markers in a `wxDataViewCtrl` powered by `RC_TREE_MODEL` (same core component used by the full board DRC dialog).
- **CCad Relevance**: 
  - DRC checks in CCad should be purely kernel-level functions that can be invoked via CLI (`ccad footprint check`) or by a headless test-runner. The GUI simply calls `Kernel::CheckFootprint()` and visualizes the resulting markers.

## 2. Footprint Properties (`dialog_footprint_properties`)
- **Purpose**: The main properties dialog for configuring a footprint instance on a board.
- **Mechanism**:
  - Covers everything: position (X, Y), orientation, board side (front/back), locked state, attributes (SMD/TH, DNP, Exclude from BOM/Pos), local clearances (net, solder mask, solder paste), zone connection type (thermal, solid), and jumper pads.
  - Manages a grid (`PCB_FIELDS_GRID_TABLE`) for footprint text fields (Reference, Value, custom properties).
  - Uses `UNIT_BINDER` and `MARGIN_OFFSET_BINDER` to handle robust UI bindings between text inputs and internal kernel dimensions.
  - Contains embedded panels like `PANEL_FP_PROPERTIES_3D_MODEL` for 3D model assignments.
- **CCad Relevance**: 
  - The properties dialog in KiCad is extremely monolithic, pulling together UI state, kernel item manipulation, and undo/redo (`BOARD_COMMIT`). In CCad, a lot of this will be replaced by a declarative property-grid approach where `Footprint` exposes a structured data schema that the UI just renders generically, avoiding massive dialog `.cpp` files.
