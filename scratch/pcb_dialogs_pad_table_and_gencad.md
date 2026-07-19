# KiCad Exploration - Batch 25: Footprint Wizard List Base, Pad Table Editor, GenCAD Export Options

## Overview
This batch covers the base classes for the footprint wizard list, the full implementation of the footprint pad table editor (a spreadsheet-like UI for editing pads), and the export options dialog for GenCAD.

## Key Findings

### Footprint Wizard List Base
- **`dialog_footprint_wizard_list_base.cpp/.h`**: FormBuilder generated UI containing the layout of the grid that holds the list of wizards and an associated log dialog for Python traceback messages.

### Pad Table Editor
- **`dialog_fp_edit_pad_table.cpp/.h`**: 
  - Provides a tabular (grid-based) view of all pads in a footprint, allowing rapid editing of pad properties.
  - **Grid Management**: Uses custom column sizes and an auto-sizing proportional approach based on initial width weights. Sets up specific column editors for different properties (e.g., `GRID_CELL_COMBOBOX` for type and shape, `GRID_CELL_TEXT_EDITOR` for distances/positions).
  - **Units Integration**: Closely tied to `UNITS_PROVIDER` for parsing and displaying metric/imperial units seamlessly in grid cells.
  - **State Management**: Uses `PAD_SNAPSHOT` to capture the original state of all pads when the dialog opens, allowing for robust cancellation (rollback) of changes if the user hits Cancel.
  - **Dynamic Updating**: Selecting a row in the grid brightens the corresponding pad on the canvas via `canvas->GetView()->Update( pad, KIGFX::REPAINT )`.
- **`dialog_fp_edit_pad_table_base.cpp/.h`**: FormBuilder generated base for the pad table dialog. Contains summary static texts for pin numbers, pin count, and duplicate pins.

### GenCAD Export Options
- **`dialog_gencad_export_options.cpp/.h`**: 
  - Exposes options specific to GenCAD export format.
  - Supports configuration either as a standalone dialog or embedded in a Job export configuration (`JOB_EXPORT_PCB_GENCAD`).
  - Options include: `FLIP_BOTTOM_PADS`, `UNIQUE_PIN_NAMES`, `INDIVIDUAL_SHAPES`, `USE_AUX_ORIGIN`, `STORE_ORIGIN_COORDS`.
