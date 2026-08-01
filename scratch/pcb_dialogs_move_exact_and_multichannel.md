# KiCad Exploration - Batch 32: Migrate 3D Models Base, Move Exact, Multichannel Generate Rule Areas

## Overview
This batch covers the base classes for the 3D models migration dialog, the Move Exact dialog (moving items by explicit dx/dy/theta), and the dialog for generating rule areas from multichannel groupings.

## Key Findings

### Migrate 3D Models Base
- **`dialog_migrate_3d_models_base.cpp/.h`**:
  - The WxFormBuilder base classes for `DIALOG_MIGRATE_3D_MODELS` (covered in Batch 31).
  - UI uses two nested `wxSplitterWindow`s: a main one splitting left (Missing) and right, and an inner one splitting the right side into middle (Candidates) and right (Preview).
  - Defines the layout, buttons (Replace, Keep, Add Directory, Open File), and lists.

### Move Exact
- **`dialog_move_exact.cpp/.h` & `dialog_move_exact_base.cpp/.h`**:
  - A dialog allowing users to move or rotate items by exact numerical values.
  - Supports Cartesian (Move X, Move Y) or Polar (Distance, Angle) coordinates via `m_polarCoords` checkbox.
  - Utilizes `UNIT_BINDER` to handle unit conversions and expressions for X, Y, and rotation.
  - Exposes rotation anchor options: Item anchor, Selection center, Local coordinates origin, or Drill/place origin.
  - Validates if the movement would place the selection outside the maximum board area (`m_bbox`).
  - `DIALOG_MOVE_EXACT::GetTranslationInIU` handles reading Cartesian or Polar inputs and outputting a Cartesian translation vector.

### Multichannel Generate Rule Areas
- **`dialog_multichannel_generate_rule_areas.cpp/.h`**:
  - Part of the multichannel toolset. Used for automatically generating rule areas (keepouts, etc.) for repeating blocks of circuitry.
  - Features a `wxNotebook` (`m_sourceNotebook`) with three tabs/grids:
    - **Sheets**: Generate rule areas bounding footprints from specific hierarchical sheets.
    - **Component Classes**: Generate rule areas bounding footprints belonging to specific component classes.
    - **Groups**: Generate rule areas bounding grouped footprints.
  - Utilizes `WX_GRID` with `GRID_TRICKS` for custom grid event handling (e.g., pasting, context menus).
  - Reads and writes to a `RULE_AREAS_DATA` struct provided by the `MULTICHANNEL_TOOL` parent.
  - Allows grouping the generated items (via `m_cbGroupItems`) and replacing existing areas (`m_cbReplaceExisting`).
