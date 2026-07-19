# KiCad Source Walk: Dialogs - DRC Job Config, Pad Enum, Footprint Exchange (Batch 17)

## Overview
This batch investigates the dialogs used for configuring DRC Jobs, sequential pad enumeration, and the footprint update/exchange mechanisms.

- **Files**:
  - `pcbnew/dialogs/dialog_drc_job_config.cpp`, `.h`
  - `pcbnew/dialogs/dialog_enum_pads.cpp`, `.h`, `_base.cpp`, `_base.h`
  - `pcbnew/dialogs/dialog_exchange_footprints.cpp`, `.h`

## 1. DRC Job Config (`dialog_drc_job_config`)
- **Purpose**: Configuration dialog for a PCB DRC batch job (`JOB_PCB_DRC`).
- **Mechanism**: Inherits from `DIALOG_RC_JOB`. Simply transfers checkboxes for "Report all track errors", "Check schematic parity", and "Refill zones" to the underlying job object.

## 2. Pad Enumeration (`dialog_enum_pads`)
- **Purpose**: Dialog to set parameters for the sequential pad numbering tool.
- **Mechanism**:
  - Exposes `m_start_number`, `m_step`, and an optional `m_prefix`.
  - The limits are enforced by `wxSpinCtrl` (0 to 999) and `wxTextCtrl` maxlength of 4 characters.
  - The data model is `SEQUENTIAL_PAD_ENUMERATION_PARAMS`.

## 3. Exchange Footprints (`dialog_exchange_footprints`)
- **Purpose**: Handles both "Update Footprint" (refreshing from library) and "Change Footprint" (swapping a footprint for a different library part).
- **Mechanism**:
  - **Match modes**: Can match against ALL footprints, SELECTED footprints, specific Reference Designator, specific Value, or a specific Library ID (`LIB_ID`).
  - **Filtering**: Iterates through `m_parent->GetBoard()->Footprints()` (in reverse to safely modify). Evaluates `isMatch()`.
  - **Execution**: Uses `PCB_EDIT_FRAME::ExchangeFootprint()`, driving it through a `BOARD_COMMIT`.
  - **Fidelity Settings**: Extensive settings on what attributes to reset vs retain during the exchange (Pad Positions, Extra pads, Text Items (Layers, Effects, Positions, Content), Fabrication attributes, Clearance overrides, 3D models).
- **CCad Relevance**: 
  - The `DIALOG_EXCHANGE_FOOTPRINTS` is mostly just a view mapping onto `PCB_EDIT_FRAME::ExchangeFootprint()`. In CCad, the logic for finding matches and pushing the updates through a transaction should reside purely in `ccad_core`. The granular update flags (`m_resetTextItemLayers`, etc.) highlight the complexity of updating a placed symbol vs destroying it and creating a new one. CCad must support these exact granular retention policies when users swap footprint packages.
