## footprint
- **File**: `pcbnew/footprint.cpp`, `pcbnew/footprint.h`
- **Purpose**: Defines the `FOOTPRINT` class, representing a physical component on the board.
- **Functionality**: 
  - Subclasses `BOARD_ITEM_CONTAINER` and contains pads (`PAD`), graphic shapes (`DRAWINGS`), zones (`ZONES`), 3D models (`FP_3DMODEL`), text fields (`PCB_FIELD`).
  - Stores local overrides for clearance, solder mask margins, and paste margins.
  - Handles net ties, jumper configurations, and footprint variant parameters (DNP, Exclude from BOM, Exclude from Pos).
  - Calculates complex bounding boxes (`GetBoundingHull()`), courtyard intersections, and renders polygons representing pads or shapes via `TransformPadsToPolySet()`.
  - Serializes to/from protocol buffers (`kiapi::board::types::FootprintInstance`).
- **Context**: A core class in the object model. Replaces the legacy `MODULE` class in older KiCad versions.
