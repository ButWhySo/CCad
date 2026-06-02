# CCad

CCad is a ground-up, machine-callable PCB design kernel for native desktop CAD software. The first milestone builds a C++ logical circuit core, deterministic JSON representation, ERC checks, and a native CLI that agents can call without driving a GUI.

## Current Phase

Phase 7 / 7: Final Polish & Release.

Progress counter: Phase 7 / 7, Sprint 173 complete on `sprint-173-pcb-graphics-text-tools`.

Phase 6 is complete. CCad now has an Agent protocol: JSON-RPC/MCP over the transaction bus, audit logs, permission gates, and benchmark harness. Phase 7 focuses on final polish, interactive footprint placement via the GUI, and GUI layout parity with KiCad.

- Typed project model.
- Deterministic JSON load/dump.
- Logical ERC diagnostics.
- CLI: `ccad help --format json`, `ccad init`, `ccad validate`, `ccad inspect`, and `ccad diff`.
- CLI PCB authoring: `ccad pcb list-nets`, `ccad pcb list-objects`, `ccad pcb get-object`, `ccad pcb route-status`, `ccad pcb set-outline`, `ccad pcb set-rules`, `ccad pcb add-layer`, `ccad pcb set-layer`, `ccad pcb remove-layer`, `ccad pcb set-layer-visibility`, `ccad pcb add-pad`, `ccad pcb set-pad`, `ccad pcb add-via`, `ccad pcb set-via`, `ccad pcb add-track`, `ccad pcb set-track`, `ccad pcb add-graphic-line`, `ccad pcb add-text`, `ccad pcb add-keepout`, `ccad pcb add-placement-region`, `ccad pcb set-region-kind`, `ccad pcb remove-object`, `ccad pcb move-object`, and `ccad pcb resize-object`.
- CLI footprint placement preserves logical net IDs when component pins already appear in project nets.
- CLI PCB pad authoring supports KiCad-style pad type, shape, drill, roundrect ratio, chamfer ratio, and multi-layer metadata.
- Agent-facing PCB pad queries expose KiCad-style pad type, shape, drill, roundrect ratio, chamfer ratio, and layer metadata through `pcb get-object` and `pcb list-objects --type pad`.
- Native library catalog metadata, lookup, and search for local/offline component-library caches.
- Physical board outline, layers, rectangular placement regions, rectangular keepouts, pads, vias, track segments, board graphic lines, and board text in project JSON.
- Project review and `ccad inspect` report the full board outline rectangle and active board-level DRC rules.
- Physical DRC for geometry, connectivity metadata, layer-aware copper connectivity and clearance, rectangular keepout occupancy, track crossing violations, board-level copper clearance, minimum track width, minimum via annular ring, design-rule value validation, stable physical object IDs, logical pad/net parity, route-request validity, route-request endpoint net parity, and route-request width policy.
- Physical DRC rejects pads and tracks placed on non-copper layers.
- Physical DRC track-endpoint connectivity now accepts geometric copper contact with same-net pads and vias, not only exact center-point matches.
- Optional Qt 6 native GUI for human review and board canvas viewing.
- Native GUI selection inspector for stable object type and ID.
- Native GUI layer/object browser and shape-level selection highlight.
- Native GUI diagnostic rows can select matching PCB canvas objects by stable ID.
- Project review and GUI diagnostics include physical DRC findings.
- Native GUI diagnostic markers are drawn over matching PCB canvas objects.
- Native GUI object browser rows can select matching PCB canvas objects by stable ID.
- Native GUI canvas rendering has explicit presentation-theme groundwork.
- Native GUI has read-only transaction timeline panel groundwork.
- Native GUI canvas items expose net metadata for net highlight groundwork.
- Native GUI object browser has net rows for net-selection workflow.
- Native GUI supports interactive footprint placement through the Add Footprint dialog, which calls `ccad_core` directly.
- Native GUI supports schematic symbol placement through the Add Symbol dialog, populating `Project::components`.
- Native GUI supports independent PCB and Schematic Canvas views through a tabbed editor interface.
- Native GUI toolbar layout follows KiCad's pcbnew organization (Top, Left, Right toolbars) for visual parity.
- Native GUI left and right PCB toolbars resolve KiCad SVG icons from `CCAD_KICAD_SRC` or a sibling `kicad_src` checkout, with tooltips preserving action names.
- Native GUI top toolbar actions now resolve KiCad-style icons and perform real Save, Board Setup, Undo, Redo, and Run DRC behavior instead of placeholder buttons.
- Native GUI can export a DRC report from the Tools menu.
- Native GUI library chooser now has KiCad-style chooser context with filtered library/name rows, details, and visual symbol/footprint previews.
- Native GUI library chooser indexes local `library-cache` entries without parsing every symbol or footprint on dialog open; selected rows are parsed lazily for details, preview, and final placement.
- Native GUI footprint previews and the board canvas use a shared KiCad-inspired layer palette so copper, silkscreen, fabrication, courtyard, edge, and user graphics do not collapse into one generic color.
- Native GUI pad rendering separates copper from solder-mask and solder-paste aperture overlays, so visible mask or paste layers no longer force hidden copper to render.
- Native GUI can dump a read-only semantic UI map as JSON for LLM/automation tooling, including stable action IDs, tab/canvas bounds, canvas-object IDs, net/layer metadata, route provenance, and click target coordinates.
- Native GUI has app-owned placement-click regression automation so left-click footprint placement can be tested through the real Qt viewport event path without manual mouse control.
- Native GUI has app-owned UI-map mouse-target automation that moves to semantic targets, captures marked screenshots, resizes the window, and repeats to prove scalable coordinates.
- Native GUI can keep a local live UI-map socket open while the GUI runs, serving repeated JSON Lines `ui.map`, `ui.target`, and `ui.epoch` requests for agents.
- Native GUI right-toolbar Add Via, Route Track, Add Keepout, Draw Graphic, Place Text, and Delete tools now enter real PCB edit modes and create or remove durable board primitives through the same Qt viewport event path used by app-owned tests.
- Native GUI has a PCB active-layer selector, exposes it as `control:active_pcb_layer`, serves active-layer query/set methods to agents, and uses the selected copper layer for footprint placement and Route Track commits.
- Native GUI unfinished toolbar tools now report visible planned-tool status and `future_tool_not_implemented` through the safe UI trigger contract instead of acting as silent stubs.
- Native GUI left toolbar Show Layers and Show Properties actions now toggle their existing panels and report `panel_toggled` through the safe UI trigger contract.
- Native GUI has a bottom Agent panel shell that can refresh the current UI-map JSON, trigger allowlisted safe UI actions by semantic ID, and expose itself as `panel:agent` for automation targeting.
- Native GUI left toolbar display controls now perform real view-state actions for grid visibility, polar cursor coordinates, inch units, full-window crosshair, ratsnest guides, net highlighting, and high-contrast display mode. Agent safe triggers return `display_state_toggled`, and UI-map action nodes expose checked state.
- Native GUI selection inspector cleans up stale editor rows immediately during rapid multi-selection changes, so net highlight and display-mode workflows do not stack old pad or track editors in the properties panel.
- Native GUI visual-validation harness timing is policy-guarded: single-preview screenshots settle for 7 seconds, while multi-target GUI validation uses a 5-second initial load and 800 ms per target/action.
- Native GUI resolves KiCad SVG icons from `CCAD_KICAD_SRC`, `F:\kicad_src`, and executable/current-directory candidates, and the build now links Qt SVG explicitly for KiCad-style toolbar icon rendering.
- Native GUI symbol placement resolves locally converted KiCad `extends` inheritance from `library-cache`, so derived symbols such as diode variants inherit parent pins before placement.
- Native GUI footprint placement accepts raw KiCad `.kicad_mod` files from the chooser as well as converted CCad footprint JSON.
- Native GUI Add behavior is editor-tab aware: PCB opens cache-backed footprint placement, while Schematic opens cache-backed symbol placement.
- Native GUI placement uses mouse-following footprint and symbol ghosts; left click commits through the core placement API, and Escape cancels before commit.
- Native GUI no longer exposes raw KiCad symbol/footprint file preview entries as normal File menu placement actions.
- Native GUI renders front and back copper with distinct KiCad-inspired default colors, and selected tracks highlight across their visible copper width.
- Native GUI canvas renders circular, oval, ratio-controlled round-rect, trapezoid, and chamfered pads as shape-aware geometry and shows through-hole drill openings as visible annular rings.
- Native GUI interactive footprint placement and movement use shape-aware, layer-colored ghost previews and convert canvas scene coordinates back to board millimeters before calling the core placement APIs.
- Native GUI canvas toolbar has Fit, Zoom Out, Zoom In, and 100% review controls.
- Sprint demo automation now has a robust screenshot fallback path that captures only the spawned CCad window by PID when `ccad_gui --screenshot` fails.
- Native GUI canvas supports CAD-style pan with middle-drag, right-drag, or Shift+left-drag, plus clamped wheel zoom and expanded scene navigation bounds.
- GUI panel/canvas test binaries are now registered in CTest and run in the default `ctest` gate.
- Native GUI canvas supports keyboard navigation: `+`, `-`, `0`, `F`, `Home`, arrow-key panning, and `W/A/S/D` panning.
- Native GUI canvas supports hold-space hand-pan mode with left-drag.
- Native GUI status bar tool state now reflects pan mode transitions (`Select`, `Pan Ready`, `Pan Drag`).
- Native GUI includes in-app navigation controls help (`Help > Navigation Controls`, shortcut `F1`).
- Native GUI project summary shows route request count, open/partial/done route progress, and routed segment count.
- Native GUI object browser lists route-request intent rows and shows route provenance on routed track rows.
- Native GUI route-request rows can select routed tracks that were generated from that request.
- `ccad inspect` reports board route progress derived from route requests and track provenance.
- GitHub CI now uses `actions/checkout@v5` for Node 24 runner compatibility.
- Native GUI starts with a larger default window size derived from desktop available bounds.
- Placement regions are serialized, authored through the CLI, inspected in review JSON, checked by DRC, rendered in the Qt canvas, and listed in the object browser.
- Board-level physical DRC rules are serialized and configurable through `ccad pcb set-rules`.
- Native GUI startup crash (status `0xC0000005`) fixed by ordering status-label initialization before pan-mode callback wiring.
- Native GUI canvas rendering now respects board layer visibility for pads and tracks, while vias remain visible because they are cross-layer objects.
- Native GUI canvas rendering is origin-aware when the board outline uses a non-zero origin.
- Native GUI cursor status uses board-origin-aware coordinates for shifted board outlines.
- Native GUI project summary shows non-zero board outline origin next to board size.
- Native GUI project summary shows active board-level DRC rule values.
- `COPPER_CLEARANCE` DRC diagnostics include the configured clearance value that triggered the finding.
- Track-width and via annular-ring DRC diagnostics include the configured threshold value that triggered the finding.
- Via annular-ring DRC diagnostics are suppressed when via drill geometry is already invalid.
- Keepout geometry DRC diagnostics are suppressed when the checked pad, via, or track has invalid dimensions.
- Copper-clearance DRC diagnostics are suppressed when the checked pad, via, or track has invalid dimensions.
- CLI diagnostic JSON includes summary counts for total diagnostics, errors, and warnings.
- Catalog validation JSON includes summary counts for total diagnostics, errors, and warnings.
- Catalog search JSON includes a summary match count for agent workflows.
- Project diffs include board layer additions, removals, and changes.
- Project diffs include board outline additions, removals, and changes.
- Project diffs include board pad additions, removals, and changes.
- Project diffs include board via, track, graphic-line, and text additions, removals, and changes.
- Project diffs include route-request intent additions, removals, and changes.
- Project diffs include board keepout and placement-region additions, removals, and changes.
- Project diffs include board design-rule changes.
- CLI diff tests cover board-level physical object entries in executable JSON output.
- CLI PCB authoring can list physical board net usage counts as compact JSON.
- CLI PCB authoring can list route-request intent records and route completion status as compact JSON.
- CLI PCB authoring can export compact route-job JSON for external router handoff, including KiCad-style pad type, shape, drill, ratio, layer, and rotation metadata.
- CLI PCB authoring can list board layer and physical object IDs as compact JSON, with optional type filtering.
- CLI PCB authoring can inspect one board layer or physical object by stable ID as compact JSON, including route provenance for tracks and layer/text metadata for board graphics and board text.
- CLI PCB authoring can remove physical board objects by stable ID.
- CLI PCB authoring can remove unused board layers by stable ID.
- CLI PCB authoring can update board layer name, kind, and visibility by stable ID.
- CLI PCB authoring can move pads, vias, keepouts, and placement regions by stable ID.
- CLI PCB authoring can resize pads, keepouts, and placement regions by stable ID.
- CLI PCB authoring can update existing track endpoints and width by stable ID.
- CLI PCB authoring can update existing via diameter and drill by stable ID.
- CLI PCB authoring can update existing track net/layer metadata and via net metadata by stable ID.
- CLI PCB authoring can update existing pad metadata, layer, and rotation by stable ID.
- CLI PCB authoring can update keepout and placement-region kind values by stable ID.
- Board JSON now has typed route-request records for early routing-assistance intent.
- CLI PCB authoring can add typed route-request records by stable endpoint object IDs.
- CLI PCB authoring can update typed route-request records by stable ID.
- CLI PCB authoring can remove typed route-request records by stable ID.
- CLI PCB authoring can apply one routed segment or one routed polyline from a route request into board tracks while preserving source request provenance.
- Core PCB metadata includes the canonical KiCad named layer set used by `.kicad_pcb` files, including 32 copper layers, paired fabrication/assembly layers, board geometry layers, user layers through `User.9`, and agent-visible KiCad layer numbers.
- CLI PCB authoring can append missing KiCad standard layers with `ccad pcb add-standard-layers --file <path>` while preserving existing and custom layers.
- `ccad inspect` and the native GUI project summary report layer breakdown counts for copper/non-copper and visible/hidden layers.

