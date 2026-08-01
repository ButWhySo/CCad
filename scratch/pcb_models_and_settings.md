# KiCad Source Walk: Batch 1 (PCB Models & Settings)

## Overview
This batch covers the initial exploration of the KiCad `pcbnew` source tree, focusing on pad number providers, the primary board header, bounding boxes, PCB IDs, and footprint editor settings. 

- **Files**: `pcbnew/array_pad_number_provider.cpp`, `pcbnew/array_pad_number_provider.h`, `pcbnew/board.h`, `pcbnew/board_bounding_box.cpp`, `pcbnew/board_bounding_box.h`, `pcbnew/pcbnew_id.h`, `pcbnew/footprint_editor_settings.cpp`

## 1. Array Pad Number Provider (`array_pad_number_provider.h`, `.cpp`)
- **Purpose**: Provides sequential pad numbers when placing arrays of pads (e.g. BGA or DIP footprints), ensuring they do not conflict with numbers already existing in a `FOOTPRINT`.
- **Mechanism**: Takes a `set` of existing pad numbers and `ARRAY_OPTIONS`. Uses a `while` loop inside `getNextNumber()` to continuously generate the next index and check if it already exists in the footprint.
- **CCad Analogue**: In CCad, array pad numbering is likely handled through our core geometry pad generation APIs (`ccad_core/pad_geometry`). CCad uses strict ID resolution and indexing, so checking for existing pad names during array creation will be a deterministic pass.

## 2. Board Bounding Box (`board_bounding_box.h`, `.cpp`)
- **Purpose**: A lightweight `EDA_ITEM` that wraps a `BOX2I`. It exists to represent the maximum extents of the board geometry, likely used for Zoom-to-Fit algorithms or rendering bounds.
- **Mechanism**: Subclasses `EDA_ITEM` with type `KICAD_T::PCB_BOUNDING_BOX_T`. Returns the `LAYER_BOARD_BOUNDING_BOX` layer in `ViewGetLayers`.
- **CCad Analogue**: CCad has its own spatial index and bounding box algorithms (e.g. `ccad_core/board_outline`). We don't need a dedicated scene graph item just for the bounding box; bounding volumes should be implicitly calculable from the primitive tree.

## 3. PCB IDs (`pcbnew_id.h`)
- **Purpose**: Defines `pcbnew_ids` enum for command IDs unique to the printed circuit board editor (Pcbnew).
- **Contents**: Includes IDs for popup menu selections (track width, via size, differential pair dimensions), toolbar actions, footprint editor views, and wizard steps.
- **CCad Analogue**: CCad uses a Qt action map (`ccad_gui/gui_map.h`) rather than global enums, and a strictly typed action bus in `ccad_cli` for command emission. 

## 4. Footprint Editor Settings (`footprint_editor_settings.cpp`)
- **Purpose**: Manages configuration, migration, and default parameters for the footprint editor (`ModEdit`).
- **Mechanism**: Registers JSON schema properties mapped to internal variables (e.g., `design_settings.silk_line_width`, `aui.show_properties`). Includes migration logic from legacy `.ini`/wxConfig keys to modern JSON config paths.
- **CCad Analogue**: CCad settings are LLM-native JSON files. The GUI configuration is heavily stripped down compared to KiCad, as CCad is agent-first. We do not need extensive AUI layout serialization; instead, we rely on semantic model properties.

## 5. Board Base Class (`board.h`)
- **Purpose**: The absolute core container of the PCB layout. Subclasses `BOARD_ITEM_CONTAINER`, `EMBEDDED_FILES`, and `PROJECT::_ELEM`.
- **Mechanism**: Owns lists of `TRACKS`, `FOOTPRINTS`, `DRAWINGS`, `ZONES`, `GENERATORS`, `MARKERS`. Manages layer visibility bitmaps (`LSET`), design settings, connectivity state, high-lighted nets, and variant systems. 
- **CCad Analogue**: CCad's `ccad_core/board` model is significantly more decentralized. Instead of a massive monolithic `BOARD` class, CCad separates geometric layers, connectivity graphs, and spatial indexing into distinct, specialized managers. 
