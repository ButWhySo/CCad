# CCad Codebase Map  This is the first file a memory-loss agent should read after `AGENTS.md`. It explains what each module owns, what not to touch casually, and which functions are the current public seams.  ## Current Progress  - Phase: 8 / 9 - Last merged sprint: Sprint 225, PCB source-walk slice 1 & Orchestrator un-stubbing - Next sprint: Sprint 226 - Sprint sizing: prefer moderate branches that group several related tasks before the full clean gate; avoid one tiny branch per small GUI affordance when compile cost dominates. - Active branch pattern: `sprint-<n>-<topic>` - Current source of truth for phase/sprint counter: `docs/devops/progress.md` - Main product direction: native C++ PCB kernel and machine-callable CLI first; Qt GUI is a human review/editor client, not the data owner.  ## Hard Rules  - Do not make the GUI own design state. GUI renders `ccad_core` models. - Do not execute design/library file contents. Treat project JSON and KiCad files as data only. - Write tests before behavior changes. - Run full gate before commits and merges:  ```powershell cmake --build build-qt --clean-first ctest --test-dir build-qt --output-on-failure ```  - On Windows, run Qt-linked tests and GUI programs with Qt's `bin` directory first on `PATH`:  ```cmd cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && ctest --test-dir build-qt --output-on-failure" ```  - If CTest reports Windows loader error `0xc0000139` before test output appears, treat it as a runtime DLL resolution problem first. Check `cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && where Qt6Core.dll"` and make sure the Qt DLL from `C:\Qt\6.11.1\mingw_64\bin` is first.  - Every commit/merge status update must include:  ```text Progress: Phase X/Y, Sprint N, <branch>, <status> ```  - Commits must have meaningful multi-line bodies. Minimum structure:  ```text <type>: <specific summary>  Progress: Phase X/Y, Sprint N, <branch>, <status>.  Why: - Problem or feature being addressed.  Changed: - File/module: what changed and why.  Behavior: - User-visible or agent-visible behavior change.  Verification: - Exact commands run and result.  Demo: - Screenshot/artifact path when relevant. ```  - Demo/screenshot outputs go under ignored `artifacts/`.  ## Build Targets  ### `ccad_core`  Native C++ kernel library. All durable model, validation, serialization, import, diff, transaction, and canvas-scene logic belongs here.  ### `ccad`  Machine-callable CLI. It should be deterministic, scriptable, and safe for LLM/tool use.  ### `ccad_gui`  Optional Qt 6 Widgets GUI. It is for human review and visual feedback. It must stay thin.  ### Test Targets  - `serialize`: project JSON and model round-trip tests. - `erc`: logical electrical-rule diagnostics. - `drc`: physical board diagnostics. - `kicad_footprint_import`: KiCad footprint import and CCad footprint JSON tests. - `library_catalog`: native catalog/provenance JSON tests. - `cli`: black-box CLI behavior. - `review`: human review summary model. - `diff`: project diff model. - `transaction`: transaction journal model. - `geometry`: unit conversion and geometric helpers. - `canvas`: GUI-independent board canvas scene model. - `schematic`: logical connectivity and hierarchy models.  ## Core Files  ### `src/ccad_core/model.hpp`  Owns the central project data structures.  Important structs:  ```cpp struct Project {   int schema_version;   std::string id;   std::string name;   std::optional<Board> board;   std::vector<Component> components;   std::vector<Net> nets;   std::vector<Constraint> constraints; }; ```  ```cpp struct Board {   Rect outline;   std::vector<Layer> layers;   std::vector<Keepout> keepouts;   std::vector<Pad> pads;   std::vector<Via> vias;   std::vector<TrackSegment> tracks; }; ```  ```cpp struct Keepout {   std::string id;   std::string kind;   Rect area; }; ```  ```cpp struct Pad {   std::string id;   std::string component_id;   std::string pin_name;   std::string net_id;   std::string layer_id;   Point position;   double rotation_degrees;   Size size; }; ```  Rules:  - `Board::pads` are placed physical pads. - `Board::keepouts` are rectangular forbidden regions checked by DRC. - `FootprintPad` is reusable library geometry and lives in `footprint.hpp`. - Add fields carefully: update JSON, canvas, DRC, CLI, tests, docs.  ### `src/ccad_core/geometry.hpp/.cpp`  Owns physical units and simple geometry.  Public functions:  ```cpp Length nanometers(std::int64_t value); Length millimeters(double value); Length mils(double value); Point maxPoint(const Rect& rect); ```  Rule:  - Store durable lengths as integer nanometers. - Convert mm/mil only at CLI/import/UI boundaries.  ### `src/ccad_core/serialize.hpp/.cpp`  Owns CCad project JSON.  Public functions:  ```cpp Project loadProjectJson(std::string_view source); std::string dumpProjectJson(const Project& project); ```  Rules:  - Output must be deterministic. - Parser is intentionally strict and rejects unknown project/board keys. - If you add model fields, update both reader and writer.  ### `src/ccad_core/erc.hpp/.cpp`  Owns logical checks over components/nets/pins.  Public functions/types:  ```cpp struct Diagnostic {   std::string severity;   std::string code;   std::string message;   std::string object_id; };  std::vector<Diagnostic> runErc(const Project& project); ```  Current checks:  - empty project warning - duplicate component IDs - duplicate pins - duplicate net members - unknown components - unknown pins  ### `src/ccad_core/drc.hpp/.cpp`  Owns physical board checks.  Public function:  ```cpp std::vector<Diagnostic> runDrc(const Project& project); ```  Current checks:  - duplicate pad/via/track IDs - unknown pad/track layers - pad/via/track positions outside board - non-positive pad/via/track dimensions - unconnected pads as warnings - unconnected vias/tracks as warnings - unknown non-empty pad/via/track net IDs - unconnected track endpoints as warnings - pad/via/track endpoints inside rectangular keepouts - track segments crossing rectangular keepouts - fixed default 0.20 mm copper clearance between different-net pads, vias, and tracks - via drill larger than diameter - zero-length tracks  ### `src/ccad_core/canvas.hpp/.cpp`  Owns GUI-independent scene data.  Public function:  ```cpp CanvasScene buildCanvasScene(const Project& project); ```  Important structs:  ```cpp struct CanvasPad {   std::string id;   std::string net_id;   std::string layer_id;   double x_units;   double y_units;   double width_units;   double height_units;   double rotation_degrees; }; ```  Rule:  - GUI rendering must consume `CanvasScene`, not inspect project geometry directly. - Canvas scene metadata supports read-only GUI browsing. Keep it semantic and derived from the kernel model.  ### `src/ccad_core/schematic.hpp/.cpp`  Owns logical connectivity and hierarchy.  Public functions:  ```cpp SchematicGraph buildConnectivity(const Project& project); void validateNetlists(const SchematicGraph& graph); ```  Rule:  - Separate logical connectivity from physical board representation.  ### `src/ccad_core/review.hpp/.cpp`  Owns human review summary.  Public function:  ```cpp ProjectReview buildReview(const Project& project); ```  Used by:  - `ccad inspect` - Qt review GUI header/cards/diagnostic table  Rule:  - Review diagnostics include both ERC and DRC results so GUI and CLI inspect see logical and physical issues through one stream.  ### `src/ccad_core/diff.hpp/.cpp`  Owns project diff model.  Public functions:  ```cpp ProjectDiff diffProjects(const Project& before, const Project& after); std::string dumpProjectDiffJson(const ProjectDiff& diff); ```  ### `src/ccad_core/transaction.hpp/.cpp`  Owns transaction journal records.  Public functions:  ```cpp Transaction makeTransaction(std::string id, std::string command,                             std::string summary, const Project& before,                             const Project& after); std::string dumpTransactionJson(const Transaction& transaction); ```  ### `src/ccad_core/footprint.hpp`  Owns reusable footprint/library data.  Important structs:  ```cpp struct FootprintPad {   std::string number;   std::string type;   std::string shape;   Point position;   double rotation_degrees;   Size size;   std::optional<Length> drill;   std::vector<std::string> layers; };  struct Footprint {   std::string name;   std::vector<FootprintPad> pads; }; ```  Rule:  - Footprints are library/package definitions. - Placed board pads are `Pad` objects in `Board`.  ### `src/ccad_core/kicad_footprint_import.hpp/.cpp`  Owns KiCad `.kicad_mod` import and CCad footprint JSON.  Public functions:  ```cpp Footprint importKiCadFootprint(std::string_view source); std::string dumpFootprintJson(const Footprint& footprint); Footprint loadFootprintJson(std::string_view source); ```  Current importer scope:  - root `(footprint "...")` - `(pad "...")` - pad type/shape - `(at x y [rotation])` - `(size width height)` - simple `(drill diameter)` - `(layers ...)`  Rules:  - Treat KiCad files as data. - Skip unsupported KiCad constructs safely. - Reject malformed s-expressions and non-footprint root. - Accept UTF-8 BOM.  ### `src/ccad_core/library_catalog.hpp/.cpp`  Owns CCad native library catalog metadata for local/offline component libraries.  Important structs:  ```cpp struct LibrarySource {   std::string name;   std::string kind;   std::string url;   std::string commit;   std::string mirror;   std::string fetched_at; };  struct LibraryItem {   std::string id;   std::string kind;   std::string name;   std::string source_path;   std::string native_path;   std::string sha256;   std::string license;   std::string provenance;   std::string usage_summary;   std::vector<std::string> layout_notes;   std::string source_confidence;   std::string review_status;   std::vector<std::string> warnings; }; ```  Public functions:  ```cpp std::string dumpLibraryCatalogJson(const LibraryCatalog& catalog); LibraryCatalog loadLibraryCatalogJson(const std::string& json); const LibraryItem* findLibraryItem(const LibraryCatalog& catalog, const std::string& id); std::vector<const LibraryItem*> searchLibraryItems(const LibraryCatalog& catalog,                                                    const std::string& query); std::vector<const LibraryItem*> searchLibraryItems(const LibraryCatalog& catalog,                                                    const std::string& query,                                                    const std::string& kind); std::vector<CatalogDiagnostic> validateLibraryCatalog(const LibraryCatalog& catalog); std::vector<CatalogDiagnostic> validateLibraryCatalog(const LibraryCatalog& catalog,                                                       const std::filesystem::path& root); ```  Rules:  - KiCad and mirror libraries are source data. - CCad runtime should query native catalog records, not repeatedly fetch or parse raw upstream files. - Huge local caches belong under ignored paths such as `library-cache/` or `catalog-cache/`, not the main source tree. - Every imported item must preserve source path, source commit/hash, checksum, license, provenance, warnings, and any available component-knowledge fields. - `usage_summary` and `layout_notes` are local agent-facing knowledge fields for choosing and placing components. - `source_confidence` records where the knowledge came from. Keep it descriptive until the ingestion taxonomy is formalized. - `review_status` is a controlled curation field. Current accepted values are `generated`, `needs_review`, `reviewed`, and `rejected`. - Catalog search indexes identity, paths, usage summary, layout notes, source confidence, and review status.  ### `src/ccad_core/agent_orchestrator.hpp/.cpp`

Owns the agent orchestration layer, deterministic goal decomposition, task dependency tracking, and tool registry.

Important structs:
```cpp
struct AgentTask {
  std::string id;
  std::string description;
  std::string tool_name;
  std::string result_json;
  TaskStatus status;
};

struct AgentGoal {
  std::string id;
  std::string description;
  GoalStatus status;
  std::vector<AgentTask> tasks;
};

class AgentOrchestrator {
public:
  void register_tool(const OrchestratorTool& tool);
  AgentGoal plan(const std::string& goal, const ProjectContext& ctx);
  AgentGoal orchestrate(const std::string& goal, const ProjectContext& ctx);
};
```

Rule:
- Task decomposition must be deterministic.
- External tools run through the standard tool dispatch callback mechanism.

### `src/ccad_gui/agent_panel.hpp/cpp`: 
Manages the agent workflow panel. It initializes the JSON-RPC C++ `AgentOrchestrator` instance and launches `src/ccad_agent/orchestrator.py` via `QProcess`, connecting the Qt UI (a modern Copilot-style interface with chat bubbles and cards) to the agent tool logic.

- `src/ccad_agent/orchestrator.py`: Python process containing the LangGraph orchestration. Implements a `Supervisor` pattern delegating to a `RouterAgent` and a `LibrarianAgent`. Initializes multi-provider models (Anthropic, Gemini, OpenAI) and OpenTelemetry/Langfuse callbacks via environment variables. Uses `sys.stdin` and `sys.stdout` for communication via a strict JSON-RPC protocol.
- `src/ccad_gui/agent_panel.hpp/.cpp`: Implements the premium, right-side chat interface dock. Layout closely mirrors the Copilot Chat UI paradigm (light grey history, dark grey user bubbles flush right, inline agent markdown, and dark grey icon-based composer). Binds the bottom tool buttons to trigger Marketplace dialogs, insert `/` templates, and send JSON-RPC provider configuration events.

### `src/ccad_agent/agent_serve.cpp`: 
Simple CLI executable representing the standalone agent backend execution.

## CLI Files  ### `src/ccad_cli/main.cpp`  Entrypoint only.  Owns:  ```cpp int main(int argc, char** argv); ```  Rule:  - Delegate process behavior to `ccad_cli::run`.  ### `src/ccad_cli/app.hpp/.cpp`  Owns top-level command dispatch, usage text, and machine-readable command metadata.  Public function:  ```cpp int run(int argc, char** argv); ```  Internal command metadata functions:  ```cpp const std::vector<CommandHelp>& commandHelp(); std::string helpJson(); int helpCommand(const std::vector<std::string>& args); ```  Command groups:  ```text ccad init ccad help --format json ccad validate ccad drc ccad inspect ccad diff ccad lib import-footprint ccad lib catalog-info ccad lib catalog-find ccad lib catalog-search ccad pcb add-pad ccad pcb add-via ccad pcb add-track ccad pcb add-keepout ccad pcb add-placement-region ccad pcb add-layer ccad pcb set-layer-visibility ccad pcb place-footprint ```  ### `src/ccad_cli/common.hpp/.cpp`  Owns shared CLI helpers for option parsing, project/footprint file I/O, JSON responses, and board mutation validation.  Important functions:  ```cpp Project loadProjectFile(const std::string& path); bool writeProjectFile(const std::string& path, const Project& project); Footprint loadFootprintFile(const std::string& path); std::map<std::string, std::string> parseOptions(...); std::string requireOption(...); Length requirePositiveMillimeters(...); double requireDoubleOption(...); double optionDoubleOrDefault(...); std::string diagnosticsJson(...); std::string reviewJson(...); Board& requireBoard(Project& project); void requireLayer(const Board& board, const std::string& layer_id); void requireInsideBoard(...); void requireUniquePadId(...); void requireUniqueViaId(...); void requireUniqueTrackId(...); Point rotateAndTranslate(...); ```  Rule:  - These helpers must stay deterministic and must not shell out or execute project/library file contents.  ### `src/ccad_cli/agent_orchestrator_cli.hpp/.cpp`

Owns the `ccad agent orchestrate` and `ccad agent plan` CLI wrapper, providing the JSON-RPC execution boundary for the Agent Orchestrator.

### `src/ccad_cli/project_commands.hpp/.cpp`  Owns project-level commands:  ```cpp int initCommand(const std::vector<std::string>& args); int validateCommand(const std::vector<std::string>& args); int drcCommand(const std::vector<std::string>& args); int inspectCommand(const std::vector<std::string>& args); int diffCommand(const std::vector<std::string>& args); ```  ### `src/ccad_cli/pcb_commands.hpp/.cpp`  Owns PCB mutation command group:  ```cpp int pcbCommand(const std::vector<std::string>& args); ```  Placement rule:  - `pcb place-footprint` maps `FootprintPad` to placed `Pad`. - `pcb add-keepout` maps command arguments to one rectangular `Keepout` and rejects duplicate IDs or areas outside the board. - Pad ID format: `<component>.<pad-number>`. - `net_id` is copied from the first logical `Net` member matching the placed component ID and footprint pad number. - If no logical net member matches, `net_id` remains empty for backward compatibility.  ### `src/ccad_cli/lib_commands.hpp/.cpp`  Owns library/import command group:  ```cpp int libCommand(const std::vector<std::string>& args); ```  Current scope:  - `lib import-footprint --in <path.kicad_mod> --out <path.json>` - `lib catalog-info --catalog <path.ccad-library.json>` - `lib catalog-find --catalog <path.ccad-library.json> --id <id>` - `lib catalog-search --catalog <path.ccad-library.json> --query <text> [--kind <kind>]` - `lib catalog-validate --catalog <path.ccad-library.json> [--root <native-library-root>]`  Rule:  - `catalog-find` and `catalog-search` must print component-knowledge fields as part of item JSON so agents do not need to parse raw catalog files after finding a candidate.  ## GUI Files  ### `src/ccad_gui/main.cpp`  Entrypoint only.  Owns:  ```cpp int main(int argc, char** argv); ```  Also owns the local GUI screenshot harness:  ```cmd build-qt\ccad_gui.exe --screenshot <project.ccad.json> <out.png> ```  The harness loads the project, shows the native Qt window, processes GUI events for 7 seconds, grabs the window through Qt, writes the PNG, and exits the launched process. Keep this path independent of foreground-window capture so it never steals or kills the user's active application.  ### `src/ccad_gui/toolbar.hpp/.cpp`  Owns common application tools and action triggers.  Important methods:  ```cpp void setupTools(); void addActionTrigger(const QString& name, std::function<void()> callback); ```  ### `src/ccad_gui/review_window.hpp/.cpp`  Owns native review window, menus, toolbar, summary cards, diagnostics table, file loading, and calls into canvas renderer.  Important methods:  ```cpp ReviewWindow(); void loadProjectPath(const std::filesystem::path& path); void applyStyle(); void openProject(); void reloadProject(); void renderReview(const ccad::ProjectReview& review); void renderCanvas(const ccad::CanvasScene& scene); void setStatusChip(const QString& text, const QString& color); ```  Rule:  - Do not parse geometry here. - Load project -> `buildReview` and `buildCanvasScene` -> render.  ### `src/ccad_gui/project_summary_panel.hpp/.cpp`  Owns the project summary dock content.  Important methods:  ```cpp void renderReview(const ccad::ProjectReview& review); void renderLoadFailure(const QString& path); ```  ### `src/ccad_gui/diagnostics_panel.hpp/.cpp`  Owns diagnostics table setup and row rendering.  Important method:  ```cpp void renderDiagnostics(const std::vector<ccad::Diagnostic>& diagnostics); QString objectIdForRow(int row) const; ```  ### `src/ccad_gui/transaction_timeline_panel.hpp/.cpp`  Owns the read-only transaction timeline table in the bottom dock.  Important methods:  ```cpp void renderTransactions(const std::vector<ccad::Transaction>& transactions); QString itemText(int row, int column) const; QString transactionIdForRow(int row) const; ```  Rule:  - The timeline displays transaction metadata and diff counts only. It must not mutate projects or replay transaction content.  ### `src/ccad_gui/selection_inspector_panel.hpp/.cpp`  Owns the read-only selected-object inspector in the right dock.  Important methods:  ```cpp void clearSelection(); void renderSelection(const QString& type, const QString& id); void renderCanvasItem(); QString rowText(const QString& label) const; ```  Rule:  - The inspector displays stable object identity from canvas item metadata. It must not mutate project state or infer board geometry.  ### `src/ccad_gui/object_browser_panel.hpp/.cpp`  Owns the read-only right-dock layer and board-object browser.  Important methods:  ```cpp void renderScene(const ccad::CanvasScene& scene); void setObjectActivatedCallback(std::function<void(QString)> callback); void setNetActivatedCallback(std::function<void(QString)> callback); int itemCount() const; QString itemText(int row) const; QString objectIdForRow(int row) const; QString netIdForRow(int row) const; ```  Rule:  - The browser renders `CanvasScene` metadata only. It must not parse project JSON, own board state, or mutate design objects. - Object rows carry stable canvas object IDs so the main window can select matching PCB canvas items without duplicating geometry knowledge. - Net rows summarize non-empty net IDs from pads, vias, and tracks, then activate canvas net selection by stable net ID. - Do not populate placeholder rows in the constructor; render rows only through `renderScene` so project-load refreshes do not clear constructor-owned items during window startup.  ### `src/ccad_gui/board_canvas_renderer.hpp/.cpp`  Owns drawing board canvas objects into `QGraphicsScene`.  Public function:  ```cpp void renderBoardCanvas(QGraphicsScene& canvas_scene, const ccad::CanvasScene& scene); void renderBoardCanvas(QGraphicsScene& canvas_scene, const ccad::CanvasScene& scene,                        const CanvasRenderTheme& theme); QString canvasObjectId(const QGraphicsItem& item); QString canvasObjectType(const QGraphicsItem& item); QString canvasObjectNetId(const QGraphicsItem& item); QString canvasObjectLayerId(const QGraphicsItem& item); bool canvasUsesShapeSelectionHighlight(const QGraphicsItem& item); bool selectCanvasObjectById(QGraphicsScene& canvas_scene, const QString& id); int selectCanvasObjectsByNetId(QGraphicsScene& canvas_scene, const QString& net_id); void addDiagnosticMarkers(QGraphicsScene& canvas_scene,                           const std::vector<ccad::Diagnostic>& diagnostics); void addDiagnosticMarkers(QGraphicsScene& canvas_scene,                           const std::vector<ccad::Diagnostic>& diagnostics,                           const CanvasRenderTheme& theme); QString canvasDiagnosticMarkerObjectId(const QGraphicsItem& item); QString canvasDiagnosticMarkerSeverity(const QGraphicsItem& item); ```  Renders:  - board outline - grid - tracks - pads, including rotation - vias - board size label - stable object type/ID metadata on selectable primitive items - stable net/layer metadata on selectable primitives where the kernel scene provides it - shape-level selection highlighting for selectable primitive items, avoiding loose Qt bounding boxes - presentation-only `CanvasRenderTheme` colors for future theme/plugin compatibility - selecting one canvas object by stable object ID for diagnostic-table linking - selecting all canvas objects on one net ID for future net highlighting - read-only diagnostic markers for diagnostics whose object IDs match selectable canvas items  ### `src/ccad_gui/board_canvas_view.hpp`  Owns editor viewport behavior.  Important method:  ```cpp void zoomToFit(); void zoomIn(); void zoomOut(); void resetZoom(); void wheelEvent(QWheelEvent* event) override; void mousePressEvent(QMouseEvent* event) override; void mouseMoveEvent(QMouseEvent* event) override; ```  ## Scripts  ### `scripts/run_sprint_demo.ps1`  Creates a demo board, adds primitives, imports KiCad footprint sample, places the footprint, runs inspect/validate/DRC, then asks `ccad_gui --screenshot` to capture the native GUI.  Run:  ```powershell powershell -ExecutionPolicy Bypass -File .\scripts\run_sprint_demo.ps1 -Name sprint-demo ```  Outputs:  - `artifacts/demos/*.ccad.json` - `artifacts/demos/*.inspect.json` - `artifacts/demos/*.validate.json` - `artifacts/demos/*.drc.json` - `artifacts/demos/*.ccad-footprint.json` - `artifacts/screenshots/*.png`  Rule:  - The script must use app-owned screenshot mode and must not capture arbitrary foreground windows or kill globally named GUI processes.

