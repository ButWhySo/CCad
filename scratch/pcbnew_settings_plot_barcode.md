## pcbnew_settings
- **File**: `pcbnew/pcbnew_settings.h`
- **Purpose**: Defines configuration structures and keys for Pcbnew's application settings.
- **Functionality**:
  - `PCBNEW_SETTINGS` derives from `PCB_VIEWERS_SETTINGS_BASE` and `APP_SETTINGS_BASE`.
  - Defines UI panel state structures (like `AUI_PANELS`, `FOOTPRINT_CHOOSER`), display options (`DISPLAY_OPTIONS`), magnetic snapping preferences (`MAGNETIC_SETTINGS`), and interactive router defaults (`m_PnsSettings`).
  - Manages migrations from legacy configuration formats.
- **Context**: Connects user preferences (stored in JSON config) with runtime state, controlling UI layouts and behavioral preferences.

## pcbplot
- **File**: `pcbnew/pcbplot.cpp`, `pcbnew/pcbplot.h`
- **Purpose**: Core API for iterating over board items and sending them to a plotter (Gerber, PDF, SVG, etc.).
- **Functionality**:
  - `BRDITEMS_PLOTTER` is a helper class derived from `PCB_PLOT_PARAMS`. It interprets logical board items (like `PAD`, `ZONE`, `FOOTPRINT`, `PCB_TEXT`) and converts them into primitive drawing commands for a given `PLOTTER` interface.
  - Implements Gerber X2 attributes generation (`AddGerberX2Header`, `AddGerberX2Attribute`) appending metadata to generated files.
- **Context**: Sits between the logical board data model and the abstracted drawing layer (`gal`/`plotter`), handling specific quirks of manufacturing output (like generating drill marks inside pads for PDF prints).

## pcb_barcode
- **File**: `pcbnew/pcb_barcode.cpp`, `pcbnew/pcb_barcode.h`
- **Purpose**: Represents a barcode graphic element on a PCB.
- **Functionality**:
  - `PCB_BARCODE` inherits from `BOARD_ITEM`.
  - Interfaces with the `zint` backend library to encode strings into 1D (Code 39, Code 128) and 2D (QR, Micro QR, Data Matrix) barcode geometries.
  - Generates polygons representing the barcode modules and caches them using `PCB_BARCODE_CACHE` alongside an embedded text label.
  - Supports knockout (negative) rendering by generating an inverted bounding polygon.
- **Context**: Adds the ability to embed machine-readable identifiers directly onto the silkscreen or copper layers.
