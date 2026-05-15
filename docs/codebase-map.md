# CCad Codebase Map

This is the first file a memory-loss agent should read after `AGENTS.md`. It explains what each module owns, what not to touch casually, and which functions are the current public seams.

## Current Progress

- Phase: 2 / 6
- Last merged sprint: Sprint 27, GUI CAD editor shell foundation
- Next sprint: Sprint 28, GUI module split and selection groundwork
- Active branch pattern: `sprint-<n>-<topic>`
- Current source of truth for phase/sprint counter: `docs/devops/progress.md`
- Main product direction: native C++ PCB kernel and machine-callable CLI first; Qt GUI is a human review/editor client, not the data owner.

## Hard Rules

- Do not make the GUI own design state. GUI renders `ccad_core` models.
- Do not execute design/library file contents. Treat project JSON and KiCad files as data only.
- Write tests before behavior changes.
- Run full gate before commits and merges:

```powershell
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
```

- Every commit/merge status update must include:

```text
Progress: Phase X/Y, Sprint N, <branch>, <status>
```

- Commits must have meaningful multi-line bodies. Minimum structure:

```text
<type>: <specific summary>

Progress: Phase X/Y, Sprint N, <branch>, <status>.

Why:
- Problem or feature being addressed.

Changed:
- File/module: what changed and why.

Behavior:
- User-visible or agent-visible behavior change.

Verification:
- Exact commands run and result.

Demo:
- Screenshot/artifact path when relevant.
```

- Demo/screenshot outputs go under ignored `artifacts/`.

## Build Targets

### `ccad_core`

Native C++ kernel library. All durable model, validation, serialization, import, diff, transaction, and canvas-scene logic belongs here.

### `ccad`

Machine-callable CLI. It should be deterministic, scriptable, and safe for LLM/tool use.

### `ccad_gui`

Optional Qt 6 Widgets GUI. It is for human review and visual feedback. It must stay thin.

### Test Targets

- `serialize`: project JSON and model round-trip tests.
- `erc`: logical electrical-rule diagnostics.
- `drc`: physical board diagnostics.
- `kicad_footprint_import`: KiCad footprint import and CCad footprint JSON tests.
- `library_catalog`: native catalog/provenance JSON tests.
- `cli`: black-box CLI behavior.
- `review`: human review summary model.
- `diff`: project diff model.
- `transaction`: transaction journal model.
- `geometry`: unit conversion and geometric helpers.
- `canvas`: GUI-independent board canvas scene model.

## Core Files

### `src/ccad_core/model.hpp`

Owns the central project data structures.

Important structs:

```cpp
struct Project {
  int schema_version;
  std::string id;
  std::string name;
  std::optional<Board> board;
  std::vector<Component> components;
  std::vector<Net> nets;
  std::vector<Constraint> constraints;
};
```

```cpp
struct Board {
  Rect outline;
  std::vector<Layer> layers;
  std::vector<Keepout> keepouts;
  std::vector<Pad> pads;
  std::vector<Via> vias;
  std::vector<TrackSegment> tracks;
};
```

```cpp
struct Keepout {
  std::string id;
  std::string kind;
  Rect area;
};
```

```cpp
struct Pad {
  std::string id;
  std::string component_id;
  std::string pin_name;
  std::string net_id;
  std::string layer_id;
  Point position;
  double rotation_degrees;
  Size size;
};
```

Rules:

- `Board::pads` are placed physical pads.
- `Board::keepouts` are rectangular forbidden regions checked by DRC.
- `FootprintPad` is reusable library geometry and lives in `footprint.hpp`.
- Add fields carefully: update JSON, canvas, DRC, CLI, tests, docs.

### `src/ccad_core/geometry.hpp/.cpp`

Owns physical units and simple geometry.

Public functions:

```cpp
Length nanometers(std::int64_t value);
Length millimeters(double value);
Length mils(double value);
Point maxPoint(const Rect& rect);
```

Rule:

- Store durable lengths as integer nanometers.
- Convert mm/mil only at CLI/import/UI boundaries.

