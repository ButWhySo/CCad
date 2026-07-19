## EDIT_TOOL
- **File**: `pcbnew/tools/edit_tool.h`, `pcbnew/tools/edit_tool.cpp`, `pcbnew/tools/edit_tool_move_fct.cpp`
- **Purpose**: The main interactive mutation tool. All modification actions on selected items flow through here.
- **Inherits**: `PCB_TOOL_BASE`.
- **Key Methods**:
  - `Move(event)` / `doMoveSelection(event, commit, autoStart)`: Main drag-and-move loop. Updates ratsnest live via `rebuildConnectivity()`. Clamps movement to safe coordinate range via `getSafeMovement()`.
  - `Drag(event)`: Invokes PNS inline router for dragging tracks. Falls back to `DragArcTrack()` for arc resizing.
  - `Rotate(event)`, `Flip(event)`, `Mirror(event)`: In-place transforms.
  - `Properties(event)`: Opens the item properties dialog.
  - `Swap(event)`, `SwapPadNets(event)`, `SwapGateNets(event)`: Positional/net swap operations.
  - `PackAndMoveFootprints(event)`: Bin-packing then move.
  - `ChangeTrackWidth(event)`, `ChangeTrackLayer(event)`: Track property update.
  - `FilletTracks(event)`: Arc-fillet at track bends.
  - `ModifyLines(event)`: Covers fillet, chamfer, extend-to-meet for graphical lines.
  - `HealShapes(event)`: Connect open shape endpoints.
  - `SimplifyPolygons(event)`: Polygon outline simplification.
  - `OutsetItems(event)`: Create outset polygon copies.
  - `BooleanPolygons(event)`: Union/subtract/intersect polygons.
  - `Remove(event)` / `DeleteItems(selection, isCut)`: Delete or cut-to-clipboard.
  - `Duplicate(event)` / `Increment(event)`: Clone with pad-number increment.
  - `MoveExact(event)`: Open exact-movement dialog.
  - `GetAndPlace(event)`: Find-and-start-move.
  - `copyToClipboard`, `cutToClipboard`, `copyToClipboardAsText`: Clipboard serialization.
  - `FootprintFilter`, `PadFilter`: Static selection narrowing callbacks.
  - `pickReferencePoint()`: Interactive reference point picker for copy/move with reference.
- **Key Internal**: `invokeInlineRouter(dragMode)` — hands off to PNS interactive router. `updateModificationPoint()` — decides whether to use centroid or individual item reference point.

---

## DRAWING_TOOL
- **File**: `pcbnew/tools/drawing_tool.h`, `pcbnew/tools/drawing_tool.cpp`
- **Purpose**: All shape-drawing and item-placement operations.
- **Inherits**: `PCB_TOOL_BASE`.
- **MODE enum**: NONE, LINE, RECTANGLE, CIRCLE, ELLIPSE, ELLIPSE_ARC, ARC, BEZIER, IMAGE, TEXT, ANCHOR, MD_POINT (point marker), DXF, DIMENSION, KEEPOUT, ZONE, GRAPHIC_POLYGON, VIA, TUNING, TABLE, BARCODE.
- **Key Methods**:
  - `DrawLine/Rectangle/Circle/Ellipse/EllipseArc/Arc/Bezier(event)`: Shape placement loops.
  - `DrawZone(event)`: Zone/keepout boundary polygon. Reads ZONE_MODE parameter (ADD/CUTOUT/SIMILAR/GRAPHIC_POLYGON).
  - `DrawVia(event)`: Via placement.
  - `DrawDimension(event)`: All dimension types (Aligned, Center, Radial, Orthogonal, Leader).
  - `DrawTable(event)`: Interactive table creation.
  - `DrawBarcode(event)`: Barcode placement.
  - `PlaceText(event)`, `PlaceReferenceImage(event)`, `PlacePoint(event)`.
  - `PlaceImportedGraphics(event)`: DXF/SVG import then placement.
  - `SetAnchor(event)`: Footprint anchor repositioning.
  - `PlaceStackup(event)` / `DrawSpecificationStackup(origin, layer, drawNow, tablesize)`: Generate a stackup table graphic from `BOARD_STACKUP`.
  - `PlaceTuningPattern(event)`: Length-tuning serpentine/accordion meander.
  - `InteractivePlaceWithPreview(event, items, preview, layers)`: Generic drag-and-place for a list of items with a lightweight preview set.
