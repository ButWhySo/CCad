## PCB_ACTIONS
- **File**: `pcbnew/tools/pcb_actions.h`, `pcbnew/tools/pcb_actions.cpp`
- **Purpose**: Central registry of all `TOOL_ACTION` static instances for the PCB editor. Every interactive operation maps to a named action here.
- **Key Action Groups**:
  - **Selection**: syncSelection, syncSelectionWithNets, selectionMenu, selectConnection, unrouteSelected, selectNet, deselectNet, selectNetChain, selectUnconnected, grabUnconnected, selectOnSchematic, filterSelection.
  - **Edit / Move**: move, moveIndividually, moveWithReference, copyWithReference, rotateCw/Ccw, flip, mirrorH/V, swap, swapPadNets, changeTrackWidth, filletTracks, filletLines, chamferLines, dogboneCorners, healShapes, extendLines, simplifyPolygons, mergePolygons, subtractPolygons, intersectPolygons, properties, moveExact, duplicateIncrement, remove, deleteFull, breakTrack, drag45Degree, dragFreeAngle.
  - **Drawing**: drawLine, drawPolygon, drawRectangle, drawCircle, drawEllipse, drawArc, drawBezier, placeText, drawTextBox, drawTable, drawAlignedDimension, drawCenterDimension, drawRadialDimension, drawOrthogonalDimension, drawLeader, placeBarcode, drawZone, drawCopperThievingZone, drawVia, drawRuleArea, drawZoneCutout, placeFootprint, placeCharacteristics, placeStackup.
  - **Line Modes**: lineModeFree, lineMode90, lineMode45, lineModeNext (cycle), closeOutline.
  - **Router**: routeSingleTrack, routeDiffPair, tuneSingleTrack, tuneDiffPair, tuneSkew, routerUndoLastSegment, routerContinueFromEnd, routerAttemptFinish, routerRouteSelected, routerRouteSelectedFromEnd, routerAutorouteSelected, routerSettingsDialog, routerDiffPairDialog, routerHighlightMode, routerShoveMode, routerWalkaroundMode, cycleRouterMode, routerInlineDrag.
  - **Generator**: regenerateAllTuning, regenerateAll, regenerateSelected, regenerateItem, genStartEdit/UpdateEdit/FinishEdit/CancelEdit/Remove, generatorsShowManager.
  - **Placement**: alignTop/Bottom/Left/Right/CenterX/CenterY, distributeHorizontally/VerticallyGaps/Centers, positionRelative, packAndMoveFootprints, autoplaceOffboard/SelectedComponents.
  - **Layer Control**: layerTop/Bottom/Inner1..30, layerNext/Prev, layerAlphaInc/Dec, layerToggle, layerPairPresetsCycle, flipBoard.
  - **Track/Via Size**: trackWidthInc/Dec, viaSizeInc/Dec, autoTrackWidth, assignNetClass.
  - **Zones**: zoneFill, zoneFillAll, zoneFillDirty, zoneUnfill, zoneUnfillAll, zoneMerge, zoneDuplicate, zonePriorityMoveToTop/Raise/Lower/Bottom.
  - **Board Export**: generateGerbers, generateDrillFiles, generatePosFile, generateReportFile, generateIPC2581File, generateODBPPFile, generateD356File, generateBOM, exportGenCAD, exportVRML, exportIDF, exportSTEP, exportHyperlynx.
  - **Global Edit**: editTracksAndVias, editTextAndGraphics, editTeardrops, globalDeletions, cleanupTracksAndVias, cleanupGraphics, updateFootprint/s, changeFootprint/s, swapLayers, removeUnusedPads.
  - **DRC**: runDRC, drcRuleEditor.
  - **Design Blocks**: placeDesignBlock, placeLinkedDesignBlock, applyDesignBlockLayout, saveToLinkedDesignBlock, saveBoardAsDesignBlock, saveSelectionAsDesignBlock, updateDesignBlockFromBoard/Selection, deleteDesignBlock, editDesignBlockProperties.
  - **Footprint Editor**: newFootprint, createFootprint, editFootprint, duplicateFootprint, renameFootprint, deleteFootprint, cutFootprint, copyFootprint, pasteFootprint, importFootprint, exportFootprint, footprintProperties, defaultPadProperties, checkFootprint, loadFpFromBoard, saveFpToBoard, placePad, explodePad, recombinePad, enumeratePads.
  - **Pad**: copyPadSettings, applyPadSettings, pushPadSettings.
  - **Microwave**: microwaveCreateGap, microwaveCreateStub, microwaveCreateStubArc, microwaveCreateFunctionShape, microwaveCreateLine.
  - **Locking**: toggleLock, lock, unlock.
  - **Display**: showRatsnest, ratsnestLineMode, netColorModeCycle, trackDisplayMode, padDisplayMode, viaDisplayMode, zoneDisplayFilled/Outline/Fractured/Triangulated/Toggle, showPadNumbers.
  - **Highlight/Cross-probe**: clearHighlight, highlightNet, toggleNetHighlight, highlightNetSelection, highlightItem, highlightNetChain.
  - **Ratsnest**: hideNetInRatsnest, showNetInRatsnest, localRatsnestTool, hideLocalRatsnest, updateLocalRatsnest.
  - **Misc**: selectionTool, pickerTool, measureTool, drillOrigin, find, findByProperties, getAndPlace, inspectClearance, inspectConstraints, diffFootprint, repeatLayout, generatePlacementRuleAreas, convertToPoly/Zone/Keepout/Lines/Arc/Tracks.
