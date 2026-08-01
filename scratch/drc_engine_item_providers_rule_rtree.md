## DRC_ENGINE
- **File**: `pcbnew/drc/drc_engine.h`, `pcbnew/drc/drc_engine.cpp`
- **Purpose**: The central DRC orchestrator. Parses rules, compiles constraints, and runs all test providers.
- **Inheritance**: `DRC_ENGINE` → `UNITS_PROVIDER`.
- **Key Methods**:
  - `InitEngine(aRulePath)`: Loads and parses a custom rules file, generates implicit rules from board settings, and compiles the full constraint map.
  - `RunTests(units, reportAllTrackErrors, testFootprints, commit)`: Iterates all registered `DRC_TEST_PROVIDER` instances and runs them.
  - `EvalRules(constraintType, a, b, layer, reporter)`: Evaluates all matching rules for two items on a layer, returning the winning `DRC_CONSTRAINT` (the most restrictive, or the highest-priority user rule).
  - `EvalZoneConnection(a, b, layer, reporter)`: Specialized rule evaluation for zone-to-pad connection type.
  - `EvalClearanceBatch(a, b, layer)`: Batch evaluation of all clearance types in one call — used by PNS router to reduce per-query overhead.
  - `GetCachedOwnClearance(item, layer, source)`: Cached own-clearance lookup for rendering performance.
  - `InvalidateClearanceCache(uuid)` / `ClearClearanceCache()` / `InitializeClearanceCache()`: Cache lifecycle management.
  - `ReportViolation(item, pos, layer, pathGenerator)`: Fires the installed `DRC_VIOLATION_HANDLER` with a new marker.
  - `QueryWorstConstraint(ruleId, constraint)`: Returns the tightest constraint value for a given type across all rules.
  - `QueryDistinctConstraints(constraintId)`: Returns distinct constraint values for use in DRC optimization.
  - `GetItemsMatchingCondition(expression, constraint, reporter)`: Evaluates a DRC expression against all board items.
  - `IsNetADiffPair(board, net, netP, netN)` / `MatchDpSuffix(netName, complementNet, baseDpName)`: Differential pair detection utilities.
  - `IsNetTieExclusion(trackNet, layer, pos, collidingItem)`: Checks if a collision is within a net-tie pad (legal crossing).
- **Constraint Resolution**: Rules stored as `DRC_ENGINE_CONSTRAINT` entries in `m_constraintMap` (keyed by `DRC_CONSTRAINT_T`). Includes implicit keepout zone bounding-box fast path optimization.
- **Caches**:
  - `m_ownClearanceCache` (`unordered_map<DRC_OWN_CLEARANCE_CACHE_KEY, int>`): Own clearance per (UUID, layer).
  - `m_netclassClearances` (`unordered_map<wxString, int>`): Netclass name → clearance.
  - `m_clearanceCacheMutex` (`shared_mutex`): Reader-writer lock for thread-safe rendering.
- **Test Providers**: `m_testProviders` — vector of registered `DRC_TEST_PROVIDER*`. One per check category.

---

## DRC_ITEM / PCB_DRC_CODE
- **File**: `pcbnew/drc/drc_item.h`, `pcbnew/drc/drc_item.cpp`
- **Purpose**: Represents a single DRC violation. Factory for all PCB DRC error types.
- **`PCB_DRC_CODE` Enum** (all error codes, `DRCE_FIRST` to `DRCE_LAST`):
  - **Electrical**: UNCONNECTED_ITEMS, SHORTING_ITEMS, ALLOWED_ITEMS, TEXT_ON_EDGECUTS, CLEARANCE, CREEPAGE, TRACKS_CROSSING, EDGE_CLEARANCE, ZONES_INTERSECT, ISOLATED_COPPER, STARVED_THERMAL, DANGLING_VIA, DANGLING_TRACK.
  - **Drill/Via/Pad**: DRILLED_HOLES_TOO_CLOSE, DRILLED_HOLES_COLOCATED, HOLE_CLEARANCE, TRACK_WIDTH, TRACK_ANGLE, TRACK_SEGMENT_LENGTH, ANNULAR_WIDTH, CONNECTION_WIDTH, DRILL_OUT_OF_RANGE, VIA_DIAMETER, PADSTACK, PADSTACK_INVALID, MICROVIA_DRILL_OUT_OF_RANGE.
  - **Footprint/Courtyard**: OVERLAPPING_FOOTPRINTS, MISSING_COURTYARD, MALFORMED_COURTYARD, PTH_IN_COURTYARD, NPTH_IN_COURTYARD.
  - **Board**: DISABLED_LAYER_ITEM, INVALID_OUTLINE.
  - **Netlist Parity**: MISSING_FOOTPRINT, DUPLICATE_FOOTPRINT, EXTRA_FOOTPRINT, NET_CONFLICT, SCHEMATIC_PARITY, FOOTPRINT_FILTERS, FOOTPRINT_TYPE_MISMATCH.
  - **Library Parity**: LIB_FOOTPRINT_ISSUES, LIB_FOOTPRINT_MISMATCH, PAD_TH_WITH_NO_HOLE, FOOTPRINT.
  - **Custom Rules**: UNRESOLVED_VARIABLE, ASSERTION_FAILURE, GENERIC_WARNING, GENERIC_ERROR.
  - **Aesthetics/DFM**: COPPER_SLIVER, SOLDERMASK_BRIDGE, SILK_MASK_CLEARANCE, SILK_EDGE_CLEARANCE, SILK_CLEARANCE, TEXT_HEIGHT, TEXT_THICKNESS.
  - **Signal Integrity**: LENGTH_OUT_OF_RANGE, NET_CHAIN_STUB_TOO_LONG, NET_CHAIN_RETURN_PATH_BREAK, SKEW_OUT_OF_RANGE, VIA_COUNT_OUT_OF_RANGE, DIFF_PAIR_GAP_OUT_OF_RANGE, DIFF_PAIR_UNCOUPLED_LENGTH_TOO_LONG.
  - **Readability**: MIRRORED_TEXT_ON_FRONT_LAYER, NONMIRRORED_TEXT_ON_BACK_LAYER.
  - **Tuning**: MISSING_TUNING_PROFILE, TUNING_PROFILE_IMPLICIT_RULES.
  - **Manufacturing**: TRACK_ON_POST_MACHINED_LAYER, TRACK_NOT_CENTERED_ON_VIA.
  - **Misc**: SCHEMATIC_FIELDS_PARITY.
