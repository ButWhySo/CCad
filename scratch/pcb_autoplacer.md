## AR_AUTOPLACER
- **File**: `pcbnew/autorouter/ar_autoplacer.h/cpp`
- **Purpose**: Heuristic-based footprint auto-placement algorithm.
- **Key Methods**: 
  - `AutoplaceFootprints()`: The main entry point to automatically position a set of footprints.
  - Generates a placement/routing matrix (`AR_MATRIX`) and iterates through footprints, attempting to find optimal placement that minimizes ratsnest length (cost) and respects bounding box keepouts and board outlines.
  - Supports testing footprints on the current side or opposite side.

## AUTOPLACE_TOOL
- **File**: `pcbnew/autorouter/autoplace_tool.h/cpp`
- **Purpose**: Interactive tool for triggering auto-placement from the UI.
- **Key Methods**: 
  - `autoplaceSelected()`: Autoplaces currently selected components.
  - `autoplaceOffboard()`: Autoplaces all off-board components onto the board.
  - Wraps the call to `AR_AUTOPLACER` within an undo/redo context.

## SPREAD_FOOTPRINTS
- **File**: `pcbnew/autorouter/spread_footprints.h/cpp`
- **Purpose**: Un-jumbles new footprints by laying them out side-by-side or in rows outside of the current board geometry to prevent overlapping.
- **Key Methods**: 
  - `SpreadFootprints()`: Takes a list of footprints and packs them into a grid relative to a `aTargetBoxPosition`. Can optionally group by sheet (`aGroupBySheet = true`).