Out of scope for the Phase 3 MVP: full interactive editing, automatic placement, a production autorouter, KiCad import/export, fabrication outputs, and network services.

## Git History Policy

For public GitHub publishing, internal markdown guidance files are kept locally for human and agent workflow but are removed from Git tracking; only `README.md` stays tracked as markdown.

When publishing a clean snapshot to an empty remote, never orphan long-term `main` history without reconnecting lineage. If a root snapshot commit is used, immediately attach the prior lineage with a non-rewriting merge, for example:

```bash
git merge --allow-unrelated-histories -s ours <full-history-branch> -m "merge: restore historical lineage onto public main"
```

This keeps the current public tree intact while restoring commit-log continuity for `git log`, debugging, and agent context retrieval.

## Setup

Install CMake and a C++20 compiler, then build the core/CLI/test targets:

```bash
cmake -S . -B build
cmake --build build
```

Use this core build when you are changing kernel, CLI, serialization, ERC, diff, transaction, or other non-GUI code.

## Verify

```bash
ctest --test-dir build --output-on-failure
```

If your local setup still has internal docs from private iterations, treat this README as the source of truth for the public GitHub branch.

## Library Cache Direction

CCad should reuse KiCad's symbol, footprint, and 3D model ecosystem through local source caches and CCad-native catalogs.

