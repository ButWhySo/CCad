## PNS::ROUTER
- **File**: `pcbnew/router/pns_router.h`, `pcbnew/router/pns_router.cpp`
- **Purpose**: Main router class. Orchestrates placement, dragging, shoving, and topology updates.
- **Modes** (`ROUTER_MODE`): PNS_MODE_ROUTE_SINGLE, PNS_MODE_ROUTE_DIFF_PAIR, PNS_MODE_TUNE_SINGLE, PNS_MODE_TUNE_DIFF_PAIR, PNS_MODE_TUNE_DIFF_PAIR_SKEW.
- **Drag Modes** (`DRAG_MODE`): DM_CORNER, DM_SEGMENT, DM_VIA, DM_FREE_ANGLE, DM_ARC, DM_COMPONENT.
- **States** (`RouterState`): IDLE, DRAG_SEGMENT, DRAG_COMPONENT, ROUTE_TRACK.
- **Key Methods**:
  - `SyncWorld()`: Populates the router NODE from the current board.
  - `StartRouting(pos, item, layer)`: Begin interactive routing from a position.
  - `Move(pos, item)`: Update routing head during mouse move.
  - `FixRoute(pos, item, forceFinish, forceCommit)`: Accept the current route at a position.
  - `ContinueFromEnd(newStartItem)`: Continue routing from the endpoint of the last route.
  - `UndoLastSegment()`: Remove the last placed segment.
  - `CommitRouting()`: Push changes to the BOARD via ROUTER_IFACE::Commit().
  - `StopRouting()`: Abort routing and revert to IDLE.
  - `StartDragging(pos, item, dragMode)`: Begin drag of a single item.
  - `StartDragging(pos, items, DM_COMPONENT)`: Begin drag of a component.
  - `FlipPosture()`: Flip the routing posture (horizontal-first vs vertical-first).
  - `SwitchLayer(layer)`: Switch active routing layer (adds via if needed).
  - `ToggleViaPlacement()`: Enable/disable via insertion during routing.
  - `ToggleCornerMode()`: Cycle through 45°, arc, free-angle corner modes.
  - `GetUpdatedItems(removed, added, heads)`: Get items modified in the last routing step.
  - `QueryHoverItems(pos, slopRadius)`: Find items near a screen position.
  - `GetNearestRatnestAnchor(otherEnd, layers, item)`: Snap to nearest ratsnest target.
- **Owned Objects**: `m_world` (NODE), `m_placer` (PLACEMENT_ALGO), `m_dragger` (DRAG_ALGO), `m_shove` (SHOVE).

---

## ROUTER_IFACE (abstract)
- **File**: `pcbnew/router/pns_router.h`
- **Purpose**: Abstract interface between PNS router logic and the KiCad board model. Implemented by `PNS_KICAD_IFACE`.
- **Key Virtual Methods**:
  - `SyncWorld(node)`: Populate the PNS NODE from the board.
  - `AddItem/UpdateItem/RemoveItem`: Push changes to the real board COMMIT.
  - `Commit()`: Finalize the BOARD_COMMIT.
  - `DisplayItem/DisplayPathLine/DisplayRatline/HideItem`: GAL preview rendering.
  - `ImportSizes(sizes, startItem, net, pos)`: Load net-class rules into SIZES_SETTINGS.
  - `StackupHeight(firstLayer, secondLayer)`: Physical board thickness between layers (for impedance).
  - `GetRuleResolver()`: Returns the RULE_RESOLVER connecting PNS to DRC_ENGINE.
  - `CalculateRoutedPathLength/CalculateRoutedPathDelay/CalculateLengthForDelay`: Length and delay computation for tuning.

---

## PNS::NODE
- **File**: `pcbnew/router/pns_node.h`, `pcbnew/router/pns_node.cpp`
- **Purpose**: The "world" of the router — a spatial and topological index of all routing items (segments, arcs, vias, solids/pads). Supports lightweight branching for speculative routing.
- **Key Concepts**:
  - **Branching**: `Branch()` creates a copy-on-write child node that tracks added/removed items vs parent. `Commit(childNode)` merges child changes to root. Used by SHOVE for springback rollback.
  - **Joints**: `JOINT` objects map positions+net+layers to the set of items touching that point. Updated automatically by add/remove.
  - **Bulk population**: `BeginBulkAdd()` / `FinalizeBulkAdd()` defers spatial index rebuilds during board sync.
- **Key Queries**:
  - `QueryColliding(item, obstacles, opts)`: Find all items colliding with a given item.
  - `NearestObstacle(line, opts)`: Find the nearest obstacle along a routing line.
  - `CheckColliding(item/set)`: Quick boolean collision check.
  - `HitTest(point)`: Find all items at a point.
  - `FindJoint(pos, layer, net)`: Look up joint at a position.
  - `AssembleLine(seg)`: Walk joints to assemble a full routed line from a segment.
  - `FindLinesBetweenJoints`: Find all lines between two specific joints.
  - `AllItemsInNet(net, items, mask)`: Get all items on a net.