Sprint 207 addendum: `src/ccad_gui/agent_panel.cpp` added the fifth reference-inspired Agent pane contract with compact header action bar, evidence thumbnails, approval preview, and UI-map target proof.

## Sprint 225 Parity & Orchestrator Addendum

Current position is Phase 9 / 9, Sprint 225, branch `sprint-225-parity-orchestrator`. Sprint 225 begins the deterministic KiCad source-walk parity mandate from Iteration 467.

`src/ccad_core/model.hpp` added KiCad analogue fields `teardrops_enabled` to `Pad` and `Via` (representing properties from `BOARD_CONNECTED_ITEM`), and added `drc_exclusions` and `ratsnest_exclusions` to `DesignRules` (representing states managed by `BOARD`).

`src/ccad_agent/orchestrator.py` removed placeholder stubs and wired real JSON-RPC `/` command routing (e.g., `/settings`, `/explain`, `/marketplace install`) into the `langgraph` ToolNode execution path. This clears the first hurdle for matching the Copilot UI style and orchestrator parity.

## Current Known Technical Debt To Avoid Expanding  - CLI commands are now split, but `src/ccad_cli/pcb_commands.cpp` should be split further once placement, routing, or net mapping grows. - `serialize.cpp` and `kicad_footprint_import.cpp` contain handwritten parsers. They are deterministic and tested, but keep scope narrow. - GUI is an early CAD editor shell, not a full editor yet. - No schematic-footprint mapping yet; placed footprint pads have empty nets. - Clearance DRC currently uses a fixed default 0.20 mm threshold. Per-netclass and per-constraint clearance rules are later work.  ## How To Add A New Feature  1. Update or create sprint spec and plan under `docs/superpowers`. 2. Update `docs/devops/progress.md`. 3. Write failing tests. 4. Implement minimum code. 5. Run focused tests. 6. Run full gate. 7. Commit with progress line in user update. 8. Update docs/features and README if user-visible. 9. Run demo script and capture screenshot when GUI-visible. 10. Merge only after full gate passes.
## Sprint 127 Addendum

Current position is Phase 4 / 6, Sprint 127, branch `sprint-127-gui-route-review`. Phase 4 has a working budget of Sprints 127 through 132 before scope review is required. Sprint 126 is committed and merged on `main`; do not treat it as awaiting commit.

The GUI route-review contract is now derived from kernel models. `ProjectReview` reports `route_request_count`, `open_route_count`, `partial_route_count`, `completed_route_count`, and `routed_segment_count`. `ccad inspect` emits those fields under board route progress. `CanvasScene` carries route-request rows plus track `source_route_request_id`, and `ObjectBrowserPanel` renders route-request rows without pretending they are drawable copper objects. Route rows activate route selection, and `BoardCanvasRenderer` selects tracks whose route provenance matches the activated request.

When changing this area, keep `ccad_gui` thin. Do not parse project JSON in the GUI to recover route status. Update `review`, `canvas`, `cli`, and GUI panel tests together because route progress is a shared CLI and GUI review contract.

Current position is Phase 4 / 6, Sprint 128, branch `sprint-128-kicad-layer-foundation`. Sprint 128 adds `src/ccad_core/layers.hpp/.cpp` as the KiCad standard PCB layer registry and append helper. The registry uses KiCad canonical layer IDs, preserves existing board layers, appends missing standard layers idempotently, and maps layer IDs to CCad layer kinds while leaving the durable JSON schema unchanged.

`ccad pcb add-standard-layers --file <path>` is implemented in `src/ccad_cli/pcb_commands.cpp` and advertised through `src/ccad_cli/app.cpp`. It is a metadata command only; it does not change board geometry, stackup materials, import/export, or copper authoring rules. Pads, tracks, route requests, and placed footprints still require layers whose `kind` is `copper`.

`tests/test_layers.cpp` covers registry count, representative KiCad IDs, representative kinds, order, custom-layer preservation, and idempotency. `tests/test_cli.cpp` covers help discovery and executable CLI behavior. `scripts/run_sprint_demo.ps1` now creates a bridge-rectifier board and exercises `pcb add-standard-layers` before GUI screenshot capture.

Current position is Phase 4 / 6, Sprint 129, branch `sprint-129-layer-review-summary`. `ProjectReview` now carries `copper_layer_count`, `non_copper_layer_count`, `visible_layer_count`, and `hidden_layer_count`. `ccad inspect` emits these values under `board.layer_summary`, and `ProjectSummaryPanel` renders them as layer breakdown and layer visibility cards. Keep future layer-review features flowing through the kernel review model first, then CLI inspect and GUI, so the GUI remains a client instead of a data owner.

## Sprint 130 Addendum

Sprint 130 completed interactive layer visibility controls in the right-dock layer browser.

## Sprint 131 Addendum

Sprint 131 finalized coordinate inspector panel details and physical unit validations.

## Sprint 132 Addendum

Sprint 132 completed interactive physical DRC design rule and object property editing in the Selection Inspector Panel.

## Sprint 133 Addendum

Current position is Phase 5 / 6, Sprint 133, branch `sprint-133-kicad-board-export`. Sprint 133 adds `src/ccad_core/kicad_pcb_export.hpp/.cpp` to serialize CCad project boards to KiCad S-expression `.kicad_pcb` files.

`ccad pcb export-kicad --file <project.ccad.json> --output <out.kicad_pcb>` is implemented in `src/ccad_cli/pcb_commands.cpp` and advertised through `src/ccad_cli/app.cpp`.

`tests/test_kicad_pcb_export.cpp` covers exact S-expression structure formatting (board outline, layers, keepouts, pads, vias, tracks, coordinates, layers, net mappings, and float precision). `tests/test_cli.cpp` validates the integration and CLI command interface.

## Sprint 145 Addendum

Sprint 145 implemented interactive footprint placement and a native Qt Library Browser dialog. The core placement engine added `moveFootprint()` in `src/ccad_core/placement.cpp` to adjust component pad coordinates. `src/ccad_gui/library_browser_dialog.hpp/.cpp` implements a filtered component search dialog substituting `QFileDialog`. `ReviewWindow` implements a state machine (`PlaceFootprint`, `MoveFootprint`, `Default`) via `eventFilter` and visualizes ghost footprints during interactions prior to invoking core placement APIs.

## Sprint 148 GUI Parity Addendum

Sprint 148 moved the native GUI closer to KiCad PCB Editor behavior while preserving the kernel-owned model rule. `src/ccad_gui/review_window.cpp` now builds KiCad-oriented left and right icon toolbars by resolving SVGs from the local KiCad source checkout through `CCAD_KICAD_SRC` or a sibling `kicad_src` directory. The same file owns interactive placement and movement state, but it still commits changes through `ccad_core::placeFootprint()` and `ccad_core::moveFootprint()` after converting canvas scene coordinates back into board millimeters.

`src/ccad_gui/board_canvas_renderer.cpp` now contains shape-aware pad path generation for `rect`, `roundrect`, `circle`, and `oval`, and it draws through-hole drill openings as separate `pad-drill` canvas objects so annular rings are visible in screenshots and tests. `src/ccad_core/serialize.cpp` accepts legacy pad `layer_id` JSON and maps it into the newer `Pad::layers` vector for old fixtures and project files.

## Sprint 149 Pad Fidelity Addendum

Sprint 149 extends KiCad-compatible pad metadata through the kernel, file formats, and GUI. `FootprintPad`, placed board `Pad`, and `CanvasPad` now carry optional `roundrect_rratio` and `chamfer_ratio` values. `src/ccad_core/kicad_footprint_import.cpp`, `src/ccad_core/kicad_footprint_export.cpp`, `src/ccad_core/serialize.cpp`, and `src/ccad_core/kicad_pcb_export.cpp` preserve those fields so library imports, project JSON, and exported KiCad board files do not collapse pad intent.

`src/ccad_gui/board_canvas_renderer.cpp` and `src/ccad_gui/review_window.cpp` now render ratio-controlled roundrect pads, trapezoid pads, and chamfered rectangles in both the committed canvas and interactive placement/move ghost previews. `scripts/run_sprint_demo.ps1` remains the official visual proof harness and now places a useful full-bridge rectifier demo plus a KiCad-imported footprint containing roundrect, trapezoid, chamfered, and through-hole circular pads.

## Sprint 150 Layer Registry Addendum

Sprint 150 corrected standard KiCad layer numbering and order. `src/ccad_core/layers.cpp` now matches current KiCad source ordering for non-copper layers: drawing/comment/ECO layers at 40-43, `Edge.Cuts` at 44, `Margin` at 45, courtyard/fab layers at 46-49, and `User.1` through `User.9` at 50-58.

`src/ccad_core/layers.hpp` exposes `standardKiCadPcbLayerNumber(id)` so code can ask the kernel for canonical KiCad layer numbers instead of re-encoding them. `src/ccad_core/kicad_pcb_export.cpp` now emits the corrected `.kicad_pcb` layer table, and `src/ccad_cli/pcb_object_queries.cpp` includes `kicad_layer_number` in layer object JSON for canonical layers.

## Sprint 151 Pad Authoring Addendum

Sprint 151 extends `src/ccad_cli/pcb_commands.cpp` so `ccad pcb add-pad` and `ccad pcb set-pad` can author KiCad-style pad metadata directly. `add-pad` accepts optional `--type`, `--shape`, `--drill-mm`, `--roundrect-rratio`, and `--chamfer-ratio`; `set-pad` accepts optional `--type`, `--shape`, `--roundrect-rratio`, and `--chamfer-ratio`.

Ratio parsing is kept in the CLI command module and rejects values outside `0.0` through `0.5` before writing project JSON. The durable storage and rendering paths were added in Sprint 149, so the CLI now feeds those existing model fields instead of creating a separate pad-authoring representation.

## Sprint 152 Rich Pad Query Addendum

Sprint 152 extends `src/ccad_cli/pcb_object_queries.cpp` so agent-facing pad lookups expose the same KiCad-style metadata that Sprint 151 can author. `pcbPadObjectJson()` now reports `pad_type`, `shape`, `layers`, optional `drill_nm`, optional `roundrect_rratio`, optional `chamfer_ratio`, position, rotation, and size. `listPcbObjectsJson(..., "pad")` includes compact pad rows with the same optional drill and ratio fields when present.

This is a query-contract change only. It does not alter the durable `ccad::Pad` model, serialization, rendering, DRC, or KiCad export paths.

## Sprint 153 Route-Job Pad Metadata Addendum

Sprint 153 extends `exportRouteJobJson()` in `src/ccad_cli/pcb_object_queries.cpp` so route jobs carry KiCad-style pad metadata for external routers and AI planning loops. Pad rows under `route_job.physical_objects.pads` keep the existing first `layer_id` field and now also include full `layers`, `pad_type`, `shape`, `rotation_degrees`, optional `drill_nm`, optional `roundrect_rratio`, and optional `chamfer_ratio`.

This is an export-contract change only. It does not change project JSON, the kernel board model, route application, DRC, or GUI rendering.

## Sprint 154 GUI Actions And Library Cache Addendum

Sprint 154 touches both the core symbol loader and the native GUI. `src/ccad_core/kicad_symbol_import.hpp/.cpp` now exposes `loadSymbolJsonFileWithLocalInheritance()`, which loads a converted symbol JSON file and, when the symbol has an `extends` parent, resolves a sibling parent JSON file to inherit missing pins and graphic primitives. This is intentionally local and file-based so raw library-cache data remains data and no code is executed.

`src/ccad_gui/symbol_placement_dialog.cpp` uses that loader for converted JSON and uses `importKiCadSymbolLibrary()` for raw `.kicad_sym` files. `src/ccad_gui/footprint_placement_dialog.cpp` now imports raw `.kicad_mod` files when the chooser returns one, preserving the existing converted JSON path.

`src/ccad_gui/library_browser_dialog.*` remains a lightweight local chooser, but it now behaves more like KiCad's chooser surface: filtered rows show library/name context and the right side shows details and preview metadata. `src/ccad_gui/review_window.*` now owns small GUI snapshot undo/redo stacks and wires Save, Board Setup, Run DRC, and DRC export to real behavior. Keep future GUI actions calling core model functions or model serialization helpers; do not move project ownership into Qt widgets.

## Sprint 155 KiCad Placement Chooser Addendum

Sprint 155 keeps the GUI thin while correcting the placement workflow. `src/ccad_gui/review_window.*` now routes Add behavior by active editor tab: PCB opens cache-backed footprint choosing and schematic opens cache-backed symbol choosing. Raw `previewFootprint()` and `previewSymbol()` helpers remain internal, but the normal File menu no longer exposes them as user authoring paths.

`ReviewWindow` now has `PlaceFootprint` and `PlaceSymbol` interaction modes with mouse-following ghost items. The PCB path stores a selected `ccad::Footprint`, converts the left-click scene point back to board millimeters, and commits through `ccad::placeFootprint()`. The schematic path stores a selected `ccad::Symbol`, converts the left-click scene point to schematic millimeters, and commits through `ccad::placeComponent()`. Escape cancels either placement mode before commit.

`src/ccad_gui/library_browser_dialog.*` now uses a KiCad-inspired table chooser instead of a flat list. It scans ignored local `library-cache` folders automatically, shows item/description/library columns, and computes pad-count or pin-count metadata by reading candidate files as data only.

`src/ccad_gui/board_canvas_renderer.*` now gives `F.Cu` and `B.Cu` different default colors and stores a selection-highlight width for selectable items. Tracks use a highlight wider than their rendered copper stroke so selection is visually tied to the whole track shape.

Resolved follow-up: Sprint 232 continuation on `sprint-232-schematic-model` stores an optional embedded `ccad::Symbol` snapshot on each placed `SchSymbol`. Saved schematic symbols now reload with their imported primitive graphics and pin-lead geometry for the supported CCad symbol JSON subset. The remaining limitation is full KiCad schematic primitive parity, not basic placed-symbol persistence.

## Sprint 156 Lazy Library Chooser Addendum

`src/ccad_gui/library_browser_dialog.*` now treats the chooser as a catalogue view first and a parser second. Dialog construction scans `library-cache/footprints` or `library-cache/symbols` by filename and suffix only, with an injectable cache root used by tests. `updateDetails()` lazily parses only the currently selected row to show pad or pin metadata and render a visual preview, and accepting the row leaves final footprint or symbol loading on the existing placement path.

Keep this invariant when expanding the chooser: opening Add Footprint or Add Symbol must not parse every cached component. Bulk metadata belongs in a future precomputed catalog/index, while selected-item metadata, selected-item preview, and final placement may parse one item as data.

The chooser can now be screenshot-tested directly through `ccad_gui --screenshot-chooser-footprint <cache-root> <out.png>` and `ccad_gui --screenshot-chooser-symbol <cache-root> <out.png>`. These modes show the real `LibraryBrowserDialog`, select the first visible row, wait the current 7-second single-preview settle time, and capture the dialog for agent inspection. `scripts/run_gui_interaction_demo.ps1` remains the mouse/keyboard harness for live placement-entry testing, including beep, two-second handoff delay, 7-second single-preview waits, and captured stdout/stderr.

Footprint preview colors use the shared `CanvasRenderTheme` and `colorForKiCadLayer()` helper from `src/ccad_gui/board_canvas_renderer.hpp`. Keep future actual-canvas and preview-canvas layer color changes routed through that shared helper so F.Cu, B.Cu, inner copper, silkscreen, fab, courtyard, edge, and user layers do not diverge between contexts.

Sprint 162 addendum: `src/ccad_gui/board_canvas_renderer.hpp/.cpp` now treats solder mask and solder paste as explicit KiCad layer classes. `F.Mask`, `B.Mask`, `F.Paste`, and `B.Paste` have distinct theme colors. Copper pad bodies render only when a copper pad layer is visible, while visible mask and paste layers render separate non-selectable `pad-mask` and `pad-paste` overlay objects tagged with layer metadata. `src/ccad_gui/library_browser_dialog.cpp` and `src/ccad_gui/review_window.cpp` now use the same layer palette for footprint chooser previews and footprint placement ghosts, so preview/ghost/canvas pad colors stay aligned.

Sprint 163 addendum: `src/ccad_gui/ui_map_server.hpp/.cpp` owns the local live UI-map server. It uses `QLocalServer` and newline-delimited JSON requests while the normal `ReviewWindow` is open. Supported methods are `ui.map`, `ui.target`, and `ui.epoch`; unsupported methods return structured `ok:false` responses. `src/ccad_gui/main.cpp` exposes `ccad_gui --serve-ui-map <project> <server-name> <ready-file>`. This is an agent-observation and targeting surface only; mutating design work should still prefer kernel transactions or explicit safe GUI actions.

Sprint 164 addendum: `src/ccad_gui/review_window.cpp` binds unfinished right-toolbar editor actions to a planned-tool status contract instead of leaving them as silent stubs. At Sprint 164, Route Track, Add Via, Add Zone, Add Keepout, Draw Graphic, Place Text, and Delete still needed real kernel-backed edit tools, so their `QAction` triggers updated the GUI status bar and `triggerSafeUiActionJson()` returned `performed:false`, `reason:"future_tool_not_implemented"`, and the action label. Later sprints supersede this for Add Via, Route Track, Add Keepout, Delete, Draw Graphic, Place Text, and Add Zone; keep the no-silent-stub contract only for editor actions that still lack durable kernel primitives. Tests live in `tests/test_gui_ui_map.cpp`.

Sprint 165 addendum: `src/ccad_gui/review_window.cpp` applies the same planned-tool contract to unfinished left-toolbar display and panel controls. Toggle Grid, Polar Coordinates, Toggle Units, Crosshair Cursor, Show Ratsnest, Net Highlight, Display Modes, Show Layers, and Show Properties now update the GUI status bar and return `future_tool_not_implemented` through `triggerSafeUiActionJson()`. The full KiCad-like behavior for these controls remains open backlog work.

Sprint 166 addendum: `src/ccad_gui/review_window.cpp` promotes Show Layers and Show Properties from planned-tool contracts to real safe actions. `action:layers_manager` toggles the existing `ObjectBrowserPanel`; `action:part_properties` toggles the existing `SelectionInspectorPanel`; both return `performed:true` and `reason:"panel_toggled"` through `triggerSafeUiActionJson()`. The remaining left-toolbar display controls still return `future_tool_not_implemented`.

Sprint 167 addendum: `src/ccad_gui/agent_panel.hpp/.cpp` owns the first native Agent panel shell. It is a Qt widget with project/epoch labels, an action-ID input, a `Refresh Map` button, a `Trigger Safe` button, and read-only JSON output. `ReviewWindow` embeds it as a bottom tab next to Diagnostics and Transactions, feeds it `uiMapJson()` and `triggerSafeUiActionJson()` callbacks, updates its project/epoch labels from `markUiMapChanged()`, and exposes `panel:agent` in both `uiMapJson()` and `uiTargetJsonById()`. Keep provider/BYOK/LangGraph/Langfuse/OpenTelemetry work outside this widget until the secrets, persistence, and trace policies are explicit.

Sprint 168 addendum: `src/ccad_gui/board_canvas_view.hpp` now owns view-local grid and crosshair overlay state through `drawBackground()` and `drawForeground()`. `src/ccad_gui/review_window.cpp` owns the left-toolbar display state booleans for grid, polar coordinates, inch units, crosshair, ratsnest, net highlight, and high-contrast mode; `triggerDisplayStateActionJson()` toggles them and `triggerSafeUiActionJson()` routes those IDs as real safe display actions. `ReviewWindow::renderPcbScene()` applies the high-contrast render theme, lightweight ratsnest overlays, and same-net selection highlighting. `uiMapJson()` includes checked state on action nodes, and `src/ccad_gui/main.cpp` extends `--test-ui-map-target-sequence` to trigger the display controls during screenshot runs. `src/ccad_gui/selection_inspector_panel.cpp` hides and detaches removed form widgets before deferred deletion so rapid multi-selection changes do not leave stale editor controls visible.

Sprint 169 addendum: `tests/test_visual_harness_policy.cpp` is a lightweight policy regression that reads `scripts/run_sprint_demo.ps1`, `scripts/run_ui_map_mouse_target_demo.ps1`, `scripts/run_gui_interaction_demo.ps1`, and `.agents/workflows/visual-validation.md` as data. It guards the current timing policy: 7 seconds for single-preview screenshots, 5000 ms initial load for multi-target GUI validation, and 800 ms per target/action. `scripts/run_gui_interaction_demo.ps1` now exposes those live-interaction timing knobs, uses a 7-second window-ready deadline instead of the stale 20-second deadline, and resolves the foreground chooser dialog before clicking the first row so live footprint/symbol preview checks actually select a catalogue item.

