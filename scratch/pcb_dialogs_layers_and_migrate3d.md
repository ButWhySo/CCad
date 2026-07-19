# KiCad Exploration - Batch 31: Generic Items List, Layer Selection, Map Layers, Migrate 3D Models

## Overview
This batch covers several smaller dialog components (Items list, Layer selection base classes) and two substantial features: Map Layers (during import) and Migrate 3D Models (WRL to STEP).

## Key Findings

### Generic Items List
- **`dialog_items_list.cpp/.h`**:
  - A generic dialog that displays a message and a collapsible pane containing a `wxListCtrl`.
  - Useful for showing a summary message ("X items have this issue") and letting the user expand to see the detailed list.
  - Exposes a `SetSelectionCallback` (`std::function<void(int)>`) so the parent can react when an item in the list is selected.

### Layer Selection Base
- **`dialog_layer_selection_base.cpp/.h`**:
  - WxFormBuilder base classes for layer selection dialogs.
  - `DIALOG_LAYER_SELECTION_BASE`: Base class with two wxGrids for left/right (or top/bottom) layers.
  - `DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE`: Base class with top/bottom layer selectors and a preset grid for common copper layer pairs (used for differential pairs or via setups).

### Map Layers
- **`dialog_map_layers.cpp/.h`**:
  - Inherits from `DIALOG_IMPORTED_LAYERS_BASE` (seen in Batch 29/30).
  - Used when importing DXF/SVG or other formats where the source has arbitrary layer names and they need to be mapped to KiCad's internal layers (e.g. `User.Drawings`, `Edge.Cuts`).
  - Has an "Auto-Match Layers" feature that attempts to map imported layers to KiCad layers if there is a known default (via `INPUT_LAYER_DESC::AutoMapLayer`).
  - Tracks unmatched layers and matched layers. Blocks completion if any "Required" layer (marked with `*`) remains unmapped.
  - Returns a `std::map<wxString, PCB_LAYER_ID>` representing the final mapping chosen by the user.

### Migrate 3D Models
- **`dialog_migrate_3d_models.cpp/.h`**:
  - Dialog shown when a board loads with `.wrl` / `.wrz` 3D model references that cannot be found. This typically happens when moving KiCad 9 boards to KiCad 10, where the standard library now uses STEP models.
  - **AutoMigrateByFilename**: A silent version that attempts to auto-migrate WRLs to STEPs if a match is found in the catalog, logging a `BOARD_COMMIT` if successful.
  - The UI has a 3-column layout (Missing models, Ranked Candidates, 3D Preview).
  - It builds a catalog of `.step`, `.stp`, `.stpz`, `.step.gz`, `.iges` files from 3D search paths.
  - Ranks candidates using a combination of directory matching and Levenshtein edit distance on the filename stem (e.g., `R_0603.wrl` vs `R_0603.step`).
  - **Preview**: Uses a dummy `BOARD`, places a dummy `FOOTPRINT` (a copy of a representative footprint for the missing model), and embeds a `EDA_3D_CANVAS` to render what the footprint will look like with the selected STEP model replacement. Transforms (scale, rotation, offset) from the old WRL model are preserved so the new STEP model lands in the same orientation.
