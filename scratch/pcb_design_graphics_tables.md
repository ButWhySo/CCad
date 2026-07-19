## PCB_ORIGIN_TRANSFORMS
- **File**: `pcbnew/pcb_origin_transforms.h/cpp`
- **Purpose**: Extends `ORIGIN_TRANSFORMS` to handle PCB-specific coordinate transformations between internal coordinates (nanometers) and display coordinates (user-selected grid origin).
- **Key Methods**: `ToDisplay/FromDisplay` for Abs/Rel and X/Y coordinates. Handles axis inversion (Y-axis down vs up).

## PCB_PLOT_PARAMS
- **File**: `pcbnew/pcb_plot_params.h/cpp`
- **Purpose**: Options when plotting/printing a board (Gerber, SVG, PDF, DXF, PS, HPGL).
- **Key**: Tracks scale, mirror, negative, DrillMarks, TextMode, PlotPadNumbers, PlotValue, PlotReference, Format, OutputDirectory. Uses `PLOT_FORMAT` enum. Can compare configs with `IsSameAs()`.

## PCB_PLOTTER
- **File**: `pcbnew/pcb_plotter.h/cpp`
- **Purpose**: Driver layer for generating board plots. Uses `PCB_PLOT_PARAMS`.
- **Key Methods**: `Plot(outputPath, layersToPlot, commonLayers, ...)`

## PCB_REFERENCE_IMAGE
- **File**: `pcbnew/pcb_reference_image.h/cpp`
- **Purpose**: A bitmap image (`BOARD_ITEM`) that can be inserted onto a PCB layer (usually for reverse-engineering or tracing).
- **Key Methods**: Wraps a `REFERENCE_IMAGE`. Supports GetWidth/SetWidth, scaling, changing opacity (handled by view layer).

## PCB_TABLE
- **File**: `pcbnew/pcb_table.h/cpp`
- **Purpose**: A tabular `BOARD_ITEM_CONTAINER` that holds `PCB_TABLECELL` items.
- **Key**: Configurable borders (stroke, width, style, color) for external and internal separators. Columns and Rows sizes are defined (`m_colWidths`, `m_rowHeights`). `m_cells` vector holds cells. 

## PCB_TEXTBOX
- **File**: `pcbnew/pcb_textbox.h/cpp`
- **Purpose**: A multi-line text box shape (`PCB_SHAPE` + `EDA_TEXT`) that wraps text automatically within its rectangular boundaries.
- **Key**: Computes `GetMinSize()` based on text wrapping. Has margins (Left/Right/Top/Bottom). Border can be drawn (strokes).

## PCB_TARGET
- **File**: `pcbnew/pcb_target.h/cpp`
- **Purpose**: A target mark (e.g. for alignment or mounting registration).
- **Key**: `m_shape` (0 = draw +, 1 = draw X). `m_size`, `m_lineWidth`, `m_pos`.

## PCB_POINT
- **File**: `pcbnew/pcb_point.h/cpp`
- **Purpose**: A 0-dimensional point (`BOARD_ITEM`) used to mark a position on a PCB (snap anchor, routing snap point). 
- **Key**: Has location `m_pos` and visual size `m_size` in board space.
