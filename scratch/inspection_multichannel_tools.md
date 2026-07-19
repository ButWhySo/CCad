## ZONE_FILLER_TOOL / PAD_TOOL / GLOBAL_EDIT_TOOL / CONVERT_TOOL

### ZONE_FILLER_TOOL
- **File**: `pcbnew/tools/zone_filler_tool.h/cpp`
- **Purpose**: Tool wrapper for zone filling. Manages ZONE_FILLER lifecycle with dirty-tracking.
- **Key**: FillAllZones/CheckAllZones, ZoneFill/All/Dirty/Unfill/UnfillAll. DirtyZone(zone) adds UUID to m_dirtyZoneIDs set. ZoneFillDirty only refills dirty zones. IsBusy() / rebuildConnectivity() after fill.

### PAD_TOOL
- **File**: `pcbnew/tools/pad_tool.h/cpp`
- **Purpose**: Pad-specific interactive/global ops in footprint editor.
- **Key**: PlacePad, EnumeratePads (sequential numbering), PadTable (spreadsheet editor), EditPad / ExitPadEditMode (WYSIWYG shape edit mode with HIGH_CONTRAST_MODE switch), RecombinePad (combine overlapping pad shapes), explodePad (decompose to constituent shapes), pastePadProperties / copyPadSettings / pushPadSettings (board-wide pad style sync).

### GLOBAL_EDIT_TOOL
- **File**: `pcbnew/tools/global_edit_tool.h/cpp`
- **Purpose**: Batch board-wide edit operations.
- **Key**: ExchangeFootprints (update/swap footprint defs), SwapLayers (remap items between layers), EditTracksAndVias/EditTextAndGraphics/EditTeardrops (style update dialogs), GlobalDeletions (batch delete by type), CleanupTracksAndVias/CleanupGraphics, RemoveUnusedPads, ZonesManager, Migrate3DModels.

### CONVERT_TOOL
- **File**: `pcbnew/tools/convert_tool.h/cpp`
- **Purpose**: Geometry type conversion.
- **Key**: CreatePolys (chains open segments/arcs → polygon zone/keepout via CONVERT_STRATEGY), CreateLines (explodes closed polygon → segments), SegmentToArc (straight → arc), OutsetItems (create outset polygon copies). Internals: makePolysFromChainedSegs / makePolysFromOpenGraphics / makePolysFromClosedGraphics.

---

## BOARD_INSPECTION_TOOL
- **File**: `pcbnew/tools/board_inspection_tool.h/cpp`
- **Purpose**: Board analysis and live inspection. Also handles ratsnest management and net highlighting.
- **Inherits**: `PCB_TOOL_BASE` + `wxEvtHandler`.
- **Key Methods**:
  - `HighlightNet(event)` / `HighlightNetChain(event)` / `ClearHighlight(event)`: Net highlight overlay. Tracks `m_currentlyHighlighted` and `m_lastHighlighted` sets for toggle.
  - `HighlightItem(event)`: Respond to Eeschema cross-probe — zoom to item and highlight net.
  - `UpdateLocalRatsnest(event)` / `HideLocalRatsnest(event)` / `LocalRatsnestTool(event)`: Per-component/selection dynamic ratsnest management. Uses `m_dynamicData` (`CONNECTIVITY_DATA*`).
  - `HideNetInRatsnest/ShowNetInRatsnest(event)`: Toggle net visibility in ratsnest.
  - `ShowBoardStatistics(event)`: Opens board stats dialog.
  - `InspectClearance(event)`: Show clearance resolution for two selected items. Uses `makeDRCEngine()` to evaluate rules and writes a `reportClearance()` report.
  - `InspectConstraints(event)`: Show DRC constraint report for a single item.
  - `DiffFootprint(footprint, parent)`: Shows a diff of a board footprint vs its library definition in `FOOTPRINT_DIFF_WIDGET`.
  - `InspectDRCError(drcItem)` / `InspectDRCErrorMenuText(drcItem)`: Drill into a specific DRC violation for more details.
  - `ShowFootprintLinks(event)`: Show schematic↔PCB footprint associations.
- **Internal helpers**: `makeDRCEngine()` (builds a short-lived DRC_ENGINE for inspection), `pickItemForInspection()` (interactive hover picker), `filterCollectorForInspection()`, `calculateSelectionRatsnest()`, `reportHeader/reportClearance/reportCompileError()`.

---

## MULTICHANNEL_TOOL
- **File**: `pcbnew/tools/multichannel_tool.h/cpp`
- **Purpose**: Repeat-layout / multichannel PCB design. Copies a reference placement+routing block to topologically matched target blocks, enabling DDR/RF/power-converter pattern-based layout.
- **Inherits**: `PCB_TOOL_BASE` + `PCB_PICKER_TOOL::RECEIVER`.
- **Core Concepts**:
  - **RULE_AREA**: A placement zone linked to a source (sheet, component class, or group). Has components, design block items, zone, center point, sheet path, rule name.
  - **RULE_AREA_COMPAT_DATA**: Result of checking whether a target area is topologically compatible with a reference area. Contains `TMATCH::COMPONENT_MATCHES` (net/footprint correspondence), mismatch reasons, affected/groupable items after copy.
  - **REPEAT_LAYOUT_OPTIONS**: Copy flags: copyRouting, connectedRoutingOnly, copyPlacement, copyOtherItems, groupItems, includeLockedItems, anchorFp.
  - **RULE_AREAS_DATA**: Container for all rule areas + compatibility map.
- **Key Methods**:
  - `AutogenerateRuleAreas(event)`: Auto-generates placement rule area zones from sheets, component classes, or groups.
  - `RepeatLayout(event, refZone)` / `RepeatLayout(event, refArea, targetArea, options)`: Copy reference block to target. Calls `resolveConnectionTopology()` then `copyRuleAreaContents()`.
  - `GeneratePotentialRuleAreas()`: Scans board for candidate rule areas.
  - `FindExistingRuleAreas()`: Finds already-created rule area zones.
  - `CheckRACompatibility(refZone)`: Checks if target RAs are topologically compatible with ref.
  - `resolveConnectionTopology(refArea, targetArea, matches, params)`: Uses `TMATCH` (topology matching from `connectivity/topo_match.h`) to find the net correspondence between ref and target components.
  - `copyRuleAreaContents(refArea, targetArea, commit, opts, compatData)`: Applies the copy — transforms footprint placements, duplicates routing, fixes up nets, and optionally groups items.
  - `fixupNet(ref, target, componentMatches)`: Reassigns net code on a target copper item based on TMATCH mapping.
  - `buildRAOutline(footprints/items, margin)`: Computes convex hull of a component set as a placement zone boundary.
  - `queryComponentsInSheet/ComponentClass/Group()`: Collectors for populating rule areas.
- **Dependency**: Uses `TMATCH::ISOMORPHISM_PARAMS` and `COMPONENT_MATCHES` from `connectivity/topo_match.h` — a graph isomorphism engine for matching netlists.