### `src/ccad_core/serialize.hpp/.cpp`

Owns CCad project JSON.

Public functions:

```cpp
Project loadProjectJson(std::string_view source);
std::string dumpProjectJson(const Project& project);
```

Rules:

- Output must be deterministic.
- Parser is intentionally strict and rejects unknown project/board keys.
- If you add model fields, update both reader and writer.

### `src/ccad_core/erc.hpp/.cpp`

Owns logical checks over components/nets/pins.

Public functions/types:

```cpp
struct Diagnostic {
  std::string severity;
  std::string code;
  std::string message;
  std::string object_id;
};

std::vector<Diagnostic> runErc(const Project& project);
```

Current checks:

- empty project warning
- duplicate component IDs
- duplicate pins
- duplicate net members
- unknown components
- unknown pins

### `src/ccad_core/drc.hpp/.cpp`

Owns physical board checks.

Public function:

```cpp
std::vector<Diagnostic> runDrc(const Project& project);
```

Current checks:

- duplicate pad/via/track IDs
- unknown pad/track layers
- pad/via/track positions outside board
- non-positive pad/via/track dimensions
- unconnected pads as warnings
- unconnected vias/tracks as warnings
- unknown non-empty pad/via/track net IDs
- unconnected track endpoints as warnings
- pad/via/track endpoints inside rectangular keepouts
- track segments crossing rectangular keepouts
- via drill larger than diameter
- zero-length tracks

### `src/ccad_core/canvas.hpp/.cpp`

Owns GUI-independent scene data.

Public function:

```cpp
CanvasScene buildCanvasScene(const Project& project);
```

Important structs:

```cpp
struct CanvasPad {
  std::string id;
  double x_units;
  double y_units;
  double width_units;
  double height_units;
  double rotation_degrees;
};
```

Rule:

- GUI rendering must consume `CanvasScene`, not inspect project geometry directly.

### `src/ccad_core/review.hpp/.cpp`

Owns human review summary.

Public function:

```cpp
ProjectReview buildReview(const Project& project);
```

Used by:

- `ccad inspect`
- Qt review GUI header/cards/diagnostic table

### `src/ccad_core/diff.hpp/.cpp`

Owns project diff model.

Public functions:

```cpp
ProjectDiff diffProjects(const Project& before, const Project& after);
std::string dumpProjectDiffJson(const ProjectDiff& diff);
```

### `src/ccad_core/transaction.hpp/.cpp`

Owns transaction journal records.

Public functions:

```cpp
Transaction makeTransaction(std::string id, std::string command,
                            std::string summary, const Project& before,
                            const Project& after);
std::string dumpTransactionJson(const Transaction& transaction);
```

### `src/ccad_core/footprint.hpp`

Owns reusable footprint/library data.

Important structs:

```cpp
struct FootprintPad {
  std::string number;
  std::string type;
  std::string shape;
  Point position;
  double rotation_degrees;
  Size size;
  std::optional<Length> drill;
  std::vector<std::string> layers;
};

struct Footprint {
  std::string name;
  std::vector<FootprintPad> pads;
};
```

Rule:

- Footprints are library/package definitions.
- Placed board pads are `Pad` objects in `Board`.

### `src/ccad_core/kicad_footprint_import.hpp/.cpp`

Owns KiCad `.kicad_mod` import and CCad footprint JSON.

Public functions:

```cpp
Footprint importKiCadFootprint(std::string_view source);
std::string dumpFootprintJson(const Footprint& footprint);
Footprint loadFootprintJson(std::string_view source);
```

Current importer scope:

- root `(footprint "...")`
- `(pad "...")`
- pad type/shape
- `(at x y [rotation])`
- `(size width height)`
- simple `(drill diameter)`
- `(layers ...)`

Rules:

- Treat KiCad files as data.
- Skip unsupported KiCad constructs safely.
- Reject malformed s-expressions and non-footprint root.
- Accept UTF-8 BOM.

### `src/ccad_core/library_catalog.hpp/.cpp`

Owns CCad native library catalog metadata for local/offline component libraries.

