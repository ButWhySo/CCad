# Technical Handover

## Vision

CCad is a native desktop PCB design kernel built for machine callers first. Agents should interact with stable typed objects, deterministic files, validation results, and eventually transactions/RPC. Human UI should render and review kernel state rather than own it.

The research basis for this architecture is documented in `docs/research/2026-05-14-llm-native-pcb-tool-report.md`. The key decision is that CCad is not a KiCad wrapper. KiCad, tscircuit/Circuit JSON, SKiDL, Atopile, Freerouting, pcb-rnd, Magic, and OpenROAD are references or interoperability targets. CCad's source of truth remains its own native kernel.

KiCad-style functionality and UI expectations are mapped in `docs/research/2026-05-14-kicad-feature-map.md`.

## Phase Roadmap

Current progress counter is tracked in `docs/devops/progress.md`. As of Sprint 20, CCad is in Phase 2 / 6: physical primitives and early board authoring.

1. Logical kernel: project, components, pins, nets, constraints, ERC, CLI.
2. Physical primitives: board outline, layers, pads, vias, tracks, rectangular keepouts, placement regions, early DRC.
3. Routing assistance: constrained route requests and external router boundary.
4. Native GUI/reviewer: render schematic/PCB state, diffs, diagnostics, and transaction review.
5. Interop: KiCad, Circuit JSON, DSN/SES, manufacturing exports.
6. Agent protocol: JSON-RPC/MCP over the same transaction bus, with audit logs and permission gates.

The first GUI phase is specified in `docs/superpowers/specs/2026-05-14-qt-review-gui-design.md`. It uses Qt 6 Widgets when available and keeps all review logic in `ccad_core`.

Sprint tracking starts in `docs/devops/sprints/2026-05-14-sprint-1-qt-review-gui.md`.

## Current Architecture

The initial codebase is C++20:

- Model objects are plain C++ structs/classes with explicit JSON conversion.
- JSON output is deterministic for diffs and replay.
- ERC and DRC return typed diagnostics for agent consumption.
- CLI commands are split into deterministic command modules.
- `ccad_gui` is optional and builds only when Qt 6 Widgets is available.
- Sprint 27 starts the CAD editor shell direction: central PCB tab, dock panels, explicit canvas fit, wheel zoom, middle-button pan, and status readouts.
- Sprint 28 split the project summary and diagnostics panels out of `ReviewWindow`; keep future GUI features similarly modular.
- Sprint 29 made canvas primitives selectable by stable object type and ID. Sprint 30 adds a dedicated read-only inspector panel for the selected object's stable type and ID.
- Sprint 31 adds read-only layer/object browsing from `CanvasScene` metadata and uses shape-level selection highlights instead of Qt bounding boxes.
- Sprint 32 links diagnostic table rows to PCB canvas selection when a diagnostic carries a stable object ID.
- Sprint 33 includes DRC diagnostics in the shared `ProjectReview` stream used by CLI inspect and the GUI.
- Sprint 34 renders read-only diagnostic markers on PCB canvas objects referenced by review diagnostics and derives selected-object highlights from each object's display color for future theme compatibility.
- Sprint 42 adds explicit canvas toolbar zoom controls and an app-owned `ccad_gui --screenshot <project> <png>` harness for sprint-end visual QA. The harness waits 20 seconds while processing Qt events, grabs the native window through Qt, and avoids foreground-window capture or global process killing.
- Large designs should not be generated as command spam. See `docs/architecture/large-design-and-component-knowledge-pipeline.md` for the intended batching, component knowledge, curation, and visual review pipeline.
- Large designs should not be generated as command spam. See `docs/architecture/large-design-and-component-knowledge-pipeline.md` for the intended batching, component knowledge, curation, and visual review pipeline.
- Physical lengths are stored as integer nanometers in `ccad_core::Length`; user-facing mm/mil values convert at API boundaries.
- `ccad_core::CanvasScene` is the tested rendering input for the Qt board canvas. GUI rendering must consume this model rather than directly inventing project geometry.
- Rectangular keepouts can be authored with `ccad pcb add-keepout` and are visible in the Qt board canvas.
- KiCad footprint import supports a basic `.kicad_mod` pad subset and treats files strictly as data.
- Native library catalog metadata supports local/offline CCad-compatible catalogs with provenance and checksum fields.
- `ccad lib catalog-info` and `ccad lib catalog-find` expose local catalog metadata to agents without network fetches.
- `ccad lib catalog-search` provides local text search over catalog ID, kind, name, source path, native path, usage summary, layout notes, source confidence, and review status.
- `ccad lib catalog-search --kind <kind>` restricts local search to one catalog item kind.
- `ccad lib catalog-validate` validates required metadata and duplicate item IDs before a catalog is trusted.
- `ccad lib catalog-validate --root <dir>` verifies local native catalog artifacts by existence and SHA-256.
- Catalog items can preserve component-knowledge fields: `usage_summary`, `layout_notes`, `source_confidence`, and `review_status`.
- `review_status` is validated when present and currently accepts `generated`, `needs_review`, `reviewed`, and `rejected`.
- Footprint placement can rotate pads and preserve logical net IDs when project nets contain matching component/pin members.
- Physical DRC currently covers duplicate IDs, unknown layers/nets, outline bounds, invalid dimensions, empty net warnings, track endpoint connectivity warnings, rectangular keepout occupancy/crossing violations, drill/diameter sanity, and zero-length tracks.

