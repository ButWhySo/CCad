## footprint_info_impl
- **File**: `pcbnew/footprint_info_impl.cpp`, `pcbnew/footprint_info_impl.h`
- **Purpose**: Concrete implementations of `FOOTPRINT_INFO` and `FOOTPRINT_LIST` to parse and cache metadata about footprints from libraries.
- **Functionality**: 
  - `FOOTPRINT_INFO_IMPL::load()` parses a footprint using `FOOTPRINT_LIBRARY_ADAPTER` and caches pad count, unique pads, description, and keywords without keeping the heavy `FOOTPRINT` in memory.
  - `FOOTPRINT_LIST_IMPL` asynchronously builds a list of `FOOTPRINT_INFO_IMPL` objects using a thread pool.
  - Handles concurrent parsing of `.kicad_mod` files across all configured libraries to rapidly populate the footprint chooser tree.
  - Caches footprint data and supports timestamp-based checking to avoid redundant full-reloads.
- **Context**: Used heavily by the `PANEL_FOOTPRINT_CHOOSER` and footprint library viewers to provide fast search and tree populations.
