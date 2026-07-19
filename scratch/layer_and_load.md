## layer_pairs
- **File**: `pcbnew/layer_pairs.cpp`, `pcbnew/layer_pairs.h`
- **Purpose**: Manage presets and the currently active "layer pair" for via/track placement routing.
- **Functionality**: 
  - `LAYER_PAIR_SETTINGS` stores a list of layer pair presets (`LAYER_PAIR_INFO`) and keeps track of the currently active pair.
  - Can distinguish between saved preset pairs and "manual" pairs set ad-hoc by the user.
  - Generates events (`PCB_LAYER_PAIR_PRESETS_CHANGED`, `PCB_CURRENT_LAYER_PAIR_CHANGED`) to notify the UI when the user alters these settings.
- **Context**: Important for the router and via placement tools to know which layers to jump between by default.

## layer_utils
- **File**: `pcbnew/layer_utils.cpp`, `pcbnew/layer_utils.h`
- **Purpose**: Helper functions for querying layer combinations and generating human-readable layer lists.
- **Functionality**: 
  - `AccumulateNames()` converts a set of layer IDs (`LSET`) into a comma-separated string of user-friendly names.
  - `GetAllFootprintLayers()` recurses through a footprint to find every layer used by its components.
  - `GetOrphanedFootprintLayers()` computes which layers would be left "homeless" if the user restricts the footprint to a specific subset of custom user layers.
- **Context**: Used heavily in footprint properties UI and DRC checks for layer mismatches.

## load_select_footprint
- **File**: `pcbnew/load_select_footprint.cpp`
- **Purpose**: UI logic and routines for loading, choosing, and saving footprints from libraries or boards.
- **Functionality**: 
  - `SelectFootprintFromLibrary()` opens the footprint chooser dialog (`FOOTPRINT_CHOOSER_FRAME`) to let the user pick a part.
  - `SelectFootprintFromBoard()` pops up an `EDA_LIST_DIALOG` for the user to select an existing footprint off the current board canvas.
  - `LoadFootprintFromBoard()` clones a selected board footprint into the footprint editor's isolated dummy board environment.
  - `SaveLibraryAs()` orchestrates copying a whole footprint library file to a new path.
- **Context**: Contains legacy/helper routines used by the footprint editor and layout tool to load physical component data into memory.
