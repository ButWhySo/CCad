## API_HANDLER_PCB
- **File**: `pcbnew/api/api_handler_pcb.h`, `pcbnew/api/api_handler_pcb.cpp`
- **Purpose**: The KiCad IPC API handler for the PCB editor. Translates protobuf-based API commands into board operations.
- **Inheritance**: `API_HANDLER_PCB` → `API_HANDLER_EDITOR`.
- **Context Object**: Holds a `BOARD_CONTEXT` (owns board + tool manager + project ref). Supports headless (no frame) mode via `isHeadless()`.
- **Key Command Groups**:
  - **Document**: `handleSaveDocument`, `handleSaveCopyOfDocument`, `handleRevertDocument`, `handleSaveDocumentToString`, `handleSaveSelectionToString`, `handleParseAndCreateItemsFromString`.
  - **Items CRUD**: `handleGetItems`, `handleGetItemsById`, `handleCreateUpdateItemsInternal` (create/update), `deleteItemsInternal`.
  - **Selection**: `handleGetSelection`, `handleClearSelection`, `handleAddToSelection`, `handleRemoveFromSelection`.
  - **Board Properties**: `handleGetStackup`, `handleGetBoardEnabledLayers`, `handleSetBoardEnabledLayers`, `handleGetGraphicsDefaults`, `handleGetBoardDesignRules`, `handleSetBoardDesignRules`, `handleGetCustomDesignRules`, `handleSetCustomDesignRules`, `handleGetBoardOrigin`, `handleSetBoardOrigin`, `handleGetBoardLayerName`.
  - **Layer/Appearance**: `handleGetVisibleLayers`, `handleSetVisibleLayers`, `handleGetActiveLayer`, `handleSetActiveLayer`, `handleGetBoardEditorAppearanceSettings`, `handleSetBoardEditorAppearanceSettings`.
  - **Connectivity/Nets**: `handleGetNets`, `handleGetConnectedItems`, `handleGetItemsByNet`, `handleGetItemsByNetClass`, `handleGetNetClassForNets`.
  - **Queries**: `handleGetBoundingBox`, `handleGetPadShapeAsPolygon`, `handleCheckPadstackPresenceOnLayers`, `handleExpandTextVariables`.
  - **Actions**: `handleRunAction`, `handleInteractiveMoveItems`, `handleRefillZones`, `handleInjectDrcError`.
  - **Jobs**: `handleRunBoardJobExport3D`, `handleRunBoardJobExportRender`, `handleRunBoardJobExportSvg`, `handleRunBoardJobExportDxf`, `handleRunBoardJobExportPdf`, `handleRunBoardJobExportPs`, `handleRunBoardJobExportGerbers`, `handleRunBoardJobExportDrill`, `handleRunBoardJobExportPosition`, `handleRunBoardJobExportGencad`, `handleRunBoardJobExportIpc2581`, `handleRunBoardJobExportIpcD356`, `handleRunBoardJobExportODB`, `handleRunBoardJobExportStats`.
- **Context**: This is the bridge between external scripts/tools (via the KiCad API server) and the PCB editor's board model. Critical for LLM-native agent command injection.

---

## toolbars_pcb_editor
- **File**: `pcbnew/toolbars_pcb_editor.h`, `pcbnew/toolbars_pcb_editor.cpp`
- **Purpose**: Defines the PCB editor's toolbar configuration and custom toolbar controls.
- **Classes**:
  - `PCB_ACTION_TOOLBAR_CONTROLS`: Static `ACTION_TOOLBAR_CONTROL` instances for `trackWidth`, `viaDiameter`, and `currentVariant` toolbar dropdowns.
  - `PCB_EDIT_TOOLBAR_SETTINGS`: Extends `TOOLBAR_SETTINGS` with the key `DefaultToolbarConfig(TOOLBAR_LOC)` override that defines the default layout for each toolbar location (left, right, top, etc.).
- **Context**: Controls which tools, dropdowns, and separators appear in each toolbar position of the PCB editor.