For large designs, CCad should use semantic batches, enriched component knowledge, local catalog search, and checkpoint-based visual review instead of thousands of primitive commands.

Policy:

- Before implementing PCB/CAD features or bugfixes, check current external references for that feature domain. KiCad official docs and developer file-format docs are the first compatibility source; use Altium, IPC-style manufacturing references, ngspice, OpenTelemetry, Langfuse, or LangGraph docs when those domains are affected.
- Do not fetch the internet repeatedly during normal design work.
- Do not vendor huge KiCad/Gitee/GitHub library dumps into this source repo.
- Keep raw upstream checkouts and converted catalogs in ignored local paths such as `library-cache/` or `catalog-cache/`.
- Store runtime/search data in CCad's own catalog format.
- Preserve source URL, mirror, commit/hash, source path, checksum, license, provenance, component-knowledge fields, and import warnings for each item.
- Use `usage_summary`, `layout_notes`, `source_confidence`, and `review_status` so agents can search local curated knowledge before using web research.
- Later, a separate `ccad-libraries` repo/package can distribute curated prebuilt catalogs.

Inspect a local catalog:

```powershell
.\build-qt\ccad.exe lib catalog-info --catalog .\catalog-cache\kicad.ccad-library.json
```

Find a catalog item by stable ID:

```powershell
.\build-qt\ccad.exe lib catalog-find --catalog .\catalog-cache\kicad.ccad-library.json --id footprint:Resistor_SMD:R_0603_1608Metric
```

Search a catalog by text:

```powershell
.\build-qt\ccad.exe lib catalog-search --catalog .\catalog-cache\kicad.ccad-library.json --query 0603
```

Limit search to one item kind:

```powershell
.\build-qt\ccad.exe lib catalog-search --catalog .\catalog-cache\kicad.ccad-library.json --query 0603 --kind footprint
```

## Windows Qt Development Loop

Run these commands from `F:\CCad` in PowerShell when you need the native desktop GUI too.

Mandatory precheck before configure/build/run:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\preflight_qt_env.ps1
```

This fails fast when the first `Qt6Core.dll` in `PATH` is not from `C:\Qt\6.11.1\mingw_64\bin`, or when `build-qt\CMakeCache.txt` is using the wrong compiler toolchain.

Configure the Qt build directory:

```powershell
cmake -S . -B build-qt `
  -DCCAD_WARNINGS_AS_ERRORS=ON `
  -DCCAD_BUILD_GUI=ON `
  -DCMAKE_PREFIX_PATH=C:\Qt\6.11.1\mingw_64
```

What it does:

- `cmake -S .` tells CMake the source tree is the current repo.
- `-B build-qt` writes generated build files into `build-qt`.
- `CCAD_WARNINGS_AS_ERRORS=ON` makes compiler warnings fail the build.
- `CCAD_BUILD_GUI=ON` requests the Qt desktop GUI target.
- `CMAKE_PREFIX_PATH=...` tells CMake where the local Qt install lives.

When to run:

- First time after cloning/creating the repo.
- After changing CMake files.
- After changing Qt install path or compiler/toolchain.
- When CMake says the cache is stale or misconfigured.

Build everything from scratch:

```powershell
cmake --build build-qt --clean-first
```

What it does:

- Deletes previous compiled outputs in `build-qt`.
- Rebuilds core library, CLI, tests, and GUI.

When to run:

- Before committing or merging.
- After broad refactors.
- After changing headers used by many targets.
- When you need confidence there are no stale object files hiding a build issue.

Run all tests:

```powershell
ctest --test-dir build-qt --output-on-failure
```

What it does:

- Runs the full CTest suite from `build-qt`.
- Prints failing test output if a test fails.

When to run:

- After every behavior change.
- Before every commit that claims working code.
- Before merging a sprint branch to `main`.

