# KiCad Source Walk: Dialogs - Find By Properties & Associations (Batch 22)

## Overview
This batch explores the "Find by Properties" dialog (a more advanced, property-grid-based search tool) and the Footprint Associations dialog (which displays linkage information between footprints and schematic symbols).

- **Files**:
  - `pcbnew/dialogs/dialog_find_base.cpp`, `.h` (Base UI for Find from previous batch)
  - `pcbnew/dialogs/dialog_find_by_properties.cpp`, `.h`, `_base.cpp`, `_base.h`
  - `pcbnew/dialogs/dialog_footprint_associations.cpp`, `.h`

## 1. Find by Properties (`dialog_find_by_properties`)
- **Purpose**: Allows selecting/finding items by matching their properties (e.g. all pads with hole size X, all footprints with certain field values). It provides a Property Grid UI for visual selection, and an expression/query editor (Scintilla) for advanced users.
- **Mechanism**:
  - Uses `PROPERTY_MANAGER` to reflectively query the properties of the selected items.
  - Computes the intersection of properties for the currently selected items to build the grid (`rebuildPropertyGrid()`).
  - Supports a query expression language. The query editor (`wxStyledTextCtrl` configured via `SCINTILLA_TRICKS`) allows typing SQL-like expressions using aliases for fields (e.g., `A.Reference == 'U1'`).
  - Evaluates expressions (likely via `PCBEXPR_EVALUATOR` or similar expression engine used elsewhere in KiCad's DRC).
- **CCad Relevance**: 
  - The reflection system (`PROPERTY_MANAGER`) and expression engine are powerful. In CCad, a similar property reflection system (already planned via typed C++ structs/JSON) will be essential for LLMs to construct dynamic queries without hardcoding logic for every specific type of object attribute.

## 2. Footprint Associations (`dialog_footprint_associations`)
- **Purpose**: Displays a read-only dialog showing the library/footprint metadata (Library Name, Footprint Name, Descriptions) and the hierarchical symbol path (Sheet path -> Symbol) that the footprint is linked to in the schematic.
- **Mechanism**:
  - Uses `FOOTPRINT_LIBRARY_ADAPTER` to load the library description from the footprint library table (`fp-lib-table`).
  - Iterates through the footprint's hierarchical `KIID_PATH` to show the sheet structure.
  - Queries the schematic side via `Kiface().ExpressMail(FRAME_SCH, ...)` to resolve schematic item metadata in real-time (IPC/Mailbox pattern between Eeschema and Pcbnew).
- **CCad Relevance**: 
  - Highlights KiCad's "ExpressMail" inter-process communication for resolving schematic cross-references. CCad's unified or clearly separated client-server model should make such cross-domain queries much cleaner (e.g., a single query to a project-level graph).
