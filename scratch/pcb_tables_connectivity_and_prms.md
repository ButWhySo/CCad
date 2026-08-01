# KiCad Source Walk: Predefined Stackup Prms, Tables, and Topology (Batch 11)

## Overview
This batch wraps up the board stackup manager, explores the board tables (characteristics and stackup), looks at an R-Tree connectivity wrapper, dives deep into a topology matching engine, and glances at barcode properties.

- **Files**:
  - `pcbnew/board_stackup_manager/stackup_predefined_prms.cpp`, `.h`
  - `pcbnew/board_tables/board_characteristics_table.cpp`
  - `pcbnew/board_tables/board_stackup_table.cpp`
  - `pcbnew/connectivity/connectivity_rtree.h`
  - `pcbnew/connectivity/topo_match.cpp`, `.h`
  - `pcbnew/dialogs/dialog_barcode_properties.cpp`

## 1. Stackup Predefined Parameters (`stackup_predefined_prms.cpp`, `.h`)
- Defines standard names and colors (used in `.gbrjob` Gerber Job files) for copper finishes, silkscreen colors, solder mask colors, and dielectrics.
- **CCad Relevance**: The CCad kernel will likely need exactly these lists when exporting to Gerber or IPC-2581. They provide a fixed vocabulary for materials and finishes that an agent could easily manipulate via standard enum strings.

## 2. Board Tables (`board_characteristics_table.cpp`, `board_stackup_table.cpp`)
- These files build `PCB_TABLE` objects (which are graphical board items) that display the board's characteristics (layer count, thickness, dimensions, min track/spacing, impedance control, etc.) and stackup details (materials, epsilon R, thickness).
- **CCad Relevance**: An LLM agent doing a DRC pre-check might want to generate or read a text version of these tables to verify the physical constraints of the design before manufacturing export.

## 3. Connectivity R-Tree (`connectivity_rtree.h`)
- A thin wrapper around `KIRTREE::DYNAMIC_RTREE` used for fast spatial querying of connectivity items based on their bounding box (`BBox()`) and layer range.

## 4. Topology Matcher (`topo_match.cpp`, `.h`)
- **Purpose**: A sophisticated graph isomorphism algorithm (`FindIsomorphism`) used to find topologically identical sections of a schematic/layout. E.g. finding that two different amplifier channels have the exact same footprint and netlist topology.
- Uses `CONNECTION_GRAPH`, `COMPONENT`, and `PIN`. It builds a connectivity graph and uses backtracking with MRV (Minimum Remaining Values) heuristics to find matches. Tie-breaking logic uses symbol UUIDs to ensure corresponding hierarchical instances match correctly.
- **CCad Relevance**: This is incredibly relevant for an AI agent performing "Layout Reuse" or "Room Copying." CCad will need a headless version of this to allow an agent to say, "Apply the layout of Channel 1 to Channel 2," requiring the kernel to map the topological isomorphism automatically.

## 5. Barcode Properties (`dialog_barcode_properties.cpp`)
- UI dialog for editing `PCB_BARCODE` objects (QR Code, Code 39, Data Matrix). Modifies standard parameters like position, size, text, orientation, and error correction level.
