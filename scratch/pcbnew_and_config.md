## pcbexpr_functions
- **File**: `pcbnew/pcbexpr_functions.cpp`
- **Purpose**: Defines built-in functions used in the DRC custom rule expression evaluator.
- **Functionality**:
  - `fromToFunc`: Validates if a track/item is part of a from-to connection.
  - `existsOnLayerFunc`: Evaluates if an item exists on a specific layer.
  - `isPlatedFunc`: Checks if an item (pad/via) is plated.
  - `intersectsCourtyardFunc`, `intersectsFrontCourtyardFunc`, `intersectsBackCourtyardFunc`: Geometric checks for footprint courtyards against other board items, caching the results for performance.
  - `intersectsAreaFunc`: Geometric checks against user-defined rule areas (`ZONE`), handling silk clearance special cases and using an R-tree for quick spatial queries.
- **Context**: Connects the logic in `pcbexpr_evaluator` to physical geometry and connectivity data, enabling complex rule evaluations.

## pcbnew
- **File**: `pcbnew/pcbnew.cpp`
- **Purpose**: The main KiFACE entry point and interface module for the `pcbnew` application within KiCad.
- **Functionality**:
  - `IFACE::OnKifaceStart`: Initializes the module, registers settings managers (like `PCBNEW_SETTINGS` and `FOOTPRINT_EDITOR_SETTINGS`), and prepares job handlers.
  - `IFACE::CreateKiWindow`: Acts as a factory for creating UI frames and dialogs (e.g., `PCB_EDIT_FRAME`, `FOOTPRINT_EDIT_FRAME`, `PANEL_FP_DISPLAY_OPTIONS`).
  - `filterFootprints`: A JSON-driven API function for asynchronously querying and filtering footprint libraries based on pin count or naming patterns, used by external plugins or dialogs.
  - `IFACE::SaveFileAs`: Deep copy mechanism for duplicating a project. Iterates through `.kicad_pcb` files replacing internal schematic links and handles library tables.
- **Context**: The foundational bridge connecting the Pcbnew editor DLL/module to the main KiCad project manager launcher, managing application lifecycle and routing external requests.

## pcbnew_config
- **File**: `pcbnew/pcbnew_config.cpp`, `pcbnew/pcbnew_config.h`
- **Purpose**: Manages the loading and saving of `pcbnew` specific project and local settings.
- **Functionality**:
  - `PCB_EDIT_FRAME::LoadProjectSettings`: Reads `PROJECT_FILE` and `PROJECT_LOCAL_SETTINGS` to restore UI state. Rebuilds net colors, hidden net states, active layers, and display opacities.
  - `PCB_EDIT_FRAME::SaveProjectLocalSettings`: Persists the current session state (e.g., UI selections, viewports, hidden netclasses) back to the project configuration.
  - `PCB_EDIT_FRAME::LoadDrawingSheet`: Resolves and loads the drawing sheet template assigned to the board.
- **Context**: Ensures a seamless transition between sessions by maintaining the designer's viewport and display preferences across reloads.
