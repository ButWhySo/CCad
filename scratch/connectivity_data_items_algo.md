## CONNECTIVITY_DATA
- **File**: `pcbnew/connectivity/connectivity_data.h`, `pcbnew/connectivity/connectivity_data.cpp`
- **Purpose**: Public API for all electrical connectivity queries on a BOARD. Wraps `CN_CONNECTIVITY_ALGO` and `RN_NET`-based ratsnest.
- **Key Methods**:
  - `Build(aBoard, aReporter)`: Full connectivity rebuild from all board items.
  - `Build(globalConnectivity, localItems)`: Incremental build for a subset of items.
  - `Add/Remove/Update(aItem)`: Incremental item lifecycle.
  - `PropagateNets(aCommit)`: Propagate net codes from pads to tracks/vias/zones.
  - `RecalculateRatsnest(aCommit)`: Rebuild the ratsnest (unconnected edges) for all nets.
  - `FillIsolatedIslandsMap(map)`: Fill `ISOLATED_ISLANDS` per (zone, layer).
  - `GetUnconnectedCount(visibleOnly)`: Remaining unconnected edges.
  - `IsConnectedOnLayer(item, layer, types)`: Check if an item has connections on a specific layer.
  - `GetConnectedItems(item, flags)`: Items physically connected to `item`. Flags: IGNORE_NETS, EXCLUDE_ZONES.
  - `GetConnectedTracks/GetConnectedPads/GetConnectedPadsAndVias(item)`: Type-filtered connection queries.
  - `GetConnectedItemsAtAnchor(item, anchor, types, maxError)`: Items connected at a specific position.
  - `GetNetItems(netCode, types)`: All items belonging to a net.
  - `GetRatsnestForNet(aNet)`: `RN_NET*` for a net's ratsnest.
  - `GetRatsnestForItems/GetRatsnestForPad/GetRatsnestForComponent(...)`: Ratsnest edges for item subsets.
  - `ComputeLocalRatsnest(items, dynamicData, offset)`: Dynamic ratsnest while moving items.
  - `ClearLocalRatsnest/HideLocalRatsnest`: Selection-based ratsnest management.
  - `TestTrackEndpointDangling(track, ignoreInPads, pos)`: Check dangling track endpoint.
  - `RunOnUnconnectedEdges(func)`: Iterate unconnected ratsnest edges.
  - `BlockRatsnestItems(items)`: Suppress ratsnest display for items.
  - `GetFromToCache()`: Access `FROM_TO_CACHE` for diff-pair/length analysis.
- **Thread Safety**: Protected by `KISPINLOCK m_lock`.

---

## CN_ITEM / CN_ANCHOR / CN_ZONE_LAYER / CN_LIST / CN_CLUSTER
- **File**: `pcbnew/connectivity/connectivity_items.h`, `pcbnew/connectivity/connectivity_items.cpp`
- **Purpose**: Internal data types for the connectivity algorithm.
- **`CN_ANCHOR`**: A physical connection point (pad center or track endpoint). Has: position, owning CN_ITEM, tag (cluster id), cluster ref, `IsDangling()`, `ConnectedItemsCount()`.
- **`CN_ITEM`**: A single BOARD_CONNECTED_ITEM wrapped for connectivity queries. Has: parent BOARD_CONNECTED_ITEM, list of CN_ANCHOR, list of physically connected CN_ITEMs, dirty/valid flags (atomic), layer range (B_Cu mapped to INT_MAX), BBox (lazily updated). `Connect(b)` links two items.
- **`CN_ZONE_LAYER`**: Subclass of CN_ITEM for one filled polygon outline of a zone on one layer. Builds a per-triangle RTree (`KIRTREE::DYNAMIC_RTREE`) for fast `ContainsPoint(p)` and `Collide(shape)` queries. Handles teardrop zones specially.
- **`CN_LIST`**: Container of CN_ITEMs with a spatial RTree index. `Add(pad/track/arc/via/zone/shape)`, `FindNearby(item, func)`, `RemoveInvalidItems(garbage)`.
- **`CN_CLUSTER`**: A set of CN_ITEMs that are physically touching and form a connected copper island. Has: origin net, origin pad, `m_conflicting` (conflicting net drivers), `HasValidNet()`, `IsOrphaned()` (no pad anchor). Used by `CONNECTIVITY_ALGO` to compute net propagation.

---

## CN_CONNECTIVITY_ALGO
- **File**: `pcbnew/connectivity/connectivity_algo.h`, `pcbnew/connectivity/connectivity_algo.cpp`
- **Purpose**: The internal engine that computes clusters of touching copper items, and propagates net codes.
- **Key Design**:
  - Maintains a `CN_LIST` for pads, tracks, arcs, vias, and zone outlines.
  - Two-phase operation: (1) build spatial index from board items; (2) sweep the index to find touching items and form `CN_CLUSTER`s.
  - `propagateNets(commit, mode)`: Assigns net codes to clusters based on connected pads. `PROPAGATE_MODE::SKIP_CONFLICTS` (default) leaves conflicting clusters unchanged. `RESOLVE_CONFLICTS` assigns the majority net.
  - `FindClusters()`: Main cluster computation — sweeps the spatial index and union-finds touching items.
  - `Build(board)` / `Add/Remove/Update(item)`: Lifecycle management.

---

## FROM_TO_CACHE
- **File**: `pcbnew/connectivity/from_to_cache.h`, `pcbnew/connectivity/from_to_cache.cpp`
- **Purpose**: Caches "from-to" topology (which pads are connected by specific routes) for diff pair analysis, length tuning, and chain topology checks.
- **Key Methods**: `Rebuild(board)`, `QueryFromToPath(startItem, endItem)`, `GetMatchingFromTo(item, netClass)`.
