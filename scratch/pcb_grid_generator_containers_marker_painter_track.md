## pcb_fields_grid_table
- **File**: `pcbnew/pcb_fields_grid_table.h`, `pcbnew/pcb_fields_grid_table.cpp`
- **Purpose**: wxGrid-backed data model for displaying and editing a `FOOTPRINT`'s PCB fields in a dialog.
- **Columns** (`PCB_FIELDS_COL_ORDER`): NAME, VALUE, SHOWN, WIDTH, HEIGHT, THICKNESS, ITALIC, LAYER, ORIENTATION, UPRIGHT, XOFFSET, YOFFSET, KNOCKOUT, MIRRORED.
- **Class**: `PCB_FIELDS_GRID_TABLE` extends both `WX_GRID_TABLE_BASE` and `std::vector<PCB_FIELD>`.
  - `GetMandatoryRowCount()`: Returns count of mandatory (non-user) fields.
  - Per-cell type-sensitive get/set (`GetValue`, `SetValue`, `GetValueAsBool`, `SetValueAsBool`, `GetValueAsLong`, `SetValueAsLong`).
  - `GetAttr(row, col)`: Returns specialized `wxGridCellAttr` for dropdowns (layers, orientations), checkboxes, readonly cells, and URL validators.
- **Validators**: Separate `FIELD_VALIDATOR` instances for field names, references, values, URLs, and non-URL fields.
- **Context**: Used in the footprint properties dialog to let users add, rename, and configure text fields.

---

## pcb_generator
- **File**: `pcbnew/pcb_generator.h`, `pcbnew/pcb_generator.cpp`
- **Purpose**: Abstract base class for procedural/parametric board item generators (e.g., tuning meanders, differential pairs).
- **Inheritance**: `PCB_GENERATOR` → `PCB_GROUP` → `BOARD_ITEM` + `EDA_GROUP`.
- **Abstract Interface**:
  - `EditStart(tool, board, commit)` / `EditFinish(...)` / `EditCancel(...)`: Lifecycle hooks for the interactive generator tool.
  - `Update(tool, board, commit)`: Called to regenerate geometry during interactive editing.
  - `Remove(tool, board, commit)`: Removes the generator and its children.
  - `GetPluralName()` / `GetCommitMessage()`: Pure virtuals for UI labeling.
- **Optional Overrides**:
  - `GetPreviewItems(tool, frame, statusOnly)`: Returns preview geometry items.
  - `MakeEditPoints(...)` / `UpdateFromEditPoints(...)` / `UpdateEditPoints(...)`: For manipulating interactive drag handles.
  - `GetProperties()` / `SetProperties(STRING_ANY_MAP)`: Key-value property access for persistence.
  - `ShowPropertiesDialog(editFrame)`: Open properties dialog.
- **Members**: `m_generatorType` (string key for registry), `m_origin` (position), optional `m_updateOrder` (topological update priority).
- **Context**: Managed by `GENERATORS_MGR` registry. Concrete examples: `PCB_TUNING_PATTERN`, `PCB_DIFF_PAIR`.

---

## pcb_item_containers
- **File**: `pcbnew/pcb_item_containers.h`
- **Purpose**: Centralizes all board-level and footprint-level container type aliases.
- **Board-level typedefs**:
  - `MARKERS` = `vector<PCB_MARKER*>`
  - `ZONES` = `vector<ZONE*>`
  - `TRACKS` = `deque<PCB_TRACK*>`
  - `FOOTPRINTS` = `deque<FOOTPRINT*>`
  - `GROUPS` = `deque<PCB_GROUP*>`
  - `GENERATORS` = `deque<PCB_GENERATOR*>`
  - `PCB_POINTS` = `deque<PCB_POINT*>`
  - `DRAWINGS` = `deque<BOARD_ITEM*>` (shared with footprint)
- **Footprint-level typedefs**:
  - `PADS` = `deque<PAD*>`
  - `PCB_FIELDS` = `deque<PCB_FIELD*>`
- **Context**: Included everywhere board collections are managed. All `BOARD` and `FOOTPRINT` member lists use these types.

---

## pcb_marker
- **File**: `pcbnew/pcb_marker.h`, `pcbnew/pcb_marker.cpp`
- **Purpose**: Visual DRC/ERC error indicator placed on the board at the error location.
- **Inheritance**: `PCB_MARKER` → `BOARD_ITEM` + `MARKER_BASE`.
- **Key Methods**:
  - `SerializeToString()` / `DeserializeFromString(wxString)`: Persistence for DRC exclusions.
  - `HitTest(...)`: Returns `false` for ratsnest markers (MARKER_RATSNEST type); delegates to `MARKER_BASE::HitTestMarker` for DRC markers.
  - `GetColorLayer()`: Returns the GAL layer based on marker severity.
  - `GetSeverity()`: Returns error/warning/info severity from the associated `RC_ITEM`.
  - `Matches(searchData)`: Matches against the RC item's error message text.
  - `SetPath(shapes, start, end)`: Sets the geometric shapes for the marker's connecting line to an error location (shown on `LAYER_DRC_SHAPES`).