Important structs:

```cpp
struct LibrarySource {
  std::string name;
  std::string kind;
  std::string url;
  std::string commit;
  std::string mirror;
  std::string fetched_at;
};

struct LibraryItem {
  std::string id;
  std::string kind;
  std::string name;
  std::string source_path;
  std::string native_path;
  std::string sha256;
  std::string license;
  std::string provenance;
  std::vector<std::string> warnings;
};
```

Public functions:

```cpp
std::string dumpLibraryCatalogJson(const LibraryCatalog& catalog);
LibraryCatalog loadLibraryCatalogJson(const std::string& json);
const LibraryItem* findLibraryItem(const LibraryCatalog& catalog, const std::string& id);
std::vector<const LibraryItem*> searchLibraryItems(const LibraryCatalog& catalog,
                                                   const std::string& query);
std::vector<const LibraryItem*> searchLibraryItems(const LibraryCatalog& catalog,
                                                   const std::string& query,
                                                   const std::string& kind);
std::vector<CatalogDiagnostic> validateLibraryCatalog(const LibraryCatalog& catalog);
std::vector<CatalogDiagnostic> validateLibraryCatalog(const LibraryCatalog& catalog,
                                                      const std::filesystem::path& root);
```

Rules:

- KiCad and mirror libraries are source data.
- CCad runtime should query native catalog records, not repeatedly fetch or parse raw upstream files.
- Huge local caches belong under ignored paths such as `library-cache/` or `catalog-cache/`, not the main source tree.
- Every imported item must preserve source path, source commit/hash, checksum, license, provenance, and warnings.

## CLI Files

### `src/ccad_cli/main.cpp`

Entrypoint only.

Owns:

```cpp
int main(int argc, char** argv);
```

Rule:

- Delegate process behavior to `ccad_cli::run`.

### `src/ccad_cli/app.hpp/.cpp`

Owns top-level command dispatch, usage text, and machine-readable command metadata.

Public function:

```cpp
int run(int argc, char** argv);
```

Internal command metadata functions:

```cpp
const std::vector<CommandHelp>& commandHelp();
std::string helpJson();
int helpCommand(const std::vector<std::string>& args);
```

Command groups:

```text
ccad init
ccad help --format json
ccad validate
ccad drc
ccad inspect
ccad diff
ccad lib import-footprint
ccad lib catalog-info
ccad lib catalog-find
ccad lib catalog-search
ccad pcb add-pad
ccad pcb add-via
ccad pcb add-track
ccad pcb add-keepout
ccad pcb place-footprint
```

### `src/ccad_cli/common.hpp/.cpp`

Owns shared CLI helpers for option parsing, project/footprint file I/O, JSON responses, and board mutation validation.

Important functions:

```cpp
Project loadProjectFile(const std::string& path);
bool writeProjectFile(const std::string& path, const Project& project);
Footprint loadFootprintFile(const std::string& path);
std::map<std::string, std::string> parseOptions(...);
std::string requireOption(...);
Length requirePositiveMillimeters(...);
double requireDoubleOption(...);
double optionDoubleOrDefault(...);
std::string diagnosticsJson(...);
std::string reviewJson(...);
Board& requireBoard(Project& project);
void requireLayer(const Board& board, const std::string& layer_id);
void requireInsideBoard(...);
void requireUniquePadId(...);
void requireUniqueViaId(...);
void requireUniqueTrackId(...);
Point rotateAndTranslate(...);
```

Rule:

- These helpers must stay deterministic and must not shell out or execute project/library file contents.

### `src/ccad_cli/project_commands.hpp/.cpp`

Owns project-level commands:

```cpp
int initCommand(const std::vector<std::string>& args);
int validateCommand(const std::vector<std::string>& args);
int drcCommand(const std::vector<std::string>& args);
int inspectCommand(const std::vector<std::string>& args);
int diffCommand(const std::vector<std::string>& args);
```

### `src/ccad_cli/pcb_commands.hpp/.cpp`

Owns PCB mutation command group:

