# CCad

CCad is a ground-up, machine-callable PCB design kernel for native desktop CAD software. The first milestone builds a C++ logical circuit core, deterministic JSON representation, ERC checks, and a native CLI that agents can call without driving a GUI.

## Current Phase

Phase 2 / 6: physical primitives and early board authoring.

Progress counter: Phase 2 / 6, Sprint 78 in progress on `sprint-78-origin-aware-cursor-status`.

- Typed project model.
- Deterministic JSON load/dump.
- Logical ERC diagnostics.
- CLI: `ccad help --format json`, `ccad init`, `ccad validate`, `ccad inspect`, and `ccad diff`.
- CLI PCB authoring: `ccad pcb set-outline`, `ccad pcb set-rules`, `ccad pcb add-layer`, `ccad pcb set-layer-visibility`, `ccad pcb add-pad`, `ccad pcb add-via`, `ccad pcb add-track`, `ccad pcb add-keepout`, and `ccad pcb add-placement-region`.
- CLI footprint placement preserves logical net IDs when component pins already appear in project nets.
- Native library catalog metadata, lookup, and search for local/offline component-library caches.
- Physical board outline, layers, rectangular placement regions, rectangular keepouts, pads, vias, and track segments in project JSON.
- Project review and `ccad inspect` report the full board outline rectangle and active board-level DRC rules.
- Physical DRC for geometry, connectivity metadata, layer-aware copper connectivity and clearance, rectangular keepout occupancy, track crossing violations, board-level copper clearance, minimum track width, minimum via annular ring, stable physical object IDs, and logical pad/net parity.
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
- Native GUI canvas toolbar has Fit, Zoom Out, Zoom In, and 100% review controls.
- Sprint demo automation now has a robust screenshot fallback path that captures only the spawned CCad window by PID when `ccad_gui --screenshot` fails.
- Native GUI canvas supports CAD-style pan with middle-drag, right-drag, or Shift+left-drag, plus clamped wheel zoom and expanded scene navigation bounds.
- GUI panel/canvas test binaries are now registered in CTest and run in the default `ctest` gate.
- Native GUI canvas supports keyboard navigation: `+`, `-`, `0`, `F`, `Home`, arrow-key panning, and `W/A/S/D` panning.
- Native GUI canvas supports hold-space hand-pan mode with left-drag.
- Native GUI status bar tool state now reflects pan mode transitions (`Select`, `Pan Ready`, `Pan Drag`).
- Native GUI includes in-app navigation controls help (`Help > Navigation Controls`, shortcut `F1`).
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

Out of scope for this phase: full interactive editing, placement engine, routing engine, KiCad import/export, fabrication outputs, and network services.

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
- Prints diagnostics as JSON.
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
- Emits JSON diagnostics.
- Exits `0` when no DRC errors exist, `1` when DRC errors exist, `2` for usage/file/parse failures.
- Reports `COPPER_CLEARANCE` when different-net copper is closer than the current fixed default clearance of `0.20 mm`.
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
.\build-qt\ccad.exe pcb set-outline --file .\build-qt\canvas-demo.ccad.json --x-mm 0 --y-mm 0 --width-mm 44 --height-mm 30
.\build-qt\ccad.exe pcb set-rules --file .\build-qt\canvas-demo.ccad.json --copper-clearance-mm 0.20 --min-track-width-mm 0.15 --min-via-annular-ring-mm 0.10
.\build-qt\ccad.exe pcb add-layer --file .\build-qt\canvas-demo.ccad.json --id In1.Cu --name "Inner 1 copper" --kind copper --visible false
.\build-qt\ccad.exe pcb set-layer-visibility --file .\build-qt\canvas-demo.ccad.json --id In1.Cu --visible true
.\build-qt\ccad.exe pcb add-pad --file .\build-qt\canvas-demo.ccad.json --id P1 --component U1 --pin 1 --net N1 --layer F.Cu --x-mm 5 --y-mm 6 --width-mm 1.5 --height-mm 1.0
.\build-qt\ccad.exe pcb add-via --file .\build-qt\canvas-demo.ccad.json --id V1 --net N1 --x-mm 8 --y-mm 9 --diameter-mm 0.8 --drill-mm 0.4
.\build-qt\ccad.exe pcb add-track --file .\build-qt\canvas-demo.ccad.json --id T1 --net N1 --layer F.Cu --start-x-mm 5 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9 --width-mm 0.25
```

What these do:

- `pcb set-outline` replaces the rectangular board outline while rejecting outlines that would leave existing pads, vias, tracks, keepouts, or placement regions outside the board.
- `pcb set-rules` updates board-level DRC defaults for copper clearance, minimum track width, and minimum via annular ring.
- `pcb add-layer` appends a board layer with stable ID, display name, kind, and optional visibility.
- `pcb set-layer-visibility` updates an existing layer's visibility flag for review surfaces.
- `pcb add-pad` appends a rectangular pad to an existing board project.
- `pcb add-via` appends a plated via with diameter and drill size.
- `pcb add-track` appends a straight copper track segment.
- All three rewrite the same `.ccad.json` file using deterministic JSON.

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
- Dimensions must be positive.
- Referenced layers must exist for pads and tracks.
- Pad, track, and placed-footprint layers must be copper layers.
- Positions and track endpoints must be inside the board outline.
- Via drill must be less than or equal to via diameter.

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

What it does:

- Starts the native Qt review GUI.
- Loads the selected `.ccad.json` project.
- Shows project counts, status, diagnostics, board outline, and current PCB primitives.

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
.\build-qt\ccad.exe pcb set-layer-visibility --file .\build-qt\canvas-demo.ccad.json --id In1.Cu --visible true
.\build-qt\ccad.exe pcb add-pad --file .\build-qt\canvas-demo.ccad.json --id P1 --component U1 --pin 1 --net N1 --layer F.Cu --x-mm 5 --y-mm 6 --width-mm 1.5 --height-mm 1.0
.\build-qt\ccad.exe pcb add-via --file .\build-qt\canvas-demo.ccad.json --id V1 --net N1 --x-mm 8 --y-mm 9 --diameter-mm 0.8 --drill-mm 0.4
.\build-qt\ccad.exe pcb add-track --file .\build-qt\canvas-demo.ccad.json --id T1 --net N1 --layer F.Cu --start-x-mm 5 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9 --width-mm 0.25
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
- Sets a non-zero-origin board outline and board-level DRC rules, adds an extra board layer, toggles that layer's visibility, adds a pad, via, two crossing tracks, a rectangular placement region, logical demo nets, and a rectangular keepout.
- Writes inspect, validate, and DRC JSON reports.
- Writes a sample KiCad `.kicad_mod` file and imports it to CCad footprint JSON.
- Places the imported footprint onto the demo board.
- First attempts Qt internal screenshot mode (`ccad_gui --screenshot`).
- If that path fails on the host, it launches the GUI normally, waits for window readiness, captures the exact CCad window bounds, and closes only the spawned GUI process.
- Default script behavior now prefers the stable window-capture path first; pass `-PreferInternalScreenshot` only when validating the native `--screenshot` code path.

When to run:

- At sprint end.
- After GUI-visible commits.
- When handing off progress for visual review.
