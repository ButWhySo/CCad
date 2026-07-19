## footprint_editor_settings
- **File**: `pcbnew/footprint_editor_settings.cpp` (and `.h`)
- **Purpose**: Defines configuration properties specific to the Footprint Editor and Footprint Chooser.
- **Functionality**: 
  - Extends `PCB_VIEWERS_SETTINGS_BASE`.
  - Maps configuration keys from `fpedit.json` (or legacy INI/registry formats) to in-memory parameters.
  - Handles UI states (panel widths, splitters), default properties (silk line width, default text size, dimensions styling), magnetic snapping preferences, polar coordinates usage, selection filter states.
  - Defines automated migrations from previous schema versions (e.g. migrating legacy text default fields or color themes).
- **Context**: Loaded during application initialization and updated whenever the user changes editor preferences in the Footprint Editor.