- **Key Edit Methods**: `Add(seg/solid/via/arc/line)`, `Remove(...)`, `Replace(old, new)`.
- **Clearance Resolution**: Delegates to `RULE_RESOLVER` via `GetClearance(a, b)`.

---

## RULE_RESOLVER (abstract, in pns_node.h)
- **Purpose**: Provides PNS with clearance and constraint values without depending on DRC_ENGINE directly. Implemented by `PNS_KICAD_IFACE_BASE`.
- **Key Methods**:
  - `Clearance(a, b)`: Required clearance between two PNS items.
  - `QueryConstraint(type, a, b, layer, constraint)`: Query a specific constraint type.
  - `DpCoupledNet/DpNetPolarity/DpNetPair`: Differential pair net resolution.
  - `IsKeepout(obstacle, item, enforce)`: Keepout/rule-area query.
  - `IsInNetTie/IsNetTieExclusion`: Net-tie exemption queries.
  - `HullCache(item, clearance, walkaroundThickness, layer)`: Cached convex hull for walkaround.

---

## PNS::SHOVE
- **File**: `pcbnew/router/pns_shove.h`, `pcbnew/router/pns_shove.cpp`
- **Purpose**: The push-and-shove engine. Moves existing tracks out of the way of a new route.
- **Key Design**:
  - Works on a **branch** of NODE to speculatively shove. If the shove is impossible or creates violations, the branch is discarded (springback).
  - `ShoveLines(currentHead)`: Main entry — shove all obstacles in the path of `currentHead`.
  - `ShoveMultiLines(heads)`: Shove multiple lines simultaneously (used for component drag).
  - `ShoveDraggingVia(via, target)`: Shove when dragging a via.
  - `SpringBack(node)`: Undo a speculative shove by discarding the branch.
  - `RewindSpringbackTo(node)`: Step back to a prior springback checkpoint.
  - Priority: SHOVE_RESULT enum — SH_OK, SH_TRY_WALK, SH_INCOMPLETE, SH_HEAD_MODIFIED, SH_EMPTY.

---

## PNS::LINE_PLACER
- **File**: `pcbnew/router/pns_line_placer.h`, `pcbnew/router/pns_line_placer.cpp`
- **Purpose**: Handles single-net interactive routing — walk, shove, and 45°/arc/free-angle corner modes.
- **Key Methods**:
  - `Start(pos, startItem)`: Initialize routing from a pad or track end.
  - `Move(pos, endItem)`: Extend the current routing head to a new position.
  - `FixRoute(pos, endItem, forceFinish)`: Lock the current route head into the board.
  - `HasPlacedAnything()`: True if at least one segment has been committed.
  - `FlipPosture()`: Switch horizontal-first vs vertical-first.
  - `ToggleVia(enable)`: Insert or remove a via at the current head.
  - Delegates to `WALKAROUND` or `SHOVE` depending on routing mode setting.

---

## PNS::DIFF_PAIR_PLACER
- **File**: `pcbnew/router/pns_diff_pair_placer.h`, `pcbnew/router/pns_diff_pair_placer.cpp`
- **Purpose**: Routes differential pairs (two coupled nets) simultaneously, maintaining gap and coupling constraints.
- **Key Methods**: `Start`, `Move`, `FixRoute` (similar to LINE_PLACER).
- Couples two LINE_PLACERs and enforces `CT_DIFF_PAIR_GAP` from RULE_RESOLVER.

---

## PNS::WALKAROUND
- **File**: `pcbnew/router/pns_walkaround.h`, `pcbnew/router/pns_walkaround.cpp`
- **Purpose**: Alternative routing mode that walks around obstacles rather than shoving them. Uses convex-hull polygon routing.
- **Modes**: WALKAROUND_RESULT enum: WR_DONE, WR_FAILED, WR_UNROUTABLE.

---

## PNS::OPTIMIZER
- **File**: `pcbnew/router/pns_optimizer.h`, `pcbnew/router/pns_optimizer.cpp`
- **Purpose**: Post-placement route optimization. Straightens, merges collinear segments, and shortens routed paths.
- **Strategies**: MERGE_SEGMENTS, MERGE_OBTUSE, SMART_PADS, FANOUT_CLEANUP, KEEP_TOPOLOGY, PRESERVE_VERTEX, RESTRICT_AREA.

---

## PNS::KICAD_IFACE / PNS_KICAD_IFACE_BASE
- **File**: `pcbnew/router/pns_kicad_iface.h`, `pcbnew/router/pns_kicad_iface.cpp`
- **Purpose**: The concrete implementation of `ROUTER_IFACE` and `RULE_RESOLVER` for KiCad.
- **Key Responsibilities**:
  - `SyncWorld(node)`: Iterates all BOARD items (tracks, pads, vias, zones as obstacles) and populates the PNS NODE.
  - `Commit()`: Applies the PNS NODE diff to the board via BOARD_COMMIT.
  - `GetRuleResolver()`: Returns `PNS_KICAD_IFACE_BASE*` as `RULE_RESOLVER*`.
  - Clearance resolution via `DRC_ENGINE::EvalClearanceBatch()`.
  - Layer mapping between PCB_LAYER_ID and PNS layer integers.
  - Length/delay computation using physical board stackup data.
