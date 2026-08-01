## Ratsnest
- **File**: `pcbnew/ratsnest/`
- **Purpose**: Computes and manages unrouted logical connections (airwires) between pads, tracks, and zones of a common net.
- **Key Classes**:
  - `RN_NET`: Calculates the optimal minimum spanning tree (MST) using Kruskal's algorithm to determine the shortest path between unconnected anchors in a net. Handles dynamic dirty-state updates when pads/tracks move.

## Push and Shove (PNS) Router
- **File**: `pcbnew/router/`
- **Purpose**: The interactive push-and-shove (PNS) routing engine for tracing and modifying PCB routes.
- **Key Components**:
  - `pns_routing_settings.h/cpp`: Manages all router settings (mode: shove, walkaround, mark obstacles), effort, snap-to-pads, DRC allowances, loop removal.
  - `pns_algo_base.h/cpp`: Base class for PNS algorithms.
  - `pns_dragger.h/cpp`: Engine for interactive dragging of existing track segments and vias.
  - `pns_diff_pair.h/cpp`: Differential pair modeling, spacing, and coupled placement logic.
  - `pns_meander_placer.h/cpp` (and diff pair variants): Length tuning and meander generation algorithms.
