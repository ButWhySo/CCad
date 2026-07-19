## fix_board_shape
- **File**: `pcbnew/fix_board_shape.cpp`, `pcbnew/fix_board_shape.h`
- **Purpose**: Modifies raw geometry of graphic board shapes to perfectly connect and close boundaries, addressing small gaps and overlaps.
- **Functionality**: 
  - `ConnectBoardShapes()`: Adjusts the start/end points or curve control points of lines, arcs, and bezier curves such that adjacent shapes share exactly the same vertices if they fall within `aChainingEpsilon`.
  - Employs KD-trees to find the closest candidate end-points efficiently.
- **Context**: Used as a pre-processing step before `ConvertOutlineToPolygon()` to ensure disjoint or slightly-off manual drawings form valid watertight polygons.