```cpp
int pcbCommand(const std::vector<std::string>& args);
```

Placement rule:

- `pcb place-footprint` maps `FootprintPad` to placed `Pad`.
- `pcb add-keepout` maps command arguments to one rectangular `Keepout` and rejects duplicate IDs or areas outside the board.
- Pad ID format: `<component>.<pad-number>`.
- `net_id` is copied from the first logical `Net` member matching the placed component ID and footprint pad number.
- If no logical net member matches, `net_id` remains empty for backward compatibility.

### `src/ccad_cli/lib_commands.hpp/.cpp`

Owns library/import command group:

```cpp
int libCommand(const std::vector<std::string>& args);
```

Current scope:

- `lib import-footprint --in <path.kicad_mod> --out <path.json>`
- `lib catalog-info --catalog <path.ccad-library.json>`
- `lib catalog-find --catalog <path.ccad-library.json> --id <id>`
- `lib catalog-search --catalog <path.ccad-library.json> --query <text> [--kind <kind>]`
- `lib catalog-validate --catalog <path.ccad-library.json> [--root <native-library-root>]`

## GUI Files

### `src/ccad_gui/main.cpp`

Entrypoint only.

Owns:

```cpp
int main(int argc, char** argv);
```

### `src/ccad_gui/review_window.hpp/.cpp`

Owns native review window, menus, toolbar, summary cards, diagnostics table, file loading, and calls into canvas renderer.

Important methods:

```cpp
ReviewWindow();
void loadProjectPath(const std::filesystem::path& path);
void applyStyle();
void openProject();
void reloadProject();
void renderReview(const ccad::ProjectReview& review);
void renderCanvas(const ccad::CanvasScene& scene);
void setStatusChip(const QString& text, const QString& color);
```

Rule:

- Do not parse geometry here.
- Load project -> `buildReview` and `buildCanvasScene` -> render.

### `src/ccad_gui/board_canvas_renderer.hpp/.cpp`

Owns drawing board canvas objects into `QGraphicsScene`.

Public function:

```cpp
void renderBoardCanvas(QGraphicsScene& canvas_scene, const ccad::CanvasScene& scene);
```

Renders:

- board outline
- grid
- tracks
- pads, including rotation
- vias
- board size label

### `src/ccad_gui/board_canvas_view.hpp`

Owns editor viewport behavior.

Important method:

```cpp
void zoomToFit();
void wheelEvent(QWheelEvent* event) override;
void mousePressEvent(QMouseEvent* event) override;
void mouseMoveEvent(QMouseEvent* event) override;
```

## Scripts

### `scripts/run_sprint_demo.ps1`

Creates a demo board, adds primitives, imports KiCad footprint sample, places the footprint, runs inspect/validate/DRC, launches GUI, captures screenshot.

Run:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_sprint_demo.ps1 -Name sprint-demo
```

Outputs:

- `artifacts/demos/*.ccad.json`
- `artifacts/demos/*.inspect.json`
- `artifacts/demos/*.validate.json`
- `artifacts/demos/*.drc.json`
- `artifacts/demos/*.ccad-footprint.json`
- `artifacts/screenshots/*.png`

## Current Known Technical Debt To Avoid Expanding

- CLI commands are now split, but `src/ccad_cli/pcb_commands.cpp` should be split further once placement, routing, or net mapping grows.
- `serialize.cpp` and `kicad_footprint_import.cpp` contain handwritten parsers. They are deterministic and tested, but keep scope narrow.
- GUI is an early CAD editor shell, not a full editor yet.
- No schematic-footprint mapping yet; placed footprint pads have empty nets.
- No clearance DRC yet.

## How To Add A New Feature

1. Update or create sprint spec and plan under `docs/superpowers`.
2. Update `docs/devops/progress.md`.
3. Write failing tests.
4. Implement minimum code.
5. Run focused tests.
6. Run full gate.
7. Commit with progress line in user update.
8. Update docs/features and README if user-visible.
9. Run demo script and capture screenshot when GUI-visible.
10. Merge only after full gate passes.
