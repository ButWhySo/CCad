# KiCad Source Walk: Dialogs - Copper Zones & Arrays (Batch 15)

## Overview
This batch covers the UI implementations for copper zone properties (and converting items to zones) and creating parametric arrays (grid/circular).

- **Files**:
  - `pcbnew/dialogs/dialog_cleanup_tracks_and_vias_base.h` (Leftover from Batch 14)
  - `pcbnew/dialogs/dialog_copper_zones.cpp`, `.h` (actually `_base.cpp/.h` but `.h` not present, implemented in `cpp`)
  - `pcbnew/dialogs/dialog_copper_zones_base.cpp`, `.h`
  - `pcbnew/dialogs/dialog_create_array.cpp`, `.h`
  - `pcbnew/dialogs/dialog_create_array_base.cpp`, `.h`

## 1. Copper Zones (`dialog_copper_zones`)
- **Purpose**: A dialog for managing copper zone properties, and notably, for converting other graphical items into copper zones.
- **Mechanism**:
  - Implements `DIALOG_COPPER_ZONE` inheriting from `DIALOG_COPPER_ZONE_BASE`.
  - It wraps `PANEL_ZONE_PROPERTIES` to display the bulk of the zone settings (clearance, min width, thermal reliefs).
  - It handles `CONVERT_SETTINGS` when the dialog is invoked to convert a shape (e.g. a bounding polygon or lines) into a zone.
    - Options include converting via `BOUNDING_HULL` or `CENTERLINE`.
    - Allows specifying a `m_Gap` (expansion) and whether to delete the original source objects (`m_DeleteOriginals`).
  - Has a shortcut to open the full `Zone Manager` (`PCB_ACTIONS::zonesManager`).

## 2. Create Array (`dialog_create_array`)
- **Purpose**: A complex parametric dialog for creating 2D grid arrays or circular arrays of footprints, pads, or graphical items.
- **Mechanism**:
  - Implements `DIALOG_CREATE_ARRAY`.
  - Driven by the `ARRAY_OPTIONS` struct (specifically `ARRAY_GRID_OPTIONS` and `ARRAY_CIRCULAR_OPTIONS`).
  - Contains extensive pad renumbering logic (Continuous, Coordinate [A1, B2], different alphabets [Hex, Alpha no IOSQXZ, etc]).
  - **Grid Array**: Controls for Nx/Ny counts, Dx/Dy spacing, X/Y offsets (for staggering), and row/col staggering.
  - **Circular Array**: Controls for Center X/Y (can be picked interactively using `PCB_PICKER_TOOL`), Point Count, Angle, Initial Offset Angle, and Item Rotation.
  - Warns the user if the array parameters would result in stacked items (overlapping items with same position/rotation).
- **CCad Relevance**: 
  - The array creation logic here is highly mature, specifically regarding pad numbering schemes (Alpha, Hex, exclusions) and stagger logic. CCad's internal CLI array commands should aim for parity with the `ARRAY_GRID_OPTIONS` and `ARRAY_CIRCULAR_OPTIONS` data structures to ensure equivalent descriptive power.