- **Internal draw primitives**: `drawShape()`, `drawArc()`, `drawOneBezier()`. `DRAW_ONE_RESULT` enum: ACCEPTED/CANCELLED/RESET/ACCEPTED_AND_RESET. `getSourceZoneForAction()` for CUTOUT/SIMILAR modes. `constrainDimension()` for 45° snapping. `getClampedDifferenceEnd/RadiusEnd()` guard against integer overflow.
- **State**: Current layer (`m_layer`), stroke params (`m_stroke`), text attributes (`m_textAttrs`), preview selection (`m_preview`).

---

## DRC_TOOL
- **File**: `pcbnew/tools/drc_tool.h`, `pcbnew/tools/drc_tool.cpp`
- **Purpose**: Tool wrapper around the DRC engine that manages the DRC dialog lifecycle and triggers test runs.
- **Key Methods**:
  - `ShowDRCDialog(parent)` / `DestroyDRCDialog()`: DIALOG_DRC lifecycle.
  - `IsDRCRunning()`: Guard flag.
  - `RunTests(reporter, refillZones, reportAllTrackErrors, testFootprints)`: Delegates to `DRC_ENGINE`. Calls `ZONE_FILLER` first if `refillZones=true`.
  - `PrevMarker/NextMarker(event)`: Navigate DRC markers on canvas.
  - `CrossProbe(marker)`: Zoom to marker and show description. Opens DRC dialog if closed.
  - `ExcludeMarker(event)`: Toggle marker exclusion.
  - `FixDRCError(drcItem)` / `FixDRCErrorMenuText(drcItem)`: Auto-fix individual DRC violations.
  - `ShowDesignRuleEditorDialog(parent)` / `DestroyDesignRuleEditorDialog()`: Rule editor dialog lifecycle.
- **Key Field**: `m_drcEngine` (`shared_ptr<DRC_ENGINE>`) — the live engine instance reused across test runs.

---

## BOARD_EDITOR_CONTROL
- **File**: `pcbnew/tools/board_editor_control.h`, `pcbnew/tools/board_editor_control.cpp`
- **Purpose**: Board-level edit-frame commands that are not tool-state-machine operations — file I/O, schematic sync, drill origin, zone priority, etc.
- **Key Groups**:
  - **File I/O**: New, Open, Save, SaveAs, SaveCopy, Revert, RescueAutosave, OpenNonKicadBoard.
  - **Export**: GenerateGerbers, GenerateDrillFiles, GeneratePosFile, ExportGenCAD, ExportVRML, ExportIDF, ExportSTEP, ExportCmpFile, ExportHyperlynx, GenBOMFileFromBoard, GenD356File, GenIPC2581File, GenerateODBPPFiles, ExportSpecctraDSN, ImportSpecctraSession.
  - **Netlist/Schematic sync**: ImportNetlist, UpdatePCBFromSchematic, UpdateSchematicFromPCB, ShowEeschema.
  - **Cross-probe**: CrossProbeToSch (passive, selection-driven), ExplicitCrossProbeToSch (user-initiated).
  - **Footprint placement**: PlaceFootprint (dialog→pick→commit). `m_placingFootprint` re-entrancy guard.
  - **Zone priority**: ZonePriorityMoveToTop/Raise/Lower/Bottom.
  - **Track/via size**: TrackWidthInc/Dec, ViaSizeInc/Dec, AutoTrackWidth.
  - **Misc**: DrillOrigin (interactive), DoSetDrillOrigin (static, no undo), ToggleLock/Lock/Unlock, AssignNetclass, Find, FindNext, FindByProperties, BoardSetup, RepairBoard, Toggle* panel visibility (LayersManager, Properties, NetInspector, Search, LibraryTree), ChangeLineMode, OnAngleSnapModeChanged.