- **Static helper**: `LayerIDToAction(layerId)` — converts a PCB_LAYER_ID to the corresponding layer-switch action.

---

## PCB_SELECTION_TOOL
- **File**: `pcbnew/tools/pcb_selection_tool.h`, `pcbnew/tools/pcb_selection_tool.cpp`
- **Purpose**: The interactive selection engine for the PCB editor. Handles single-click, shift-click, box selection, lasso selection, and cross-probe sync.
- **Inheritance**: `PCB_SELECTION_TOOL` → `SELECTION_TOOL` → `PCB_TOOL_BASE`.
- **Key Features**:
  - Click selection with disambiguation menu for overlapping items.
  - Box selection (SelectRectArea) and lasso selection (SelectPolyArea) via `SELECTION_AREA`.
  - Net-chain selection: `selectConnection`, `expandConnection`, `selectNet`, `selectNetChain`.
  - Group enter/exit: `EnterGroup()`, `ExitGroup()`.
  - Table cell selection: range select with shift-click, toggle with ctrl-click, rect select within table.
  - Locked item filtering: `FilterCollectorForLockedItems`, `ReportFilteredLockedItems`.
  - Hierarchy awareness: `FilterCollectorForHierarchy` — avoids selecting both parent and child.
  - Cross-probe sync: `syncSelection`, `syncSelectionWithNets`, `doSyncSelection`.
  - Filter pipeline: `FilterCollectedItems(collector, multiSelect)` applies `PCB_SELECTION_FILTER_OPTIONS`.
- **Stop Conditions for connection expansion**: `STOP_AT_JUNCTION`, `STOP_AT_SEGMENT`, `STOP_AT_PAD`, `STOP_NEVER`.
- **Key Public Methods**: `GetSelection()`, `RequestSelection(filter)`, `SelectAllItemsOnNet(netCode)`, `GuessSelectionCandidates(collector, pos)`, `RebuildSelection()`, `FindItem(item)`, `Selectable(item)`, `FilterCollectorFor*`.

---

## ZONE_MODE enum (pcb_actions.h)
- `ADD`: New zone with fresh settings.
- `CUTOUT`: Cutout from an existing zone.
- `SIMILAR`: New zone reusing settings of an existing zone.
- `GRAPHIC_POLYGON`: Graphical polygon (not electrically filled).