Sprint 170 addendum: `src/ccad_gui/review_window.hpp/.cpp` extends the interaction state machine with `AddVia`, `RouteTrack`, and `AddKeepout`. `ReviewWindow::enterAddViaMode()`, `enterRouteTrackMode()`, and `enterAddKeepoutMode()` are the human toolbar entries, while `commitViaPlacementForAutomation()`, `commitTrackPlacementForAutomation()`, `commitKeepoutPlacementForAutomation()`, and `deleteBoardObjectForAutomation()` are the app-owned agent/test hooks. These hooks send clicks through the Qt viewport event path and then write durable `ccad::Via`, `ccad::TrackSegment`, and `ccad::Keepout` objects into `project_cache_` before saving deterministic project JSON and re-rendering the core-derived view.

`ReviewWindow::triggerSafeUiActionJson()` now returns `editor_tool_selected` for `action:add_tracks`, `action:add_via`, and `action:add_keepout_area`, and routes `action:delete_cursor` to real selected-object deletion. Sprint 173 and Sprint 174 later promote Draw Graphic, Place Text, and Add Zone to durable kernel-backed tools. Keep this distinction for future tools: do not wire a GUI button to fake behavior unless the kernel model can persist the result.

`src/ccad_gui/main.cpp` now exposes `--test-place-via-click`, `--test-route-track-click`, `--test-place-keepout-click`, and `--test-delete-board-object` for LLM/native harnesses. These are regression and automation paths, not replacements for the kernel CLI. Layer and net selection work should keep the GUI thin by deriving context from the loaded kernel model, then passing explicit layer/net context into the same edit-mode hooks.

Sprint 171 addendum: `src/ccad_gui/review_window.hpp/.cpp` owns active PCB layer editor state as `active_pcb_layer_id_` plus the top-toolbar `active_layer_selector_`. The selector is rebuilt from `project_cache_.board->layers`, accepts copper layers only, defaults to `F.Cu` when available, and is intentionally not serialized into board JSON. `activePcbLayerJson()` and `setActivePcbLayerForAutomation()` expose the state to tests and agents, `uiMapJson()` exports `active_pcb_layer_id` and the stable `control:active_pcb_layer` node, and `UiMapServer` serves `ui.active_layer` plus `ui.set_active_layer`.

Footprint placement and Route Track now call `activePcbLayerOrDefault()` before entering their edit modes, so `ccad_core::placeFootprint()` and the GUI-created `ccad::TrackSegment` receive the selected copper layer. Via placement remains through-board, but the placement ghost uses the active copper layer color for context.

Sprint 172 addendum: `src/ccad_gui/review_window.hpp/.cpp` owns active PCB net editor state as `active_pcb_net_id_` plus the top-toolbar `active_net_selector_`. The selector is rebuilt from top-level `project_cache_.nets` plus non-empty board pad/via/track net IDs, defaults to the first available net, and is intentionally not serialized into board JSON. `activePcbNetJson()` and `setActivePcbNetForAutomation()` expose the state to tests and agents, `uiMapJson()` exports `active_pcb_net_id` and the stable `control:active_pcb_net` node, and `UiMapServer` serves `ui.active_net` plus `ui.set_active_net`.

Sprint 175 addendum: `src/ccad_gui/agent_panel.hpp/.cpp` now has a live-query row with method and JSON payload inputs plus a `Live Query` action. `ReviewWindow::runAgentUiQueryJson()` is the shared in-process dispatcher for the Agent panel and `UiMapServer`; it supports `ui.map`, `ui.map_delta`, `ui.find`, `ui.target`, `ui.target_board_point`, `ui.trigger_safe`, `ui.active_layer`, `ui.set_active_layer`, `ui.active_net`, `ui.set_active_net`, and `ui.epoch`. `ReviewWindow::uiMapDeltaJson()` is a first-slice low-token polling path: current epochs return no nodes, stale epochs return the current map until true dirty-node batching is implemented. `ReviewWindow::uiHitTestJson(x, y)` scans exported `global_rect` values and returns the smallest visible/enabled node containing the logical screen point. `ReviewWindow::uiNearestCanvasObjectJson(x_mm, y_mm, canvas, limit)` performs a first linear scan over selectable rendered canvas items and returns nearest object candidates before a future spatial index. `ReviewWindow::runAgentUiQueryJson()` and `UiMapServer` expose these as `ui.map_compact`, `ui.role_summary`, `ui.hit_test`, and `ui.nearest_canvas_object`.

Sprint 176 addendum: `ReviewWindow` now derives low-token agent query responses from the same `uiMapJson()` source of truth instead of creating a second UI state model. `uiMapCompactJson(role, limit)` returns compact nodes with stable id, role, label, visibility/enabled state, target center, and CAD metadata such as canvas, object id, type, net, layer, and route provenance while omitting heavy rectangles. `uiRoleSummaryJson()` returns per-role counts. `uiMapDeltaJson()` still returns an empty node list for current epochs, but stale epochs now return compact `nodes` rather than embedding the full nested map.

Sprint 177 addendum: `ReviewWindow::runAgentUiQueryJson()` now also exposes direct first-batch interaction tools for agents without creating a separate automation model. `ui.click` resolves semantic IDs through the existing target map and then either dry-runs, triggers allowlisted safe actions and tab changes through `triggerSafeUiActionJson()`, clicks named Agent-panel buttons, focuses controls, or selects a `canvas_object:*`. `ui.double_click` shares the same safety boundary and currently serves target-aware dry runs. `ui.type_text` writes only to whitelisted Agent-panel line edits. `ui.key` implements Escape cancellation for active placement or edit modes. `ui.select_canvas_object` and `ui.get_selection` use `board_canvas_renderer` metadata roles and `selectCanvasObjectById()`, so selected pads, vias, tracks, zones, graphics, text, keepouts, and regions report stable object IDs, type, net, layer, and route provenance. `ui.wait_for_epoch` pumps Qt events for a bounded timeout so live agents can poll for `ui_epoch` changes.

Add Via and Route Track now call `activePcbNetOrDefault()` when writing new `ccad::Via` and `ccad::TrackSegment` objects, so manually created copper no longer defaults to empty net assignment when a board net is available. Footprint placement intentionally remains component-pin driven and should not be overridden by the active PCB net selector.

Sprint 178 addendum: `ReviewWindow::runAgentUiQueryJson()` now exposes map-driven viewport input through `ui.canvas_click` and `ui.canvas_drag`. `ReviewWindow::uiCanvasClickJson()` maps a board millimeter point through `boardPositionToScene()`, `QGraphicsView::mapFromScene()`, and the PCB viewport global mapping, then sends a real `QMouseEvent` press/release pair to `canvas_view_->viewport()`. `ReviewWindow::uiCanvasDragJson()` performs a two-point gesture for current CCad line/rectangle tools: first point, mouse move for ghost preview refresh, and second point. These methods intentionally drive `ReviewWindow::eventFilter()` instead of mutating `project_cache_` directly, so Add Via, Route Track, Add Zone, Add Keepout, Draw Graphic, and Place Text keep the same save, re-render, selection, active-layer, and active-net behavior as human GUI interaction. Responses include dry-run targeting metadata, interaction mode before and after, focus state, and board object counts for agent verification.

Sprint 179 addendum: `ReviewWindow::runAgentUiQueryJson()` now exposes higher-level agent PCB workflow methods on top of Sprint 178 viewport input. `ui.current_tool` and `ui.cancel_tool` expose the current interaction mode and Escape cancellation path. `ui.place_via`, `ui.route_track`, `ui.add_zone`, `ui.add_keepout`, `ui.draw_graphic`, and `ui.place_text` are thin wrappers that call `triggerSafeUiActionJson()` for the matching toolbar action, then delegate to `uiCanvasClickJson()` or `uiCanvasDragJson()`. `ui.delete_object` selects the target with `uiSelectCanvasObjectJson()` and then invokes `action:delete_cursor`. These methods return nested activation/gesture/deletion evidence plus top-level object counts, and they must keep using the existing GUI event path instead of hidden direct board mutations.

Sprint 180 addendum: `ReviewWindow::runAgentUiQueryJson()` now exposes agent project evidence methods without adding a second project state model. `uiScreenshotJson(path, dry_run)` captures the visible `ReviewWindow` with `QWidget::grab()`, reports path, format, dimensions, and device pixel ratio, and only writes a PNG when dry-run is false. `projectContextJson()` and `projectObjectCountsJson()` serialize `project_cache_`, active PCB layer/net state, interaction mode, current path, and `ui_epoch`. `projectReviewJson()` wraps `ccad_core::buildReview(project_cache_)`, while `projectErcJson()`, `projectDrcJson()`, and `projectDiagnosticsJson()` wrap `ccad_core::runErc()` and `ccad_core::runDrc()`. Keep these methods read-only except for the explicit screenshot file write, and keep them in the shared dispatcher so the Agent panel and `UiMapServer` use one protocol.

Sprint 181 addendum: `ReviewWindow` now separates `buildUiMapJson()` from public `uiMapJson()` so internal compact/find/hit-test queries can inspect current Qt state without clearing dirty tracking. Public full-map reads acknowledge the current epoch and clear the dirty queue. `markUiMapChanged(dirty_ids, dirty_roles)` increments `ui_map_epoch_` and records semantic node IDs plus broad roles when callers know the changed surface; empty calls mark a conservative full compact fallback. `uiMapDeltaJson(since_epoch)` returns `dirty_node_count`, `changed_roles`, `dirty_ids`, `full_snapshot`, and compact nodes instead of blindly returning every node for every stale epoch. `uiWaitForDeltaJson(since_epoch, timeout_ms)` pumps Qt events for at most 5000 ms and returns the same delta contract through the Agent panel and `UiMapServer` as `ui.wait_for_delta`. `uiNearestCanvasObjectJson()` now derives an in-call uniform-grid candidate filter from rendered selectable `QGraphicsItem` bounds and reports `index_kind`, `indexed_object_count`, and `scanned_candidate_count`; this is still a render-derived targeting helper, not a second board model.

Sprint 182 addendum: `ReviewWindow` now keeps `ui_map_dirty_history_`, a bounded vector of dirty UI-map events with `from_epoch`, `ui_epoch`, dirty IDs, dirty roles, and full-snapshot flags. `uiMapDeltaJson()` aggregates retained events newer than `since_epoch` and reports `dirty_event_count`, while preserving full compact fallback when history is too old or a full render dirtied the map. `uiWatchDeltaJson(since_epoch, timeout_ms, max_events)` returns a bounded array of dirty events after briefly pumping Qt events, and is exposed as `ui.watch_delta`. `rebuildUiMapIndexCache()` derives an epoch-keyed cache from `buildUiMapJson()` with `by_id` and `by_role` hash indexes over compact nodes. `uiIndexStatsJson()`, `uiGetNodeJson(id)`, and `uiNodesByRoleJson(role, limit)` expose that cache through `ui.index_stats`, `ui.get_node`, and `ui.nodes_by_role` in the same `runAgentUiQueryJson()` dispatcher used by the Agent panel and `UiMapServer`. Cache invalidation is tied to `markUiMapChanged()`, not to screenshots or a second GUI state model.

## Sprint 157 Read-only UI Map Addendum

Sprint 157 adds the first native GUI semantic map for future LLM and automation harnesses. `ReviewWindow::uiMapJson()` exports a snapshot JSON document with `schema_version`, `ui_epoch`, and `nodes`. Nodes include stable action IDs such as `action:add_footprint`, tabs such as `tab:pcb`, canvases such as `canvas:pcb`, and rendered CAD objects such as `canvas_object:JAC1.1`. Canvas-object rows preserve object type, net ID, layer ID, route-request provenance, scene-space bounds, screen-space bounds, visibility, interactivity, and target coordinates.

`ccad_gui --dump-ui-map <project.ccad.json> <out.json>` owns the script-facing dump path. It loads a project, shows the window, lets Qt settle briefly, writes the same JSON map, and exits. This is read-only and must not mutate the project.

`ccad_gui --validate-ui-map-targets <project.ccad.json> <out.json>` owns the live targeting proof path. It moves the cursor to each visible/enabled exported target and verifies the target through Qt widget, tab, canvas, or `QGraphicsScene` hit-testing. Hidden or disabled nodes are counted as skipped. Use this before claiming the map is useful for automation.

Sprint 158 adds selective target queries. `ReviewWindow::uiTargetJsonById(id)` resolves semantic IDs from the UI map to one target response. `ReviewWindow::uiTargetJsonForBoardPoint(x_mm, y_mm)` maps a PCB board-space millimeter point through the current board origin, canvas scene scale, `QGraphicsView::mapFromScene()`, and viewport global coordinates. The executable modes are `ccad_gui --ui-target-id <project> <id> <out.json>` and `ccad_gui --ui-target-board-point <project> <x-mm> <y-mm> <out.json>`. Target JSON must include logical Qt coordinates, physical pixel coordinates, and device pixel ratio so external mouse tools do not guess high-DPI scaling.

Sprint 159 adds safe direct action triggering. `ReviewWindow::triggerSafeUiActionJson(id)` executes only non-destructive view/navigation actions and tab switches. The executable mode is `ccad_gui --ui-trigger-safe <project> <id> <out.json>`. Keep mutating, file-writing, dialog-opening, and placement actions refused until they have explicit kernel/transaction equivalents or human approval gates.

The implementation uses current Qt Widgets APIs: `QWidget::mapToGlobal()` for screen-space widget regions, `QTabBar::tabRect()` for tab targets, and `QGraphicsItem::sceneBoundingRect()` plus renderer-attached data roles for CAD canvas objects. Future high-performance work should be event-driven: use QApplication or widget event filters, dirty-node batching, high-DPI device-pixel-ratio fields, clipping-aware visible rectangles, z-order ordering, and a spatial index such as an R-tree or quadtree if hit-testing grows beyond simple linear scans.

`ReviewWindow::~ReviewWindow()` disconnects canvas scene signals and clears selection before child widgets are destroyed. This prevents selection-change callbacks from touching the selection inspector during teardown, which the Sprint 157 GUI map test exposed.

Sprint 160 fixes the GUI placement commit path. When `ReviewWindow::eventFilter()` commits footprint, symbol, or move interactions, it must write `project_cache_`, close and validate the stream, cancel the interaction mode, and only then call `reloadProject()`. Do not call `reloadProject()` while the project file stream is still open; on Windows this can reload an empty or partially flushed file and block automation behind a parse-error `QMessageBox`.

`ReviewWindow::commitFootprintPlacementForAutomation()` is an app-owned test hook that loads a footprint, enters the same placement state used by the GUI, sends a left-click event through the `QGraphicsView` viewport, and returns compact JSON. It is exposed by `ccad_gui --test-place-footprint-click <project> <footprint> <x-mm> <y-mm> <out.json>` for regression and harness work only.

KiCad SVG icon lookup in `review_window.cpp` now checks `CCAD_KICAD_SRC`, `F:\kicad_src`, current-working-directory candidates, and executable-directory candidates. `CMakeLists.txt` links `Qt6::Svg` for the GUI and review-window tests; CI installs `qt6-svg-dev` for the Linux GUI lane.

Sprint 161 extends the UI map and harness. `ReviewWindow::uiMapJson()` now includes menu nodes and panel nodes in addition to actions, tabs, canvases, and canvas objects. `ReviewWindow::uiTargetJsonById()` resolves those menu and panel IDs. `ccad_gui --test-ui-map-target-sequence <project> <out-dir> <name>` is an app-owned automation mode that moves the real cursor to semantic targets, waits, captures marked screenshots, resizes the window, repeats, and writes a JSON report. `scripts/run_ui_map_mouse_target_demo.ps1` wraps that mode with the docs beep, two-second handoff, stdout/stderr capture, and artifact reporting.

## Sprint 173 PCB Graphics/Text Addendum

Sprint 173 promotes the PCB Draw Graphic and Place Text toolbar actions from planned-tool contracts to durable board primitives. `src/ccad_core/model.hpp` now owns `BoardGraphic` and `BoardText` collections on `Board`; `src/ccad_core/serialize.cpp`, `src/ccad_core/canvas.cpp`, `src/ccad_core/drc.cpp`, `src/ccad_core/diff.cpp`, and `src/ccad_core/kicad_pcb_export.cpp` carry those objects through JSON round-trip, canvas exposure, validation, diffing, and KiCad `gr_line`/`gr_text` export. Keep future graphic shapes and text-property expansion in the kernel first, then expose them through CLI and GUI clients.

`src/ccad_cli/pcb_commands.cpp` owns `pcb add-graphic-line` and `pcb add-text`, while `src/ccad_cli/pcb_object_queries.cpp` owns compact `get-object`, `list-objects --type graphic`, `list-objects --type text`, route-job summary counts, removal support, and layer-use guards for the new objects. These commands are the agent-native mutation path and must stay deterministic.

`src/ccad_gui/review_window.hpp/.cpp` now owns `DrawGraphic` and `PlaceText` interaction modes plus `commitGraphicLinePlacementForAutomation()` and `commitBoardTextPlacementForAutomation()`. Human clicks and app-owned automation both go through the Qt viewport event path before saving the model and re-rendering the core-derived canvas. `src/ccad_gui/board_canvas_renderer.cpp`, `src/ccad_gui/object_browser_panel.cpp`, and `src/ccad_gui/selection_inspector_panel.cpp` render and inspect graphic/text objects. The default graphic layer prefers visible `Dwgs.User`; the default text layer prefers visible `F.SilkS`; both avoid hidden default layers when a visible compatible layer exists.

`scripts/run_sprint_demo.ps1` now uses the 7-second single-preview wait policy and adds visible `Dwgs.User` graphic lines plus `F.SilkS` board text to the bridge-rectifier demo before screenshot capture. The Sprint 173 visual proof is `artifacts\screenshots\sprint173-pcb-graphics-text-tools-final-20260603-021600.png`.

## Sprint 174 PCB Zone Tool Addendum

Sprint 174 promotes the PCB Add Zone toolbar action from planned-tool contract to durable first-slice copper-zone authoring. `src/ccad_core/model.hpp` now owns `BoardZone` on `Board`; `src/ccad_core/serialize.cpp`, `src/ccad_core/canvas.cpp`, `src/ccad_core/drc.cpp`, `src/ccad_core/diff.cpp`, and `src/ccad_core/kicad_pcb_export.cpp` carry zones through JSON round-trip, canvas exposure, validation, diffing, and KiCad `zone`/`polygon`/`filled_polygon` export. This is a deterministic preview-fill slice, not KiCad's full refill solver, thermal relief, island removal, cutout, or zone-manager implementation.

`src/ccad_cli/pcb_commands.cpp` owns `pcb add-zone`, rectangular zone construction, point-list zone construction, board/net/layer guards, and zone removal through `remove-object`. `src/ccad_cli/pcb_object_queries.cpp` owns compact zone `get-object`, `list-objects --type zone`, and route-job summary/export fields so agents can see zone obstacles without scraping full project JSON. Layer-use guards now treat zones as real layer references.

`src/ccad_gui/review_window.hpp/.cpp` now owns `AddZone` interaction mode plus `commitZonePlacementForAutomation()`. Human clicks and app-owned automation both go through the Qt viewport event path before saving the model and re-rendering the core-derived canvas. `src/ccad_gui/board_canvas_renderer.cpp`, `src/ccad_gui/object_browser_panel.cpp`, and `src/ccad_gui/selection_inspector_panel.cpp` render and inspect zones. The official sprint demo adds a visible `B.Cu` bridge-rectifier zone, and the Sprint 174 visual proof is `artifacts\screenshots\sprint174-pcb-zone-tool-final-20260603-031321.png`.

## Sprint 183 Agent Protocol Catalog Addendum

`src/ccad_gui/review_window.cpp` now owns the first static Agent-panel/live-socket protocol catalog. The shared dispatcher `ReviewWindow::runAgentUiQueryJson()` supports `agent.methods`, `agent.method_schema`, and `agent.quickstart` in addition to the existing `ui.*` and `project.*` methods. The catalog entries are JSON objects with method name, category, title, description, read-only and mutation flags, project requirement flag, dry-run support, `inputSchema`, output summary, and example payloads.

Keep the catalog synchronized with every future method added to `runAgentUiQueryJson()`. `agent.methods` is the current LLM-native discovery surface for the GUI protocol, while `agent.method_schema` is the one-method lookup path used by agents that already know a method name. `agent.quickstart` is the compact operational guide for the live UI-map loop: discover, inspect project context, confirm indexes, target by ID or role, act with dry-run where possible, observe with `ui.watch_delta`, and use `ui.screenshot` for visual proof.

## Sprint 184 Library Placement Stability Addendum

`src/ccad_core/model.hpp` now lets `Component` carry an optional placed `ccad::Symbol` snapshot. `src/ccad_core/placement.cpp` stores that snapshot in `placeComponent()`, and `src/ccad_core/serialize.cpp` reads and writes the nested `component.symbol` JSON object. Future component-field changes must keep old projects without `symbol` compatible while preserving deterministic output for projects that do have snapshots.

`src/ccad_core/canvas.cpp` now builds schematic scenes from stored symbol primitives. `buildSchematicScene()` transforms symbol graphics by component position and rotation, and `buildCanvasScene(const Symbol&)` emits pin lead lines plus rectangle polygons. `CanvasComponent::has_symbol_graphics` tells `src/ccad_gui/board_canvas_renderer.cpp` to skip the old generic placeholder rectangle when real symbol geometry exists. The renderer has separate `symbol` and `symbol_pin` palette entries, so schematic primitives are visually distinct from PCB copper layers.

`src/ccad_cli/sch_commands.hpp/.cpp` owns the new schematic command group. The first command is `ccad sch place-symbol --file <project> --symbol <symbol.json> --component <id> --at-x-mm <x> --at-y-mm <y> [--rotation-deg <deg>]`. It loads one converted symbol JSON with local inheritance resolution, rejects duplicate component IDs and empty symbols, writes the project file through the normal CLI write path, and exists for agent-native schematic authoring before full GUI schematic editing is complete.

`src/ccad_gui/library_browser_dialog.hpp/.cpp` now returns `LibrarySelection`, which includes source path, library name, item name, file name, and source kind. `result()` still returns the legacy path for existing callers. `SymbolPlacementDialog` and `FootprintPlacementDialog` carry that selection metadata in their placement results; symbol placement uses the selected item name when loading a KiCad `.kicad_sym`, so future multi-symbol rows can disambiguate by item name instead of by file path only.

