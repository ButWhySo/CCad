## pcb_base_edit_frame
- **File**: `pcbnew/pcb_base_edit_frame.h`, `pcbnew/pcb_base_edit_frame.cpp`
- **Purpose**: Abstract base class for Pcbnew editing frames.
- **Functionality**:
  - Derived from `PCB_BASE_FRAME`.
  - Manages GUI panels for editing (e.g., `PANEL_SELECTION_FILTER`, `APPEARANCE_CONTROLS`, `LAYER_PAIR_SETTINGS`, `PCB_VERTEX_EDITOR_PANE`).
  - Provides a framework for Undo/Redo operations (`SaveCopyInUndoList`, `RestoreCopyFromUndoList`, etc.).
  - Handles item editing dialog invocations (e.g., Text Properties, Shape Properties).
  - Handles library creation and selection dialogs.
- **Context**: The foundational GUI class used by the main PCB editor and footprint editor to manage state related to active editing, selection, and undo history.

## pcb_base_frame
- **File**: `pcbnew/pcb_base_frame.h`, `pcbnew/pcb_base_frame.cpp`
- **Purpose**: Base frame for all PCB-related windows in KiCad.
- **Functionality**:
  - Derived from `EDA_DRAW_FRAME`.
  - Holds the active `BOARD` object and the Tool Manager (`m_toolManager`).
  - Manages interaction with the GAL (Graphics Abstraction Layer) canvas, including zooming, focusing on items (`FocusOnItem`), and origin transforms.
  - Manages the 3D Viewer window state (`Update3DView`).
  - Provides getters/setters for board design settings, plot settings, and grid/page configurations.
- **Context**: Connects the generic EDA drawing frame with PCB-specific data (the `BOARD`), handling rendering updates, tools dispatching, and core interaction features common across the footprint editor and board editor.

## pcb_board_outline
- **File**: `pcbnew/pcb_board_outline.h`, `pcbnew/pcb_board_outline.cpp`
- **Purpose**: Logical `BOARD_ITEM` to represent the outer boundary of the PCB.
- **Functionality**:
  - Inherits from `BOARD_ITEM` (type `PCB_BOARD_OUTLINE_T`).
  - Represents the board outline geometry (Edge Cuts) as a single logical entity for selection or bounding box queries, rather than a raw set of line segments.
  - Returns `LAYER_BOARD_OUTLINE_AREA` for its layer.
- **Context**: Typically used for selecting or operating on the board boundary as a whole, simplifying collision and rendering queries.
