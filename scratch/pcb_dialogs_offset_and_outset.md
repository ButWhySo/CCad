# KiCad Exploration - Batch 34: Offset Item, Outset Items

## Overview
This batch covers the Offset Item dialog (for translating items relative to their current position) and the Outset Items dialog (used for creating inflated/deflated outlines around selection, often used for courtyards or custom keepsouts).

## Key Findings

### Offset Item
- **`dialog_offset_item.cpp/.h` & `dialog_offset_item_base.cpp/.h`**:
  - Dialog for entering a numerical offset to translate a selection.
  - Supports Cartesian (Offset X, Offset Y) or Polar (Distance, Angle) coordinates.
  - Very similar in structure and implementation to `DIALOG_MOVE_EXACT`, but focused strictly on translation (no anchor points or absolute rotation logic).
  - Contains `UNIT_BINDER`s for X and Y components.
  - "Reset" buttons clear the input fields. `OnClear` sets them to the original offset given to the dialog.

### Outset Items
- **`dialog_outset_items.cpp/.h` & `dialog_outset_items_base.cpp`**:
  - A dialog to configure how items are "outset" (expanded or shrunk outwards/inwards). This is typically used in footprint editors or PCB editors to auto-generate courtyards, keepouts, or thickened lines around an existing shape.
  - Provides predefined common outset distances (e.g., IPC dense courtyard 0.15mm, IPC normal courtyard 0.25mm) and line widths.
  - Populates a `wxComboBox` history with both presets and recent values using `fillOptionList`.
  - **Inputs / Features**:
    - `m_outset`: The distance to expand/shrink.
    - `m_roundCorners`: Checkbox to round corners of the outset shape.
    - `m_roundToGrid` / `m_roundingGrid`: Forces rectangular outsets to align to a specific grid multiple (e.g., 0.01mm).
    - `m_copyLayers`: If checked, the outset items stay on the original source layers. If unchecked, uses a `PCB_LAYER_BOX_SELECTOR` to pick a target layer.
    - `m_copyWidths`: If checked, copies source line widths; otherwise uses `m_lineWidth`.
    - `m_lineWidth`: Explicit line width for the new items. Has a "Layer Default" button to fetch width from board design settings.
    - `m_deleteSourceItems`: Checkbox to replace the original items with the newly created outset items.
  - Maps to `OUTSET_ROUTINE::PARAMETERS`.