- **`DRC_ITEM` class**: Extends `RC_ITEM`. Created via factory `DRC_ITEM::Create(errorCode)`. Has `m_violatingRule` and `m_violatingTest` to trace which rule and provider triggered the violation.
- **`DRC_ITEMS_PROVIDER` class**: `RC_ITEMS_PROVIDER` adapter for browsing DRC markers on a `BOARD`, filtered by severity and marker type.

---

## DRC Test Providers (overview)
- **File**: `pcbnew/drc/drc_test_provider*.cpp`
- **Purpose**: Each file implements a specialized DRC check. All are `DRC_TEST_PROVIDER` subclasses registered with `DRC_ENGINE`.
- **Providers and their checks**:
  - `drc_test_provider_copper_clearance`: Track-to-track, track-to-pad, pad-to-pad clearance.
  - `drc_test_provider_hole_to_hole`: Drill-to-drill separation.
  - `drc_test_provider_hole_size`: Min/max drill sizes.
  - `drc_test_provider_annular_width`: Via annular ring size.
  - `drc_test_provider_connection_width`: Copper connection width (net connections).
  - `drc_test_provider_track_width`: Track width min/max.
  - `drc_test_provider_track_angle`: Angle between connected tracks.
  - `drc_test_provider_track_segment_length`: Segment length min/max.
  - `drc_test_provider_via_diameter`: Via diameter min/max.
  - `drc_test_provider_connectivity`: Unconnected net checks.
  - `drc_test_provider_courtyard_clearance`: Footprint courtyard overlap.
  - `drc_test_provider_edge_clearance`: Copper-to-board-edge clearance.
  - `drc_test_provider_silk_clearance`: Silkscreen clearance to pads/copper/edge.
  - `drc_test_provider_solder_mask`: Solder mask web thickness.
  - `drc_test_provider_physical_clearance`: Physical item clearances (non-copper).
  - `drc_test_provider_creepage`: Creepage distance violations.
  - `drc_test_provider_disallow`: Rule-area keepout violations.
  - `drc_test_provider_misc`: Miscellaneous checks (pad stack validity, etc.).
  - `drc_test_provider_footprint_checks`: Basic footprint integrity.
  - `drc_test_provider_library_parity`: Library footprint vs board footprint comparison.
  - `drc_test_provider_schematic_parity`: Board vs schematic netlist comparison.
  - `drc_test_provider_matched_length`: Length/skew/via count matching for high-speed design.
  - `drc_test_provider_diff_pair_coupling`: Differential pair gap and coupling check.
  - `drc_test_provider_sliver_checker`: Detects copper slivers (thin disconnected copper).
  - `drc_test_provider_zone_connections`: Zone thermal spoke and island checks.
  - `drc_test_provider_text_dims`: Text height and thickness min/max.
  - `drc_test_provider_text_mirroring`: Correct text orientation per layer.

---

## DRC_RULE / DRC_CONSTRAINT
- **File**: `pcbnew/drc/drc_rule.h`, `pcbnew/drc/drc_rule.cpp`
- **Purpose**: Parsed representation of a `.kicad_dru` design rule.
- **Key Fields**:
  - `DRC_RULE`: Has a name, optional condition (`DRC_RULE_CONDITION*`), and a list of `DRC_CONSTRAINT` objects.
  - `DRC_CONSTRAINT`: Has a `DRC_CONSTRAINT_T` type (clearance/track_width/via_count/etc.) and a `MINOPTMAX<int>` value triple (min/opt/max).
  - `DRC_CONSTRAINT_T`: Enum of all constraint types (clearance, track_width, via_count, hole_size, annular_width, connection_width, skew, length, diff_pair_gap, physical_clearance, edge_clearance, silk_clearance, disallow, zone_connection, teardrops, assertion, micro_via, blind_buried_via, etc.).
  - Implicit source: `DRC_IMPLICIT_SOURCE` — marks rules auto-generated from board settings (netclasses, design rules, zone keepouts).

---

## DRC_RTREE
- **File**: `pcbnew/drc/drc_rtree.h`
- **Purpose**: Spatial index for DRC clearance queries. Wraps an RTree for fast O(log n) bounding-box query of board items by layer and geometry.
- **Key Design**: Stores `BOARD_ITEM*` pointers indexed by per-layer bounding boxes. Used in copper clearance, hole-to-hole, and edge clearance tests to rapidly find candidate pairs without O(n²) exhaustive search.