`src/ccad_gui/main.cpp` owns `--screenshot-project <project> <out.png>`, a clean loaded-project screenshot mode that does not enter the measurement overlay. `src/ccad_gui/review_window.cpp` now switches the editor tab to Schematic for schematic-only projects with placed components or wires. This keeps GUI review tied to the project data shape rather than always showing an empty PCB view first.

## Sprint 185 KiCad Symbol Catalog Scale Addendum

`src/ccad_core/kicad_symbol_import.hpp/.cpp` now owns `KiCadSymbolLibraryItem` and `listKiCadSymbolLibraryItems()`. This API parses a raw KiCad `.kicad_sym` library enough to list top-level symbols and `extends` metadata without constructing full `ccad::Symbol` geometry for every item. Nested unit/body `symbol` nodes remain inside their parent and must not become chooser rows.

`src/ccad_gui/library_browser_dialog.cpp` now uses the listing API for raw `.kicad_sym` files. It creates one `QTreeWidgetItem` per top-level symbol, stores the source file path plus selected item name in `LibrarySelection`, carries `extends`, and passes the item name to lazy preview loading. Keep future cache indexing compatible with this row identity: path alone is not enough for a symbol library that contains more than one symbol.

## Sprint 186 Agent Harness Foundation Addendum

`src/ccad_gui/review_window.hpp/.cpp` now owns a right-side `agent_dock_` for the existing `AgentPanel`. The panel is no longer a page in `bottom_tabs_`; Diagnostics and Transactions remain the only bottom tabs. `panel:agent` remains the stable panel ID, and `tab:agent` is now a semantic dock alias. Keep this alias stable because target harnesses, Agent-panel live actions, and external scripts already use it. The UI map and target query include `dock_area:"right"` for the Agent pane, and `triggerSafeUiActionJson("tab:agent")` shows and raises the dock while preserving the `tab_selected` response reason.

`ReviewWindow::~ReviewWindow()` disconnects `agent_dock_` before child teardown. Do not remove that guard: the dock can emit `visibilityChanged` while Qt destroys children, and the connected lambda calls `markUiMapChanged()` plus `updateAgentPanelContext()`.

`ReviewWindow::runAgentUiQueryJson()` now supports read-only harness metadata methods: `agent.harness_context`, `agent.run_profile`, `agent.safety_policy`, `agent.provider_policy`, `agent.observability_config`, `agent.evidence_manifest_schema`, and `agent.tool_guide`. The static catalog in `agentMethodCatalogArray()` must stay synchronized with every future `agent.*`, `ui.*`, and `project.*` dispatcher branch. These methods define metadata only; they must not call remote models, export telemetry, or store secrets.

`src/ccad_cli/agent_commands.cpp` now exposes direct headless metadata commands and matching JSON-RPC routes through `ccad agent serve`. Keep direct commands and serve methods aligned when adding future harness fields. `src/ccad_cli/agent_policy.hpp/.cpp` owns provider-free command policy classification, dry-run decisions, approval-required metadata, and shared `agent serve` read/write gate decisions. `src/ccad_cli/agent_session.hpp/.cpp` owns provider-free local Agent session files, checkpoint append, canonical session readback, and replay manifest JSON. `src/ccad_cli/app.cpp` includes help metadata for `agent methods`, `agent harness-context`, `agent tool-guide`, `agent state`, `agent tasks`, `agent evidence`, `agent approvals`, `agent session-schema`, `agent session-new`, `agent session-state`, `agent checkpoint-add`, `agent replay`, `agent policy-schema`, `agent policy-check`, and `agent dry-run`.

## Sprint 187 Agent Panel Workspace Addendum

`src/ccad_gui/agent_panel.hpp/.cpp` now owns a compact Agent workspace UI: project, UI-map epoch, status, workspace context, diagnostic summary, result state, safe-action input, live method/payload inputs, raw JSON output, and preset buttons for `agent.harness_context`, `project.diagnostics`, `agent.tool_guide`, and output clearing. The panel still does not own project state or provider state; all project and UI state comes through callbacks or `setWorkspaceContext()`.

`ReviewWindow::renderReview()` caches the latest diagnostic error and warning counts in `last_diagnostic_error_count_` and `last_diagnostic_warning_count_`. `ReviewWindow::updateAgentPanelContext()` passes those cached counts plus active tab, active PCB layer, active PCB net, interaction mode, project label, and UI-map epoch into the Agent panel. Keep this path cheap because it runs from `markUiMapChanged()`; expensive ERC/DRC work belongs in explicit `project.diagnostics` or review-generation calls.

The new Agent panel buttons use semantic object names `action:agent_preset_harness_context`, `action:agent_preset_project_diagnostics`, `action:agent_preset_tool_guide`, and `action:agent_clear_output`. They are ordinary `QPushButton` widgets so the existing UI-map exporter and generic `ui.click` path can discover and trigger them without a second automation path.

`ReviewWindow::validateUiMapTargetsJson()` treats `tab:agent` as a dock-title rectangle target rather than requiring `QApplication::widgetAt()` to find a child widget. Keep that behavior because the Qt dock title can be native chrome and may legitimately return no widget even when the semantic target is inside the dock-title rectangle.

## Sprint 188 Agent Task Workspace Addendum

`src/ccad_gui/agent_panel.hpp/.cpp` now adds the first local task state to the right-side Agent dock. The stable UI-map IDs are `control:agent_goal`, `action:agent_stage_goal`, `action:agent_pin_evidence`, and `action:agent_clear_evidence`. `AgentPanel::stageGoal()` copies the trimmed goal input into a visible task-state label, `pinEvidence()` stores a bounded in-memory evidence summary derived from the current output/result context, and `clearEvidence()` resets the queue. The queue is intentionally local-only and capped at eight entries; do not add provider calls, project-file secrets, or remote telemetry to this widget.

`AgentPanel::workspaceStateJson()` is the canonical GUI snapshot for this first task workspace. It includes schema version, workspace kind, staged goal, task state, evidence count, evidence entries, project, UI epoch, workspace context, diagnostic summary, status, result state, action ID, live method, and live payload. `ReviewWindow::agentWorkspaceStateJson()` exposes that snapshot as `agent.workspace_state` through the same dispatcher used by the Agent panel and the live UI-map socket, and `agentMethodCatalogArray()` must keep advertising it with every future catalog update.

`ReviewWindow::uiTypeTextJson()` now allows text entry into `control:agent_goal`, and `triggerAgentButtonClickJson()` reaches the new task/evidence buttons through ordinary Qt object names. `src/ccad_gui/main.cpp` includes the task controls in the app-owned target-sequence harness so visual validation proves their semantic coordinates before and after resize.

## Sprint 189 Agent Approval Workspace Addendum

`src/ccad_gui/agent_panel.hpp/.cpp` now owns one local approval lane in the right-side Agent dock. The stable UI-map IDs are `control:agent_approval_request`, `action:agent_request_approval`, `action:agent_approve_next`, `action:agent_decline_next`, `action:agent_cancel_approval`, and `action:agent_clear_approvals`. `requestApproval()` creates one pending local approval from the trimmed request text, while accept, decline, cancel, and clear resolve or reset that local state without mutating the project or calling a provider.

`AgentPanel::workspaceStateJson()` now includes approval fields alongside the Sprint 188 task/evidence fields: pending approval count, pending request, current approval input, approval status label, and last decision. The decision values are `none`, `pending`, `accept`, `decline`, and `cancel`. Keep these values stable because they are the first bridge between a future durable human-in-the-loop runner and the native GUI workspace.

`ReviewWindow::uiTypeTextJson()` now allows `control:agent_approval_request`, and the existing named-button click path triggers the approval controls directly. `src/ccad_gui/main.cpp` includes the approval controls in target-sequence validation. `ReviewWindow` also caps `SelectionInspectorPanel` height in the right-side Layers / Objects dock so the Agent dock has visible room for task and approval controls while keeping the Layers / Objects panel available.

## Sprint 190 Agent Harness Backlog Map Addendum

Sprint 190 is a documentation and planning sprint that turns `docs/req_agentHarness.md` from a raw requirements pile into the structured backlog map in `docs/devops/backlog.md`. No GUI or kernel behavior changed in this sprint. The key decision is that the existing Agent dock is only a functional skeleton: Sprints 186 through 189 added a right-side dock, protocol metadata, task state, pinned evidence, and local approvals, but the real product target is a full-height vertical agent workspace with session controls, goal input, task checklist, reasoning/tool stream, evidence cards, approval cards, trace links, CLI parity, durable sessions, BYOK/BYOT configuration, and EDA-specific tools.

Future agent-harness work should consume the backlog sections named `Current Agent UI Gap`, `Agent Workspace UI and CLI Roadmap`, `Prompt, Context, Memory, and Runbooks`, `Durable Orchestration and State`, `Human Approval, Policy, and Safety`, `Tool System, MCP, Connectors, and Native Actions`, `Observability, BYOT, and Run History`, `GUI Map, Coordinate Streaming, and Agent-Control Geometry`, `EDA-Specific Harness and KiCad/Altium Parity`, `Library, Datasheet, BOM, Simulation, and Manufacturing Agents`, `Evaluation, CI, and Regression Harness`, and `Agent Harness Sprint Sequence`. Sprint 191 should start with the vertical Agent pane redesign and preserve all stable semantic IDs already used by the UI-map harness.

## Sprint 191 Agent Panel Vertical Workspace Addendum

`src/ccad_gui/review_window.cpp` now splits the right dock area horizontally between Layers / Objects and Agent. This is a tested invariant: `tests/test_gui_ui_map.cpp` parses the UI map and requires `panel:agent` to sit to the right of `panel:layers_objects` with comparable height, so future changes should not return the Agent pane to a stacked lower dock. `panel:agent` and `tab:agent` remain stable, and `triggerSafeUiActionJson("tab:agent")` still raises the dock.

`src/ccad_gui/agent_panel.hpp/.cpp` now owns the first vertical workspace shell. It keeps all earlier live-query, task, evidence, and approval IDs, and adds `control:agent_command_input`, `action:agent_submit_command`, `action:agent_footer_request_context`, and `action:agent_footer_trigger_drc`. The footer context action calls `runHarnessContextPreset()`, and the footer DRC action calls `runDiagnosticsPreset()`. `submitCommand()` stages a local command and writes a compact `agent_command_staged` JSON event to the stream; it does not call providers or mutate the project.

`AgentPanel::workspaceStateJson()` now includes `panel_layout`, session labels, command input, and staged command text. Keep `panel_layout:"vertical_agent_workspace"` stable for future CLI/session parity work. `ReviewWindow::uiTypeTextJson()` allows text into the new command input, and `src/ccad_gui/main.cpp` includes the command and footer actions in app-owned target-sequence validation. The visible approval Accept/Decline row is part of the Sprint 191 visual contract because target validation proved `action:agent_approve_next` lands on the visible button, not a clipped off-screen widget.

## Sprint 192 Agent CLI Workspace State Addendum

`src/ccad_cli/agent_commands.cpp` now owns the first headless Agent workspace-state parity contract. Direct commands `ccad agent state`, `ccad agent tasks`, `ccad agent evidence`, and `ccad agent approvals` emit deterministic JSON without launching Qt, calling providers, exporting telemetry, or reading GUI-only in-memory state. The matching JSON-RPC routes are `agent.state`, `agent.workspace_state`, `agent.tasks`, `agent.evidence`, and `agent.approvals`.

The CLI state documents intentionally report a headless empty workspace with `durable_store:"not_configured"`. Do not remove that field or pretend the CLI can see the live GUI Agent dock until a durable session/checkpoint store exists. Sprint 193 should introduce the shared local session schema, checkpoint metadata, and replay identifiers that allow GUI and CLI state to converge without hidden process coupling.

The `agent.methods`, `agent.quickstart`, `agent.harness_context`, and `agent.tool_guide` command outputs must stay synchronized with every new `agent.*` state route. `tests/test_agent_serve.cpp` is the current contract test for direct CLI output, method catalog coverage, tool-guide discovery, and JSON-RPC route parity.

## Sprint 193 Agent Session Checkpoints Addendum

`src/ccad_cli/agent_session.hpp/.cpp` now owns the local Agent session checkpoint file contract. The file is deterministic JSON with `session_kind:"ccad_agent_session"`, `session_id`, `thread_id`, title, optional project path, timestamps, `durability:"local_json_checkpoint_file"`, provider and trace flags set false, local `ccad-agent-session:<session_id>` resource URI, checkpoint count, and ordered checkpoint objects.

`ccad agent session-new` creates a local session file. `ccad agent session-state` reads and canonicalizes it. `ccad agent checkpoint-add` appends one checkpoint with a stable sequence and `ccad-agent-checkpoint:<session_id>/<checkpoint_id>` resource URI. `ccad agent replay` emits the replay manifest used by future resumable runners. The read-only JSON-RPC routes are `agent.session_schema`, `agent.session_state`, and `agent.replay_manifest`.

Do not store API keys, provider tokens, prompt bodies, screenshots, or design-file contents automatically in this session file. The current file is metadata and artifact references only. Future GUI binding should load this file explicitly rather than relying on hidden process-local Agent panel state.

## Sprint 194 Agent Policy Gates Addendum

`src/ccad_cli/agent_policy.hpp/.cpp` owns the first Agent command policy classifier. It must remain argv-only: do not open project files, parse design data, call providers, shell out, or execute commands from this module. The classifier returns structured risk metadata, including read/write permission requirements, project/file mutation flags, dry-run state, approval-required state, approval reason, risk level, and final decision. Unknown command families are intentionally conservative and require write permission plus approval.

`src/ccad_cli/agent_commands.cpp` routes direct commands `agent policy-schema`, `agent policy-check`, and `agent dry-run` through that module and exposes JSON-RPC routes `agent.policy_schema` and `agent.policy_check`. The same classifier is now the central gate for `execute` and MCP `tools/call`; do not reintroduce local ad hoc `is_write` checks in `agent_commands.cpp`.

## Sprint 195 Agent Evidence Manifests Addendum

`src/ccad_gui/agent_panel.hpp/.cpp` owns the first structured GUI evidence-card model. `AgentPanel::pinEvidence()` parses the current Agent output, classifies producer methods such as `ui.screenshot`, `project.drc`, `project.erc`, `project.diagnostics`, and `agent.tool_guide`, then stores bounded local cards with stable card IDs, kind, title, summary, method, artifact path, diagnostic counts, image dimensions, trace-ready IDs, timestamp, and source. It must keep cards lightweight: do not inline screenshots, reports, prompt bodies, provider outputs, API keys, or full project JSON. The card widgets use stable object names such as `card:agent_evidence_1` so the UI map can target them.

`AgentPanel::workspaceStateJson()` now emits both the legacy `evidence` summary array and the structured `evidence_cards` array. `src/ccad_gui/review_window.cpp` and `src/ccad_cli/agent_commands.cpp` both document the same evidence-manifest fields through `agent.evidence_manifest_schema`; keep those schema surfaces synchronized when adding future evidence kinds, trace fields, or durable-session bindings.

`src/ccad_gui/main.cpp` target-sequence validation now clicks the Agent `Trigger DRC` and `Pin` controls through the existing `ui.click` dispatcher before taking the pin-target screenshot. This keeps visual validation tied to semantic GUI actions and proves the evidence card can be produced without provider calls or a separate automation-only path.

`agent.methods`, `agent.quickstart`, `agent.harness_context`, `agent.tool-guide`, and `src/ccad_cli/app.cpp` help metadata must stay synchronized with policy methods. Future GUI runner, durable-session, provider, and observability work should consume this policy object before executing mutating tools instead of guessing permission requirements from method names.

## Sprint 196 Agent Workspace Command Center Addendum

`src/ccad_gui/agent_panel.hpp/.cpp` now owns a bounded local `ActivityEvent` model in addition to task, evidence, and approval state. The visible Activity stream is rendered with stable card IDs such as `card:agent_activity_1`, and `AgentPanel::workspaceStateJson()` serializes `visual_style:"command_center_dark"`, `workspace_layout_version:2`, `visible_sections`, `permission_label`, `activity_event_count`, and the `activity_events` array. These fields describe local GUI state only; do not treat the model/mode/policy chips as provider execution or secret storage.

The Agent pane session strip now exposes functional icon `QPushButton` controls named `action:agent_header_request_context`, `action:agent_header_trigger_drc`, and `action:agent_header_clear_output`. They call the same local harness-context, diagnostics, and clear-output functions as the footer/preset controls. Keep them as real buttons, not painted decorations, so `ui.click` can operate them.

`src/ccad_gui/review_window.cpp` now exports named passive Agent widgets whose object names start with `panel:`, `tab:`, `label:`, or `card:` as UI-map nodes and single-target responses. This is intentionally generic but currently used by Agent command-center regions such as `panel:agent_session_strip`, `panel:agent_mode_strip`, `panel:agent_activity_stream`, `tab:agent_command`, `tab:agent_evidence`, `tab:agent_approvals`, and `label:agent_permission_chip`. Passive widget target rectangles are clipped against containing `QScrollArea` viewports when possible so mouse markers land on the visible part of the panel.

`src/ccad_gui/main.cpp` target-sequence validation now includes the new Agent command-center regions and header actions before the existing command, task, evidence, and approval controls. Future Agent UI additions should add semantic IDs and target-sequence coverage in the same sprint as the UI work.

## Sprint 197 Agent BYOK Configuration Addendum

`src/ccad_cli/agent_provider_config.hpp/.cpp` owns the headless no-secret provider configuration contract. It returns three JSON surfaces: `agentProviderConfigSchemaJson()`, `agentProviderConfigTemplateJson()`, and `agentProviderStatusJson()`. The schema lists supported provider families and accepted environment-variable names, the template is disabled by default and stores env-var names only, and status checks only whether environment variables are present. It must never emit actual secret values, call network APIs, read project files, automate consumer web sessions, or enable provider execution.

`src/ccad_cli/agent_commands.cpp` remains the dispatcher and metadata catalog. It exposes direct CLI commands `provider-config-schema`, `provider-config-template`, and `provider-status`, and JSON-RPC routes `agent.provider_config_schema`, `agent.provider_config_template`, and `agent.provider_status`. Keep `agent.methods`, `agent.quickstart`, `agent.harness_context`, `agent.tool-guide`, and `src/ccad_cli/app.cpp` help metadata synchronized when adding future provider surfaces.

The first supported provider families are `openai`, `openai_compatible`, `anthropic`, `google_gemini`, and `local_model_server`. `google_gemini` carries the current key-restriction warning and 2026-06-19 unrestricted-key cutoff from Google documentation. Sprint 197 is headless metadata only; Sprint 205 adds GUI provider-readiness controls and UI-map targeting. Provider execution, routing, and secret-value handling still require a later runner sprint with explicit approvals and audit rules.

## Sprint 198 Agent Observability Configuration Addendum

`src/ccad_cli/agent_observability_config.hpp/.cpp` owns the headless trace-export configuration contract. It returns five JSON surfaces: `agentObservabilityConfigJson()`, `agentTraceExportSchemaJson()`, `agentTraceExportTemplateJson()`, `agentTraceRedactionPolicyJson()`, and `agentTraceExportDryRunJson()`. The schema tracks OpenTelemetry GenAI and Langfuse-ready metadata, the template is disabled by default, the redaction policy refuses prompt/tool/screenshot/design-file export by default, and the dry run reports only env-var presence with `headers_value:"redacted"`. It must never emit OTLP header values, call network APIs, export telemetry, read project files, or enable tracing without an explicit future runner.

`src/ccad_cli/agent_commands.cpp` exposes direct CLI commands `trace-export-schema`, `trace-export-template`, `trace-redaction-policy`, and `trace-export-dry-run`, plus JSON-RPC routes `agent.trace_export_schema`, `agent.trace_export_template`, `agent.trace_redaction_policy`, and `agent.trace_export_dry_run`. Keep `agent.methods`, `agent.quickstart`, `agent.harness_context`, `agent.tool-guide`, `agent.observability_config`, and `src/ccad_cli/app.cpp` help metadata synchronized when adding future observability surfaces.

This is a configuration and redaction sprint, not a tracer. GUI trace links, durable run IDs, real OpenTelemetry SDK integration, Langfuse export, and model/tool span emission belong in later sprints and must start with fresh references plus tests.

## Sprint 199 Agent Panel UI Polish Addendum

`src/ccad_gui/agent_panel.hpp/.cpp` now owns local run-control state for the native Agent dock. The stable UI-map IDs are `panel:agent_trace_strip`, `label:agent_trace_chip`, `label:agent_session_chip`, `panel:agent_run_controls`, `label:agent_run_state_chip`, `action:agent_pause_run`, `action:agent_resume_run`, `action:agent_stop_run`, `panel:agent_active_plan`, and `panel:agent_plan_row_1` through `panel:agent_plan_row_3`. The run buttons only update local GUI state and append Activity events; they must not start provider execution, export telemetry, mutate projects, or pretend a durable runner exists.

`AgentPanel::workspaceStateJson()` now emits `visual_style:"agent_command_center_dense"` and `workspace_layout_version:3` along with `run_state`, `run_label`, trace/session labels, and bounded `plan_items`. Keep the version and field names stable for future CLI/session parity work. If a later sprint binds these controls to a real runner, it should preserve the current local state fields and add explicit durable session, policy, trace, and provider metadata instead of replacing the existing contract.

`src/ccad_gui/main.cpp` target-sequence validation includes the new Agent trace, run, and plan IDs before and after resize. `ReviewWindow` dirty-ID paths include those IDs so showing or raising the Agent dock refreshes the UI-map state. Future Agent UI additions should follow the same rule: add stable semantic IDs, focused `gui_agent_panel`/`gui_ui_map` coverage, and official visual plus target-harness proof in the same sprint.

## Sprint 200 KiCad CLI Evidence Addendum

`src/ccad_cli/agent_kicad_evidence.hpp/.cpp` owns the first headless KiCad evidence contract. It builds structured command arrays and artifact manifests for `kicad-cli` evidence jobs instead of accepting free-form shell strings. The supported first-slice kinds are `pcb-drc`, `sch-erc`, `pcb-export-gerbers`, `pcb-export-drill`, `pcb-export-pos`, `pcb-export-ipc2581`, and `pcb-export-odb`.

