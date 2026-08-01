## ZONE_FILLER
- **File**: `pcbnew/zone_filler.h`, `pcbnew/zone_filler.cpp`
- **Purpose**: Performs the copper pour fill algorithm for all zone types (solid, hatch, copper thieving).
- **Key Method**: `Fill(aZones, aCheck, aParent)`: Top-level fill dispatcher that fills a list of ZONE objects, resolving thermal reliefs, clearances, hatch patterns, and island removal.
- **Algorithm Pipeline** (per zone per layer):
  1. `fillSingleZone(zone, layer, fillPolys)`: Main per-zone/layer entry point.
  2. `fillCopperZone(...)`: For copper layers: smoothes outline, subtracts higher-priority zones (`subtractHigherPriorityZones`), knocks out thermal reliefs (`knockoutThermalReliefs`), builds per-pad/track/zone clearances (`buildCopperItemClearances`, `buildDifferentNetZoneClearances`), handles hatch with `addHatchFillTypeOnZone`.
  3. `fillNonCopperZone(...)`: For non-copper (technical) layers: fills with the smoothed outline directly.
  4. `addHatchFillTypeOnZone(...)`: Stamps a hatch grid into the fill area, preserving thermal ring connectivity.
  5. `addCopperThievingPattern(...)`: Stamps netless copper shapes (dots, squares, hatch) for plating balance.
  6. `buildThermalSpokes(...)`: Constructs thermal spoke geometry for pad thermal reliefs.
  7. `buildHatchZoneThermalRings(...)`: Builds arc/polygon thermal rings for hatch zones.
  8. `postKnockoutMinWidthPrune(...)`: Deflate/reflate to remove min-width violations from zone-to-zone knockouts.
  9. `refillZoneFromCache(...)`: Fast iterative refill using pre-knockout fill snapshot cache.
  10. `connect_nearby_polys(...)`: Creates minimum-width bridging strands between nearby polygon islands.
- **Members**: `m_board`, `m_boardOutline`, `m_commit`, `m_progressReporter`, `m_maxError`, `m_worstClearance`, `m_preKnockoutFillCache` (optimization cache for iterative refill), `m_debugZoneFiller`.
- **Thread Safety**: Fill polys cache is mutex-guarded.

---

## ZONE_SETTINGS
- **File**: `pcbnew/zone_settings.h`, `pcbnew/zone_settings.cpp`
- **Purpose**: Data-transfer object (DTO) for all zone parameters, used by zone property dialogs.
- **Key Enums**:
  - `ZONE_FILL_MODE`: POLYGONS (solid), HATCH_PATTERN (grid), COPPER_THIEVING (netless stamping).
  - `THIEVING_PATTERN`: DOTS, SQUARES, HATCH.
  - `ZONE_BORDER_DISPLAY_STYLE`: NO_HATCH, DIAGONAL_FULL, DIAGONAL_EDGE, INVISIBLE_BORDER.
  - `ISLAND_REMOVAL_MODE`: ALWAYS, NEVER, AREA.
  - `PLACEMENT_SOURCE_T`: SHEETNAME, COMPONENT_CLASS, GROUP_PLACEMENT, DESIGN_BLOCK.
- **Supporting Structs**:
  - `ZONE_LAYER_PROPERTIES`: Per-layer zone properties (currently: optional `hatching_offset`).
  - `THIEVING_SETTINGS`: Thieving pattern parameters (pattern, element_size, gap, line_width, stagger, orientation).
- **Key Methods**:
  - `operator<<(const ZONE&)`: Imports all properties from a ZONE.
  - `ExportSetting(ZONE& target, aFullExport)`: Exports all or partial properties to a ZONE.
  - `CopyFrom(ZONE_SETTINGS, aCopyFull)`: Copies settings (optionally partial, for default zone settings).
  - `SetupLayersList(wxDataViewListCtrl*, PCBFrame, layers)`: Helper for zone dialog layer list population.
  - `GetDefaultSettings()`: Returns the global default ZONE_SETTINGS.
- **Context**: Passed between zone property dialogs and ZONE objects. Prevents dialogs from directly mutating live zone data until user confirms.

---

## TRACKS_CLEANER
- **File**: `pcbnew/tracks_cleaner.h`, `pcbnew/tracks_cleaner.cpp`
- **Purpose**: Cleans up routing defects: duplicate vias, short-circuit tracks, dangling tracks, collinear segment merging, tracks inside pads.
- **Key Method**: `CleanupBoard(aDryRun, aItemsList, aCleanVias, aRemoveMisConnected, aMergeSegments, aDeleteUnconnected, aDeleteTracksinPad, aDeleteDanglingVias, aReporter)`: Master cleanup routine with per-category flags.
- **Internal Cleanup Steps**:
  - `removeShortingTrackSegments()`: Removes segments connecting two different nets (short circuits).
  - `deleteDanglingTracks(aTracks, aVias)`: Removes segments/vias connected only on one end.
  - `deleteTracksInPads()`: Removes tracks fully inside a pad footprint.
  - `cleanup(dupVias, nullSegs, dupSegs, mergeSegs)`: Geometry-based: duplicate vias, zero-length segments, collinear merging.
  - `mergeCollinearSegments(seg1, seg2)`: Merges two collinear same-width/layer segments.
  - `testMergeCollinearSegments(seg1, seg2, dummy)`: Tests collinearity without modifying connectivity.
  - `testTrackEndpointIsNode(track, testStart, testEnd)`: Checks if an endpoint has multiple connections (is a node).
- **Members**: `m_brd`, `m_commit`, `m_dryRun`, `m_itemsList`, `m_reporter`, `m_connectedItemsCache` (O(n²) connection cache), `m_filter` (optional item filter callback).
- **Context**: Invoked via "Cleanup Tracks and Vias" tool. Supports dry-run mode to preview changes without modifying the board.

---

## zone_utils
- **File**: `pcbnew/zone_utils.h`, `pcbnew/zone_utils.cpp`
- **Purpose**: Standalone utility functions for zone management.
- **Functions**:
  - `MergeZonesWithSameOutline(aZones)`: Takes ownership of a set of zones, merges those with identical outlines and nets on different layers into single multi-layer zones, returns the consolidated result.
  - `AutoAssignZonePriorities(aBoard, aReporter)`: Analyzes overlapping zones, counts pads/vias per net in each overlap region, assigns higher priority to the net with more items. Runs pair-wise analysis in parallel via KiCad thread pool. Returns true if any priorities changed.
- **Context**: Used by the zone manager and fill engine to simplify multi-layer zone topology and resolve fill ordering conflicts.
