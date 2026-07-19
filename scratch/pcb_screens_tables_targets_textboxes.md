# KiCad Source Walk: Batch 4 (Screens, Tables, Targets, Textboxes)

## Overview
This batch covers the PCB screen properties and several complex graphical board items like tables, table cells, text boxes, and target markers.

- **Files**:
  - `pcbnew/pcb_screen.cpp`
  - `pcbnew/pcb_table.h`, `pcbnew/pcb_table.cpp`
  - `pcbnew/pcb_tablecell.h`, `pcbnew/pcb_tablecell.cpp`
  - `pcbnew/pcb_target.h`, `pcbnew/pcb_target.cpp`
  - `pcbnew/pcb_textbox.cpp` (and implicitly its header)

## 1. PCB Screen
- **Purpose**: Tracks view-level state like active layers and routing pairs.
- **Mechanism**: `PCB_SCREEN` inherits from `BASE_SCREEN`. Sets defaults like `F_Cu` for active and top route layers, and `B_Cu` for bottom route layer.

## 2. Tables and Table Cells
- **Purpose**: Allows users to draw structured tables with text (e.g., custom BOMs, title block data on the canvas).
- **Mechanism**: 
  - `PCB_TABLE` manages a grid of `PCB_TABLECELL` objects. It handles rows, columns, resizing (via `Autosize()`), border/separator styling, and matrix transforms (rotate, mirror, flip).
  - `PCB_TABLECELL` inherits from `PCB_TEXTBOX`. It knows its `m_rowSpan` and `m_colSpan`. It supports text variable resolution (e.g. `${ROW}`, `${COL}`, `${ADDR}`) by delegating to the footprint or board resolver, then formatting the text using its font.
- **CCad Analogue**: CCad kernel should model tables purely as data arrays or 2D string matrices with cell styles, relying on the GUI to render them, rather than deeply nesting graphical `PCB_TEXTBOX` nodes into a `PCB_TABLE` board item. 

## 3. PCB Target
- **Purpose**: A manufacturing or alignment target marker on the board.
- **Mechanism**: Defined by a position, shape (cross `+` or `X`), size, and line width. It implicitly lives on `Edge_Cuts` by default, but conceptually appears on all layers for manufacturing alignment.
- **CCad Analogue**: Targets in CCad should just be footprints with specific metadata, or parametric standard primitives, rather than an explicit `PCB_TARGET` class.

## 4. PCB Textbox
- **Purpose**: A rectangular region filled with multiline, aligned, wrapped text.
- **Mechanism**: Inherits from both `PCB_SHAPE` (to draw the rectangular boundary and handle bounding box/hit tests) and `EDA_TEXT` (to handle font, alignment, and string data). Supports complex protobuf serialization `kiapi::board::types::BoardTextBox`. Uses `KIFONT::FONT::LinebreakText()` to dynamically wrap text within the box width.
- **CCad Analogue**: Text boxes are essential for documentation. CCad will decouple the geometric boundary from the text content model to prevent the diamond-inheritance-like blending of `PCB_SHAPE` and `EDA_TEXT`.
