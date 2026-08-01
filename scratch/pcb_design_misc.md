## UNDO_REDO / PCB_BASE_EDIT_FRAME
- **File**: `pcbnew/undo_redo.cpp`
- **Purpose**: Core undo/redo logic for PCB editor.
- **Key Mechanics**: Uses `PICKED_ITEMS_LIST` which wraps `ITEM_PICKER`. Reverses operations using `PutDataInPreviousState`. Tracks changed items and bounding boxes to do partial redraws (e.g., ratsnest rebuild, or `dirty_rule_areas` for rule areas). Pushes batch events via `OnItemsCompositeUpdate`.

## PROJECT_PCB
- **File**: `pcbnew/project_pcb.cpp`
- **Purpose**: PCB-specific extensions to `PROJECT`.
- **Key Methods**: `FootprintLibAdapter()` to lazy-load the `FOOTPRINT_LIBRARY_ADAPTER`. `Get3DCacheManager()` to manage 3D model cache for the project.

## VIA_PROTECTION_UI_MIXIN
- **File**: `pcbnew/via_protection_ui_mixin.h`
- **Purpose**: A mixin class for UI elements dealing with IPC-4761 via protection types (Tented, Covered, Plugged, Filled, Capped).
- **Key**: Maps standard IPC4761 presets (e.g. Type VII - filled and capped) to PCB_VIA drill/padstack attributes.

## ZONE_LAYER_PROPERTIES_GRID / ZONE_SETTINGS_BAG
- **File**: `pcbnew/zone_layer_properties_grid.h`, `pcbnew/zone_settings_bag.h/cpp`
- **Purpose**: UI handling for editing zones, particularly mapping zone settings across cloned instances during bulk edits.
- **Key**: `ZONE_SETTINGS_BAG` maintains `m_zonesCloneMap` to sync priority changes across zone clones. `LAYER_PROPERTIES_GRID_TABLE` maps layer offset/properties.

## PLOT_BRDITEMS_PLOTTER
- **File**: `pcbnew/plot_brditems_plotter.cpp`
- **Purpose**: Implementation of plotting routines for individual board items.
- **Key**: Draws pads (`PlotPad`), pad numbers, graphic items, text, textboxes, targets, dimensions via the generic `m_plotter` interface (Gerber, DXF, etc.).
