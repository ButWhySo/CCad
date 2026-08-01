## pcb_shape
- **File**: `pcbnew/pcb_shape.h`, `pcbnew/pcb_shape.cpp`
- **Purpose**: Generic graphical shape item placed on any board layer (lines, arcs, circles, rectangles, polygons, Bezier curves).
- **Inheritance**: `PCB_SHAPE` → `BOARD_CONNECTED_ITEM` + `EDA_SHAPE` (mixin carrying shape geometry, stroke, fill).
- **Key Overrides**:
  - `IsConnected()`: True if the shape is on a copper layer and can carry net info.
  - `SetLayer/GetLayer`: Layer assignment; also controls multi-layer display.
  - `GetConnectionPoints()`: Returns natural start/end electrical connection points.
  - `GetStroke/SetStroke`: Line width, style (solid, dashed, dotted), end caps.
  - `GetCorners()`: Returns 4 corners for rectangular shapes.
  - `TransformShapeToPolygon(...)`: Converts the shape (including curves) to a polygon approximation for DRC/clearance.
  - `TransformShapeToPolySet(...)`: Same but includes fill and hatching details (for 3D view).
  - `BeginEdit/ContinueEdit/CalcEdit/EndEdit`: Interactive drawing lifecycle delegates to `EDA_SHAPE`.
  - `IsProxyItem()`: True for items that are synthetic drawing proxies.
  - `UpdateHatching()`: Refreshes the fill hatch pattern cache.
  - `HasSolderMask/GetLocalSolderMaskMargin`: Per-shape solder mask override support.
- **Context**: The workhorse graphical element for PCB copper and silkscreen geometry. All zone outlines, text frames, board outlines, footprint courtyard/fab/silk graphics use this.

---

## pcb_text
- **File**: `pcbnew/pcb_text.h`, `pcbnew/pcb_text.cpp`
- **Purpose**: Text item placed on a PCB layer; base class for `PCB_FIELD`.
- **Inheritance**: `PCB_TEXT` → `BOARD_ITEM` + `EDA_TEXT` (mixin carrying text string, font, alignment, mirroring).
- **Key Members/Methods**:
  - `GetShownText(aAllowExtraText, aDepth)`: Returns expanded text with variable substitution.
  - `KeepUpright()`: Adjusts angle when parent footprint rotates to keep text readable.
  - `GetDrawRotation()`: Returns drawing rotation accounting for footprint rotation.
  - `TextHitTest(...)`: Hit testing against the rendered text bounding hull.
  - `TransformTextToPolySet(...)`: Converts stroked character geometry to polygon set for DRC.
  - `GetKnockoutCache(font, resolvedText, maxError)`: Returns cached polygon for knockout (negative) text rendering.
  - `buildBoundingHull(buffer, renderedText, clearance)`: Builds a polygon hull around rendered text for proximity queries.
  - `m_knockout_cache`: Lazily cached knockout poly set (invalidated on text/style changes).
  - `StyleFromSettings(BOARD_DESIGN_SETTINGS, checkSide)`: Applies board text defaults.
  - `ShowSyntaxHelp(parentWindow)`: Static — opens the variable-syntax help dialog.
- **Context**: Used directly for free-standing board text and as base class for footprint fields (ref, value, datasheet).

---

## pcb_view
- **File**: `pcbnew/pcb_view.h`, `pcbnew/pcb_view.cpp`
- **Purpose**: PCB-specific subclass of `KIGFX::VIEW`, managing the rendering layer graph for all board items.
- **Inheritance**: `PCB_VIEW` → `KIGFX::VIEW`.
- **Key Overrides**:
  - `Add(VIEW_ITEM*, priority)`: Adds an item to the correct rendering layers (handles `FOOTPRINT` expansion to child items).
  - `Remove(VIEW_ITEM*)`: Mirrors `Add` — removes item and all sub-items from view.
  - `Update(VIEW_ITEM*, flags)` / `Update(VIEW_ITEM*)`: Invalidates item and its sub-items in the view, propagating dirty flags to GAL.
  - `UpdateCollidingItems(aStaleAreas, aTypes)`: Marks `KIGFX::REPAINT` on all items of specified types that intersect any of the given stale bounding boxes. Used for efficient partial redraws.
  - `UpdateDisplayOptions(PCB_DISPLAY_OPTIONS)`: Updates painter settings for the new display preferences (e.g., high-contrast mode, zone fill display).
