## footprint_libraries_utils
- **File**: `pcbnew/footprint_libraries_utils.cpp`
- **Purpose**: High-level utility routines for the Footprint Editor related to footprints and libraries.
- **Functionality**: 
  - Importing footprints from external files (`ImportFootprint`).
  - Exporting footprints (`ExportFootprint`) in various formats.
  - Creating new libraries (`CreateNewLibrary`).
  - Selecting and appending to libraries (`SelectLibrary`).
  - Provides the UI logic around interacting with `FOOTPRINT_LIBRARY_ADAPTER` (deleting, saving).
- **Context**: Connects user actions (via GUI) to the underlying library I/O and footprint table abstractions.

## footprint_library_adapter
- **File**: `pcbnew/footprint_library_adapter.cpp`, `pcbnew/footprint_library_adapter.h`
- **Purpose**: Interfaces with the global `LIBRARY_MANAGER` to manage footprint-specific library access.
- **Functionality**: 
  - Extends `LIBRARY_MANAGER_ADAPTER`.
  - Maintains preloaded footprint caches (a map of library nickname to vector of `FOOTPRINT*`).
  - Has a static `PreloadedFootprintsMutex` and `GlobalLibraries` leak-at-exit cache.
  - Loads individual footprints (`LoadFootprint`, `LoadFootprintWithOptionalNickname`), enumerates footprints (`GetFootprints`, `GetFootprintNames`).
  - Handles writing to libraries (`SaveFootprint`) and creating plugins (`createPlugin`) via `PCB_IO_MGR`.
- **Context**: The primary API surface for footprint I/O at the library level, decoupling the footprint editor from specific plugin details.
