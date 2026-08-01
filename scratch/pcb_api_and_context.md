# KiCad Source Walk: PCB API, Enums & Board Context (Batch 7)

## Overview
This batch enters the `api/` directory in PCBNew, focusing on the mapping between internal KiCad enumerations and Protocol Buffer definitions, utilities for creating board items, and the abstraction layer (`BOARD_CONTEXT`) that allows tools and APIs to operate regardless of whether the PCB editor GUI is open or running headlessly.

- **Files**:
  - `pcbnew/zone_settings_bag.h`
  - `pcbnew/api/api_pcb_enums.cpp`
  - `pcbnew/api/api_pcb_utils.cpp`, `.h`
  - `pcbnew/api/board_context.cpp`, `.h`
  - `pcbnew/api/headless_board_context.cpp`, `.h`

## 1. PCB Enums & Protobuf Translations
- **`api_pcb_enums.cpp`**: Massive translation file implementing `ToProtoEnum` and `FromProtoEnum` templates.
- Maps internal KiCad C++ `enum` types (e.g., `PAD_ATTRIB`, `PAD_SHAPE`, `VIATYPE`, `ZONE_CONNECTION`, `DRC_CONSTRAINT_T`, `UNCONNECTED_LAYER_MODE`, `ZONE_FILL_MODE`, etc.) to gRPC/Protobuf representations (e.g., `types::PadType`, `types::PadStackShape`).
- **CCad Analogue**: CCad is designed as an LLM-native kernel that acts as the source of truth, heavily relying on JSON/protobuf-style IPC to communicate with external frontends or agents. A rigid layer mapping C++ structs/enums to over-the-wire schema is exactly the architecture expected in `ccad_core` when interfacing with the transaction server.

## 2. API Utilities (`api_pcb_utils.cpp`)
- **`CreateItemForType`**: A simple factory function that instantiates the correct subclass (e.g., `PCB_TRACK`, `PCB_VIA`, `PCB_TEXT`, `PAD`, `ZONE`) based on a `KICAD_T` enum type and assigns it to a parent `BOARD_ITEM_CONTAINER`.
- **`PackLayerSet` / `UnpackLayerSet`**: Helpers for translating a KiCad `LSET` (bitmask of layers) to and from a Protobuf `RepeatedField<int>`.

## 3. Board Context (`board_context.h`, `headless_board_context.h`)
- **`BOARD_CONTEXT`**: An interface providing access to the `BOARD`, `PROJECT`, `TOOL_MANAGER`, and `KIWAY`. It represents the "environment" the board is running in.
- **`PCB_EDIT_FRAME_CONTEXT`**: Implementation of `BOARD_CONTEXT` that proxies calls to the active GUI (`PCB_EDIT_FRAME`).
- **`HEADLESS_BOARD_CONTEXT`**: Implementation of `BOARD_CONTEXT` that owns the `BOARD` and `TOOL_MANAGER` directly, allowing the application to run DRC, plot, export, and save the board without ever instantiating a window or `wxFrame`.
- **CCad Analogue**: This is critical proof of KiCad's efforts to decouple the UI from the domain logic. CCad's `ccad_core` must natively support a headless context out of the box because it is first and foremost an agentic engine. The CLI surface relies on an equivalent to `HEADLESS_BOARD_CONTEXT` to manipulate boards via `ccad pcb add-keepout` and similar commands.
