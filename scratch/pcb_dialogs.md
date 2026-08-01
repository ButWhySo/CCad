# PCBNew Dialogs (Part 1)

## Barcode Properties
- **dialog_barcode_properties**: `wxWidgets` dialog for editing barcode (e.g., QR code, DataMatrix) parameters placed on the board. CCad equivalent would be in `selection_inspector_panel` for barcode items.

## Board Reannotate
- **dialog_board_reannotate**: UI for geographical reannotation of reference designators on the PCB (e.g. top-to-bottom, left-to-right). CCad implementation should be natively handled by a command or dedicated Qt dialog invoking `ccad pcb reannotate`.

## Board Setup
- **dialog_board_setup**: Main project/board configuration dialog containing layers setup, design rules (clearance, track widths), and solder mask settings. CCad handles a subset of this in `ReviewWindow::showBoardSetup` (currently manipulating `ccad::DesignRules`).

## Board Statistics
- **dialog_board_statistics**: Displays component counts, board size, drill hole counts, and pad counts. 
- **dialog_board_stats_job**: The background worker/job thread that computes these statistics so it doesn't block the UI for large boards. CCad equivalent would be a standalone statistics command or property panel widget.

## Cleanup Graphics
- **dialog_cleanup_graphics**: UI to trigger the `graphics_cleaner.cpp` algorithms (remove null shapes, merge redundant lines). CCad natively supports this via `ccad pcb clean-graphics` and doesn't explicitly need this exact dialog class.

## Cleanup Tracks & Vias
- **dialog_cleanup_tracks_and_vias**: UI to run track/via cleanup algorithms (removing dangling tracks, merging collinear tracks). CCad natively supports cleanup via backend algorithms rather than wxWidgets.

## Copper Zones
- **dialog_copper_zones**: UI for configuring zone filling parameters (clearance, min width, pad connection style). CCad delegates these settings to the Selection Inspector pane when a zone is active.

## Create Array
- **dialog_create_array**: Tool to stamp an array of footprints/objects based on a grid or circular pattern. CCad exposes array stamping via CLI ccad pcb create-array.

## Dimension Properties
- **dialog_dimension_properties**: Adjusts dimension line thickness, text size, and format. Handled natively in CCad via properties panel.

## DRC
- **dialog_drc** / **dialog_drc_job_config**: Main dialog for configuring and running the Design Rule Check. CCad executes this seamlessly via ccad::runDrc(project_cache_) and displays it in diagnostics_panel.cpp.

## Enum Pads
- **dialog_enum_pads**: UI for bulk-renumbering pads on a footprint based on geometric order. CCad delegates this operation to native CLI (ccad pcb enum-pads) and internal heuristics.

## Exchange Footprints
- **dialog_exchange_footprints**: Dialog to swap all instances of one footprint with another (or by reference matching). CCad handles footprint updates natively through the sync engine, or via CLI ccad pcb exchange-footprints.

## Export 2581
- **dialog_export_2581**: UI for IPC-2581 export configuration (BOM, Netlist, Copper). CCad exposes this through the exporter interface in exporters/ without a dedicated wxWidgets class.

## Exporter Dialogs
- **dialog_export_idf**, **dialog_export_odbpp**, **dialog_export_step**, **dialog_export_svg**, **dialog_export_vrml**: UI wrappers for exporting the board to various mechanical or manufacturing formats. CCad bypasses these wxWidgets classes and handles configuration purely through exporter config data structures and the CLI.

## Search & Filter
- **dialog_filter_selection**: Controls which object types (Tracks, Vias, Pads, Text) are included when performing a box-select. CCad handles this natively via the selection filter matrix in ReviewWindow.
- **dialog_find** / **dialog_find_by_properties**: Search UI to locate items by text or specific properties. CCad uses the object_browser_panel and native JSON queries for this functionality.

## Footprint Utilities
- **dialog_footprint_associations**: UI for mapping schematic symbols to PCB footprints during annotation. CCad handles this directly via the unified project_cache and ccad pcb update-from-schematic.
- **dialog_footprint_checker**: Standalone DRC utility that specifically checks a footprint for errors (e.g. overlapping pads, missing courtyards). CCad rolls this into the global DRC engine.