## KiCad Library Reuse Policy

KiCad symbols, footprints, and 3D model references are important source data for future compatibility, but CCad should not vendor a bulk KiCad library dump into the main source tree.

Planned approach:

- Build importers and indexers before bulk ingestion.
- Treat KiCad libraries as external data, never executable code.
- Avoid fetching during normal design work; prefer pinned local source caches and native CCad catalog files.
- Preserve source URL, mirror, commit/hash, original path, license, and import diagnostics per library item.
- Normalize imported assets into a controlled CCad catalog format.
- Keep large binary/model caches outside this repo or in a dedicated artifact/catalog repository.
- Use official KiCad libraries as the primary source when possible; mirrors such as Gitee can be fallback sources only when provenance and checksums match.

## Current Source Layout

- `src/ccad_core/`: source-of-truth kernel model, serialization, ERC, DRC, review, diff, transactions, canvas scene, and KiCad footprint import.
- `src/ccad_cli/`: machine-callable CLI split into `app`, `common`, `project_commands`, `pcb_commands`, and `lib_commands`.
- `src/ccad_gui/`: native Qt GUI split into `ReviewWindow`, board canvas renderer, board canvas view, and a small `main.cpp`.
- `tests/`: C++ CTest targets for serialize, ERC, DRC, footprint import, CLI, review, diff, transaction, geometry, and canvas behavior.
- `docs/codebase-map.md`: maintained memory-loss map for agents. Read it before non-trivial edits.

## Development Commands

```bash
cmake -S . -B build -DCCAD_WARNINGS_AS_ERRORS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

To force a non-GUI build:

```bash
cmake -S . -B build -DCCAD_BUILD_GUI=OFF
```

Local Windows Qt build used in this repo:

```powershell
cmake -S . -B build-qt -DCCAD_WARNINGS_AS_ERRORS=ON -DCCAD_BUILD_GUI=ON -DCMAKE_PREFIX_PATH=C:\Qt\6.11.1\mingw_64
cmake --build build-qt
$env:PATH = "C:\Qt\6.11.1\mingw_64\bin;$env:PATH"
.\build-qt\ccad_gui.exe
```

For `cmd.exe` or agents that prefer one-line commands, use the same runtime environment explicitly:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && cmake --build build-qt --clean-first && ctest --test-dir build-qt --output-on-failure"
```

If a built test or GUI executable fails before printing test output with Windows status `0xc0000139`, debug it as a loader/DLL problem first. The most common cause in this repo is running a Qt-linked executable without `C:\Qt\6.11.1\mingw_64\bin` first on `PATH`, or with another incompatible Qt DLL earlier on `PATH`. Confirm the DLL resolution order with:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && where Qt6Core.dll"
```

## CI/CD

GitHub Actions workflow lives in `.github/workflows/ci.yml`. It configures CMake, builds, and runs CTest on pushes to `main` and `phase-*`, plus pull requests to `main`.

There is no remote configured in this local repo. CI config is present for future GitHub use.

## Test Policy

Use TDD for production behavior. Each new kernel rule or CLI behavior should have a focused test that fails before implementation. Keep tests behavioral and avoid mocking domain code.

## Security Posture

- Project JSON is data only.
- No dynamic import or code execution from design files.
- No network calls in kernel or CLI.
- No secrets needed for local tests or CI.
- Future importers should validate input before mapping into kernel objects.
