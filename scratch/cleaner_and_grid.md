## graphics_cleaner
- **File**: `pcbnew/graphics_cleaner.cpp`, `pcbnew/graphics_cleaner.h`
- **Purpose**: Helper algorithm used to identify and clean up redundant or problematic graphical items on the board.
- **Functionality**: 
  - `GRAPHICS_CLEANER::CleanupBoard()` performs various optional cleanup passes:
    - Removes zero-length / null shapes (e.g. circles with 0 radius, segments with start=end).
    - Removes perfectly duplicate/superimposed graphical shapes.
    - Merges sets of 4 independent lines that form a perfect rectangle into a `RECTANGLE` shape.
    - Merges overlapping/adjacent pads of the same net or footprint pad number (when in the footprint editor).
    - Fixes board outlines (merging collinear segments or fixing minor gaps) by calling `ConnectBoardShapes`.
- **Context**: Used heavily by the "Tools -> Cleanup Tracks and Vias / Graphics" dialogs.

## grid_layer_box_helpers
- **File**: `pcbnew/grid_layer_box_helpers.cpp`, `pcbnew/grid_layer_box_helpers.h`
- **Purpose**: Provides specialized UI components to render and edit PCB layers within a `wxGrid` cell.
- **Functionality**: 
  - `GRID_CELL_LAYER_RENDERER`: Renders a grid cell with a color swatch representing the layer and the layer's name.
  - `GRID_CELL_LAYER_SELECTOR`: Provides a dropdown combobox (`PCB_LAYER_BOX_SELECTOR`) for the user to change the layer within a grid cell edit session.
- **Context**: Used in various data grids that involve PCB layer selection (e.g., custom design rules grid, footprint pad properties).
