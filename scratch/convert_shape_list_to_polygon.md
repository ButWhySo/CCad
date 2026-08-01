## convert_shape_list_to_polygon
- **File**: `pcbnew/convert_shape_list_to_polygon.cpp`, `pcbnew/convert_shape_list_to_polygon.h`
- **Purpose**: Algorithms for assembling fragmented graphical segments (lines, arcs, bezier curves) into closed polygons, predominantly used to detect and build board outlines and cutouts.
- **Functionality**: 
  - `BuildBoardPolygonOutlines()`: Iterates all items on the `Edge_Cuts` layer, chaining endpoints into closed `SHAPE_POLY_SET` outlines using KD-trees (`nanoflann`) for performance.
  - Detects malformed/overlapping outlines and self-intersecting boundaries.
  - Distinguishes between the external board bounding outline (parents = even) and internal cutouts/holes (parents = odd).
  - Can fallback to using the bounding box of items if no valid edge cuts are present (`aInferOutlineIfNecessary`).
- **Context**: Essential for 3D Viewer, DRC (checking if items are outside the board), Zone filling, and exporting manufacturing formats (STEP/Gerber), all of which require a well-defined topological board boundary.
