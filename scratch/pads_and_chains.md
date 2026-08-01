## net_chain_bridging
- **File**: `pcbnew/net_chain_bridging.cpp`, `pcbnew/net_chain_bridging.h`
- **Purpose**: Computes the bridging length contributed by a single footprint to a net chain. 
- **Functionality**:
  - `FootprintChainBridgingLength`: Evaluates the bridging distance when a footprint has pads on different nets within the same net chain (e.g. 0-ohm resistors, transformers).
  - `BoardChainBridgingLength`: Sums chain bridging length across every footprint on the board.
  - `EnumerateChainBridges`: Generates edge pairs representing logical chain bridges, consumed by `CHAIN_TOPOLOGY`.
  - `PartitionNetChainAroundNet`: Identifies sections of a net chain "before" and "after" a specific net, traversing through footprint bridges (passives).
- **Context**: Used heavily in DRC and tuning. Critical for length-matching topologies where signals cross through passives (e.g., termination resistors) without breaking the logical length calculation.

## pad
- **File**: `pcbnew/pad.cpp`, `pcbnew/pad.h`
- **Purpose**: The core definition of a PCB pad (`PAD` class).
- **Functionality**:
  - Represents a single connection point (PTH, SMD, NPTH, Aperture) belonging to a `FOOTPRINT`.
  - Aggregates copper shapes, soldermask margins, solderpaste margins, thermal reliefs, and hole drills.
  - Generates effective polygons representing the complex geometries (e.g., custom shape primitives, chamfered rects, rounded rects).
  - Stores network electrical attributes (`NETINFO_ITEM`, pin type, pin function, die length).
- **Context**: The foundational primitive of physical footprints and electrical connectivity on a board.

## padstack
- **File**: `pcbnew/padstack.cpp`
- **Purpose**: A data structure separating complex multilayer definitions (via/pad stacks) from the physical pad instantiations.
- **Functionality**:
  - Encapsulates properties that vary by layer: hole shapes, secondary/tertiary drills (backdrilling), post-machining (counterbore/countersink).
  - Supports properties per-layer or identical across all layers (MODE::NORMAL vs MODE::FRONT_INNER_BACK).
  - Handles complex serialization/deserialization for the KiCad internal format.
- **Context**: Modern PCB design requires pads that are not strictly symmetrical through all layers (e.g. differing antipads, removal of unused inner layer pads). `PADSTACK` handles this complexity within the `PAD`.
