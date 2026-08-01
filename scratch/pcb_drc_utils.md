## DRC Utilities and Providers
- **File**: `pcbnew/drc/`
- **drc_item.h/cpp**: Defines `DRC_ITEM` representing a single DRC violation marker. Holds the gigantic `PCB_DRC_CODE` enum enumerating every single failure condition (e.g., `DRCE_CLEARANCE`, `DRCE_CREEPAGE`, `DRCE_VIA_DANGLING`, `DRCE_TRACK_ANGLE`). Uses `DRC_ITEMS_PROVIDER` to supply the marker lists to UI dialogs.
- **drc_chain_topology.h/cpp**: Analyzes a net's routed copper graph using an RTree-based graph extraction. Calculates `TrunkLength`, `TrunkDelay`, and extracts `Stubs` (branches off the main trunk) for delay-tuning rules.
- **drc_creepage_utils.h/cpp**: Calculates creepage distance along the surface of the board, steering around cutouts and grooves via a grid/graph search (`CREEPAGE_GRAPH` and `CREEP_SHAPE`).
- **drc_interactive_courtyard_clearance.h/cpp**: A fast, specialized test provider used during footprint move/placement to immediately highlight overlapping courtyards before they are dropped onto the canvas.
