# KiCad Source Walk: Batch 6 (Wizard Toolbars, Zone Basics, Zone Undo/Redo)

## Overview
This batch covers the toolbar configurations for the Footprint Viewer and Wizard, and begins exploring the zone (copper pour and rule area) infrastructure in PCBNew, specifically undo/redo state management and settings management.

- **Files**:
  - `pcbnew/toolbars_footprint_viewer.cpp`, `.h`
  - `pcbnew/toolbars_footprint_wizard.cpp`, `.h`
  - `pcbnew/zones.h`
  - `pcbnew/zones_functions_for_undo_redo.cpp`
  - `pcbnew/zone_settings_bag.cpp`
  - *(Note: `pcbnew/zone_layer_properties_grid.cpp` was not found in the source tree during this pass)*

## 1. Toolbars (Viewer & Wizard)
- **`toolbars_footprint_viewer.cpp`**: Configures the footprint viewer toolbars. It provides actions to navigate previous/next footprint, standard zoom controls, 3D viewer, and tools to place/measure. 
- **`toolbars_footprint_wizard.cpp`**: A minimalistic toolbar configuration for the Footprint Wizard, exposing actions to reset wizard parameters and export the generated footprint to the editor.
- **CCad Analogue**: Standard UI boilerplate that will be entirely replaced by Qt/QML actions in CCad. 

## 2. Zones (Basics & Settings)
- **`zones.h`**: Defines the `ZONE_CONNECTION` enum (`NONE`, `THERMAL`, `FULL`, `THT_THERMAL`), which dictates how pads inside a zone connect to the poured copper. It also defines default values (e.g., `ZONE_THERMAL_RELIEF_GAP_MM`, `ZONE_CLEARANCE_MM`).
- **`zone_settings_bag.cpp`**: Implements `ZONE_SETTINGS_BAG`. It extracts `ZONE_SETTINGS` from all copper zones on a board into a decoupled structure (`m_zoneSettings`) so a dialog or property grid can edit the settings safely. It tracks zone priority changes (sorting by `HigherPriority()`) and only commits the priority change back to the actual zone objects if the order actually changed, to prevent VCS churn.
- **CCad Analogue**: Zone settings (clearances, priorities, connection rules) should be strictly defined in the transaction/document model. The concept of an intermediate "bag" to hold pending UI changes is standard MVP (Model-View-Presenter) design that CCad's Qt layer will also need when presenting complex dialogs.

## 3. Zones (Undo / Redo)
- **`zones_functions_for_undo_redo.cpp`**: Contains specialized undo/redo logic for zones because editing a single zone outline can cause it to merge with other zones or fracture into multiple new zones (due to cutouts or self-intersections). 
  - `ZONE::IsSame()`: A deep comparison function checking if two zones are geometrically and parametrically identical.
  - `UpdateCopyOfZonesList()`: A highly specialized function that diffs the board's actual zone list against a saved snapshot pick-list. It detects which zones were unmodified, which were deleted (merged away), and which are brand new items spawned by the polygon fracture/combine process.
- **CCad Analogue**: KiCad's approach to undoing zone merges/fractures relies on full snapshots and diffs. CCad's transactional kernel should track the explicit addition/removal of `BOARD_ITEM` IDs during a single transaction, natively supporting undo/redo of complex polygon fracturing without needing manual diffing logic.
