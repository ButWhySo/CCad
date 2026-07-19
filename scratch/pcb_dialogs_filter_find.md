# KiCad Source Walk: Dialogs - Filter Selection & Find (Batch 21)

## Overview
This batch covers the legacy VRML base UI (remaining from previous batch), the Selection Filter dialog (for selectively highlighting/selecting specific item types), and the Find Dialog (for searching by string).

- **Files**:
  - `pcbnew/dialogs/dialog_export_vrml_base.cpp`, `.h` (Base UI for VRML export)
  - `pcbnew/dialogs/dialog_filter_selection.cpp`, `.h`, `_base.cpp`, `_base.h`
  - `pcbnew/dialogs/dialog_find.cpp`, `.h`

## 1. Selection Filter (`dialog_filter_selection`)
- **Purpose**: Controls which types of items (footprints, tracks, vias, zones, texts, drawings, etc.) are included in the active selection when the user performs a drag-select or multi-select operation.
- **Mechanism**:
  - Manipulates a simple `OPTIONS` struct: `includeFootprints`, `includeTracks`, `includeVias`, etc.
  - Features a tri-state "All items" checkbox that updates automatically based on the tally of other checkboxes (`GetSuggestedAllItemsState()`).
  - When returning `wxID_OK`, the caller (`PCB_BASE_FRAME` or `SELECTION_TOOL`) reads the `OPTIONS` struct and filters the selection list.
- **CCad Relevance**: 
  - Selection filtering is a core interaction paradigm. In CCad, selection filtering should likely be pushed to the kernel or a dedicated view-model layer so that script/headless consumers can also perform bounding-box selections with type-filters applied without invoking a GUI dialog.

## 2. Find Dialog (`dialog_find`)
- **Purpose**: A floating dialog to search the board for items (footprints by ref/value, text items, markers, nets, zones) matching a specific string or wildcard.
- **Mechanism**:
  - Inherits from `BOARD_LISTENER` to invalidate its internal cache (`m_upToDate = false`) whenever the board is modified (item added, removed, changed).
  - Caches a list of matched items (`m_hitList`) which is a `std::deque<BOARD_ITEM*>`.
  - Iterates over all footprints, drawings, zones, markers, and nets to match the `EDA_SEARCH_DATA` parameters (`m_frame->GetFindReplaceData()`).
  - Supports traversing forward/backward through `m_hitList` and uses `ACTIONS::selectItem` to focus the item on the canvas.
- **CCad Relevance**: 
  - The `BOARD_LISTENER` pattern here is good—the Find dialog observes the board and invalidates its cache. In CCad, search is a core query API of the kernel. The GUI should simply send a query `Search(string, options)` and receive a list of IDs to highlight. The heavy lifting of iterating over all elements and string matching (wildcards, regex) belongs in the core C++ kernel.