`src/ccad_cli/agent_commands.cpp` dispatches direct commands `agent kicad-evidence-schema`, `agent kicad-evidence-plan`, `agent kicad-evidence-dry-run`, and `agent kicad-evidence-run`, and exposes matching JSON-RPC routes `agent.kicad_evidence_schema`, `agent.kicad_evidence_plan`, `agent.kicad_evidence_dry_run`, and `agent.kicad_evidence_run`. Keep `agent.methods`, `agent.quickstart`, `agent.harness_context`, `agent.tool-guide`, and `src/ccad_cli/app.cpp` help metadata synchronized with those route names whenever this surface expands.

The run path is intentionally guarded. Direct `kicad-evidence-run` returns `executed:false` unless the caller supplies `--execute`, and the JSON-RPC route rejects `execute:true` through read-only `agent serve` with `-32604` and `external_process_file_write`. `src/ccad_cli/agent_policy.hpp/.cpp` remains the policy source of truth and still classifies from argv tokens only; do not make the policy module read project files or inspect KiCad inputs.

The first implementation invokes a quoted command line only after explicit execution is requested. Future work can replace that internal launcher with a stronger process-spawn abstraction without changing the public JSON contract, as long as the plan/dry-run/run split and no-network/no-secret guarantees remain stable.

## Sprint 201 Agent Panel Visual Refinement Addendum

`src/ccad_gui/agent_panel.hpp/.cpp` now exposes a fourth visual contract for the native Agent pane. `AgentPanel::workspaceStateJson()` reports `visual_style:"agent_reference_panel_v4"` and `workspace_layout_version:4`, and the `visible_sections` array includes `status_rail`, `command_composer`, `plan_deck`, `evidence_lane`, and `approval_lane`. These fields are presentation and automation metadata only; they do not imply provider execution, telemetry export, or project mutation.

The stable new UI-map IDs are `panel:agent_status_rail`, `panel:agent_command_composer`, `panel:agent_plan_deck`, `panel:agent_evidence_lane`, and `panel:agent_approval_lane`. They are additive. Existing IDs such as `panel:agent_session_strip`, `panel:agent_mode_strip`, `panel:agent_trace_strip`, `panel:agent_run_controls`, `panel:agent_active_plan`, `panel:agent_evidence_tray`, `panel:agent_approval_card`, `control:agent_command_input`, and the run/evidence/approval buttons remain valid for older automation.

Future Agent pane work should stop treating style tweaks as complete product progress unless they also bind visible controls to durable session state, policy decisions, provider configuration, trace IDs, runner queues, or EDA-specific tools. Keep the current semantic IDs stable and add focused `gui_agent_panel` or `gui_ui_map` coverage before changing the pane contract again.

## Sprint 202 Agent Session GUI Binding Addendum

`src/ccad_gui/agent_panel.hpp/.cpp` now owns the first GUI binding to CCad's local `.ccad-agent-session.json` metadata contract. The implementation deliberately stays inside the GUI layer for this sprint and reads or writes only narrow JSON metadata: `session_id`, `thread_id`, ordered checkpoint summaries, latest checkpoint ID, replayability, and the local session path. Future work that needs deeper schema sharing should move the parser into `ccad_core` instead of linking the GUI to `ccad_cli`.

The session strip adds stable semantic IDs `panel:agent_session_binding`, `control:agent_session_path`, `action:agent_load_session`, `action:agent_checkpoint_session`, and `label:agent_session_status`. `AgentPanel::workspaceStateJson()` adds `durable_session_bound`, `session_file_path`, `session_path_input`, `durable_session_id`, `thread_id`, `checkpoint_count`, `latest_checkpoint_id`, `replayable`, and `session_status` while preserving the Sprint 201 visual style and all older command, run, evidence, approval, and plan fields.

`src/ccad_gui/main.cpp` target-sequence validation and `src/ccad_gui/review_window.cpp` dirty-ID refresh paths include the new session controls, so the app-owned target harness can hit them before and after resize. `tests/test_gui_agent_panel.cpp` contains the red/green contract for binding a temp session file and appending `gui-checkpoint-N` through the panel.

This is not a provider runner, not LangGraph execution, and not observability export. It is the durable local session metadata bridge that later policy UI binding, run queues, trace IDs, and provider-backed execution should build on.

## Sprint 203 Agent Policy GUI Binding Addendum

`src/ccad_core/agent_policy.hpp/.cpp` now owns the Agent command policy classifier. It is intentionally argv-only and must not open project files, parse design/library contents, call providers, shell out, or execute commands while classifying. `src/ccad_cli/agent_policy.hpp/.cpp` remains as a compatibility include/source path and aliases the core API into `ccad_cli`, so existing CLI and JSON-RPC call sites keep their namespace while GUI code can include the core classifier directly.

`src/ccad_gui/agent_panel.hpp/.cpp` owns the first GUI policy-preview binding. The stable UI-map IDs are `panel:agent_policy_surface`, `label:agent_policy_decision`, `label:agent_policy_risk`, `control:agent_policy_dry_run`, and `action:agent_policy_preview`. `AgentPanel::workspaceStateJson()` emits policy fields for decision, risk, approval requirement, approval reason, dry-run state, would-execute state, read-only state, project/file mutation flags, normalized command, and normalized args. Previewing or submitting a CCad CLI-shaped command classifies it locally only; it does not execute the command, call providers, export telemetry, write project files, or store secrets.

`src/ccad_gui/review_window.cpp` now treats `QCheckBox` widgets whose object names start with `control:` as normal UI-map controls. The map export includes checked state, target validation checks the widget hit target, `uiTargetJsonById()` resolves it, and `uiClickJson()` toggles it through the real Qt checkbox path and reports the checked state. Keep this generic checkbox support because future Agent/provider/policy controls will likely use checkboxes and toggles.

`tests/test_gui_agent_panel.cpp` covers read, write, approval-required, and dry-run policy state in the Agent pane. `tests/test_gui_ui_map.cpp` covers `control:agent_policy_dry_run` in validator, target lookup, and semantic click. Future policy or run-queue work should extend those tests before changing the public `agent.workspace_state` policy field names.

## Sprint 204 Agent Trace Links GUI Binding Addendum

`src/ccad_gui/agent_panel.hpp/.cpp` now owns local trace-link metadata for the native Agent pane. The implementation is intentionally GUI-local and metadata-only: it creates deterministic local trace IDs and span IDs, links them to the currently bound session ID and thread ID when available, and reports export state as disabled. Do not reinterpret this as an OpenTelemetry exporter, Langfuse integration, provider runner, prompt recorder, secret store, or project mutation path.

The stable UI-map IDs are `panel:agent_trace_links`, `label:agent_trace_id`, `label:agent_span_id`, `label:agent_trace_status`, `label:agent_trace_export_status`, and `action:agent_new_trace_context`. `AgentPanel::workspaceStateJson()` emits `trace_id`, `span_id`, `trace_status`, `trace_export_status`, `trace_backend`, `trace_export_enabled`, `trace_link`, `trace_link_available`, `trace_session_id`, `trace_thread_id`, and `trace_content_policy`. Keep the content policy value conservative unless a later runner/exporter sprint adds explicit redaction, approval, and storage controls.

`src/ccad_gui/main.cpp` includes the trace-link IDs in app-owned target-sequence validation, and `src/ccad_gui/review_window.cpp` includes the same IDs in dirty-node refresh paths. `ReviewWindow::uiTargetJsonById()` now scrolls targetable buttons, line edits, checkboxes, panels, and labels into ancestor `QScrollArea` viewports before returning target coordinates. Preserve that behavior because the Agent pane is taller than the viewport and semantic automation must receive coordinates for visible targets, not stale negative offscreen widget positions.

`ReviewWindow::validateUiMapTargetsJson()` explicitly validates `action:agent_new_trace_context`. For canvas-object validation, it samples multiple points inside the scene bounds and accepts the first point where Qt hit-testing returns the intended object. Keep this sampled hit-probe approach for tracks, graphic lines, and other thin or overlapped CAD items because a bounding-box center can be empty space or covered by another item.

## Sprint 205 Agent Provider Controls GUI Binding Addendum

`src/ccad_gui/agent_panel.hpp/.cpp` now owns visible no-secret provider-readiness controls for the native Agent pane. The stable IDs are `panel:agent_provider_controls`, `control:agent_provider_family`, `control:agent_provider_model`, `action:agent_provider_refresh_status`, `label:agent_provider_status`, `label:agent_provider_env`, and `label:agent_provider_execution_status`. Keep this binding metadata-only: it may report provider family, model hint, environment-variable names, presence-only readiness, and disabled execution status, but it must not call providers, probe networks, automate consumer browser accounts, display secret values, write provider secrets to project files, export telemetry, or mutate the active project.

`AgentPanel::workspaceStateJson()` now includes provider fields alongside policy, session, trace, evidence, approval, and run-state fields. The provider fields are deliberately conservative: `provider_execution_enabled:false`, `provider_secret_value_visible:false`, `provider_network_probe_enabled:false`, `provider_browser_account_automation_enabled:false`, `provider_project_file_secret_storage:false`, `provider_secret_value_policy:"never_emit_secret_values"`, and `provider_readiness_policy:"env_presence_only_no_network_probe"`. Future provider-runner work must add new explicit approval, redaction, and audit contracts instead of relaxing these fields silently.

`src/ccad_gui/review_window.cpp` now treats `QComboBox` controls whose object names start with `control:` as first-class UI-map controls. Map export reports value/current text, target validation scrolls the combo into ancestor scroll areas before hit-testing, `uiTargetJsonById()` resolves combo coordinates, and `uiClickJson()` can focus combo controls. `ui.type_text` now allows `control:agent_provider_model`. Keep this generic combo/text bridge because future runner/provider controls should be driven through semantic IDs rather than guessed screen coordinates.

`src/ccad_gui/main.cpp` includes the provider IDs in the app-owned target sequence. Sprint 205 proof used `artifacts/screenshots/sprint205-agent-provider-controls-gui-binding-final-20260605-150230.png` and `artifacts/screenshots/sprint205-agent-provider-controls-gui-binding-targets-target-sequence.json`, with inspected provider target screenshots before and after resize. Focused `gui_agent_panel` and `gui_ui_map` tests passed, and the full sprint-end Qt build plus CTest gate passed 36 of 36 tests.
- **Sprint 189 (Agent Approval Workspace)** is complete on `sprint-189-agent-approval-workspace`. It adds the first local approval lane to the right-side Agent dock with a targetable approval request input, Request, Accept, Decline, Cancel, Clear, visible approval status, and approval fields in `agent.workspace_state`. Focused `gui_agent_panel` and `gui_ui_map` coverage passed, official visual validation produced `artifacts/screenshots/sprint189-agent-approval-workspace-final-20260604-210030.png`, target validation produced `artifacts/screenshots/sprint189-agent-approval-workspace-targets-target-sequence.json` with every approval target found before and after resize, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 217 (Marketplace & Slash Command UI)** is complete on `sprint-217-marketplace-slash-ui`. It adds a dynamic `/` command autocomplete popup directly over the `chat_input_`, wiring keyboard events (Up/Down/Enter/Escape) to slash-command templates like `/route`. It transitions `AgentMarketplaceDialog` to natively load JSON components from the live HTTP catalog via `QNetworkAccessManager`. It also completes the integration of the right-side Agent panel buttons (Settings, STT, Context, File Attachment), binding them to dispatch JSON-RPC commands like `agent.provider_config_schema` directly to the `orchestrator.py` standard input. Focused integration test scripts manually verified the CLI provider-status and orchestrator JSON-RPC pipe. The full Qt build passed and visual validation was captured for the popup overlay.

`tests/test_gui_agent_panel.cpp` covers initial empty trace state, visible trace widgets, icon-backed trace action styling, deterministic local trace context creation, session/thread correlation, and metadata-only export status. `tests/test_gui_ui_map.cpp` covers UI-map exposure, target lookup, validation, and semantic `ui.click` trace action execution.

## Sprint 206 Agent Run Queue Foundation Addendum

`src/ccad_gui/agent_panel.hpp/.cpp` now owns the first local run-queue foundation for the native Agent pane. The stable IDs are `panel:agent_run_queue`, `label:agent_run_queue_status`, `label:agent_run_queue_counts`, `label:agent_run_queue_current_step`, `action:agent_cancel_run_queue`, and `action:agent_clear_run_queue`. The queue is intentionally inserted near the top of the Agent pane so the official screenshot shows it in the first viewport.

`AgentPanel::workspaceStateJson()` now includes queue fields alongside run state, session, policy, trace, provider, evidence, and approval fields. The queue fields include `run_queue_available`, `run_queue_id`, `run_queue_thread_id`, `run_queue_session_id`, `run_queue_status`, `run_queue_depth`, `run_queue_completed_count`, `run_queue_failed_count`, `run_steps_total`, `run_step_current`, `run_step_current_index`, `run_queue_cancelable`, `run_queue_provider_execution_enabled:false`, `run_queue_worker_thread_enabled:false`, `run_queue_trace_export_enabled:false`, `run_queue_external_process_enabled:false`, `run_queue_project_mutation_enabled:false`, `run_queue_persistence`, `run_queue_policy`, and `run_queue_steps`. Keep these fields local and conservative until a later durable runner sprint adds persisted queue files, worker ownership, provider execution, and trace export behind approvals.

The cancel and clear buttons update local queue state only. They write compact output events named `agent_run_queue_canceled` and `agent_run_queue_cleared`, append Activity events, and keep provider execution, worker threads, telemetry export, external process execution, and project mutation disabled. Do not reinterpret these buttons as a runner implementation.

`src/ccad_gui/review_window.cpp` includes the queue IDs in dirty-node refresh and validates `action:agent_cancel_run_queue` plus `action:agent_clear_run_queue` as visible Agent buttons. `src/ccad_gui/main.cpp` includes the same queue IDs in app-owned target-sequence validation so the harness moves to them and screenshots them before and after resize.

Sprint 206 proof used `artifacts/screenshots/sprint206-agent-run-queue-foundation-final2-20260605-153920.png` and `artifacts/screenshots/sprint206-agent-run-queue-foundation-targets-target-sequence.json`. The target report had 156 entries, zero missing targets, and 12 queue-related screenshots, including initial and resized captures for the queue panel, status, counts, current step, cancel action, and clear action. Focused `gui_agent_panel` and `gui_ui_map` tests passed after red tests proved the missing queue contract, and the sprint-end full Qt build plus CTest gate passed 36 of 36 tests.

## Sprint 207 Agent Panel Reference Polish Addendum

The Agent pane now reports `visual_style:"agent_reference_panel_v5"` and `workspace_layout_version:5`. It adds `panel:agent_header_action_bar`, `panel:agent_evidence_thumbnail_strip`, four evidence thumbnail cards, `panel:agent_approval_preview`, `panel:agent_approval_preview_artifact`, approval summary/delta labels, `panel:agent_footer_quick_actions`, `action:agent_quick_request_context`, and `action:agent_quick_trigger_drc`. The old header/footer context and DRC buttons remain alive, and advanced provider/session/trace/policy/run panels remain targetable lower in the scroll. Focused `gui_agent_panel` and `gui_ui_map` tests passed after red tests proved the missing v5 contract. Visual proof used `artifacts/screenshots/sprint207-agent-panel-reference-polish-v4-20260605-163527.png`; target proof used `artifacts/screenshots/sprint207-agent-panel-reference-polish-targets-target-sequence.json` with 182 entries, zero missing targets, 26 v5-region captures, and empty stderr. The sprint-end full Qt build plus CTest gate passed 36 of 36 tests.
## Sprint 220 KiCad PCB API Parity Addendum

`src/ccad_cli/pcb_object_queries.hpp/.cpp` now owns the first KiCad PCB API parity query helpers for headless PCB inspection. The new read-only surfaces are `listPcbObjectsByNetJson`, `listPcbConnectedObjectsJson`, `listPcbEnabledLayersJson`, `listPcbVisibleLayersJson`, `getPcbLayerNameJson`, `getPcbBoardStackupJson`, `getPcbDesignRulesJson`, and `getPcbOutlineJson`. They intentionally return explicit `kicad_handler` and parity-scope fields where the CCad model is narrower than KiCad.

`src/ccad_cli/pcb_commands.cpp` dispatches the matching CLI commands: `pcb list-by-net`, `pcb list-connected`, `pcb list-enabled-layers`, `pcb list-visible-layers`, `pcb get-layer-name`, `pcb get-board-stackup`, `pcb get-rules`, and `pcb get-outline`. Keep these commands read-only. Mutating layer, rule, and object commands already exist separately and should not be silently called from these query paths.

`src/ccad_cli/agent_commands.cpp` exposes `agent pcb-api-schema` and JSON-RPC `agent.pcb_api_schema` as the Agent-visible KiCad handler mapping. Keep this schema synchronized with `src/ccad_cli/app.cpp` help metadata and with every future KiCad PCB API parity command. The current schema maps KiCad handlers for items, layers, stackup, rules, bounding box, nets, items by net, and connected items, while explicitly marking board origin, graphics defaults, custom rules, pad polygon extraction, net classes, selection, and active-layer mutation as not complete.

`src/ccad_core/agent_orchestrator.cpp` currently decomposes goals into a context task plus `agent.plan_with_provider`. Provider-backed planning must remain blocked by default with `provider_execution_disabled` until a future approved runner explicitly turns provider execution on. `tests/test_agent_orchestrator.cpp` covers this safety state in the serialized task list.

The local KiCad audit source for this slice is `F:\kicad_src\pcbnew\api\api_handler_pcb.h` and `F:\kicad_src\pcbnew\api\api_handler_pcb.cpp`. Future work should continue through the rest of `F:\kicad_src\pcbnew\api` one file at a time, then move deeper into `pcbnew` only after the API folder gaps are recorded and covered.

## Sprint 221 KiCad PCB API Utility Layer-Set Addendum

`src/ccad_core/layers.hpp/.cpp` now owns the first CCad analogue for KiCad `api_pcb_utils.cpp` layer-set packing behavior. `expandKiCadLayerSet(layer_selectors, board)` expands KiCad wildcard selectors such as `*.Cu`, `*.Mask`, `*.Paste`, `*.SilkS`, `*.Fab`, `*.CrtYd`, and `*.Adhes` against the current board layer list, de-duplicates matches in board order, and preserves unmatched selectors so validation can report them. `standardKiCadPcbLayerNumbersForSet(layer_ids)` converts resolved CCad layer IDs to canonical KiCad PCB layer numbers while omitting custom or unknown layer IDs.

`src/ccad_core/placement.cpp` uses `expandKiCadLayerSet` after the existing front/back placement flip for footprint pad layers. This means a KiCad through-hole footprint with `*.Cu` / `*.Mask` pads produces concrete placed-pad layers when the board has matching layers, while explicit front-only SMD layers still flip to back-only layers during bottom placement.

`src/ccad_cli/pcb_object_queries.cpp` now emits board-context pad layer-set metadata for agent-facing read surfaces. `pcb list-objects --type pad`, `pcb list-by-net`, `pcb list-connected`, and `pcb export-route-job` include `resolved_layers` plus `kicad_layer_numbers`. Direct `pcb get-object` still reports the raw stored `layers` array because it has no board context and remains useful for debugging imported wildcard selectors.

The local KiCad audit source for this slice is `F:\kicad_src\pcbnew\api\api_pcb_utils.h` and `F:\kicad_src\pcbnew\api\api_pcb_utils.cpp`. This slice does not implement KiCad's full `CreateItemForType` board-item factory, selection state, full connectivity solver, netclass model, custom-rule model, or pad polygon extraction.

## Sprint 222 KiCad PCB API Items, Matrix, Autoplace, and Spread Addendum

`src/ccad_cli/agent_commands.cpp` now records the complete first-pass KiCad PCB API folder ledger in `agentPcbApiSchemaJson()`. It includes all registered handlers from `api_handler_pcb.cpp`, enum groups from `api_pcb_enums.cpp`, `BOARD_CONTEXT` methods from `board_context.h/.cpp`, and `HEADLESS_BOARD_CONTEXT` lifecycle notes from `headless_board_context.h/.cpp`. Keep this ledger synchronized with every future KiCad PCB editor parity command and advance the recorded source walk when the next `pcbnew` file is audited.

`src/ccad_core/pad_number_provider.hpp/.cpp` owns deterministic pad-number sequence helpers adapted from KiCad `array_pad_number_provider`. The public surface is `provideArrayPadNumbers(const PadNumberProviderRequest&)`, returning the current number, next number, and requested preview list. It has no project mutation dependency and is tested by `tests/test_pad_number_provider.cpp`.

`src/ccad_core/autorouter_matrix.hpp/.cpp` owns the first grid occupancy and cost-field primitive adapted from KiCad `autorouter/ar_matrix`. `AutorouterMatrix` configures grid-aligned board bounds, stores top/bottom side cells, traces filled rectangles, supports write/or/xor/and/add operations, builds distance maps from occupied cells, creates keepout-cost rectangles, and can query rectangle occupancy or accumulated distance cost. It is tested by `tests/test_autorouter_matrix.cpp`.

`src/ccad_core/autoplacer.hpp/.cpp` owns first-slice footprint autoplacement adapted from KiCad `autorouter/ar_autoplacer` and `autoplace_tool`. `planFootprintAutoPlacement(board, footprint, pad_nets, layer_id, grid_step)` uses explicit board/keepout/pad checks plus `AutorouterMatrix` occupancy and distance costs. `src/ccad_cli/pcb_commands.cpp` exposes this through `pcb autoplace-footprint`, then calls the existing `placeFootprint()` kernel path so placed pads use the same serialization, net mapping, and DRC behavior as manual placement.

`src/ccad_core/spread_footprints.hpp/.cpp` owns first-slice component spread behavior adapted from KiCad `autorouter/spread_footprints`. `spreadFootprintComponents(board, request)` groups pads by component ID, naturally sorts reference designators, preserves internal pad offsets, and moves selected groups into a deterministic non-overlapping lane. `src/ccad_cli/pcb_commands.cpp` exposes this through `pcb spread-footprints`.

