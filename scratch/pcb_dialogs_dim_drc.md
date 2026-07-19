# KiCad Source Walk: Dialogs - Dimension Properties & DRC (Batch 16)

## Overview
This batch covers the UI implementations for modifying dimension properties (line thickness, arrows, formats) and the core Design Rule Check (DRC) dialog interface.

- **Files**:
  - `pcbnew/dialogs/dialog_dimension_properties.cpp`, `.h`, `_base.cpp`, `_base.h`
  - `pcbnew/dialogs/dialog_drc.cpp`, `.h`, `_base.cpp`, `_base.h`

## 1. Dimension Properties (`dialog_dimension_properties`)
- **Purpose**: Exposes settings for PCB dimensions (Leader, Center, Aligned, etc.).
- **Mechanism**:
  - Adapts its UI based on the specific dimension type (`PCB_DIM_LEADER_T`, `PCB_DIM_CENTER_T`, etc.), hiding inapplicable controls (e.g., arrow length for Center dimensions).
  - Handles units conversion formatting (Mils, mm, Inches, Auto).
  - Handles text positioning (Manual vs Auto) and keeps text aligned to dimension lines (`GetKeepTextAligned()`).
  - Implements a preview window (`m_staticTextPreview`) dynamically updating via an internal clone (`m_previewDimension`) while the user edits parameters.

## 2. Design Rules Checker (`dialog_drc`)
- **Purpose**: The primary interface for launching the DRC engine and reviewing violations.
- **Mechanism**:
  - Implements a modeless dialog using a throttle-based yield to remain responsive while heavy engine operations execute.
  - Relies on `DRC_TOOL` to invoke `DRCEngine::InitEngine` and `RunTests`.
  - Tabbed interface (`m_runningResultsBook`) to flip between "Tests Running" (progress bar) and Results.
  - Results are populated via `RC_TREE_MODEL` into distinct DataView lists:
    - **Violations** (`m_markerDataView`)
    - **Unconnected Items** (`m_unconnectedDataView`)
    - **Schematic Parity** (`m_footprintsDataView`)
    - **Ignored Tests** (`m_ignoredList`)
  - Provides rich crossprobing. When a violation is selected, it resolves the UUIDs (`RC_ITEM::GetMainItemID()`, etc.) and commands the `PCB_EDIT_FRAME` to focus/zoom to the violating coordinates and activate the involved layers.
- **CCad Relevance**:
  - The DRC dialog logic here shows how to aggregate and present `RC_ITEM`s efficiently (via data views and tree models). Since CCad's DRC engine must be fundamentally headless and CLI-driven, the UI layer's primary job is rendering the `DRCE_` errors returned by the kernel and providing crossprobe logic. The separation of `DRC_TOOL` running the test and the UI displaying the tree is a pattern we should emulate, ensuring the CLI can just format the tree directly.