For agents or shells using `cmd.exe`, keep the Qt runtime path in the same one-line command:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && ctest --test-dir build-qt --output-on-failure"
```

GitHub Actions CI now runs three lanes by default: Linux core (`CCAD_BUILD_GUI=OFF`), Linux GUI (`CCAD_BUILD_GUI=ON` + Qt install + `QT_QPA_PLATFORM=offscreen` for headless GUI tests), and Windows core (`CCAD_BUILD_GUI=OFF`), and triggers on pushes to `main`, `phase-*`, and `sprint-*`, plus pull requests to `main`.

If CTest fails before any CCad test output with Windows status `0xc0000139`, assume a DLL loader problem first. Check that the intended Qt DLL is found before any other Qt install:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && where Qt6Core.dll"
```

## CLI

Create an empty project:

```bash
build/ccad init --name demo --out demo.ccad.json
```

Validate a project:

```bash
build/ccad validate demo.ccad.json
```

`validate` prints JSON diagnostics. Exit code `0` means no ERC errors. Exit code `1` means at least one error.

Discover commands as machine-readable JSON:

```bash
build/ccad help --format json
```

What it does:

- Emits deterministic command metadata with command names, summaries, and usage strings.
- Gives agents a stable command discovery surface without scraping human usage text.

When to run:

- Before an agent chooses a CLI command.
- When documenting or testing command availability.
- After adding a new CLI command to confirm it is discoverable.

Create a starter board project on Windows:

```powershell
.\build-qt\ccad.exe init --name canvas-demo --width-mm 42 --height-mm 28 --out .\build-qt\canvas-demo.ccad.json
```

What it does:

- Creates a deterministic `.ccad.json` project file.
- Adds a `42 mm x 28 mm` board outline.
- Adds default front/back copper layers.

When to run:

- When you need a quick board file for GUI review.
- When testing the board canvas path.
- When starting a small manual experiment.

Inspect a project:

```powershell
.\build-qt\ccad.exe inspect .\build-qt\canvas-demo.ccad.json
```

What it does:

- Loads a project.
- Emits machine-readable review JSON: project summary, counts, status, diagnostics, and board metadata including board origin and size.

When to run:

- When an agent needs project state without opening the GUI.
- When debugging whether counts/status match the file.
- Before handing a design state to another tool.

Validate a project on Windows:

```powershell
.\build-qt\ccad.exe validate .\build-qt\canvas-demo.ccad.json
```

What it does:

- Runs ERC diagnostics.
- Prints diagnostics and summary counts as JSON.
- Exits `0` if there are no ERC errors, `1` if ERC errors exist, `2` for usage/file/parse failures.

When to run:

- After editing project JSON.
- Before opening a project for review.
- In CI or agent workflows as a correctness gate.

Run physical DRC:

```powershell
.\build-qt\ccad.exe drc .\build-qt\canvas-demo.ccad.json
```

What it does:

- Runs kernel-level physical checks on board primitives.
- Emits JSON diagnostics and summary counts.
- Exits `0` when no DRC errors exist, `1` when DRC errors exist, `2` for usage/file/parse failures.
- Reports `COPPER_CLEARANCE` when different-net copper is closer than the configured board-level copper clearance, including that configured value in the diagnostic message.
- Reports invalid rule values when board-level copper clearance, minimum track width, or minimum via annular ring are non-positive in project JSON.
- Reports minimum track-width and via annular-ring violations with the configured threshold value in the diagnostic message.
- Prioritizes invalid via drill geometry over derived annular-ring violations to keep diagnostics focused.
- Prioritizes invalid pad, via, and track dimensions over derived keepout geometry violations.
- Prioritizes invalid pad, via, and track dimensions over derived copper-clearance violations.
- Treats layer-bound pad/track copper as colliding only when they share a copper layer; vias still interact with copper across layers.
- Reports `PAD_NON_COPPER_LAYER` or `TRACK_NON_COPPER_LAYER` when copper primitives reference a valid layer whose kind is not `copper`.

When to run:

- After adding pads, vias, or tracks.
- Before opening the GUI for review.
- Before treating a generated board as a valid intermediate artifact.

Import a KiCad footprint:

```powershell
.\build-qt\ccad.exe lib import-footprint --in .\path\to\R_0805_2012Metric.kicad_mod --out .\build-qt\R_0805_2012Metric.ccad-footprint.json
```

What it does:

- Reads a KiCad `.kicad_mod` footprint file as data.
- Imports the footprint name and basic pad geometry.
- Writes deterministic CCad footprint JSON.

When to run:

- When reusing existing KiCad footprint library assets.
- When building a CCad package/footprint catalog.
- Before future placement commands consume library footprints.

Current limitation:

- The importer handles a narrow pad subset: number, type, shape, position, size, simple drill, and layers.
- Advanced KiCad footprint constructs are skipped safely for now.

Place an imported footprint on a board:

```powershell
.\build-qt\ccad.exe pcb place-footprint --file .\build-qt\canvas-demo.ccad.json --footprint .\build-qt\R_0805_2012Metric.ccad-footprint.json --component R1 --at-x-mm 16 --at-y-mm 14 --layer F.Cu --rotation-deg 90
```

What it does:

- Loads CCad footprint JSON.
- Places each footprint pad onto the board as a board pad.
- Generates stable pad IDs such as `R1.1` and `R1.2`.
- Preserves `net_id` from logical project nets when a matching component/pin net member exists.
- Rotates footprint-local pad centers and pad orientation when `--rotation-deg` is provided.

When to run:

- After importing a KiCad footprint.
- When generating board geometry from reusable library data.
- Before GUI review of placed package pads.

Current limitation:

- Pads without a matching logical net member keep an empty `net_id` and DRC reports that as a warning.
- Flipping, courtyard checks, automatic symbol-footprint assignment, and full schematic parity are later work.

Add PCB primitives through the CLI:

