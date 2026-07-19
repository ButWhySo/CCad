# KiCad Exploration - Batch 29: Global Edit Text/Graphics UI, Global Edit Tracks & Vias, Imported Layers UI

## Overview
This batch completes the Global Edit Text and Graphics dialog UI base, covers the Global Edit Tracks and Vias dialog logic and UI, and touches on the base UI classes for the Imported Layers dialog.

## Key Findings

### Global Edit Text and Graphics (UI Base)
- **`dialog_global_edit_text_and_graphics_base.cpp/.h`**: 
  - WxFormBuilder generated base class for the global text and graphics editor.
  - Exposes the complex UI: filters by net, layer, footprint, reference, and an action panel allowing users to set line thickness, font, boldness, italicness, size, keep upright, visibility, and centering options.
  - Also displays a preview table for layer default values.

### Global Edit Tracks and Vias
- **`dialog_global_edit_tracks_and_vias.cpp/.h`**: 
  - Iterates over board tracks (`PCB_TRACE_T`, `PCB_ARC_T`) and vias (`PCB_VIA_T`) to globally apply property changes based on filtering criteria.
  - **Targets**: Tracks, Through vias, Microvias, Blind vias, Buried vias.
  - **Filters**: By Net, Netclass, Layer, Track width, and Via diameter.
  - **Actions**: Set to specific layer, specific track width, specific via size, change via annular ring settings, via protection features, or set to Netclass/Custom rule values.
  - Modification is performed via `m_parent->SetTrackSegmentWidth( aItem, aUndoList, false )`, relying heavily on the parent frame (PCB_EDIT_FRAME) logic to inject changes correctly. `BOARD_COMMIT` is not used here directly; instead, it uses a raw `PICKED_ITEMS_LIST` and manual ratsnest recalculations.
- **`dialog_global_edit_tracks_and_vias_base.cpp/.h`**: 
  - WxFormBuilder generated UI class matching the above logic.

### Imported Layers (UI Base)
- **`dialog_imported_layers_base.cpp/.h`**: 
  - WxFormBuilder generated UI class.
  - Purpose: Manages mapping of imported layers (from DXF, SVG, CAD, etc.) into KiCad standard layers.
  - Consists of two `wxListCtrl` boxes (`Unmatched Layers` and `Matched Layers`) and `wxButton`s to add, remove, remove all, or Auto-Match layers.
