# KiCad Source Walk: Board Stackup Manager Panels (Batch 10)

## Overview
This batch continues in the `pcbnew/board_stackup_manager/` directory, focusing on the UI panels used in the "Board Setup" dialog to configure the physical stackup constraints and board finishes.

- **Files**:
  - `pcbnew/board_stackup_manager/panel_board_finish.cpp`, `.h`
  - `pcbnew/board_stackup_manager/panel_board_finish_base.cpp`, `.h`
  - `pcbnew/board_stackup_manager/panel_board_stackup.cpp`, `.h`
  - `pcbnew/board_stackup_manager/panel_board_stackup_base.cpp`, `.h`

## 1. Panel Board Finish (`panel_board_finish.cpp`, `.h`, `_base`)
- **Purpose**: A UI panel configuring the final board manufacturing finishes.
- **Controls**:
  - `m_cbEgdesPlated`: Checkbox for plated board edges.
  - `m_choiceFinish`: Dropdown for copper finish (e.g., HASL, ENIG, Immersion Tin).
  - `m_choiceEdgeConn`: Dropdown for edge card connector beveling constraints.
- **Backend Link**: Modifies fields directly on the `BOARD_STACKUP` object associated with `BOARD_DESIGN_SETTINGS`.

## 2. Panel Board Stackup (`panel_board_stackup.cpp`, `.h`, `_base`)
- **Purpose**: The primary UI panel for defining the physical layer stack.
- **Structure**:
  - `m_choiceCopperLayers`: Sets the total number of copper layers.
  - `m_impedanceControlled`: Enables entering advanced constraint data (Epsilon R, Loss Tangent).
  - **Dynamic Grid (`m_fgGridSizer`)**: Dynamically builds rows of widgets for every configured layer (Dielectric, Copper, Paste, Mask). 
  - Each row uses a helper struct `BOARD_STACKUP_ROW_UI_ITEM` mapping a `BOARD_STACKUP_ITEM` to its UI controls (color, thickness, material, etc.).
- **CCad Analogue**: CCad's kernel (`ccad_core`) will store this stackup as the ground truth. A GUI client like CCad Qt will likely need an IPC message such as `ccad pcb set-stackup` to pass structured JSON defining these thicknesses and materials. The LLM agent will use the text representation (similar to `BuildStackupReport`) to understand Z-axis constraints when routing or verifying DRC.