```powershell
.\build-qt\ccad.exe pcb list-nets --file .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe pcb list-objects --file .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe pcb list-objects --file .\build-qt\canvas-demo.ccad.json --type track
.\build-qt\ccad.exe pcb get-object --file .\build-qt\canvas-demo.ccad.json --id F.Cu
.\build-qt\ccad.exe pcb set-outline --file .\build-qt\canvas-demo.ccad.json --x-mm 0 --y-mm 0 --width-mm 44 --height-mm 30
.\build-qt\ccad.exe pcb set-rules --file .\build-qt\canvas-demo.ccad.json --copper-clearance-mm 0.20 --min-track-width-mm 0.15 --min-via-annular-ring-mm 0.10
.\build-qt\ccad.exe pcb add-layer --file .\build-qt\canvas-demo.ccad.json --id In1.Cu --name "Inner 1 copper" --kind copper --visible false
.\build-qt\ccad.exe pcb add-standard-layers --file .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe pcb set-layer --file .\build-qt\canvas-demo.ccad.json --id In1.Cu --name "Inner signal copper" --kind copper --visible true
.\build-qt\ccad.exe pcb set-layer-visibility --file .\build-qt\canvas-demo.ccad.json --id In1.Cu --visible true
.\build-qt\ccad.exe pcb remove-layer --file .\build-qt\canvas-demo.ccad.json --id In1.Cu
.\build-qt\ccad.exe pcb add-pad --file .\build-qt\canvas-demo.ccad.json --id P1 --component U1 --pin 1 --net N1 --layers F.Cu --x-mm 5 --y-mm 6 --width-mm 1.5 --height-mm 1.0
.\build-qt\ccad.exe pcb set-pad --file .\build-qt\canvas-demo.ccad.json --id P1 --component U1 --pin 1 --net N1 --layers F.Cu --rotation-deg 90
.\build-qt\ccad.exe pcb add-via --file .\build-qt\canvas-demo.ccad.json --id V1 --net N1 --x-mm 8 --y-mm 9 --diameter-mm 0.8 --drill-mm 0.4
.\build-qt\ccad.exe pcb set-via --file .\build-qt\canvas-demo.ccad.json --id V1 --diameter-mm 1.0 --drill-mm 0.5 --net N2
.\build-qt\ccad.exe pcb add-track --file .\build-qt\canvas-demo.ccad.json --id T1 --net N1 --layer F.Cu --start-x-mm 5 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9 --width-mm 0.25
.\build-qt\ccad.exe pcb add-route-request --file .\build-qt\canvas-demo.ccad.json --id RR1 --net N1 --from P1 --to V1 --preferred-layer F.Cu --policy shortest_safe --width-mm 0.25
.\build-qt\ccad.exe pcb set-route-request --file .\build-qt\canvas-demo.ccad.json --id RR1 --net N1 --from V1 --to T1 --preferred-layer B.Cu --policy prefer_back --width-mm 0.30
.\build-qt\ccad.exe pcb export-route-job --file .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe pcb apply-route-segment --file .\build-qt\canvas-demo.ccad.json --request-id RR1 --track-id RT1 --layer F.Cu --start-x-mm 5 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9
.\build-qt\ccad.exe pcb apply-route-polyline --file .\build-qt\canvas-demo.ccad.json --request-id RR1 --track-prefix RTP --points-mm "5,6;6.5,7.5;8,9"
.\build-qt\ccad.exe pcb route-status --file .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe pcb set-track --file .\build-qt\canvas-demo.ccad.json --id T1 --start-x-mm 6 --start-y-mm 7 --end-x-mm 9 --end-y-mm 10 --width-mm 0.30 --net N2 --layer B.Cu
.\build-qt\ccad.exe pcb set-region-kind --file .\build-qt\canvas-demo.ccad.json --id K1 --kind routing
.\build-qt\ccad.exe pcb move-object --file .\build-qt\canvas-demo.ccad.json --id V1 --x-mm 9 --y-mm 10
.\build-qt\ccad.exe pcb resize-object --file .\build-qt\canvas-demo.ccad.json --id P1 --width-mm 2.0 --height-mm 1.2
.\build-qt\ccad.exe pcb remove-object --file .\build-qt\canvas-demo.ccad.json --id T1
```

What these do:

- `pcb list-nets` emits compact physical net usage counts for pads, vias, and tracks.
- `pcb route-status` emits compact route progress counts for open, partial, and completed route requests, derived from current route requests and track provenance.
- `inspect` also reports board route progress in review JSON so GUI and agents share the same route-review contract.
- `pcb export-route-job` emits a compact deterministic route-job JSON envelope with schema/version metadata, units, board outline, design rules, layers, pads, vias, tracks, keepouts, placement regions, and route requests; pass `--request-id <id>` to export one request. Pad entries include KiCad-style `pad_type`, `shape`, `layers`, optional `drill_nm`, optional `roundrect_rratio`, optional `chamfer_ratio`, position, rotation, and size so external routers and AI tools can reason about actual pad geometry.
- `pcb list-objects` emits compact board layer and physical object rows as JSON, with optional type filtering for `layer`, `pad`, `via`, `track`, `keepout`, or `placement_region`. Pad rows include KiCad-style `pad_type`, `shape`, optional `drill_nm`, optional `roundrect_rratio`, and optional `chamfer_ratio`.
- `pcb get-object` emits one board layer, pad, via, track, keepout, or placement region by stable ID as compact JSON. Pad objects include their KiCad-style type, shape, drill, ratio, layer, position, rotation, and size metadata.
- `pcb set-outline` replaces the rectangular board outline while rejecting outlines that would leave existing pads, vias, tracks, keepouts, or placement regions outside the board.
- `pcb set-rules` updates board-level DRC defaults for copper clearance, minimum track width, and minimum via annular ring.
- `pcb add-layer` appends a board layer with stable ID, display name, kind, and optional visibility.
- `pcb add-standard-layers` appends any missing KiCad standard named layers without removing existing custom layers.
- `pcb set-layer` updates an existing layer's display name, kind, and visibility while rejecting non-copper kind changes for layers referenced by pads or tracks.
- `pcb remove-layer` removes an unused board layer by stable ID.
- `pcb set-layer-visibility` updates an existing layer's visibility flag for review surfaces.
- `pcb add-pad` appends a KiCad-style pad to an existing board project. Optional metadata includes `--type`, `--shape`, `--drill-mm`, `--roundrect-rratio`, and `--chamfer-ratio`.
- `pcb set-pad` updates an existing pad's component, pin, net, layers, type, shape, roundrect ratio, chamfer ratio, and rotation.
- `pcb add-via` appends a plated via with diameter and drill size.
- `pcb set-via` updates an existing via diameter, drill size, and optional net ID.
- `pcb add-track` appends a straight copper track segment.
- `pcb set-track` updates an existing straight track segment's endpoints, width, optional net ID, and optional copper layer ID.
- `pcb set-route-request` updates an existing route-request intent record's net, endpoint object IDs, preferred copper layer, policy, and width.
- `pcb remove-route-request` removes an existing route-request intent record by stable ID.
- `pcb apply-route-segment` appends one track segment from a route request's net and width, records `source_route_request_id`, and defaults to the request's preferred layer when `--layer` is omitted; use `--complete false` for intermediate route segments, otherwise the satisfied request is removed.
- `pcb apply-route-polyline` expands `--points-mm "x,y;x,y;..."` into multiple track segments with generated IDs from `--track-prefix`, preserves route request provenance on every segment, and follows the same completion behavior as `pcb apply-route-segment`.
- `pcb set-region-kind` updates an existing keepout or placement region kind.
- `pcb move-object` moves a pad or via center, or a keepout or placement-region origin, by stable ID.
- `pcb resize-object` resizes a pad, keepout, or placement region by stable ID.
- `pcb remove-object` removes one physical board object by stable ID.
- These commands rewrite the same `.ccad.json` file using deterministic JSON.

