# KiCad Exploration - Batch 24: Footprint Properties (Editor) & Wizard List

## Overview
This batch covers the footprint properties dialog when launched from the **Footprint Editor** (`dialog_footprint_properties_fp_editor`), its UI definitions, and the `dialog_footprint_wizard_list` which shows available Python footprint generators.

## Key Findings

### Footprint Editor Properties Dialog
- **`dialog_footprint_properties_fp_editor.cpp/.h`**: This dialog is tailored for the footprint editor (where the footprint is the primary object, rather than a component on a board).
- **Stackup Modes**: 
  - Supports `EXPAND_INNER_LAYERS` (default, expands inner copper) and `CUSTOM_LAYERS`.
  - In custom layer mode, it allows configuring specific copper layer counts (up to 32) and enabling specific user layers, which is crucial for complex footprints (like RF components or embedded passives) that need specific layer assignments.
- **Data Binding**:
  - Uses `UNIT_BINDER` and `MARGIN_OFFSET_BINDER` for robust data entry of clearances (solder mask, solder paste offset/ratio, net clearance).
- **Grid Models**:
  - `LAYERS_GRID_TABLE`: A custom `WX_GRID_TABLE_BASE` derived class that manages the presentation of layers in the grid UI, interacting with `GRID_CELL_LAYER_RENDERER` and `GRID_CELL_LAYER_SELECTOR`.
- **Properties Handled**: 
  - Net tie groups (pads allowed to short nets).
  - Jumper groups.
  - Component type (TH, SMD, Unspecified), DNP, board only.
- **Embedded Files**: Integrates with `PANEL_EMBEDDED_FILES` to allow files to be embedded directly into the footprint design.

### Footprint Wizard List Dialog
- **`dialog_footprint_wizard_list.cpp/.h`**: Provides a UI to select from available footprint wizards.
- **Architecture**:
  - Footprint wizards are Python scripts loaded via KiCad's scripting API.
  - The dialog queries the `FOOTPRINT_WIZARD_MANAGER` which tracks the registered `FOOTPRINT_WIZARD` plugins.
  - It uses a simple grid `m_footprintGeneratorsGrid` to list wizard names and descriptions.

## Architectural Notes
The distinction between the board-level footprint properties dialog and the footprint-editor-level footprint properties dialog highlights KiCad's dual-context architecture. The footprint editor operates in a context where the footprint *is* the board, so properties like custom stackups are exposed directly, whereas on a real board, the footprint must conform to the board's global stackup.
