## PCB IO and Netlist Updating
- **pcb_io (pcb_io_mgr.h/cpp)**: The `IO_MGR` factory for loading plugins that serialize/deserialize board files. Supports modern `KICAD_SEXP`, legacy formats, Altium, Eagle, etc.
- **netlist_reader (board_netlist_updater.h/cpp)**: The core update engine that syncs schematic changes to the physical `BOARD`. Handles component additions, footprint swapping, reference/value/timestamp updates, netlist reassignments, and pin function synchronization.
- **navlib (nl_pcbnew_plugin.h)**: 3Dconnexion SpaceMouse hardware interface for PCB canvas panning/zooming.