- **Members**: `m_pathShapes` (PCB_SHAPE segments for the path), `m_pathStart`/`m_pathEnd`/`m_pathLength`.
- **Context**: Created by DRC engine and placed on the board; not a persistent board object (recreated each DRC run). Ratsnest markers have special rendering behavior.

---

## pcb_painter
- **File**: `pcbnew/pcb_painter.h`, `pcbnew/pcb_painter.cpp`
- **Purpose**: GAL painter for all PCB board items. Implements `PAINTER::Draw()` dispatch to typed draw overloads.
- **Classes**:
  - `PCB_RENDER_SETTINGS`: Extends `RENDER_SETTINGS` with PCB-specific options:
    - `LoadDisplayOptions(PCB_DISPLAY_OPTIONS)`: Maps high-contrast, zone/pad/track display modes.
    - `GetColor(BOARD_ITEM*, layer)`: Board-specific color resolution (net colors, netclass colors, hidden nets, chain highlights).
    - Net color override maps: `m_netColors` (indexed by netcode), `m_netclassColors`, `m_hiddenNets`.
    - Per-type opacity overrides: `m_trackOpacity`, `m_viaOpacity`, `m_padOpacity`, `m_zoneOpacity`, `m_imageOpacity`, `m_filledShapeOpacity`.
    - `m_highlightedNetChain`: Active chain highlight name.
  - `PCB_PAINTER`: Dispatches `Draw(VIEW_ITEM*, layer)` to typed private overloads for every PCB item type:
    - `draw(PCB_TRACK*,...)`, `draw(PCB_ARC*,...)`, `draw(PCB_VIA*,...)`, `draw(PAD*,...)`, `draw(PCB_SHAPE*,...)`, `draw(PCB_REFERENCE_IMAGE*,...)`, `draw(PCB_FIELD*,...)`, `draw(PCB_TEXT*,...)`, `draw(PCB_TEXTBOX*,...)`, `draw(PCB_TABLE*,...)`, `draw(FOOTPRINT*,...)`, `draw(PCB_GROUP*,...)`, `draw(ZONE*,...)`, `draw(PCB_BARCODE*,...)`, `draw(PCB_DIMENSION_BASE*,...)`, `draw(PCB_POINT*,...)`, `draw(PCB_TARGET*,...)`, `draw(PCB_MARKER*,...)`, `draw(PCB_BOARD_OUTLINE*,...)`.
    - Helpers: `getLineThickness`, `getDrillShape`, `getPadHoleShape`, `getViaDrillSize`, `strokeText`, `renderNetNameForSegment`, `drawBackdrillIndicator`, `drawPostMachiningIndicator`.
- **Context**: The central rendering engine. Every rendering path goes through this painter.

---

## pcb_track / pcb_arc / pcb_via
- **File**: `pcbnew/pcb_track.h`, `pcbnew/pcb_track.cpp`
- **Purpose**: Core board routing primitives.
- **Class Hierarchy**:
  - `PCB_TRACK` (type `PCB_TRACE_T`): Base track segment with `m_Start`, `m_End`, `m_width`. Supports `GetLength()`, `GetDelay()`, `IsPointOnEnds()`, `TransformShapeToPolygon()`, `GetEffectiveShape()`, `ApproxCollinear()`. Has `m_hasSolderMask` / `m_solderMaskMargin` for track-level solder mask control.
  - `PCB_ARC` (type `PCB_ARC_T`): Curved track with `m_Mid` (midpoint). Provides `GetRadius()`, `GetAngle()`, `GetArcAngleStart()`, `GetArcAngleEnd()`, `IsCCW()`, `IsDegenerated()`.
  - `PCB_VIA` (type `PCB_VIA_T`): Via extending PCB_TRACK. Holds a `PADSTACK` for per-layer annular ring sizes and drill geometry.
    - **Via types** (`VIATYPE`): THROUGH, BLIND, BURIED, MICROVIA. Exposed through synthetic scan types for DRC.
    - **Drill parameters**: Primary drill (`SetPrimaryDrillSize`, `GetDrillValue`), secondary (backdrill), and tertiary drills (stacked blind/buried).
    - **Post-machining**: Front/back counterbore or countersink settings (`SetFrontPostMachining`, etc.).
    - **Tenting/covering/plugging/capping/filling**: Per-face finish modes.
    - **Layer pair**: `SetLayerPair(top, bottom)`, `LayerPair(top*, bottom*)`, `SanitizeLayers()`.
    - `FlashLayer(aLayer)`: Returns whether the via is physically present (annular ring) on a given layer.
    - `ConditionallyFlashed(layer)`: True if the via pad may be removed from this layer (based on `UNCONNECTED_LAYER_MODE`).
    - `ValidateViaParameters(...)`: Static validator returning structured `VIA_PARAMETER_ERROR`.
- **Context**: The fundamental copper routing types. All routing, DRC, connectivity, and export logic operates on these.