The local KiCad audit sources for this slice are `F:\kicad_src\pcbnew\api\api_handler_pcb.cpp`, `api_handler_pcb.h`, `api_pcb_enums.cpp`, `board_context.cpp`, `board_context.h`, `headless_board_context.cpp`, `headless_board_context.h`, `F:\kicad_src\pcbnew\array_pad_number_provider.cpp`, `array_pad_number_provider.h`, `F:\kicad_src\pcbnew\autorouter\ar_autoplacer.cpp`, `ar_autoplacer.h`, `ar_matrix.cpp`, `ar_matrix.h`, `autoplace_tool.cpp`, `autoplace_tool.h`, `spread_footprints.cpp`, and `spread_footprints.h`. The next strict top-level PCB editor file after this autorouter starter slice is `F:\kicad_src\pcbnew\board.cpp`, followed by `board.h`.

## Sprint 223 KiCad Board Document Model Addendum

Sprint 223 audits KiCad `F:\kicad_src\pcbnew\board.cpp` and `F:\kicad_src\pcbnew\board.h` against CCad's project model. The stale early-map sketch that shows `Project` as owning `std::optional<Board> board` plus top-level components, nets, and constraints is superseded. The durable model is now `Project` with independent `std::vector<Board> boards` and `std::vector<Schematic> schematics`, so a project may contain a board without a schematic, a schematic without a board, or multiple documents that later project-structure work can link explicitly.

`src/ccad_core/model.hpp` owns the primary-document helpers `primaryBoard(Project&)`, `primaryBoard(const Project&)`, `primarySchematic(Project&)`, `primarySchematic(const Project&)`, and `ensurePrimarySchematic(Project&)`. Board-owned behavior must use `primaryBoard()` and must not invent a schematic just to run physical operations. Schematic authoring commands may call `ensurePrimarySchematic()` because they are explicitly creating logical design data.

`src/ccad_core/drc.cpp` now treats absent schematics as valid for board-only physical checks. Pads, vias, tracks, zones, keepouts, graphics, route requests, layer references, geometry, clearance, and board containment still run from the board document. Logical checks that require a schematic netlist, such as unknown schematic nets, unknown components, pin membership, and duplicate schematic-net members, run only when `primarySchematic()` exists. This is the first CCad analogue of KiCad's board document independence and prevents board-only DRC from producing irrelevant schematic errors.

`src/ccad_core/erc.cpp`, `review.cpp`, `bom_export.cpp`, `pnp_export.cpp`, `diff.cpp`, `placement.cpp`, and `kicad_pcb_export.cpp` now follow the same document split. ERC returns no diagnostics when there is no schematic document and still warns on an explicitly empty schematic. Review summaries can describe board-only projects without claiming missing schematic errors. BOM export returns a deterministic header for no-schematic projects. Pick-and-place export uses board footprints with blank values when no schematic component metadata exists. Diff treats absent schematics as empty logical documents. Footprint placement no longer creates or requires a schematic, while symbol placement creates the primary schematic intentionally.

`src/ccad_core/kicad_pcb_export.cpp` gathers KiCad net declarations from both the optional schematic netlist and board-local copper references on pads, vias, tracks, and zones. Board-only KiCad PCB export therefore preserves physical net IDs instead of silently dropping them just because no schematic document is linked.

The matching CLI and agent surfaces in `src/ccad_cli/pcb_commands.cpp`, `src/ccad_cli/sch_commands.cpp`, and `src/ccad_cli/agent_orchestrator_cli.cpp` use the helper APIs instead of indexing `project.schematics[0]` for product behavior. Tests may still index `schematics[0]` inside fixtures that explicitly created a schematic; production code should not do this outside guarded serialization compatibility.

The reference sources checked for this sprint are the local KiCad board implementation files above, the KiCad PCB Editor manual at `https://docs.kicad.org/9.0/en/pcbnew/pcbnew.html`, the KiCad Doxygen `BOARD` class page at `https://docs.kicad.org/doxygen/classBOARD.html`, and the KiCad S-expression PCB file-format notes at `https://dev-docs.kicad.org/en/file-formats/sexpr-pcb/`. Future strict walk work should continue from the next unported `pcbnew` file after `board.h`, and every new slice must record the exact local KiCad files plus current external references before code edits.

## Sprint 226 KiCad Board Design Settings Handover

The deterministic KiCad PCB editor source walk reached `F:\kicad_src\pcbnew\board_design_settings.cpp`. KiCad keeps board-level physical rule ranges in `BOARD_DESIGN_SETTINGS::ValidateDesignRules()`, with zero allowed for many minima and negative ranges allowed for some mask, paste, silk, and copper-edge style settings.

CCad's first analogue is `src/ccad_core/board_design_settings.hpp/.cpp`. The public function is:

```cpp
std::vector<DesignRuleValidationError> validateDesignRules(const DesignRules& rules);
```

`src/ccad_core/drc.cpp` now delegates board design-rule diagnostics to this helper. Keep future Board Setup GUI controls, CLI rule editors, agent rule summaries, and manufacturing export checks pointed at the same helper instead of recreating independent range logic.

Coverage lives in `tests/test_board_design_settings.cpp` and `tests/test_drc.cpp`. The red failure for this slice was the missing `ccad_core/board_design_settings.hpp` include; the focused green checks are `board_design_settings` and `drc` CTest selectors after building `ccad_board_design_settings_tests` and `ccad_drc_tests`.

## Sprint 226 KiCad BOARD_ITEM Metadata Handover

The deterministic KiCad PCB editor source walk reached `F:\kicad_src\pcbnew\board_item.cpp` and `F:\kicad_src\include\board_item.h`. KiCad's `BOARD_ITEM` is the common base for board-contained objects and centralizes layer identity, layer-set description, groupability, hole checks, locked/knockout state, and GAL view-layer reporting.

CCad's first analogue is metadata-only and lives in `src/ccad_core/board_item.hpp/.cpp`. The public helpers are overloads of:

```cpp
BoardItemMetadata boardItemMetadata(const Board& board, const Pad& item);
BoardItemMetadata boardItemMetadata(const Board& board, const Via& item);
BoardItemMetadata boardItemMetadata(const Board& board, const TrackSegment& item);
BoardItemMetadata boardItemMetadata(const Board& board, const BoardGraphic& item);
BoardItemMetadata boardItemMetadata(const Board& board, const BoardText& item);
BoardItemMetadata boardItemMetadata(const Board& board, const BoardZone& item);
```

The helper derives `kicad_base_class`, `kicad_groupable`, `primary_layer_id`, `layer_ids`, `layer_mask_description`, `side_specific`, `is_on_copper_layer`, `has_hole`, `has_drilled_hole`, `locked`, `knockout`, `view_layer_ids`, and `parity_scope` without mutating the project schema. Pads resolve KiCad wildcard layer selectors through `expandKiCadLayerSet()`, vias report the active board copper layer set, tracks and single-layer drawings report their concrete layer, and zones report their stored layer IDs.

`src/ccad_cli/pcb_object_queries.cpp` owns the JSON formatter for this metadata. The object JSON helpers now take `const Board&` so `pcb get-object`, `pcb list-objects`, `pcb list-by-net`, and `pcb list-connected` can expose the same KiCad-shaped board-item envelope for pads, vias, tracks, graphics, texts, and zones. Keepouts and placement regions are intentionally not reported as `BOARD_ITEM` objects in this slice because their current CCad model is a constraint-region abstraction rather than a confirmed KiCad object analogue.

Coverage lives in `tests/test_cli.cpp`. The red failure for this slice was `test failure: pcb get-object exposes KiCad board item base class`; the focused green checks are the direct `ccad_cli_tests.exe` run and the `cli` CTest selector after building `ccad_cli_tests`.

## Sprint 226 KiCad BOARD_LOADER State Handover

The deterministic KiCad PCB editor source walk reached `F:\kicad_src\pcbnew\board_loader.cpp`, `F:\kicad_src\pcbnew\board_loader.h`, and `F:\kicad_src\qa\tests\pcbnew\test_board_loader.cpp`. KiCad's `BOARD_LOADER` delegates board file loading to `PCB_IO_MGR`, optionally configures the plugin, then runs post-load initialization that attaches the board to the project, prepares the DRC engine, resolves DRC exclusions, rebuilds connectivity and net lists, synchronizes netclasses and component classes, syncs tuning profile properties, and updates user units.

CCad's first analogue lives in `src/ccad_core/board_loader.hpp/.cpp`. The public helper is:

```cpp
BoardLoadState summarizeLoadedBoard(const Project& project,
                                    const BoardLoadOptions& options = BoardLoadOptions{});
```

The helper is deliberately derived state, not a file parser or plugin manager. It reports `kicad_class:"BOARD_LOADER"`, `source_format:"CCAD_JSON"`, initialized versus raw load mode, board attachment, design-rule readiness, DRC readiness, connectivity readiness, netlist readiness, user-unit readiness, board/schematic counts, layer counts, object counts, board net counts, and explicit `pending_kicad_loader_steps`. It also reports `persistent_drc_engine:false` with `drc_engine_model:"stateless_ccad_runDrc"` so agents do not confuse CCad's current stateless DRC with KiCad's allocated `DRC_ENGINE`.

`src/ccad_cli/pcb_commands.cpp` exposes the helper through `pcb load-state --file <path> [--initialize true|false]`. The default mirrors KiCad's initialized load path. Passing `--initialize false` mirrors KiCad's `OPTIONS::initialize_after_load = false` test case by reporting no project attachment, no DRC readiness, and no connectivity readiness while still parsing the CCad project JSON as data.

Coverage lives in `tests/test_board_loader.cpp` and `tests/test_cli.cpp`. The red failure for this slice was the missing `ccad_core/board_loader.hpp` include; the focused green checks are the `board_loader` and `cli` CTest selectors after building `ccad_board_loader_tests` and `ccad_cli_tests`.

## Sprint 226 KiCad BOARD_STACKUP Handover

The deterministic KiCad PCB editor source walk reached `F:\kicad_src\pcbnew\board_stackup_manager\board_stackup.cpp`, `board_stackup.h`, `dielectric_material.cpp`, `dielectric_material.h`, `board_stackup_reporter.cpp`, and `board_stackup_reporter.h`. KiCad's `BOARD_STACKUP` builds a physical fabrication stack from board-enabled silk, paste, mask, copper, and dielectric layers, carries material and finish metadata, can report total board thickness, and computes copper-layer distances for height-aware length calculations.

CCad's first analogue lives in `src/ccad_core/board_stackup.hpp/.cpp`. The public helpers are:

```cpp
BoardStackup buildDefaultBoardStackup(const Board& board);
Length buildBoardThicknessFromStackup(const BoardStackup& stackup);
Length boardStackupLayerDistance(const BoardStackup& stackup,
                                 const std::string& first_layer_id,
                                 const std::string& second_layer_id);
```

The helper is derived from current `Board::layers` and `Board::design_rules.board_thickness`. It does not add durable project schema fields yet. The derived stackup orders top technical layers, copper layers, dielectric layers, and bottom technical layers in KiCad style. It uses KiCad's default copper thickness of 0.035 mm, solder-mask thickness of 0.01 mm, FR4 dielectric material, epsilon-r 4.5, loss tangent 0.02, and distributes dielectric thickness over the gaps between active copper layers. `boardStackupLayerDistance()` mirrors KiCad's current copper/dielectric summation behavior, including half-thickness handling for internal start or stop copper layers.

`src/ccad_cli/pcb_object_queries.cpp` exposes this through `pcb get-board-stackup --file <path>`. The JSON now reports `kicad_class:"BOARD_STACKUP"`, `kicad_parity_scope:"default_stackup_first_slice"`, stackup item rows, computed stackup thickness, finish metadata, copper layer distance rows, and the previous enabled-layer rows for compatibility. `src/ccad_cli/agent_commands.cpp` maps `GetBoardStackup` to `default_stackup_first_slice` so agent tool planners no longer think this command is only an enabled-layer order query.

Future work must not overclaim this slice. Editable stackup persistence, material-library editing, dielectric sublayers, locked impedance-controlled thickness, copper finish, edge connectors, edge plating, stackup serialization/import, Gerber job stackup export, impedance calculations, 3D board thickness, and route/via length integration remain backlog.

Coverage lives in `tests/test_board_stackup.cpp` and `tests/test_cli.cpp`. The red failures for this slice were the missing `ccad_core/board_stackup.hpp` include, the missing KiCad stackup class in `pcb get-board-stackup`, and the stale `enabled_layer_order` agent schema scope. Focused green checks are the `board_stackup` and `cli` CTest selectors after building `ccad_board_stackup_tests` and `ccad_cli_tests`.

## Sprint 226 KiCad Board Statistics Handover

The deterministic KiCad PCB editor source walk reached `F:\kicad_src\pcbnew\board_statistics.cpp`, `F:\kicad_src\pcbnew\board_statistics.h`, `F:\kicad_src\pcbnew\board_statistics_report.cpp`, `F:\kicad_src\pcbnew\board_statistics_report.h`, and `F:\kicad_src\qa\tests\pcbnew\test_board_statistics.cpp`. KiCad's first directly useful headless behaviors here are `CollectDrillLineItems()`, which scans pads and vias, normalizes drill shape, x/y size, plated state, source kind, copper start layer, copper stop layer, and aggregates identical rows by quantity, and the report path that packages board dimensions, areas, counts, minimums, board thickness, and drill rows.

CCad's first analogue lives in `src/ccad_core/board_statistics.hpp/.cpp`. The public helpers are:

```cpp
std::vector<DrillLineItem> collectDrillLineItems(const Board& board);
BoardStatisticsReport buildBoardStatisticsReport(const Board& board,
                                                 std::string project_name = "",
                                                 std::string board_name = "");
struct DrillLineItemCompare;
```

`DrillLineItem` currently reports `x_size`, `y_size`, `shape`, `plated`, `source`, nullable `start_layer_id`, nullable `stop_layer_id`, and `quantity`. Pads use the current `Pad::drill`, KiCad-style pad type strings such as `np_thru_hole` to determine plating, and `expandKiCadLayerSet()` to derive copper start/stop layers. Vias report plated round drill rows across the board copper span. `BoardStatisticsReport` currently reports the KiCad report reference, the declared first-slice parity scope, optional project and board names, board outline dimensions, rectangular board area, object counts, nullable minimum track width and drill diameter, board thickness, and drill rows.

`src/ccad_cli/pcb_commands.cpp` exposes these helpers through `pcb drill-statistics --file <path>` and `pcb board-statistics --file <path>`. The drill JSON output includes `kicad_reference:"board_statistics"`, `parity_scope:"drill_line_items_first_slice"`, `summary.unique_drill_rows`, `summary.total_drill_count`, and a `drill_holes` array with count, shape, x/y size, plated state, source, and nullable start/stop layers. The report JSON output includes `kicad_reference:"board_statistics_report"`, `parity_scope:"summary_report_first_slice"`, `board_outline`, `board_width_nm`, `board_height_nm`, `board_area_square_mm`, `counts`, nullable minimums, `board_thickness_nm`, and `drill_holes`. Keep both commands truthful as the board model grows; do not add report fields that are not derived from current board data.

Remaining report gaps are exact polygonal board area, copper areas, courtyard area, footprint density, geometry-wide minimum clearance, localized text report formatting, subtract-hole options, slot drill x/y models, and backdrill-style data.

Coverage lives in `tests/test_board_statistics.cpp` and `tests/test_cli.cpp`. The red failures for this slice were the missing `ccad_core/board_statistics.hpp` include, the missing `pcb drill-statistics` help entry, and the missing `pcb board-statistics` help entry. The focused green checks are the `board_statistics` and `cli` CTest selectors after building `ccad_board_statistics_tests` and `ccad_cli_tests`.

## Sprint 226 KiCad BOARD_ITEM_CONTAINER Handover

The deterministic KiCad PCB editor source walk reached `F:\kicad_src\pcbnew\board_item_container.h`. KiCad's `BOARD_ITEM_CONTAINER` is the board-item ownership interface used by board-like and footprint-like containers. Its useful headless contract is `Add()` with `INSERT`, `APPEND`, `BULK_APPEND`, and `BULK_INSERT`, `Remove()` with `NORMAL` and `BULK`, and `Delete()` implemented by removing the item from the container before deleting it.

CCad's first analogue lives in `src/ccad_core/board_item_container.hpp/.cpp`. The public helpers are:

```cpp
BoardItemContainerSummary summarizeBoardItemContainer(const Board& board);
std::optional<BoardContainerItemRef> findBoardContainerItem(const Board& board,
                                                            std::string_view id);
bool hasBoardContainerItemId(const Board& board, std::string_view id);
void requireUniqueBoardContainerItemId(const Board& board, std::string_view id);
BoardContainerRemoveResult removeBoardContainerItem(Board& board,
                                                    std::string_view id,
                                                    BoardContainerRemoveMode mode);
```

The helper currently operates over CCad's typed board vectors. Pads, vias, tracks, graphics, texts, and zones are treated as current board-item analogues, while keepouts and placement regions are included as board-contained constraint items so the existing physical-object uniqueness and removal behavior stays shared. This is intentionally not a full KiCad ownership tree; footprint-local child ownership, connectivity skip behavior, undo/view notifications, item parent pointers, and pointer lifetime semantics remain future work.

`src/ccad_cli/pcb_commands.cpp` now routes `pcb remove-object --file <path> --id <id> [--mode normal|bulk]` through `removeBoardContainerItem()`. The emitted JSON includes `kicad_container_class:"BOARD_ITEM_CONTAINER"`, `kicad_method:"Delete"`, `remove_mode`, `kind`, `index`, and `kicad_delete_semantics`, which gives agent tooling a truthful KiCad-shaped delete/remove envelope.

Coverage lives in `tests/test_board_item_container.cpp` and `tests/test_cli.cpp`. The red failures for this slice were the missing `ccad_core/board_item_container.hpp` include and the missing `--mode` option in `pcb remove-object`. The focused green checks are the `board_item_container` and `cli` CTest selectors after building `ccad_board_item_container_tests` and `ccad_cli_tests`.

## Sprint 226 KiCad BOARD_TEXT_VAR_ADAPTER Handover

The deterministic KiCad PCB editor source walk reached `F:\kicad_src\pcbnew\board_text_var_adapter.cpp`, `F:\kicad_src\pcbnew\board_text_var_adapter.h`, `F:\kicad_src\pcbnew\api\api_handler_pcb.cpp`, and `F:\kicad_src\common\api\api_handler_common.cpp`. KiCad's adapter listens to board item add/remove/change callbacks and feeds `TEXT_VAR_TRACKER`; KiCad's API exposes project-level `GetTextVariables` and `SetTextVariables`, then PCB-level `ExpandTextVariables` resolves strings through `BOARD::ResolveTextVar()`.

CCad's first analogue lives in `src/ccad_core/board_text_var_adapter.hpp/.cpp`, with durable storage in `Project::text_variables`. The public helpers are:

```cpp
std::vector<TextVariableReference> collectTextVariableReferences(
    std::string_view text, const std::map<std::string, std::string>& variables);
std::string expandTextVariables(std::string_view text,
                                const std::map<std::string, std::string>& variables,
                                std::vector<TextVariableReference>* references = nullptr);
std::vector<ExpandedBoardText> expandBoardTexts(const Project& project,
                                                const Board& board);
```

`src/ccad_core/serialize.cpp` reads and writes top-level project `text_variables` as a deterministic JSON object. `src/ccad_cli/project_commands.cpp` exposes `project set-text-variable --file <path> --key <name> --value <text>` and `project list-text-variables --file <path>`. `src/ccad_cli/pcb_commands.cpp` exposes `pcb expand-text-variables --file <path> [--text <value>]`, returning `kicad_handler:"ExpandTextVariables"`, `kicad_source:"BOARD::ResolveTextVar"`, `parity_scope:"project_text_variable_expansion_first_slice"`, the variable map, expanded text rows, and resolved/unresolved reference metadata. `src/ccad_cli/agent_commands.cpp` maps KiCad `ExpandTextVariables` to this command in the PCB API schema.

This helper intentionally does not yet implement KiCad's live `TEXT_VAR_TRACKER`, listener invalidation, footprint source-key fanout such as `${U1:FIELD}`, title-block variables, barcode text dependencies, or GUI repaint invalidation. Keep future GUI/editor work on top of the kernel helper rather than duplicating expansion logic in Qt.

Coverage lives in `tests/test_board_text_var_adapter.cpp` and `tests/test_cli.cpp`. The red failures for this slice were the missing `ccad_core/board_text_var_adapter.hpp` include and the missing `pcb expand-text-variables` help entry. The focused green checks are the `board_text_var_adapter` and `cli` CTest selectors after building `ccad_board_text_var_adapter_tests` and `ccad_cli_tests`.

## Sprint 226 KiCad Legacy Board BOM Handover

The deterministic KiCad PCB editor source walk reached `F:\kicad_src\pcbnew\build_BOM_from_board.cpp`. KiCad's PCB editor legacy BOM export rejects boards with no footprints, skips footprints marked `FP_EXCLUDE_FROM_BOM`, groups by footprint value plus footprint ID, naturally sorts designators, sorts groups by the first designator, and writes a fixed semicolon CSV header with `Id`, `Designator`, `Footprint`, `Quantity`, `Designation`, and `Supplier and ref`.

CCad's first analogue lives in `src/ccad_core/bom_export.hpp/.cpp`:

```cpp
std::string exportBoardToBomCsv(const Project& project);
```

`exportBoardToBomCsv()` consumes placed footprint metadata from `Board::footprints`, not raw pads. The model entry is:

```cpp
struct BoardFootprint {
    std::string reference;
    std::string value;
    std::string footprint_name;
    std::string layer_id;
    Point position;
    double rotation_degrees;
    bool exclude_from_bom;
};
```

`src/ccad_core/placement.cpp` appends one `BoardFootprint` whenever `placeFootprint()` places footprint pads. The value comes from an explicit placement value when provided, otherwise from a matching schematic component part string when available. `src/ccad_core/serialize.cpp` persists the `footprints` array in board JSON. `src/ccad_core/kicad_footprint_import.cpp` preserves root footprint `exclude_from_bom` attributes in CCad footprint JSON, so imported footprints can opt out of the board-side BOM.

