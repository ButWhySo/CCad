# KiCad Source Walk: Board Stackup Manager (Batch 9)

## Overview
This batch enters the `pcbnew/board_stackup_manager/` directory, focusing on the physical material parameters of a PCB's layer stackup (copper, dielectrics, soldermask, and silkscreen).

- **Files**:
  - `pcbnew/board_stackup_manager/board_stackup_reporter.cpp`, `.h`
  - `pcbnew/board_stackup_manager/dialog_dielectric_list_manager.cpp`, `.h`
  - `pcbnew/board_stackup_manager/dialog_dielectric_list_manager_base.cpp`, `.h`
  - `pcbnew/board_stackup_manager/dielectric_material.cpp`, `.h`

## 1. Dielectric Materials (`dielectric_material.cpp`, `.h`)
- **`DIELECTRIC_SUBSTRATE`**: A struct modeling a physical material used in the PCB stackup. It tracks:
  - `m_Name` (e.g., "FR4", "Polyimide", "PTFE")
  - `m_EpsilonR` (Relative permittivity, critical for impedance calculations)
  - `m_LossTangent` (tanδ, dielectric loss tangent, critical for high-frequency signal integrity).
- **`DIELECTRIC_SUBSTRATE_LIST`**: Maintains a predefined library of standard materials used for Dielectrics, Soldermask, and Silkscreen.
- **CCad Analogue**: In an LLM-native workflow, a material stackup is a key constraint input for the LLM when planning high-speed routing. The data definitions in `dielectric_material` are exactly the type of JSON/protobuf schema `ccad_core` will need to track and expose over the IPC API to agents.

## 2. Dialog Material Manager (`dialog_dielectric_list_manager.cpp`, `.h`, `_base`)
- A wxWidgets dialog (`DIALOG_DIELECTRIC_MATERIAL`) allowing users to pick a predefined material or input custom `EpsilonR` and `LossTangent` values.
- It inherits from the wxFormBuilder generated base class `DIALOG_DIELECTRIC_MATERIAL_BASE`.
- **CCad Analogue**: CCad's Qt6 GUI will eventually need a way to visualize the stackup, but the source of truth for the stackup parameters will live purely in `ccad_core`.

## 3. Stackup Reporter (`board_stackup_reporter.cpp`, `.h`)
- **`BuildStackupReport`**: Generates a human-readable ASCII report of the `BOARD_STACKUP` layers, including types (dielectric, copper), sublayers, thicknesses, materials, Epsilon R, and Loss Tangent.
- It also reports on board finish types (e.g., ENIG, HASL), edge plating, and impedance control constraints.
- **CCad Analogue**: This is extremely similar to how an agent would request the stackup state. CCad will likely require an IPC command like `ccad pcb get-stackup` that dumps this exact information into a structured JSON format so the LLM agent can understand the physical board constraints before routing.
