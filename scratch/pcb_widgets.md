## Widgets
- **File**: `pcbnew/widgets/`
- **Purpose**: Defines custom wxWidgets UI components used throughout the board editor.
- **Key Modules**:
  - `appearance_controls.h/cpp`: The right-hand side panel controlling layer visibility, net colors, and presets.
  - `net_inspector_panel.h/cpp`: The Net Inspector view for examining delays, lengths, and tuning statuses across the board's nets.
  - `panel_selection_filter.h/cpp`: The bottom right panel that determines which item types (pads, tracks, vias, text) are currently selectable.
  - `panel_footprint_chooser.h/cpp`: The footprint selector dialog panel.