When to run:

- When an agent needs to mutate a board without manual JSON editing.
- When building a small reproducible PCB test case.
- Before launching the GUI to review generated primitive geometry.

Current command guards:

- The project must already have a board.
- Outline width and height must be positive.
- Rule dimensions must be positive.
- Primitive IDs must be unique within their primitive type.
- Layer IDs must be unique.
- Removed layers must exist and must not be referenced by pads or tracks.
- Dimensions must be positive.
- Referenced layers must exist for pads and tracks.
- Pad, track, and placed-footprint layers must be copper layers.
- Updated pad layers must be copper layers, and updated pad rotation must remain inside the board outline.
- Positions and track endpoints must be inside the board outline.
- Updated track endpoints and copper width margin must remain inside the board outline.
- Via drill must be less than or equal to via diameter.
- Updated via diameter and drill must stay valid and fully inside the board outline.
- Moved pads, vias, keepouts, and placement regions must remain fully inside the board outline.
- Resized pads, keepouts, and placement regions must remain fully inside the board outline.
- Removal IDs must match an existing pad, via, track, keepout, or placement region.

Rectangular keepouts:

- Keepouts are represented in project JSON, can be authored through the CLI, are visible in the Qt board canvas, and are checked by DRC.
- A keepout has `id`, `kind`, and rectangular `area`.
- DRC errors if a pad center, via center, track endpoint, or track segment lies inside or crosses a keepout.

Add a keepout through the CLI:

```powershell
.\build-qt\ccad.exe pcb add-keepout --file .\build-qt\canvas-demo.ccad.json --id K1 --kind placement --x-mm 20 --y-mm 10 --width-mm 4 --height-mm 3
```

Example JSON fragment:

```json
"keepouts": [
  {
    "id": "keepout-mounting-hole",
    "kind": "placement",
    "area": {
      "x_nm": 20000000,
      "y_nm": 20000000,
      "width_nm": 4000000,
      "height_nm": 3000000
    }
  }
]
```

Current limitation:

- These commands create raw primitives only. They do not run full DRC, enforce schematic parity, solve placement, or route nets automatically yet.

Placement regions:

- Placement regions are represented in project JSON, can be authored through the CLI, are visible in the Qt board canvas, are listed in review/object browser surfaces, and are checked by DRC.
- A placement region has `id`, `kind`, and rectangular `area`.
- Current accepted kinds are `component` and `module`.
- Placement regions are guidance geometry for future placement engines and human/agent review. They do not automatically move components yet.

Add a placement region through the CLI:

```powershell
.\build-qt\ccad.exe pcb add-placement-region --file .\build-qt\canvas-demo.ccad.json --id PR1 --kind component --x-mm 8 --y-mm 6 --width-mm 12 --height-mm 8
```

Example JSON fragment:

```json
"placement_regions": [
  {
    "id": "power-cluster",
    "kind": "component",
    "area": {
      "x_nm": 8000000,
      "y_nm": 6000000,
      "width_nm": 12000000,
      "height_nm": 8000000
    }
  }
]
```

## GUI

CCad can build an optional native Qt 6 Widgets review GUI named `ccad_gui`. It is intended for human co-working and review while the kernel and CLI remain the source of truth.

If Qt 6 Widgets is installed, CMake builds it automatically:

```bash
cmake -S . -B build-qt -DCCAD_BUILD_GUI=ON -DCMAKE_PREFIX_PATH=C:\Qt\6.11.1\mingw_64
cmake --build build-qt
```

If Qt is not installed, core, CLI, and tests still build.

On Windows, add Qt DLLs to `PATH` before launching the GUI:

```powershell
$env:PATH = "C:\Qt\6.11.1\mingw_64\bin;$env:PATH"
.\build-qt\ccad_gui.exe
```

Open a project directly:

```powershell
$env:PATH = 'C:\Qt\6.11.1\mingw_64\bin;' + $env:PATH
.\build-qt\ccad_gui.exe .\build-qt\canvas-demo.ccad.json
```

Dump a read-only semantic UI map for agent tooling:

```powershell
$env:PATH = 'C:\Qt\6.11.1\mingw_64\bin;' + $env:PATH
.\build-qt\ccad_gui.exe --dump-ui-map .\artifacts\demos\sprint156-layer-color-final.ccad.json .\artifacts\demos\ui-map.json
```

The map reports stable IDs such as `action:add_footprint`, `control:active_pcb_layer`, `tab:pcb`, `canvas:pcb`, and `canvas_object:JAC1.1`, plus screen-space target coordinates and CAD metadata. The top-level map also reports `active_pcb_layer_id`.

Validate the exported target coordinates against the live Qt window:

```powershell
$env:PATH = 'C:\Qt\6.11.1\mingw_64\bin;' + $env:PATH
.\build-qt\ccad_gui.exe --validate-ui-map-targets .\artifacts\demos\sprint156-layer-color-final.ccad.json .\artifacts\demos\ui-map-target-validation.json
```

The validator moves the cursor to every visible/enabled exported target and verifies that Qt hit-testing resolves the expected widget, tab, canvas, or canvas item. Hidden or disabled nodes are reported as skipped.

Query one target by semantic ID:

```powershell
$env:PATH = 'C:\Qt\6.11.1\mingw_64\bin;' + $env:PATH
.\build-qt\ccad_gui.exe --ui-target-id .\artifacts\demos\sprint156-layer-color-final.ccad.json action:add_footprint .\artifacts\demos\ui-target-add-footprint.json
```

The native GUI also exposes its bottom Agent panel as `panel:agent`:

