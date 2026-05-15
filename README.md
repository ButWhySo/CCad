# CCad

CCad is a ground-up, machine-callable PCB design kernel for native desktop CAD software. The first milestone builds a C++ logical circuit core, deterministic JSON representation, ERC checks, and a native CLI that agents can call without driving a GUI.

## Current Phase

Phase 2 / 6: physical primitives and early board authoring.

Progress counter: Phase 2 / 6, Sprint 38 merged; Sprint 39 planning. See `docs/devops/progress.md`.

- Typed project model.
- Deterministic JSON load/dump.
- Logical ERC diagnostics.
- CLI: `ccad help --format json`, `ccad init`, `ccad validate`, `ccad inspect`, and `ccad diff`.
- CLI PCB authoring: `ccad pcb add-pad`, `ccad pcb add-via`, `ccad pcb add-track`, and `ccad pcb add-keepout`.
- CLI footprint placement preserves logical net IDs when component pins already appear in project nets.
- Native library catalog metadata, lookup, and search for local/offline component-library caches.
- Physical board outline, layers, rectangular keepouts, pads, vias, and track segments in project JSON.
- Physical DRC for geometry, connectivity metadata, rectangular keepout occupancy, and track crossing violations.
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

Out of scope for this phase: full interactive editing, placement engine, routing engine, KiCad import/export, fabrication outputs, and network services.

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

See `docs/features/implemented-features.md` for a complete feature-by-feature usage and testing guide.

Agents should read `docs/codebase-map.md` before editing code. It is the maintained map of files, modules, public functions, and invariants.

## Library Cache Direction

CCad should reuse KiCad's symbol, footprint, and 3D model ecosystem through local source caches and CCad-native catalogs.

For large designs, CCad should use semantic batches, enriched component knowledge, local catalog search, and checkpoint-based visual review instead of thousands of primitive commands. See `docs/architecture/large-design-and-component-knowledge-pipeline.md`.

For large designs, CCad should use semantic batches, enriched component knowledge, local catalog search, and checkpoint-based visual review instead of thousands of primitive commands. See `docs/architecture/large-design-and-component-knowledge-pipeline.md`.

Policy:

- Do not fetch the internet repeatedly during normal design work.
- Do not vendor huge KiCad/Gitee/GitHub library dumps into this source repo.
- Keep raw upstream checkouts and converted catalogs in ignored local paths such as `library-cache/` or `catalog-cache/`.
- Store runtime/search data in CCad's own catalog format.
- Preserve source URL, mirror, commit/hash, source path, checksum, license, provenance, and import warnings for each item.
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
- Emits machine-readable review JSON: project summary, counts, status, diagnostics, and board metadata.

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
.\build-qt\ccad.exe pcb add-pad --file .\build-qt\canvas-demo.ccad.json --id P1 --component U1 --pin 1 --net N1 --layer F.Cu --x-mm 5 --y-mm 6 --width-mm 1.5 --height-mm 1.0
.\build-qt\ccad.exe pcb add-via --file .\build-qt\canvas-demo.ccad.json --id V1 --net N1 --x-mm 8 --y-mm 9 --diameter-mm 0.8 --drill-mm 0.4
.\build-qt\ccad.exe pcb add-track --file .\build-qt\canvas-demo.ccad.json --id T1 --net N1 --layer F.Cu --start-x-mm 5 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9 --width-mm 0.25
```

What these do:

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
- Primitive IDs must be unique within their primitive type.
- Dimensions must be positive.
- Referenced layers must exist for pads and tracks.
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
.\build-qt\ccad.exe pcb add-pad --file .\build-qt\canvas-demo.ccad.json --id P1 --component U1 --pin 1 --net N1 --layer F.Cu --x-mm 5 --y-mm 6 --width-mm 1.5 --height-mm 1.0
.\build-qt\ccad.exe pcb add-via --file .\build-qt\canvas-demo.ccad.json --id V1 --net N1 --x-mm 8 --y-mm 9 --diameter-mm 0.8 --drill-mm 0.4
.\build-qt\ccad.exe pcb add-track --file .\build-qt\canvas-demo.ccad.json --id T1 --net N1 --layer F.Cu --start-x-mm 5 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9 --width-mm 0.25
.\build-qt\ccad.exe pcb add-keepout --file .\build-qt\canvas-demo.ccad.json --id K1 --kind placement --x-mm 20 --y-mm 10 --width-mm 4 --height-mm 3
.\build-qt\ccad.exe inspect .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe validate .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe drc .\build-qt\canvas-demo.ccad.json
$env:PATH = 'C:\Qt\6.11.1\mingw_64\bin;' + $env:PATH
.\build-qt\ccad_gui.exe .\build-qt\canvas-demo.ccad.json
```

Generate demo artifacts and a GUI screenshot:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_sprint_demo.ps1 -Name sprint7-drc-demo
```

What it does:

- Creates `artifacts/demos/sprint7-drc-demo.ccad.json`.
- Adds a pad, via, track, and rectangular keepout through the CLI.
- Writes inspect, validate, and DRC JSON reports.
- Writes a sample KiCad `.kicad_mod` file and imports it to CCad footprint JSON.
- Places the imported footprint onto the demo board.
- Launches the Qt GUI and saves a screenshot under `artifacts/screenshots/`.

When to run:

- At sprint end.
- After GUI-visible commits.
- When handing off progress for visual review.
