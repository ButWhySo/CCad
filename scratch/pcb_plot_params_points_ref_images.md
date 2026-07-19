# KiCad Source Walk: Batch 3 (Plot Params, Points, Reference Images)

## Overview
This batch covers plotter parameter structures and parsing, the `PCB_POINT` object, and the `PCB_REFERENCE_IMAGE` object used in the board editor.

- **Files**:
  - `pcbnew/pcb_plotter.h`
  - `pcbnew/pcb_plot_params.h`, `pcbnew/pcb_plot_params.cpp`
  - `pcbnew/pcb_plot_params_parser.h`
  - `pcbnew/pcb_point.h`, `pcbnew/pcb_point.cpp`
  - `pcbnew/pcb_reference_image.h`, `pcbnew/pcb_reference_image.cpp`

## 1. Plotter and Plot Parameters
- **Purpose**: Defines configuration variables for generating outputs like Gerber, PDF, DXF, etc.
- **Mechanism**: `PCB_PLOT_PARAMS` stores a massive state structure including boolean flags for what items to include (e.g. `m_plotPadNumbers`, `m_plotReference`), Gerber precision settings, layer selections, scaling, PDF properties, etc. `PCB_PLOT_PARAMS_PARSER` uses the legacy S-expression parser `PCB_PLOT_PARAMS_LEXER` to deserialize these settings from board files.
- **CCad Analogue**: CCad has export mechanisms, but instead of writing a manual recursive descent token parser, configuration will be strongly typed using modern C++ JSON structures and serialization schemas native to the CCad kernel.

## 2. PCB Point
- **Purpose**: A zero-dimensional geometric anchor that can be placed on a layer. Used primarily for alignment, snap anchors, or custom pad definitions.
- **Mechanism**: Derived from `BOARD_ITEM`. Internally stores `m_pos` (location) and `m_size` (visual display size). Uses a cross-hair style bounding box hit-test (two intersecting line segments and a circle).
- **CCad Analogue**: CCad kernel supports native geometric primitives. While `PCB_POINT` is useful for footprint authoring snaps, CCad prefers an explicit geometric snap system managed by the geometry kernel rather than distinct board items that just float as markers.

## 3. PCB Reference Image
- **Purpose**: Allows users to place raster reference images (e.g. PNGs/JPEGs) onto the board canvas for tracing footprints or recreating existing boards.
- **Mechanism**: Encapsulates a `REFERENCE_IMAGE` object. Includes scale, PPI (pixels per inch), rotation, and transformations. The image is drawn on a specific layer, usually `LAYER_DRAW_BITMAPS`, but visibility is tied to the `PCB_LAYER_ID` it is attached to. Uses Protobuf serialization (`Serialize()`/`Deserialize()`) via `kiapi::board::types::ReferenceImage`.
- **CCad Analogue**: Reference images are standard fare for CAD tools. CCad will utilize Qt’s `QGraphicsPixmapItem` tied to the logical scene graph for rendering, rather than building custom bitmap transformation rendering paths in the kernel. The kernel merely needs to store the URI/payload and transform matrix of the reference image.