```powershell
$env:PATH = 'C:\Qt\6.11.1\mingw_64\bin;' + $env:PATH
.\build-qt\ccad_gui.exe --ui-target-id .\artifacts\demos\sprint156-layer-color-final.ccad.json panel:agent .\artifacts\demos\ui-target-agent-panel.json
```

The panel itself can refresh the current UI-map JSON and trigger allowlisted safe actions such as `action:zoom_in`. It is a local shell only; provider keys, BYOK routing, LangGraph orchestration, and OpenTelemetry/Langfuse tracing remain future integration work.

Query one PCB board point:

```powershell
$env:PATH = 'C:\Qt\6.11.1\mingw_64\bin;' + $env:PATH
.\build-qt\ccad_gui.exe --ui-target-board-point .\artifacts\demos\sprint156-layer-color-final.ccad.json 8 17 .\artifacts\demos\ui-target-board-point.json
```

Target responses include logical Qt pixels, physical pixels, and the device-pixel ratio so OS-level input tools do not have to infer high-DPI scaling.

Trigger a safe non-destructive GUI action directly by semantic ID:

```powershell
$env:PATH = 'C:\Qt\6.11.1\mingw_64\bin;' + $env:PATH
.\build-qt\ccad_gui.exe --ui-trigger-safe .\artifacts\demos\sprint156-layer-color-final.ccad.json action:zoom_in .\artifacts\demos\ui-action-zoom-in.json
```

Only a small allowlist of view/navigation actions is executable this way. Mutating, dialog-opening, or file-writing actions return `performed:false` with a reason field.

Left-toolbar display controls such as `action:grid`, `action:polar_coord`, `action:unit_inch`, `action:cursor_shape`, `action:show_ratsnest`, `action:net_highlight`, and `action:contrast_mode` are implemented safe display actions. They return `performed:true`, `reason:"display_state_toggled"`, and their current state, while the UI map exposes checked state for action nodes. Left-toolbar panel controls `action:layers_manager` and `action:part_properties` are implemented safe actions and return `reason:"panel_toggled"`. Right-toolbar PCB editor entries `action:add_tracks`, `action:add_via`, `action:add_keepout_area`, `action:add_graphical_segments`, and `action:text` now return `performed:true`, `reason:"editor_tool_selected"`, and a concrete mode value, while `action:delete_cursor` removes the selected board primitive when the model supports that object type. Remaining editor tools such as `action:add_zone` are not silently ignored; they still return `performed:false`, `reason:"future_tool_not_implemented"`, and the user-facing label while the GUI status bar shows the same planned-tool state.

Query or set the GUI PCB active layer:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --ui-active-layer artifacts\demos\sprint171-pcb-active-layer-context-final.ccad.json artifacts\demos\sprint171-active-layer.json"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --ui-set-active-layer artifacts\demos\sprint171-pcb-active-layer-context-final.ccad.json B.Cu artifacts\demos\sprint171-set-active-layer.json"
```

The active layer is GUI/editor state, not extra board JSON. It lists and accepts board copper layers only. Footprint placement and Route Track use the selected active copper layer; via placement remains through-board but uses the active copper color for its placement ghost.

Query or set the GUI PCB active net:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --ui-active-net artifacts\demos\sprint172-pcb-active-net-context-final.ccad.json artifacts\demos\sprint172-active-net.json"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --ui-set-active-net artifacts\demos\sprint172-pcb-active-net-context-final.ccad.json DC_NEG artifacts\demos\sprint172-set-active-net.json"
```

The active net is GUI/editor state, not extra board JSON. It derives available nets from top-level project nets and existing board copper net IDs, exposes `control:active_pcb_net` in the UI map, and is used by Add Via and Route Track when they create new copper. Footprint placement still maps pad nets from component-pin connectivity.

Run the app-owned placement-click regression harness:

```powershell
$env:PATH = 'C:\Qt\6.11.1\mingw_64\bin;' + $env:PATH
.\build-qt\ccad_gui.exe --test-place-footprint-click .\artifacts\demos\sprint160-placement-click-smoke.ccad.json .\artifacts\demos\sprint159-safe-ui-actions-final-R_0805_2012Metric.ccad-footprint.json 18 12 .\artifacts\demos\sprint160-placement-click-result.json
```

This loads the project in the native GUI, enters footprint placement mode, sends the placement click through the real Qt viewport event path, writes compact result JSON, and exits. It is intended for regression and agent harness work, not for normal user authoring.

The same app-owned viewport-click path can exercise first-batch PCB edit tools:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --test-place-via-click artifacts\demos\sprint170-pcb-edit-tool-entry-final.ccad.json 15 11 artifacts\demos\sprint170-via-result.json"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --test-route-track-click artifacts\demos\sprint170-pcb-edit-tool-entry-final.ccad.json 8 9 15 11 artifacts\demos\sprint170-track-result.json"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --test-place-keepout-click artifacts\demos\sprint170-pcb-edit-tool-entry-final.ccad.json 20 10 25 14 artifacts\demos\sprint170-keepout-result.json"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --test-delete-board-object artifacts\demos\sprint170-pcb-edit-tool-entry-final.ccad.json V1 artifacts\demos\sprint170-delete-result.json"
```

These modes enter the same Add Via, Route Track, Add Keepout, and Delete paths that humans invoke from the right toolbar, then return compact JSON such as `reason:"placed"` or `reason:"deleted"` with the updated object count or deleted type.

Run the app-owned UI-map mouse-target harness:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_ui_map_mouse_target_demo.ps1 -Name sprint161-ui-map-mouse-targets -ProjectPath .\artifacts\demos\sprint160-placement-crash-ci-final.ccad.json
```

The script plays the docs beep, waits two seconds, launches the GUI target-sequence mode, lets the window settle for five seconds, moves the cursor to semantic targets such as Select, Measure, Save, File, the properties/DRC panel, and the Agent tab, waits about 800 ms per target before screenshots, saves marked PNGs, resizes the window, repeats the same targets, and writes a JSON report.