`src/ccad_cli/pcb_commands.cpp` exposes this through:

```text
ccad pcb export-board-bom --file <project.ccad.json> --output <board-bom.csv>
```

The command writes the CSV to disk and is intentionally board-side only. It does not replace schematic-driven BOM generation, manufacturer/supplier enrichment, procurement workflows, or configurable report generation. Future footprint deletion, movement, library provenance, PnP, and GUI selection work must keep `Board::footprints` synchronized with concrete pads instead of assuming pads alone are a complete footprint instance.

Coverage lives in `tests/test_bom_export.cpp` and `tests/test_cli.cpp`. The red failures for this slice were missing `BoardFootprint`, `Board::footprints`, `exportBoardToBomCsv()`, and the `pcb export-board-bom` help entry. The focused green checks are the `bom_export` and `cli` CTest selectors after building `ccad_bom_export_tests` and `ccad_cli_tests`.

## Sprint 226 KiCad CLEANUP_ITEM Handover

The deterministic KiCad PCB editor source walk reached `F:\kicad_src\pcbnew\cleanup_item.cpp` and `F:\kicad_src\pcbnew\cleanup_item.h`. KiCad's cleanup item code defines the user-facing cleanup actions shared by tracks/vias and graphics cleanup flows, while `VECTOR_CLEANUP_ITEMS_PROVIDER` exposes indexed cleanup rows and erases rows only on deep delete.

CCad's first analogue lives in `src/ccad_core/cleanup_item.hpp/.cpp`:

```cpp
enum class CleanupActionCode;
struct CleanupActionInfo;
const std::vector<CleanupActionInfo>& cleanupActionCatalog();
const CleanupActionInfo* findCleanupActionInfo(CleanupActionCode code);
std::string cleanupActionTitle(CleanupActionCode code);
class CleanupActionProvider;
```

The catalog includes all thirteen KiCad action rows from this source slice, split into `tracks_and_vias` and `graphics` domains. Stable CCad IDs such as `shorting_track`, `redundant_via`, `zero_length_track`, `duplicate_graphic`, and `lines_to_rect` are used for CLI and future agent tools, while `kicad_offset` preserves the source order used by the KiCad enum block.

`src/ccad_cli/pcb_commands.cpp` exposes the catalog through:

```text
ccad pcb cleanup-actions
```

The command is read-only and does not require a project file. It emits the KiCad class and provider names, the vector-indexed row semantics, the parity scope, and every action row. Future cleanup-engine work should use this catalog instead of inventing different action names, then add actual tracks cleaner, graphics cleaner, dialog preview, and apply/delete behavior in separate tested slices.

Coverage lives in `tests/test_cleanup_item.cpp` and `tests/test_cli.cpp`. The red failure for this slice was the missing `src/ccad_core/cleanup_item.cpp` source. The focused green checks are the `cleanup_item` and `cli` CTest selectors after building `ccad_cleanup_item_tests` and `ccad_cli_tests`.

## Sprint 226 GUI Empty/Board-Only Load Survival Handover

`src/ccad_gui/review_window.cpp` must treat the loaded board as optional in every UI callback that can run during project load, tab changes, `zoomToFit()`, scene selection, and teardown. The Sprint 226 crash path came from `BoardCanvasView::zoomToFit()` notifying viewport state while an empty project had no `project_cache_.boards[0]`. `ReviewWindow::updateCursorStatus()` now passes an optional board into `formatCursorStatus()`, and `ReviewWindow::updateSelectionStatus()` renders generic selection and inspector state when no board exists.

Keep this invariant when editing the GUI: compile success is not enough. After every GUI-affecting build, run a focused Qt test with the Qt DLL path first, then launch the actual `ccad_gui.exe` and prove it survives for 5 to 7 seconds on the touched path. When a visual artifact is relevant, use the official beep-and-screenshot harness instead of a custom one-off screenshot path.

The focused regression is `tests/test_gui_ui_map.cpp`. It loads an empty project and a board-only project through `ReviewWindow`, checks that `uiMapJson()` still returns a schema marker after each load, shows the window, and keeps the event loop alive for 7 seconds. The focused green command is:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && ctest --test-dir build-qt -R gui_ui_map --output-on-failure"
```

The official visual proof for this fix is:

```cmd
cmd /c powershell -NoProfile -ExecutionPolicy Bypass -File scripts\run_sprint_demo.ps1 -BuildDir build-qt -Name sprint226-gui-crash-smoke -GuiWaitSeconds 7
```

The verified screenshot artifact is `artifacts\screenshots\sprint226-gui-crash-smoke-20260621-180502.png`.

## Sprint 226 KiCad GENERAL_COLLECTOR Locked-Item Handover

The deterministic KiCad PCB editor source walk reached `F:\kicad_src\pcbnew\collectors.cpp` and `F:\kicad_src\pcbnew\collectors.h`. KiCad's `GENERAL_COLLECTOR` filters board-item candidates by scan set, layer, visibility, footprint side, text side, pad side, via type, net, ignored tracks, zone-fill policy, and locked-item policy. CCad's first analogue is deliberately narrower: it implements the persisted locked state and `IgnoreLockedItems()` behavior in the existing headless collector.

`src/ccad_core/model.hpp` now gives `Pad`, `Via`, `TrackSegment`, `BoardGraphic`, `BoardText`, `BoardZone`, and `BoardFootprint` a `locked` flag. `src/ccad_core/serialize.cpp` reads those optional `locked` fields and writes them only when true. `src/ccad_core/board_item.cpp` carries the concrete lock state into `BoardItemMetadata`, and `src/ccad_core/board_collector.cpp` copies it into `BoardCollectorCandidate` before applying `BoardCollectionGuide::ignore_locked_items`.

The CLI surface is `ccad pcb collect-items --ignore-locked true`. `src/ccad_cli/pcb_commands.cpp` now emits `locked` on each collector row, keeps locked objects visible by default, and excludes them only when the guide asks for that policy. Coverage lives in `tests/test_board_collector.cpp` and `tests/test_cli.cpp`; the focused green selectors are `board_collector` and `cli`.

This slice does not implement KiCad's full collector behavior. Preferred-layer secondary result ordering, footprint child item scans, footprint/text side filters, pad/via type filters, zone-fill suppression, ignored-track lists, and interactive hit testing are still backlog.

## Sprint 226 Safe Project Write Handover

`src/ccad_cli/common.cpp::writeProjectFile()` now calls `ccad::dumpProjectJson(project)` before opening the output file. This matters because the standard output-file stream path truncates the destination when it is opened for output, so a serializer exception or stale-object crash could otherwise turn a valid project into a zero-byte file.

The focused bug was reproduced through `pcb add-text` after the model layout changed in Sprint 226. A clean rebuild removed the stale-object crash, and the code hardening prevents the same failure mode from causing data loss if serialization fails again. This is not yet atomic temp-file replacement; future persistence work should add a write-temp, flush, and rename path for crash-safe replacement.

## Sprint 226 KiCad ConvertOutlineToPolygon Handover

The deterministic KiCad PCB editor source walk reached `F:\kicad_src\pcbnew\convert_shape_list_to_polygon.cpp` and `F:\kicad_src\pcbnew\convert_shape_list_to_polygon.h`. KiCad's `ConvertOutlineToPolygon` logic converts board or footprint shape lists into polygon sets, chains outline edges, handles non-line shapes, detects unclosed and self-intersecting contours, nests holes, and can infer a fallback rectangular outline.

CCad's first analogue lives in `src/ccad_core/board_outline_polygon.hpp/.cpp`:

```cpp
struct BoardOutlinePolygonOptions;
struct BoardOutlinePolygonReport;
BoardOutlinePolygonReport buildBoardOutlinePolygonReport(
    const Board& board,
    const BoardOutlinePolygonOptions& options = BoardOutlinePolygonOptions{});
