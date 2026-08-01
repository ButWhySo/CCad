# KiCad Source Walk: Batch 5 (Text, Plotting, Layers, Toolbars)

## Overview
This batch covers the PCB textbox header, text formatting help documentation, track/via types, plotting infrastructure, layer selection UI, and footprint editor toolbar configuration.

- **Files**:
  - `pcbnew/pcb_textbox.h`
  - `pcbnew/pcb_text_help_md.h`
  - `pcbnew/pcb_track_types.h`
  - `pcbnew/plotprint_opts.h`
  - `pcbnew/plot_board_layers.cpp`
  - `pcbnew/sel_layer.cpp`
  - `pcbnew/toolbars_footprint_editor.cpp`
  - `pcbnew/toolbars_footprint_editor.h`

## 1. PCB Text and Textbox
- **`pcb_textbox.h`**: As seen earlier, `PCB_TEXTBOX` inherits from `PCB_SHAPE` and `EDA_TEXT` for drawing bounded, wrapped text.
- **`pcb_text_help_md.h`**: A generated markdown help string defining the KiCad text variable evaluation syntax. It covers:
  - Text formatting: `^{superscript}`, `_{subscript}`, `~{overbar}`.
  - Footprint/Board Variables: `${refdes:field}`, `${LAYER}`.
  - Pad/Pin functions: `${refdes:NET_NAME(pad)}`, `${refdes:PIN_NAME(pad)}`.
  - Math/Conditionals: `@{2 + 3}`, `@{if(condition, true_val, false_val)}`.
- **CCad Analogue**: CCad should definitely adopt a robust text variable evaluation engine, ideally supporting simple mathematical and conditional expressions for dynamic BOMs and drawing templates.

## 2. Tracks and Vias
- **`pcb_track_types.h`**: Defines via types (`THROUGH`, `BURIED`, `BLIND`, `MICROVIA`) and various manufacturing modes (`TENTING_MODE`, `COVERING_MODE`, `PLUGGING_MODE`, etc.).
- **CCad Analogue**: CCad already implements tracks and vias. Advanced manufacturing attributes (like tenting and plugging) could be implemented as metadata fields or explicit typed enum properties on vias.

## 3. Plotting
- **`plot_board_layers.cpp`**: Central logic for exporting PCB layers to Gerber/DXF/PDF/etc. Handles solder mask expansion, pad inflation/deflation (via Minkowski sum logic or simple radius adjustments for rectangles/custom pads), thermal reliefs, drill marks, and courtyard handling for DNP (Do Not Populate) components.
- **CCad Analogue**: Plotting logic is heavy on geometry transformation. CCad should rely on standard polygon clipping libraries (like Clipper2) to cleanly calculate mask expansions and thermal reliefs at export-time, instead of mixing drawing code with geometry modification logic.

## 4. UI: Layer Selection & Toolbars
- **`sel_layer.cpp`**: Provides dialogs for selecting a single layer or a pair of copper layers (e.g., for routing). Uses `wxGrid` extensively to show layer colors and names.
- **`toolbars_footprint_editor.cpp`**: Configures the toolbars (Top, Left, Right) for the footprint editor frame, defining which actions map to which buttons.
- **CCad Analogue**: CCad's UI will be built in Qt/QML, so wxWidgets grid/toolbar implementations are only useful as functional reference for what actions need to exist.