Run the live UI-map local socket server:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --serve-ui-map artifacts\demos\sprint162-pad-layer-rendering-fidelity-final.ccad.json ccad-ui-map-demo artifacts\demos\ccad-ui-map-demo.ready.txt"
```

While the GUI stays open, connect to the named local socket and send one JSON request per line. Current methods are `{"method":"ui.map"}`, `{"method":"ui.target","id":"menu:file"}`, `{"method":"ui.epoch"}`, `{"method":"ui.active_layer"}`, and `{"method":"ui.set_active_layer","layer_id":"B.Cu"}`. Responses are newline-delimited JSON values.

What it does:

- Starts the native Qt review GUI.
- Loads the selected `.ccad.json` project.
- Shows project counts, route progress, status, diagnostics, board outline, and current PCB primitives.

When to run:

- When a human wants to review what the kernel/CLI generated.
- When checking visual canvas behavior.
- After `init`, `inspect`, or JSON edits when visual confirmation is needed.

Typical local loop:

```powershell
cmake -S . -B build-qt `
  -DCCAD_WARNINGS_AS_ERRORS=ON `
  -DCCAD_BUILD_GUI=ON `
  -DCMAKE_PREFIX_PATH=C:\Qt\6.11.1\mingw_64
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
.\build-qt\ccad.exe init --name canvas-demo --width-mm 42 --height-mm 28 --out .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe pcb set-outline --file .\build-qt\canvas-demo.ccad.json --x-mm 0 --y-mm 0 --width-mm 44 --height-mm 30
.\build-qt\ccad.exe pcb set-rules --file .\build-qt\canvas-demo.ccad.json --copper-clearance-mm 0.20 --min-track-width-mm 0.15 --min-via-annular-ring-mm 0.10
.\build-qt\ccad.exe pcb add-layer --file .\build-qt\canvas-demo.ccad.json --id In1.Cu --name "Inner 1 copper" --kind copper --visible false
.\build-qt\ccad.exe pcb set-layer --file .\build-qt\canvas-demo.ccad.json --id In1.Cu --name "Inner signal copper" --kind copper --visible true
.\build-qt\ccad.exe pcb set-layer-visibility --file .\build-qt\canvas-demo.ccad.json --id In1.Cu --visible true
.\build-qt\ccad.exe pcb add-pad --file .\build-qt\canvas-demo.ccad.json --id P1 --component U1 --pin 1 --net N1 --layers F.Cu --x-mm 5 --y-mm 6 --width-mm 1.5 --height-mm 1.0
.\build-qt\ccad.exe pcb add-via --file .\build-qt\canvas-demo.ccad.json --id V1 --net N1 --x-mm 8 --y-mm 9 --diameter-mm 0.8 --drill-mm 0.4
.\build-qt\ccad.exe pcb set-via --file .\build-qt\canvas-demo.ccad.json --id V1 --diameter-mm 1.0 --drill-mm 0.5 --net N2
.\build-qt\ccad.exe pcb add-track --file .\build-qt\canvas-demo.ccad.json --id T1 --net N1 --layer F.Cu --start-x-mm 5 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9 --width-mm 0.25
.\build-qt\ccad.exe pcb add-route-request --file .\build-qt\canvas-demo.ccad.json --id RR1 --net N1 --from P1 --to V1 --preferred-layer F.Cu --policy shortest_safe --width-mm 0.25
.\build-qt\ccad.exe pcb set-route-request --file .\build-qt\canvas-demo.ccad.json --id RR1 --net N1 --from V1 --to T1 --preferred-layer B.Cu --policy prefer_back --width-mm 0.30
.\build-qt\ccad.exe pcb export-route-job --file .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe pcb apply-route-segment --file .\build-qt\canvas-demo.ccad.json --request-id RR1 --track-id RT1 --layer F.Cu --start-x-mm 5 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9
.\build-qt\ccad.exe pcb set-track --file .\build-qt\canvas-demo.ccad.json --id T1 --start-x-mm 6 --start-y-mm 7 --end-x-mm 9 --end-y-mm 10 --width-mm 0.30 --net N2 --layer B.Cu
.\build-qt\ccad.exe pcb add-layer --file .\build-qt\canvas-demo.ccad.json --id F.SilkS --name "Front silkscreen" --kind silkscreen --visible true
.\build-qt\ccad.exe pcb add-layer --file .\build-qt\canvas-demo.ccad.json --id Dwgs.User --name "User drawings" --kind user --visible true
.\build-qt\ccad.exe pcb add-graphic-line --file .\build-qt\canvas-demo.ccad.json --id G1 --layer Dwgs.User --start-x-mm 3 --start-y-mm 4 --end-x-mm 16 --end-y-mm 4 --width-mm 0.15
.\build-qt\ccad.exe pcb add-text --file .\build-qt\canvas-demo.ccad.json --id BT1 --layer F.SilkS --text "Bridge rectifier" --x-mm 8 --y-mm 22 --size-x-mm 1.5 --size-y-mm 1.5 --rotation-deg 0
.\build-qt\ccad.exe pcb add-placement-region --file .\build-qt\canvas-demo.ccad.json --id PR1 --kind component --x-mm 11 --y-mm 4 --width-mm 12 --height-mm 8
.\build-qt\ccad.exe pcb add-keepout --file .\build-qt\canvas-demo.ccad.json --id K1 --kind placement --x-mm 20 --y-mm 10 --width-mm 4 --height-mm 3
.\build-qt\ccad.exe inspect .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe validate .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe drc .\build-qt\canvas-demo.ccad.json
$env:PATH = 'C:\Qt\6.11.1\mingw_64\bin;' + $env:PATH
.\build-qt\ccad_gui.exe .\build-qt\canvas-demo.ccad.json
```

Generate demo artifacts and a GUI screenshot:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_sprint_demo.ps1 -Name sprint42-gui-canvas-mvp-review
```

What it does:

- Creates `artifacts/demos/sprint42-gui-canvas-mvp-review.ccad.json`.
- Sets a non-zero-origin board outline and board-level DRC rules, adds KiCad standard layers, toggles layer visibility, adds a full bridge rectifier PCB demo with pads, vias, tracks, board graphic lines, board text, a rectangular placement region, logical demo nets, and a rectangular keepout.
- Writes inspect, validate, and DRC JSON reports.
- Writes a sample KiCad `.kicad_mod` file and imports it to CCad footprint JSON.
- Places the imported footprint onto the demo board.
- First attempts Qt internal screenshot mode (`ccad_gui --screenshot`).
- If that path fails on the host, it launches the GUI normally, waits for window readiness, captures the exact CCad window bounds, and closes only the spawned GUI process.
- Uses a 7-second single-preview settle window by default.
- Default script behavior now prefers the stable window-capture path first; pass `-PreferInternalScreenshot` only when validating the native `--screenshot` code path.

When to run:

- At sprint end.
- After GUI-visible commits.
- When handing off progress for visual review.
