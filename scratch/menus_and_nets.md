## menubar_footprint_editor
- **File**: `pcbnew/menubar_footprint_editor.cpp`
- **Purpose**: Creates the main top-level menu bar for the Footprint Editor window.
- **Functionality**: 
  - Iteratively builds the menus (File, Edit, View, Place, Inspect, Tools, Preferences).
  - Binds UI actions (e.g., `PCB_ACTIONS::newFootprint`, `ACTIONS::save`) to their respective menu items.
- **Context**: Used once at startup (and potentially recreated when language changes) to populate the wxMenuBar for `FOOTPRINT_EDIT_FRAME`.

## menubar_pcb_editor
- **File**: `pcbnew/menubar_pcb_editor.cpp`
- **Purpose**: Creates the main top-level menu bar for the PCB Editor window.
- **Functionality**: 
  - Similar to the footprint editor menubar builder, but contains the full suite of board-level actions (e.g., Fabrication Outputs, Routing tools, Zone management).
  - Also includes conditional menus depending on whether Pcbnew is running standalone or inside the KiCad project manager (e.g., hiding "Save As" when managed).
- **Context**: Used to populate the wxMenuBar for `PCB_EDIT_FRAME`.

## netinfo
- **File**: `pcbnew/netinfo.h`, `pcbnew/netinfo_item.cpp`, `pcbnew/netinfo_list.cpp`
- **Purpose**: Data structures for managing electrical nets (connections between pads/tracks).
- **Functionality**: 
  - `NETINFO_ITEM`: Represents a single net. Stores its netcode (integer ID), netname (hierarchical string), short netname, and a pointer to its `NETCLASS` (design rules).
  - `NETINFO_LIST`: A container that owns all `NETINFO_ITEM`s for a board. Maintains dictionaries for O(1) lookups by netcode or netname.
  - Automatically handles the "unconnected" pseudo-net (netcode 0).
  - Includes logic for auto-assigning netcodes and resolving short/display names when hierarchical netnames conflict.
- **Context**: The foundational connectivity data structure. Every pad, track, via, and zone references a `NETINFO_ITEM` (via its netcode) to determine electrical connectivity.
