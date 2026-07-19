## build_BOM_from_board
- **File**: `pcbnew/build_BOM_from_board.cpp`
- **Purpose**: Generates a rudimentary CSV Bill of Materials (BOM) directly from the board footprint data.
- **Functionality**: 
  - Iterates over all `FOOTPRINT` items in the board.
  - Skips footprints that have the `FP_EXCLUDE_FROM_BOM` attribute set.
  - Groups footprints that have identical values and library footprint IDs (`FPID`).
  - Sorts references within each group and sorts the final list by the first reference string.
  - Outputs the grouped results into a CSV file containing columns for ID, Designator, Footprint, Quantity, Designation, and Supplier.
- **Context**: A built-in basic BOM exporter triggered from the PCB editor's UI (`BOARD_EDITOR_CONTROL::GenBOMFileFromBoard`).