- **Context**: Owned by `PCB_DRAW_PANEL_GAL`. All board items (tracks, footprints, zones, etc.) are registered here for rendering.

---

## zone
- **File**: `pcbnew/zone.h`, `pcbnew/zone.cpp`
- **Purpose**: Represents a copper fill zone or a rule area (keepout) on one or more PCB layers.
- **Inheritance**: `ZONE` → `BOARD_CONNECTED_ITEM`.
- **Dual Role**: `GetIsRuleArea()` distinguishes copper zones from rule areas; `IsConnected()` returns false for rule areas.
- **Outline Storage**: `m_Poly` (`SHAPE_POLY_SET*`) — outline with holes. `Outline()`, `AppendCorner()`, `NewHole()`, `RemoveCutout()`.
- **Fill Storage**: `m_FilledPolysList` (`map<PCB_LAYER_ID, shared_ptr<SHAPE_POLY_SET>>`) — filled copper per layer; thread-safe via `m_filledPolysListMutex`.
- **Key Properties**:
  - Priority: `SetAssignedPriority(unsigned)` / `GetAssignedPriority()`.
  - Layer set: multi-layer (`SetLayerSet/GetLayerSet`), thread-safe.
  - Clearance: `GetLocalClearance()`, `SetLocalClearance()`.
  - Fill mode: `ZONE_FILL_MODE` (SOLID / HATCH / COPPER_THIEVING). Hatch params: thickness, gap, orientation, smoothing.
  - Thermal relief: `GetThermalReliefGap()`, `GetThermalReliefSpokeWidth()`.
  - Pad connection: `ZONE_CONNECTION` enum (INHERITED/NONE/THERMAL/FULL/THT_THERMAL).
  - Smoothing: corner radius, `BuildSmoothedPoly(...)`.
  - Fill flags: `SetFillFlag/GetFillFlag`, `IsFilled`, `NeedRefill/SetNeedRefill`.
  - Island tracking: `IsIsland(layer, polyIdx)`, `SetIsIsland(layer, polyIdx)`, `m_insulatedIslands`.
  - Teardrop: `IsTeardropArea()`, `GetTeardropAreaType()`, `SetTeardropAreaType(TEARDROP_TYPE)`.
  - Copper thieving: `IsCopperThieving()`, `SetThievingSettings()` (pattern, element_size, gap, line_width, stagger, orientation). Forces net = 0.
- **Rule Area (Keepout) parameters**:
  - `GetIsRuleArea()` / `SetIsRuleArea()`.
  - `GetDoNotAllowTracks()` / `SetDoNotAllowTracks()`.
  - `GetDoNotAllowVias()` / `SetDoNotAllowVias()`.
  - `GetDoNotAllowPads()` / `SetDoNotAllowPads()`.
  - `GetDoNotAllowCopperPour()` / `SetDoNotAllowCopperPour()` (alias for zone fills).
  - `GetDoNotAllowFootprints()` / `SetDoNotAllowFootprints()`.
  - `IsConflicting()`: True if rule area excludes footprints (participates in courtyard DRC during move).
- **Key Transform Methods**: `Move`, `MoveEdge`, `Rotate`, `Flip`, `Mirror`, `UnFill`.
- **Polygon Queries**:
  - `HitTestFilledArea(layer, pos)`: Hit tests the filled copper.
  - `HitTestCutout(pos, outlineIdx*, holeIdx*)`: Hit tests zone cutout holes.
  - `HitTestForCorner(...)` / `HitTestForEdge(...)`: Fine-grained outline editing hit tests.
  - `GetInteractingZones(layer, sameNet*, otherNet*)`: Returns intersecting zones for fill merging.
  - `TransformSolidAreasShapesToPolygon(layer, buffer)`: For 3D view export.
  - `TransformSmoothedOutlineToPolygon(buffer, clearance, error, errorLoc, boardOutline)`: For zone fill algorithm.
- **Thread Safety**: Layer set and filled polys list are mutex-guarded. `CacheTriangulation(layer)` uses task submitter for parallel triangulation.
- **Isolated Islands**: `ISOLATED_ISLANDS` struct tracks `m_IsolatedOutlines` and `m_SingleConnectionOutlines` per fill — used to flag DRC violations.