```

The helper is read-only. It scans `Board::graphics` for `kind == "line"` on `Edge.Cuts`, chains exact endpoint matches, reports ordered `Point` rows and source graphic IDs, derives a bounding box from a closed chain, and records KiCad provenance through `kicad_source`, `kicad_function`, and `parity_scope`. When `infer_outline_if_necessary` is set, the helper can emit the current rectangular `Board::outline` as a fallback and marks `used_inferred_outline` so agents know this is not a real Edge.Cuts contour.

`src/ccad_cli/pcb_commands.cpp` exposes the report through:

```text
ccad pcb outline-polygon --file <project.ccad.json> [--infer true|false]
```

Keep this helper truthful as geometry support grows. Do not mark the report as full KiCad parity until arcs, circles, rectangles, polygons, ellipses, hole nesting, disjoint outlines, endpoint tolerance, self-intersection diagnostics, and footprint Edge.Cuts holes are implemented and covered. Coverage lives in `tests/test_board_outline_polygon.cpp` and `tests/test_cli.cpp`; the focused green selectors are `board_outline_polygon` and `cli`.

## Sprint 226 KiCad Cross-Probing Packet Handover

The deterministic KiCad PCB editor source walk reached `F:\kicad_src\pcbnew\cross-probing.cpp`. KiCad's PCB side accepts cross-probing command packets such as `$NET`, `$NETS`, `$PART`, `$PAD`, `$SELECT`, and `$CLEAR`, then uses Kiway/socket mail and editor frame state to select, highlight, clear, zoom, and synchronize items between Pcbnew and Eeschema.

CCad maps the first headless, agent-usable slice into `src/ccad_core/cross_probing.hpp/.cpp`. The main API is `resolveCrossProbePacket(const Project&, std::string_view)`. It returns `CrossProbeReport` with KiCad source metadata, packet kind, requested nets, target rows, diagnostics, clear-highlight state, selection focus state, and explicit pending KiCad features. The implementation resolves net packets to schematic net rows plus board pads, vias, tracks, and zones on the same net. It resolves part and pad packets to placed board footprints, schematic components, and board pads. It resolves `$SELECT` entries for `F<reference>` and `P<reference>/<pad>` and records unsupported sheet entries as diagnostics.

The CLI surface is `ccad pcb cross-probe --file <project.ccad.json> --packet <packet>`. It is a reporting command only and does not mutate the project. Focused coverage lives in `tests/test_cross_probing.cpp` and `tests/test_cli.cpp`. The verified commands were `cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && ctest --test-dir build-qt -R cross_probing --output-on-failure"` and `cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && ctest --test-dir build-qt -R cli --output-on-failure"`.

Do not treat this as complete KiCad cross-probing parity. Remaining work includes live GUI highlight and clear operations, cross-editor selection state, zoom/center behavior, Kiway/socket dispatch, sheet-path prefix support, full net-chain highlighting, DRC/config/custom-rule remote packets, and schematic editor reciprocal packet formatting.

## Sprint 224 KiCad Board Bounding Box Handover

The deterministic KiCad PCB editor source walk reached `F:\kicad_src\pcbnew\board_bounding_box.cpp` and `F:\kicad_src\pcbnew\board_bounding_box.h`. KiCad's `BOARD_BOUNDING_BOX` is an `EDA_ITEM` wrapper around a shared `BOX2I`; it returns `BOARD_BOUNDING_BOX` from `GetClass()`, exposes `LAYER_BOARD_BOUNDING_BOX` from `ViewGetLayers()`, and sets `SKIP_STRUCT`, so it is view/runtime metadata rather than a serialized board object.

CCad's durable source of truth remains `Board::outline`. The analogue lives in `src/ccad_core/canvas.hpp/.cpp` as `CanvasBoardBoundingBox`, carried by `CanvasScene::board_bounding_boxes`. `buildCanvasScene(const Board&)` derives one item with `id:"board.bounding_box"`, `class_name:"BOARD_BOUNDING_BOX"`, `layer_id:"LAYER_BOARD_BOUNDING_BOX"`, `skip_struct:true`, and millimeter coordinates from the outline. `src/ccad_cli/pcb_object_queries.cpp` mirrors that metadata through `pcb get-outline` with `kicad_class`, `kicad_view_layer`, `kicad_skip_struct`, and `bounding_box` fields for headless agents.

Coverage lives in `tests/test_canvas.cpp` and `tests/test_cli.cpp`. The intended red failure was the missing `CanvasScene::board_bounding_boxes` member. Focused green checks are the `canvas` and `cli` CTest selectors after building `ccad_canvas_tests` and `ccad_cli_tests`.

The KiCad source walk then reached `F:\kicad_src\pcbnew\board_commit.cpp` and `F:\kicad_src\pcbnew\board_commit.h`. KiCad's `BOARD_COMMIT` is a tool/editor transaction boundary that stages board-item changes, updates undo, view, connectivity/ratsnest, zones, teardrops, component-class caches, board-outline refresh, solder-mask display, and dirty notifications. CCad maps the currently useful headless analogue into `src/ccad_core/transaction.hpp/.cpp`: `Transaction` now contains `CommitImpact`, derived from `ProjectDiff`, and `dumpTransactionJson()` emits an `impact` object for CLI audit logs and future agent refresh decisions.

Coverage for the commit-impact analogue lives in `tests/test_transaction.cpp`. The intended red failure was the missing `Transaction::impact` member; the focused green check is the `transaction` CTest selector after building `ccad_transaction_tests`.

The KiCad source walk then reached `F:\kicad_src\pcbnew\board_connected_item.cpp` and `F:\kicad_src\pcbnew\board_connected_item.h`. KiCad's `BOARD_CONNECTED_ITEM` is the shared connected-board-item base for net-owned objects. CCad does not yet have the full KiCad connectivity graph or durable netclass model, so the analogue is deliberately limited to explicit metadata on the connectable query/export rows that already exist.

`src/ccad_cli/pcb_object_queries.cpp` owns this connected-item metadata through `appendConnectedItemMetadata()`. Pads, vias, tracks, and zones returned by `pcb list-objects`, `pcb list-by-net`, `pcb list-connected`, direct object lookup, and `pcb export-route-job` now include `connected_item:true`, `kicad_connected_class:"BOARD_CONNECTED_ITEM"`, net-name fields, `net_class_name:"Default"`, `net_class_scope:"default_netclass_until_model_exists"`, the compatibility alias `netclass_scope`, `local_ratsnest_visible:true`, and `teardrops_supported` flags. Tests live in `tests/test_cli.cpp`; the red failure that proved the contract checked the missing agent-facing `net_class_scope` key.

## Sprint 227 Addendum

Sprint 227 added `src/ccad_core/fix_board_shape.hpp/.cpp` which implements the `ConnectBoardShapes` logic. It utilizes a KD-tree powered by `src/ccad_core/nanoflann.hpp` to maintain the optimized $O(N \log N)$ closest-point spatial search complexity. The KD-tree indexing preserves KiCad's performance while porting the core algorithm into the CCad kernel. 


## Sprint 228 Addendum

Sprint 228 evaluated `initpcb.cpp` through `padstack.cpp` from the KiCad source walk. GUI elements, redundant net models, and complex advanced-routing padstacks (KiCad 8) were deferred to the backlog. No core logic was added, keeping CCad's kernel thin and preserving the flat Pad/Track model.


## Sprint 229 Addendum

Sprint 229 evaluated `pcb_barcode.cpp` through `project_pcb.cpp` from the KiCad source walk. GUI elements, rendering painters, and core primitives already handled by CCad (outline, tracks, pads, shapes) were omitted. Advanced geometry elements like barcodes, dimensions, fields, groups, plotting, tables, and standalone text were deferred to the backlog. No core logic was added.


## Sprint 230 Addendum

Sprint 230 concluded the KiCad source walk. Files evaluating zones, undo/redo transactions, and toolbars were omitted because CCad handles transactions natively and GUI states via Qt. Copper pours (zones) and advanced track merging were deferred to the backlog.


## Sprint 231 Addendum

Sprint 231 fixed the CI/CD pipeline by un-ignoring the agent visual workflow file in `.gitignore`, ensuring `visual_harness_policy` tests pass on remote runners.


## Sprint 232 Addendum

Sprint 232 began the KiCad schematic source walk in `eeschema`. Files evaluating BOM plugins and cross-probing were omitted because CCad handles these natively. Advanced schematic hierarchy features like connection graphs and junctions were deferred to the backlog.

Sprint 232 continuation restored schematic symbol snapshot persistence after the schematic model rename. `src/ccad_core/model.hpp` now lets `SchSymbol` carry an optional embedded `Symbol` snapshot, `src/ccad_core/placement.cpp` stores the selected imported symbol during `placeComponent()`, `src/ccad_core/serialize.cpp` accepts both legacy `components` and current `symbols` arrays while writing the embedded snapshot, and `src/ccad_core/canvas.cpp` expands the snapshot through `buildCanvasScene(const Symbol&)` into transformed schematic canvas primitives. Compatibility readers still accept legacy component `part` and pin `kind` fields so older local fixtures do not break.

The visual proof for that continuation is `artifacts\screenshots\sprint232-schematic-symbol-snapshot.png`, captured from `artifacts\demos\sprint232-schematic-symbol-snapshot.ccad.json` with empty stderr. The broader GUI survival proof is `artifacts\screenshots\sprint232-symbol-snapshot-proof-internal-20260629-181543.png`.


## Sprint 233 Addendum

Sprint 233 continued the KiCad schematic source walk. Redundant application lifecycle files, GUI grids, and JSON/serialization elements were omitted. Schematic graphics imports and junction generation heuristics were deferred to the backlog.


## Sprint 234 Addendum

Sprint 234 concluded the KiCad schematic source walk. Redundant application GUI files, Qt-handled GAL painters, and JSON/serialization elements were omitted. Schematic primitives (symbols, pins, wires) and hierarchical net algorithms were deferred to the backlog.


## Sprint 235 Addendum

Sprint 235 began the KiCad Gerber Viewer source walk in `gerbview`. Files evaluating wxWidgets GUI events and file loaders were omitted. Gerber aperture macros, D-code definitions, and Excellon drill parsers were deferred to the backlog.


## Sprint 236 Addendum

Sprint 236 concluded the KiCad Gerber Viewer source walk. Redundant application GUI files and legacy PNG export modules were omitted. Gerber parsing primitives, RS-274X syntax engines, and polygon flash generators were deferred to the backlog.


## Sprint 237 Addendum

Sprint 237 concluded the KiCad 3D Viewer source walk. Redundant application GUI files, OpenGL context wrappers, and hardware mouse inputs were omitted. 3D parsing caches (STEP/IGES), scene graphs, and raytracing shaders were deferred to the backlog.


## Sprint 238 Addendum

Sprint 238 concluded the KiCad Footprint Assignment (cvpcb) source walk. Redundant application GUI files and standalone inter-process IPC were omitted. Netlist bridging and auto-association logic were deferred to the backlog.


## Sprint 239 Addendum

Sprint 239 concluded the KiCad Autorouter and Interactive Router source walk. Redundant application GUI tools and GAL bindings were omitted. Core PNS algorithms, differential pair routing, length tuning meanders, and component spreading heuristics were deferred to the backlog.


## Sprint 240 Addendum

Sprint 240 concluded the KiCad External EDA Formats source walk. Redundant native s-expression and legacy format serialization handlers were omitted. Binary parsers and translation layers for Altium, Eagle, Allegro, Cadstar, and other third-party formats were deferred to the backlog.


## Sprint 241 Addendum

Sprint 241 concluded the KiCad auxiliary tools source walk. Redundant application GUI files and standalone application loops were omitted. Core page layout designs, transmission line math, and raster-to-polygon engines were deferred to the backlog.


- pcb_barcode.cpp, pcb_dimension.cpp, pcb_group.cpp, pcb_reference_image.cpp, pcb_table.cpp, pcb_target.cpp, pcb_text.cpp from KiCad have been mapped to their respective Board objects in src/ccad_core/model.hpp and exposed via the CLI and GUI rendering layer.

## Sprint 244 Addendum

Sprint 244 implemented the footprint losslessness harness in `ccad_core` and the CLI:
- **Harness**: Developed `verifyFootprintLosslessness` in `src/ccad_core/footprint_losslessness.cpp` to perform structural comparisons of properties, pads, shape constraints, and layer sets.
- **CLI**: Added the `lib verify-footprint-losslessness` subcommand to compare imported vs candidate footprints and emit structured JSON diagnostics.
- **Tests**: Created unit tests in `tests/test_footprint_losslessness.cpp` verifying name, pad count, and shape mismatch detection.


## Sprint 245 Addendum

Sprint 245 implemented KiCad footprint oval drill support in `ccad_core`, placement logic, and the losslessness verification harness:
- **Data Structures**: Added `drill_height` and `drill_shape` optional fields to `FootprintPad` in `src/ccad_core/footprint.hpp`.
- **Importer/JSON**: Updated `src/ccad_core/kicad_footprint_import.cpp` parser to read `(drill oval width height ...)` syntax and serialize/deserialize `drill_height_nm` and `drill_shape` JSON properties.
- **Placement**: Updated `ccad_core::placeFootprint` in `src/ccad_core/placement.cpp` to correctly assign height and shape to placed padstacks when using oval drills.
- **Losslessness Verification**: Extended `src/ccad_core/footprint_losslessness.cpp` to check for drill height and shape mismatches.
- **Tests**: Added dedicated oval drill import and losslessness check test cases in `tests/test_kicad_footprint_import.cpp` and `tests/test_footprint_losslessness.cpp`. Verified all 57 CTest suites pass successfully.


## Sprint 246 Addendum

Sprint 246 implemented Unicode/UTF-8 path support on Windows for the command-line interface and internal filesystem calls:
- **Helpers**: Added `src/ccad_core/filesystem_u8.hpp` containing `ccad::u8ToPath` and `ccad::pathToU8` conversion helpers.
- **CLI Entry**: Updated `src/ccad_cli/main.cpp` on Windows to intercept the command line using `GetCommandLineW` and `CommandLineToArgvW` to preserve Unicode characters, converting them to UTF-8 before dispatch.
- **File Streams**: Migrated all stream constructors (`std::ifstream` and `std::ofstream`) in `src/ccad_cli/common.cpp`, `src/ccad_cli/lib_commands.cpp`, `src/ccad_cli/project_commands.cpp`, `src/ccad_cli/pcb_commands.cpp`, and `src/ccad_cli/agent_session.cpp` to use `ccad::u8ToPath`.
- **Library Catalog**: Updated `src/ccad_core/library_catalog.cpp` to map native paths through `ccad::u8ToPath`.
- **Verification**: Verified importing symbols with Unicode names successfully on Windows (e.g. `π120U30.kicad_sym`), ran full CTest suite (100% pass), and confirmed the GUI renders correctly via the visual validation harness.


## Sprint 247 Addendum

Sprint 247 implemented a fast spatial grid index for canvas objects in the native review GUI to optimize nearest-neighbor searches:
- **Spatial Grid**: Created `src/ccad_gui/spatial_index.hpp` implementing `CanvasSpatialIndex` with a custom grid cell partition, populating graphics items by overlapping grid cells.
- **Cache Integration**: Linked `CanvasSpatialIndex::rebuild` into `ReviewWindow::rebuildUiMapIndexCache` to automatically re-index the canvas items whenever a UI map epoch update occurs.
- **JSON Queries**: Updated the `ui.nearest_canvas_object` JSON query in `ReviewWindow::uiNearestCanvasObjectJson` to fetch candidates using `spatial_index_.queryNearest(scene_point)` instead of traversing all scene graphics items.
- **Tests**: Created a unit test suite in `tests/test_canvas_spatial_index.cpp` verifying spatial grid insertion, range queries, and coordinate logic. Verified all 58 CTest targets pass.


## Sprint 248 Addendum

Sprint 248 extended the deterministic GUI action capabilities inside the review window automation layer:
- **Scroll Operations**: Added `uiScrollJson` to find and programmatically adjust `QScrollBar` values in vertical or horizontal directions on the canvas viewport and target panels.
- **Key Sequences**: Updated `uiKeyJson` using `QKeySequence` to synthesize key press and release events containing modifier keys (e.g. `Ctrl`, `Shift`, `Alt`) to support richer short-cuts.
- **Selection double-clicks**: Extended `uiClickJson` to support double clicks on QAbstractItemView components (e.g. QListView/QTreeView) and canvas objects by sending matching events.
- **Dialog/Menu Automation**: Automated finding active QDialog and QMenu actions inside the target window, allowing the automation harness to trigger button clicks and actions.
- **Property inspector**: Added `uiEditPropertiesJson` allowing properties modification.





## Current handover: Sprint 358 DRC provider parity

`src/ccad_core/drc.cpp` now owns typed checks for through-hole pad drill presence and minimum size, pad copper-to-board-edge clearance, and via-to-via drilled-hole clearance. Tests live in `tests/test_drc.cpp`; KiCad source comparison is recorded in `scratch/055-056_drc_provider_to_ccad_implementation.md` and the sprint record. The latest proof is `artifacts/screenshots/sprint358-hole-proof-20260916-025459.png`; provider credentials remain unavailable by design, while the agent panel preserves local CCad functionality.
## Sprint 359 handover

`src/ccad_core/3d_fastmath.cpp` now provides correct `FastMath3D::fastSin` and `fastCos` behavior through `<cmath>`, covered by `tests/test_3d_fastmath.cpp` and CTest `3d_fastmath`. No performance promise is made until a measured approximation contract exists.
## Sprint 360 handover

`Math3D::transform` in `src/ccad_core/3d_math.cpp` now applies matrix scale/translation and homogeneous `w` normalization; coverage is in `tests/test_3d_fastmath.cpp`. There is no 3D GUI integration claim yet.
## Sprint 361 handover

`NearestNeighborConnectivity::computeOptimalRatnests` now returns a deterministic minimum-spanning tree for supplied `RatnestNode` values. It is core-only; `DynamicRatnestGraph` population and GUI rendering remain separate pending a typed connectivity integration.
## Sprint 362 handover

`DynamicRatnestGraph::buildGraph` now creates deterministic per-net nodes from board pads, vias, and track segment endpoints. Track nodes use `<track-id>:start` and `<track-id>:end`; empty net/id entries are omitted. Connected-component subtraction and GUI overlay are not yet implemented.
## Sprint 363 handover

`ReviewWindow::addRatsnestOverlays` now calls `NearestNeighborConnectivity` over canvas pads, vias, and track endpoints, then renders canonical MST edges. Toggle behavior remains `action:show_ratsnest`; physical subtraction of already-routed connections is intentionally not claimed.
## Sprint 364 handover

`EventDrivenRatnest::onBoardModified` now rebuilds graph nodes, enumerates board nets, computes MST edges, and publishes them to `DynamicRatnestGraph`. The GUI consumes the same core MST during render; UI-map count assertions and routed-component subtraction remain next.
## Sprint 365 handover

UI-map target lookup now handles mnemonic menu names, the Agent dock member, QTextEdit controls, and stable agent composer object names. The current official target sequence reports 15/15 found in both passes.
## Sprint 367 handover

`DesignRules.max_track_width` and `max_via_diameter` are persisted and configurable via CLI; zero disables each maximum. DRC emits `TRACK_TOO_WIDE` and `VIA_DIAMETER_ABOVE_MAXIMUM`. Validation rejects a positive maximum below its corresponding minimum.
## Sprint 368 handover

CLI help now advertises both maximum-rule flags. `test_cli` verifies emitted JSON fields, while `test_serialize` verifies exact load/dump round trips. Official visual proof remains clean; continue ordered scratch survey at 058.
## Sprint 369 handover

`TrackLengthTuning::calculateCurrentLength` now returns net-scoped track length in millimetres and is covered by `track_length_tuning`. `applyTuning` still returns unsupported until meander geometry and transaction APIs are defined.
## Sprint 370 handover

Length measurement includes matching `TrackArc` geometry using three-point circular sweep; collinear input falls back to two chords. Meander mutation remains explicitly unsupported.
## Sprint 371 handover

Net length measurement optionally includes board-thickness contribution per matching via, controlled by `DesignRules.use_height_for_length_calcs`; this is a full-board via approximation until blind/buried via layer spans exist.
## Sprint 372 handover

DRC now emits `SILK_CLEARANCE` when axis-aligned F/B.SilkS text box overlaps configured clearance around copper pads. Rotation-aware text geometry and silk-via/outline/zone checks are not yet implemented.
## Sprint 373 handover

Silkscreen text clearance also checks valid copper vias. Text geometry remains axis-aligned; silk outline/zone checks remain pending.
## Sprint 374 handover

Silkscreen text clearance now checks valid copper track segments using track half-width. Rotation-aware text and silk outline/zone checks remain pending.
## Sprint 375 handover

Silkscreen text edge clearance now checks bounding-box corners against board outline edge distance. Rotation-aware geometry and silk-to-zone checks remain pending.
## Sprint 376 handover

Silkscreen text clearance now checks copper zone polygon overlap/proximity. Text model remains axis-aligned and does not yet represent glyph strokes.
## Sprint 377 handover

Complete silk clearance chain is verified across pads, vias, tracks, board edge, and zones. Next parity work must first define BoardFootprint-to-SchSymbol reference semantics.
## Sprint 379 handover

`checkSchematicFootprintParity` compares annotated schematic refs with explicit board footprint refs, plus pad component IDs for metadata-light boards. Emits missing, extra, duplicate diagnostics. DNP/BOM and pin mapping remain next.
## Sprint 380 handover

Parity now emits `FOOTPRINT_BOM_PARITY` when matching schematic `in_bom` and board `exclude_from_bom` disagree. Pin-level membership remains next.
## Sprint 381 handover

Explicit board footprints now require matching pads for on-board schematic pins; missing mappings emit `MISSING_PAD`. Pad-only imported boards remain metadata-light and are not over-constrained.
## Sprint 382 handover

Schematic parity respects `SchSymbol.on_board`; off-board symbols are excluded from missing-footprint and missing-pad requirements.
## Sprint 383 handover

`checkSolderMaskBridges` emits `SOLDERMASK_BRIDGE` for different non-empty nets whose valid copper pad mask boxes leave less than `DesignRules.solder_mask_min_width` web after global `solder_mask_expansion`. Current geometry is axis-aligned and pad-only; NPTH, per-pad overrides, and exact apertures remain deferred.
## Sprint 384 handover

`DesignRules::min_text_height` persists through project JSON and `pcb set-rules --min-text-height-mm`. `checkBoardTexts` emits `TEXT_HEIGHT_BELOW_MINIMUM` for positive board text whose declared height is below the rule. Text stroke thickness is not inferable from current `BoardText`; add an explicit stroke-width field before implementing KiCad-like thickness/glyph checks.
## Sprint 385 handover

`BoardText::mirrored` persists through JSON and is optionally set by `pcb add-text --mirrored`. `checkBoardTexts` emits `MIRRORED_TEXT_ON_FRONT_LAYER` for mirrored non-`B.` text and `NONMIRRORED_TEXT_ON_BACK_LAYER` for unmirrored `B.` text. Future layer classification should use canonical layer metadata instead of prefix-only fallback.
## Sprint 386 handover

`DesignRules::min_track_angle_degrees` and `max_track_angle_degrees` persist through JSON and `pcb set-rules`. `checkTrackAngles` emits `TRACK_ANGLE` for same-net, same-layer straight segments sharing an exact endpoint outside the configured range. Arc junctions and custom-rule scoping remain future work.
## Sprint 387 handover

`DesignRules::min_track_segment_length` and `max_track_segment_length` persist through JSON and `pcb set-rules`. `checkTrackSegmentLengths` emits `TRACK_SEGMENT_LENGTH` for straight `TrackSegment` items outside configured bounds. `TrackArc` items remain unhandled by this DRC provider.
## Sprint 388 handover

`checkTrackSegmentLengths` now measures `TrackArc` items using three-point circular sweep, with two-chord fallback for collinear points, and emits `TRACK_SEGMENT_LENGTH` against the same bounds as straight segments.
## Sprint 389 handover

BoardText stroke_width and DesignRules min_text_thickness persist through JSON. CLI sets declared stroke width; DRC emits TEXT_THICKNESS_BELOW_MINIMUM when enabled and missing or undersized. Font-outline collapse analysis remains deferred.
## Sprint 390-391 handover

`DrcTestProviderClearance` now reports different-net pad/pad, pad/via, and via/via proximity using copper clearance and conservative circularized pad extents. `tests/test_cli.cpp` escapes dollar tokens on POSIX shells so KiCad-style text variables reach CCad unchanged. Forward declarations for model structs now use `struct`, math constants are portable under warnings-as-errors, and explicit casts satisfy MSVC. Full integration DRC remains authoritative for production diagnostics; provider is independently tested.
## Sprint 392 handover

`DrcTestProviderEdgeClearance` reports error code 4 for pads and vias whose conservative copper radius plus `copper_edge_clearance` reaches a rectangular board edge. It uses `distancePointToSegment` and converts nanometers to millimeters before comparison. Track and polygon edge providers remain separate backlog work; provider is not yet wired into primary DRC aggregator.
## Sprint 393 handover

Primary `runDrc` now checks via center-to-edge distance minus via radius and emits `VIA_EDGE_CLEARANCE`; GUI diagnostics consume this normal `Diagnostic` path. Target proof used a rebuilt `ccad.exe` and `ccad_gui.exe` on a moved near-edge via, confirming JSON diagnostics and rendered board state. Pad edge diagnostics already existed in `checkPads`.
## Sprint 394 handover

`DrcTestProviderUnrouted` groups pad/via endpoints by net, adds track endpoints, unions exact-coordinate track connections, and reports an unrouted net when physical nodes remain in multiple components. Sprint 395 also wires equivalent deterministic checking into primary `runDrc`, producing `UNROUTED_NET` diagnostics consumed by CLI review and Qt diagnostics/markers. Track arcs remain a follow-up connectivity case.

## Sprint 395 handover

Primary DRC now calls `checkUnroutedPhysicalNets` after physical geometry checks. It uses exact nanometre endpoint equality and ignores empty-net physical items. Regression lives in `tests/test_drc.cpp`; valid fixture connectivity remains clean, while two disconnected `N1` pads produce object id `N1`. Qt diagnostics already consumes `runDrc`, so no GUI-specific adapter was required. Visual proof: `artifacts/screenshots/sprint395_unrouted_primary_proof-20260916-112440.png` and its teardrop-before companion; DRC proof: `artifacts/demos/sprint395_unrouted_primary_proof.drc.json`.

## Sprint 396 handover

`NetTieDrc` now implements its first model-supported slice. Registered ties require pads for both declared nets on the component; intersections are permitted only inside their axis-aligned span; missing-net ties are reported. Exact courtyard geometry, track/zone bridge validation, and aggregate DRC integration remain deferred because `BoardFootprint` lacks a courtyard/bridge-region model.

## Sprint 397 handover

`RouterTool` now owns a minimal route gesture: start/update store an in-progress segment, commit appends a deterministic `TrackSegment` with F.Cu/B.Cu selection and 0.25 mm width, cancel discards it, and `routeTrack` performs the complete gesture. Net selection, snapping, multi-segment routing, and GUI action wiring remain next.

## Sprint 398 handover

`ReviewWindow::commitTrackPlacementForAutomation` now invokes `RouterTool` directly for semantic `ui.route_track` calls, then assigns active net/layer metadata, saves, and re-renders. Live proof used `scripts/live_agent_route_demo.py`: 42 route calls all returned `performed=true`; `inspect` reported 42 tracks and 6 vias. Target screenshot: `artifacts/screenshots/sprint398-live-route-target.png`.

## Sprint 399 handover

`RouterTool::setActiveNet` enables same-net endpoint snapping during `updateRouting`; nearest pad/via within 0.75 mm becomes route endpoint, while empty active-net state permits generic routing. Commit writes the active net directly. GUI semantic route path supplies active net before gesture execution. Snapping remains point-only; arc/multi-segment and clearance-aware routing remain next.

## Sprint 400 handover

`RouterTool::routeTrack` splits diagonal requests at `(end_x, start_y)`, commits both connected segments, and preserves one-segment behavior for horizontal/vertical routes. `ReviewWindow::uiWorkflowRouteTrackJson` now calls the shared route commit path rather than simulating two canvas clicks, so semantic GUI routes receive multi-segment behavior. Live proof: 42 calls, 84 persisted tracks.

## Sprint 401 handover

`RouterTool::commitRouting` rejects a candidate segment when its centerline approaches a different-net pad closer than board copper clearance plus the fixed 0.125 mm route half-width. Rejection clears in-progress state without mutating tracks. This is pad-only first slice; track/via/zone obstacle indexing and route error response remain next.
## Sprint 402 handover (2026-09-16)

`RouterTool` now checks existing same-layer tracks belonging to another net before committing a segment. Collision uses nanometre segment intersection plus endpoint-to-segment clearance, so distant collinear tracks do not falsely block. `routeTrack` accepts an optional layer and stops atomically when a Manhattan segment is rejected; GUI semantic route calls pass the selected copper layer. Pad and track obstacle checks remain intentionally local; vias, zones, arcs, and richer width-aware clearance remain backlog work.
## Sprint 403 handover (2026-09-16)

`RouterTool::commitRouting` rejects a candidate segment near a different-net via using via radius plus copper clearance and route half-width. Since `Via` currently models no layer span, check applies on F.Cu and B.Cu. This is conservative for through vias; blind/buried span-aware routing remains deferred.
## Sprint 404 handover (2026-09-16)

`RouterTool::commitRouting` checks `BoardZone` outlines for filled zones with a different net on the active layer. Candidate segments are rejected on polygon crossing, endpoint interior, or boundary-clearance proximity; zone clearance plus route half-width is used. Zone holes, filled-island topology, priority interactions, and thermal relief semantics remain deferred.
## Sprint 405 handover (2026-09-16)

`RouterTool::commitRouting` checks different-net `TrackArc` items on the active layer. Collision uses start-mid and mid-end chord intersections plus clearance to start, mid, and end vertices, including arc width. This is a conservative approximation; exact swept-circle geometry and arc endpoint connectivity remain deferred.
## Sprint 406 handover (2026-09-16)

`segmentTouchesArc` samples the quadratic curve from `TrackArc.start` through `mid` to `end` at 16 intervals, checks each envelope segment for intersection, and checks sampled vertices against route clearance. This catches curved-span crossings missed by endpoint-only checks. It is intentionally conservative and not yet KiCad-equivalent exact circular-arc geometry.
## Sprint 407 handover (2026-09-16)

`segmentTouchesArc` derives the circumcenter, radius, orientation, and midpoint-containing sweep from `TrackArc.start/mid/end`; it samples that circular sweep into 16 segments and checks route intersection plus sampled-point clearance. Collinear arcs use quadratic fallback. Adaptive sampling and exact analytic arc-to-segment distance remain future precision improvements.
## Sprint 408 handover (2026-09-16)

Arc collision now derives circular geometry from three points, chooses orientation from the signed cross product, and unwraps sweep through the midpoint before sampling 16 arc intervals. This aligns obstacle shape with stored circular TrackArc semantics; clearance still uses sampled vertices/segments and can be made adaptive later.
## Sprint 409 handover (2026-09-16)

`AgentPanel::appendChatMessage` classifies provider-unavailable/status notices by text prefix and assigns `agentRole=noticeCard`; both standalone AgentPanel and ReviewWindow styles define compact amber warning treatment. Provider messaging remains local-status only and does not expose secrets or enable network probing.
## Sprint 410 handover (2026-09-16)

`RouterTool::routeBlocked()` exposes the last gesture's obstacle result. `ReviewWindow::commitTrackPlacementForAutomation` maps a no-new-track result to `blocked_obstacle` when this flag is set, otherwise preserving `zero_length`; successful routes remain `placed`. Response schema remains version 1 with track count.
## Sprint 411 handover (2026-09-16)

`RouterTool::blockedReason()` exposes the guard that rejected the latest gesture. `ReviewWindow::commitTrackPlacementForAutomation` preserves schema v1 and returns `blocked_obstacle_<class>` for pad, via, arc, zone, or track rejection, making retry/planning decisions actionable without exposing board secrets.
## Sprint 412 handover (2026-09-16)

Track collision margin is `board copper clearance + route half-width + existing track half-width`; centerline intersection remains an immediate rejection. This improves width-aware safety for imported/wide traces. Exact polygonal copper shape and per-net rule overrides remain future work.
## Sprint 413 handover (2026-09-16)

`tests/test_router_width_clearance.cpp` is the focused contract for existing-track width margin. CMake registers it as `router_width_clearance`; it constructs a 4 mm different-net F.Cu track, attempts a nearby N1 route, and verifies no mutation plus `routeBlocked()`. Keep this separate test when changing clearance math.
Sprint 414 added serialized `BoardZone::holes` and hole-aware routing exceptions, preserving conservative zone boundaries. Router portability now uses explicit TrackArc coordinate casts and consistent `Project` struct declarations; width-aware track distances convert geometry nanometres to millimetres before comparing rule clearance. Core warnings-as-errors and Qt test gates were revalidated.
Sprint 415 made the CLI cross-probe test safe against POSIX `$` expansion and removed the invalid orphan gitlink `assets/vscode-copilot-release`; this prevents false Linux test failures and checkout cleanup warnings.
Sprint 416 adds `BoardFootprint::front_courtyard` and `back_courtyard` polygon arrays, persisted only when non-empty, plus `DrcTestProviderCourtyard` polygon overlap detection. Polygon coordinates are currently CCad board coordinates; KiCad local-coordinate import and malformed/missing policy remain backlog items.
Sprint 419 adds typed `CanvasFootprint` scene identities; Sprint 420 wires Edit > Find to select those footprint records by case-insensitive reference and refresh the inspector/status path. Canvas footprint rendering remains a review marker, not yet full KiCad graphics.
Sprint 421 `importSpecctraSes` carries active `(net ...)` context through nested route nodes and assigns it to created `TrackSegment` and `Via` records. SES padstack dimension resolution is deferred because current API receives session text without DSN library context.
Sprint 423 extends `importSpecctraSes` to index route-library padstacks and derive via diameter from circular shapes. Drill remains 0.3 mm unless future ID parsing or DSN context supplies a value.
Sprint 424 parses the KiCad Specctra drill convention `<prefix>:<drill>_mil` in SES via padstack IDs, converting mils to nanometres; IDs without this convention retain the 0.3 mm fallback.
Sprint 425 adds optional `Via::start_layer_id` and `end_layer_id`, serialized when present; SES padstack circle layers infer the span in source order. Existing callers omit fields and retain prior through-via assumptions.
Sprint 426 validates referenced SES via padstacks: known definitions require at least one circular shape, while absent definitions retain legacy diameter/drill fallback.
Sprint 427 keeps new `Via` layer-span fields aggregate-initializer compatible through default values and trailing placement; existing designated callers need no edits.
Sprint 428 adds serialized `Via::via_type` (default `through`); SES inference marks microvias by small drill, through vias by F.Cu/B.Cu span, blind vias when one endpoint is outer copper, and buried vias otherwise.
Sprint 429 exposes `via_type`, `start_layer_id`, and `end_layer_id` in `pcb get-object` via JSON and includes `via_type` in board object summaries.
Sprint 430 renders via type and layer span in `SelectionInspectorPanel`; fields are read-only display metadata while existing diameter/drill editing remains unchanged.
Sprint 431 `checkVias` validates `via_type` vocabulary and requires distinct known layers for non-through vias, emitting `VIA_TYPE_INVALID`, `VIA_LAYER_SPAN_INVALID`, or `VIA_LAYER_SPAN_UNKNOWN_LAYER`.
Sprint 432 Excellon export writes `; CCAD_VIA ...` comments for non-through/span-aware vias; Excellon tool paths remain unchanged because standard format lacks native via-type semantics.

Sprint 433 Excellon export validates non-through via spans before emitting output, preventing incomplete blind, buried, or microvia metadata from reaching manufacturing files.

Sprint 434 `JobManager` tracks active tasks and uses a completion condition variable, so `waitAll()` cannot return while a worker is still executing. Regression coverage is in `tests/test_job_manager.cpp`.

Sprint 435 `JobManager` catches exceptions raised by background tasks, reports them to stderr, and keeps worker threads alive for subsequent tasks; active-task completion bookkeeping remains guaranteed.

Sprint 436 `AgentRunner::load_queue()` restores pending goals from its own serialized JSON schema, including nested context and tool-argument values, with structural validation before enqueueing.
