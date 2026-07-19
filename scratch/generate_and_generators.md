## generate_footprint_info
- **File**: `pcbnew/generate_footprint_info.cpp`, `pcbnew/generate_footprint_info.h`
- **Purpose**: Generates rich HTML formatting for footprint metadata (description, keywords, documentation links).
- **Functionality**: 
  - Exposes `GenerateFootprintInfo()` to take a `FOOTPRINT_LIBRARY_ADAPTER` and `LIB_ID` and produce an HTML string.
  - Extracts datasheet URLs from footprint fields or parsing standard patterns in the description text.
  - Formats text for display in a `wxHtmlWindow` (used in footprint choosers/viewers).
- **Context**: Used to present footprint metadata nicely in the UI browser panes.

## generators_mgr
- **File**: `pcbnew/generators_mgr.cpp`, `pcbnew/generators_mgr.h`
- **Purpose**: Manages the registry and factory for `PCB_GENERATOR` objects.
- **Functionality**: 
  - Singleton `GENERATORS_MGR` mapping string type IDs to creation functions.
  - Exposes `CreateFromType()` to instantiate a generator dynamically.
  - Provides a templated `REGISTER` struct to allow generator implementations to self-register statically at startup.
- **Context**: Part of a newer procedural generation architecture (generators in KiCad 8+) for parametric PCB shapes, teardrops, length matching, etc.
