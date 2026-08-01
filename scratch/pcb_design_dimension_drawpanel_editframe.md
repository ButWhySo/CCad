## pcb_design_block_utils
- **File**: `pcbnew/pcb_design_block_utils.cpp`
- **Purpose**: Implements design block library operations from the PCB editor.
- **Key Functions**:
  - `SaveBoardAsDesignBlock(aLibraryName)`: Saves the entire current board layout as a named design block in a library.
  - `SaveSelectionAsDesignBlock(aLibraryName)`: Takes the current active selection and saves it as a new design block; if a group is selected, the group name seeds the block name; if not, wraps the selection in a new group automatically.
  - `UpdateDesignBlockFromBoard(aLibId)`: Updates an existing design block's layout from the full board.
  - `UpdateDesignBlockFromSelection(aLibId)`: Updates an existing design block's layout from the current selection.
  - `saveSelectionToDesignBlock(aNickname, aSelection, aBlock)`: Internal helper that creates a temporary board containing copies of selected items with re-wired net info, serializes it to a temp file, and invokes `DesignBlockLibs()->SaveDesignBlock()`.
- **Design Pattern**: All public methods guard against missing library selection, gather the desired items, call the private `saveSelectionToDesignBlock` helper, then refresh the design block pane to reflect the new state. Deep-clones groups to avoid shallow-copy aliasing.

---

## pcb_dimension
- **File**: `pcbnew/pcb_dimension.h`, `pcbnew/pcb_dimension.cpp`
- **Purpose**: Abstract base and concrete subclasses for PCB measurement annotation items.
- **Class Hierarchy**:
  - `PCB_DIMENSION_BASE` (extends `PCB_TEXT`): Abstract base. Holds feature points (`m_start`/`m_end`), units, precision, prefix/suffix, arrow style, cached shapes vector, and calls `updateGeometry()` when properties change.
  - `PCB_DIM_ALIGNED`: Aligned dimension — crossbar stays parallel with feature vector. Has `m_height` offset and extension height.
  - `PCB_DIM_ORTHOGONAL`: Like aligned, but locks crossbar to X or Y axis. Has `DIR` enum (`HORIZONTAL`/`VERTICAL`).
  - `PCB_DIM_RADIAL`: Radius/diameter annotation with a `m_leaderLength`. Geometry flows from center (`m_start`) to arc point (`m_end`) to knee point, then text.
  - `PCB_DIM_LEADER`: Pointer annotation with an arrowhead at `m_start`, jog at `m_end`, and text line. Has optional `DIM_TEXT_BORDER` (NONE/RECTANGLE/CIRCLE/ROUNDRECT).
  - `PCB_DIM_CENTER`: Cross-hair center mark. `m_start` is center, `m_end` is end of one cross leg.
- **Key Enums**: `DIM_UNITS_FORMAT`, `DIM_PRECISION`, `DIM_TEXT_POSITION` (OUTSIDE/INLINE/MANUAL), `DIM_UNITS_MODE` (INCH/MILS/MM/AUTOMATIC), `DIM_ARROW_DIRECTION` (INWARD/OUTWARD), `DIM_TEXT_BORDER`.
- **Context**: All dimension subtypes serialize via protobuf `Serialize`/`Deserialize`. Geometry is recalculated lazily via virtual `updateGeometry()`.

---

## pcb_draw_panel_gal
- **File**: `pcbnew/pcb_draw_panel_gal.h`, `pcbnew/pcb_draw_panel_gal.cpp`
- **Purpose**: Specialized `EDA_DRAW_PANEL_GAL` subclass for the PCB canvas.
- **Key Methods**:
  - `DisplayBoard(aBoard, aReporter)`: Populates the KIGFX::VIEW with all board items so they can be rendered by the GAL backend.
  - `SetDrawingSheet(aDrawingSheet)`: Attaches a `DS_PROXY_VIEW_ITEM` (title block) to the view.
  - `UpdateColors()`: Propagates color settings to painter and GAL.
  - `SetHighContrastLayer(PCB_LAYER_ID)` / `SetTopLayer(PCB_LAYER_ID)`: PCB-aware versions that handle copper pairing and other layer-specific logic.
  - `SyncLayersVisibility(aBoard)`: Applies the board's layer visibility settings to GAL rendering targets.
  - `RedrawRatsnest()`: Forces the ratsnest view item to repaint.
  - `GetView()`: Returns the `KIGFX::PCB_VIEW*` (PCB-specific view subclass).
- **Members**: Owns `m_drawingSheet` (unique_ptr to `DS_PROXY_VIEW_ITEM`) and `m_ratsnest` (unique_ptr to `RATSNEST_VIEW_ITEM`).
- **Context**: The rendering surface for the PCB editor. Wraps OpenGL/Cairo GAL backends. Layer ordering and dependencies are set in `setDefaultLayerOrder()` and `setDefaultLayerDeps()`.

---

## pcb_edit_frame
- **File**: `pcbnew/pcb_edit_frame.h`, `pcbnew/pcb_edit_frame.cpp`
- **Purpose**: Main frame for the Pcbnew PCB editor. Top of the GUI hierarchy for board editing.
- **Inheritance**: `PCB_EDIT_FRAME` → `PCB_BASE_EDIT_FRAME` → `PCB_BASE_FRAME` → `EDA_DRAW_FRAME`.
- **Key Responsibilities**:
  - Board I/O: `OpenProjectFiles`, `SavePcbFile`, `SavePcbCopy`, `Clear_Pcb`.
  - Schematic cross-probing: `ExecuteRemoteCommand` (socket), `KiwayMailIn`, `SendCrossProbeItem`, `SendCrossProbeNetName`, `SendSelectItemsToSch`.
  - Netlist update: `FetchNetlistFromSchematic`, `ReadNetlistFromFile`, `OnNetlistChanged`.
  - Export: `DoGenFootprintsPositionFile`, `ExportVRML_File`, `Export_IDF3`, `ExportSpecctraFile`, `ImportSpecctraSession`.
  - Footprint management: `ShowFootprintPropertiesDialog`, `ShowExchangeFootprintsDialog`, `ExchangeFootprint`, `ExportFootprintsToLibrary`.
  - Design blocks: `SaveBoardAsDesignBlock`, `SaveSelectionAsDesignBlock`, `UpdateDesignBlockFromBoard`, `UpdateDesignBlockFromSelection`.
  - UI: `ReCreateLayerBox`, `UpdateTitle`, `ToggleLayersManager`, `ToggleNetInspector`, `ToggleSearch`, `ToggleLibraryTree`, `ShowBoardSetupDialog`.
  - Routing: `SetTrackSegmentWidth`, `Edit_Zone_Params`.
  - Autosave: `DoAutoSave`.
  - API bridge: `CanAcceptApiCommands` (guards IPC handler).
- **Members**: Holds `m_designBlocksPane`, `m_exportNetlistAction`, `m_prevIconVal`, plus IPC API server/handler pointers under `KICAD_IPC_API` ifdef.

---

## pcb_field
- **File**: `pcbnew/pcb_field.h`, `pcbnew/pcb_field.cpp`
- **Purpose**: Represents a named text field attached to a `FOOTPRINT` item (e.g., Reference, Value, Datasheet, or user-defined).
- **Inheritance**: `PCB_FIELD` → `PCB_TEXT` → `BOARD_ITEM`.
- **Key Members**:
  - `m_id` (`FIELD_T` enum): Identifies standard fields (REFERENCE, VALUE, DATASHEET) vs. USER fields.
  - `m_ordinal`: Sort order for user fields.
  - `m_name`: Display name for the field.
- **Key Methods**:
  - `IsReference()`, `IsValue()`, `IsDatasheet()`, `IsComponentClass()`: Convenience predicates.
  - `IsMandatory()`: Returns true for built-in fields (ref/value/datasheet).
  - `GetName(aUseDefaultName)`: Returns field name, falling back to default if empty.
  - `GetCanonicalName()`: Language-agnostic name suitable for map lookups.
  - `GetShownText()`: Renders expanded text with variable substitution.
  - `GetOrdinal()` / `SetOrdinal()`: Position in user field list.
  - `ViewGetLOD()`: Controls rendering zoom threshold.
  - `HasHypertext()`: True if the field contains a hyperlink.
- **Context**: Used in footprint serialization and the footprint properties dialog. Discriminated by `FIELD_T` enum (REFERENCE=0, VALUE=1, DATASHEET=2, USER fields use ordinal).

---

## pcb_group
- **File**: `pcbnew/pcb_group.h`, `pcbnew/pcb_group.cpp`
- **Purpose**: A logical, transparent container for a set of `BOARD_ITEM`s.
- **Inheritance**: `PCB_GROUP` → `BOARD_ITEM` + `EDA_GROUP` (mixin).
- **Key Properties**:
  - Parent is always the `BOARD`, not a sub-group.
  - Position/layer/bounding box are derived from member items.
  - Supports optional `LIB_ID` `DesignBlockLibId` to link back to a design block.
- **Key Methods**:
  - `GetBoardItems()`: Returns the flat `unordered_set<BOARD_ITEM*>` of members.
  - `TopLevelGroup(aItem, aScope, isFootprintEditor)`: Walks parent chain to find the highest ancestor group within a given scope.
  - `WithinScope(aItem, aScope, isFootprintEditor)`: Tests if an item belongs to the given group scope.
  - `DeepClone()`: Recursively clones all children, fixing up group membership.
  - `DeepDuplicate(addToParentGroup, aCommit)`: Like DeepClone but can add the duplicate to the same parent group via a commit.
  - `RunOnChildren(aFunction, aMode)`: Iterates all direct (or recursive) child items.
- **Selection Semantics**: Selecting a group implicitly selects its members; operations like commit/view-update treat the member set explicitly.
