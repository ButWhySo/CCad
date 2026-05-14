# Implemented Features

This document tracks user-visible and agent-visible features that exist in the repo, how to use them, how to test them, and where they are implemented.

## Build System And DevOps

Status: implemented.

Files:

- `CMakeLists.txt`
- `.github/workflows/ci.yml`
- `.gitignore`
- `docs/devops/sprints/2026-05-14-sprint-1-qt-review-gui.md`

What it does:

- Builds a native C++20 project with CMake.
- Builds `ccad_core`, `ccad`, tests, and optional `ccad_gui`.
- Runs tests through CTest.
- GitHub Actions has a core job and a Linux Qt GUI compile job.

Use:

```bash
cmake -S . -B build -DCCAD_WARNINGS_AS_ERRORS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Core-only build:

```bash
cmake -S . -B build -DCCAD_WARNINGS_AS_ERRORS=ON -DCCAD_BUILD_GUI=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected result:

- CMake configure succeeds.
- Build succeeds.
- CTest reports all tests passing.

## Logical Project Model

Status: implemented.

Files:

- `src/ccad_core/model.hpp`
- `src/ccad_core/model.cpp`
- `tests/test_serialize.cpp`

What it does:

- Represents project metadata.
- Represents components, pins, nets, net members, and constraints.
- Represents optional physical board data: outline and layers.
- Provides the data model used by CLI, ERC, review model, and future GUI/RPC clients.

Test:

```bash
cmake --build build --target ccad_tests
ctest --test-dir build -R serialize --output-on-failure
```

Expected result:

- `serialize` test passes.

## Deterministic JSON Serialization

Status: implemented.

Files:

- `src/ccad_core/serialize.hpp`
- `src/ccad_core/serialize.cpp`
- `tests/test_serialize.cpp`

What it does:

- Converts `Project` objects to stable JSON.
- Loads JSON back into `Project`.
- Escapes JSON strings correctly for control characters, quotes, and slashes.
- Rejects malformed JSON such as trailing garbage and trailing commas.

Test:

```bash
ctest --test-dir build -R serialize --output-on-failure
```

Expected result:

- Round-trip JSON test passes.
- Escape tests pass.
- Malformed JSON rejection tests pass.

## ERC Diagnostics

Status: implemented.

Files:

- `src/ccad_core/erc.hpp`
- `src/ccad_core/erc.cpp`
- `tests/test_erc.cpp`

What it does:

- Runs logical electrical-rule checks.
- Reports typed diagnostics with severity, code, message, and object ID.
- Detects unknown components, unknown pins, duplicate component IDs, duplicate pins, duplicate net members, and empty projects.

Test:

```bash
cmake --build build --target ccad_erc_tests
ctest --test-dir build -R erc --output-on-failure
```

Expected result:

- `erc` test passes.

## Native CLI

Status: implemented.

Files:

- `src/ccad_cli/main.cpp`
- `tests/test_cli.cpp`

What it does:

- Creates empty CCad project files.
- Creates project files with board outline when given `--width-mm` and `--height-mm`.
- Adds PCB pads, vias, and track segments to existing board projects.
- Runs physical DRC diagnostics with `ccad drc`.
- Validates CCad project files.
- Inspects projects and emits review JSON.
- Diffs two project files and emits machine-readable diff JSON.
- Prints ERC diagnostics as JSON.
- Returns `0` when validation has no errors.
- Returns `1` when ERC errors exist.
- Returns `2` for CLI usage, file, or parse failures.

Build:

```bash
cmake --build build --target ccad
```

Use:

```bash
build/ccad init --name demo --out demo.ccad.json
build/ccad validate demo.ccad.json
```

On Windows PowerShell:

```powershell
.\build\ccad.exe init --name demo --out demo.ccad.json
.\build\ccad.exe init --name board --width-mm 42 --height-mm 28 --out board.ccad.json
.\build\ccad.exe validate demo.ccad.json
.\build\ccad.exe inspect demo.ccad.json
.\build\ccad.exe diff before.ccad.json after.ccad.json
.\build\ccad.exe drc board.ccad.json
.\build\ccad.exe pcb add-pad --file board.ccad.json --id P1 --component U1 --pin 1 --net N1 --layer F.Cu --x-mm 5 --y-mm 6 --width-mm 1.5 --height-mm 1.0
.\build\ccad.exe pcb add-via --file board.ccad.json --id V1 --net N1 --x-mm 8 --y-mm 9 --diameter-mm 0.8 --drill-mm 0.4
.\build\ccad.exe pcb add-track --file board.ccad.json --id T1 --net N1 --layer F.Cu --start-x-mm 5 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9 --width-mm 0.25
```

PCB authoring command behavior:

- `pcb add-pad` writes one rectangular pad into `board.pads`.
- `pcb add-via` writes one via into `board.vias`.
- `pcb add-track` writes one straight track segment into `board.tracks`.
- Commands mutate the file passed through `--file`.
- Commands reject missing boards, duplicate primitive IDs, invalid numeric dimensions, unknown layers, out-of-board positions, and via drill larger than via diameter.

Current limitation:

- These commands are explicit primitive authoring verbs, not automatic placement or routing.
- They do not yet enforce schematic-layout parity or physical DRC beyond command argument guards.

DRC command behavior:

- `drc <path>` runs physical board checks.
- Reports duplicate primitive IDs, unknown layers, geometry outside board outline, invalid dimensions, via drill larger than diameter, and zero-length track segments.
- Exits `0` for no DRC errors, `1` for DRC errors, and `2` for usage/file/parse failure.

Test:

```bash
cmake --build build --target ccad_cli_tests
ctest --test-dir build -R cli --output-on-failure
```

Expected result:

- CLI creates a project file.
- CLI validates a clean file with exit code `0`.
- CLI validates an invalid file with nonzero exit and JSON diagnostics.
- CLI PCB commands append primitives and reject invalid mutations.
- CLI DRC command emits JSON physical diagnostics.

## Project Review Model

Status: implemented.

Files:

- `src/ccad_core/review.hpp`
- `src/ccad_core/review.cpp`
- `tests/test_review.cpp`

What it does:

- Builds a human-review summary from a `Project`.
- Reports project ID/name.
- Reports component, net, and constraint counts.
- Carries ERC diagnostics.
- Produces status text such as `Clean: 1 component, 1 net, 1 constraint`, `Warnings: 1`, or `Errors: 1, warnings: 0`.

Test:

```bash
cmake --build build --target ccad_review_tests
ctest --test-dir build -R review --output-on-failure
```

Expected result:

- `review` test passes.

## Project Diff Model

Status: implemented.

Files:

- `src/ccad_core/diff.hpp`
- `src/ccad_core/diff.cpp`
- `tests/test_diff.cpp`

What it does:

- Compares two project objects.
- Reports added, removed, and changed components, nets, and constraints.
- Provides counts and entries for CLI, GUI, and future transaction review.

Test:

```bash
cmake --build build-qt --target ccad_diff_tests
ctest --test-dir build-qt -R diff --output-on-failure
```

Expected result:

- `diff` test passes.

## Physical Units And Board Primitives

Status: implemented.

Files:

- `src/ccad_core/geometry.hpp`
- `src/ccad_core/geometry.cpp`
- `src/ccad_core/model.hpp`
- `src/ccad_core/serialize.cpp`
- `tests/test_geometry.cpp`
- `tests/test_serialize.cpp`

What it does:

- Stores physical lengths as integer nanometers.
- Converts millimeters and mils deterministically.
- Represents points, sizes, rectangles, board outline, and layers.
- Represents first drawable PCB primitives: pads, vias, and track segments.
- Preserves primitive IDs, net IDs, layer IDs, component/pin ownership, and geometry through JSON round-trip.
- Serializes board data in project JSON.

Example board JSON shape:

```json
{
  "id": "proj-primitive-demo",
  "name": "primitive-demo",
  "board": {
    "outline": {
      "origin": { "x_nm": 0, "y_nm": 0 },
      "size": { "width_nm": 42000000, "height_nm": 28000000 }
    },
    "layers": [
      { "id": "F.Cu", "name": "Front copper", "kind": "signal" },
      { "id": "B.Cu", "name": "Back copper", "kind": "signal" }
    ],
    "pads": [
      {
        "id": "pad-u1-1",
        "component_id": "U1",
        "pin_name": "1",
        "net_id": "GND",
        "layer_id": "F.Cu",
        "position": { "x_nm": 5000000, "y_nm": 6000000 },
        "size": { "width_nm": 1500000, "height_nm": 1000000 }
      }
    ],
    "vias": [
      {
        "id": "via-gnd-1",
        "net_id": "GND",
        "position": { "x_nm": 8000000, "y_nm": 9000000 },
        "diameter_nm": 800000,
        "drill_nm": 400000
      }
    ],
    "tracks": [
      {
        "id": "trk-gnd-1",
        "net_id": "GND",
        "layer_id": "F.Cu",
        "start": { "x_nm": 5000000, "y_nm": 6000000 },
        "end": { "x_nm": 8000000, "y_nm": 9000000 },
        "width_nm": 250000
      }
    ]
  }
}
```

Test:

```bash
cmake --build build-qt --target ccad_geometry_tests ccad_tests
ctest --test-dir build-qt -R "geometry|serialize" --output-on-failure
```

Expected result:

- `geometry` and `serialize` tests pass.

## Board Canvas Scene Model

Status: implemented.

Files:

- `src/ccad_core/canvas.hpp`
- `src/ccad_core/canvas.cpp`
- `tests/test_canvas.cpp`

What it does:

- Converts project board data into a deterministic canvas scene model.
- Reports whether a board exists.
- Converts board width/height from nanometers to millimeter view units.
- Converts pads, vias, and track segments into millimeter view units.
- Keeps GUI rendering inputs testable outside Qt.

Test:

```bash
cmake --build build-qt --target ccad_canvas_tests
ctest --test-dir build-qt -R canvas --output-on-failure
```

Expected result:

- `canvas` test passes.

## Transaction Journal Model

Status: implemented.

Files:

- `src/ccad_core/transaction.hpp`
- `src/ccad_core/transaction.cpp`
- `tests/test_transaction.cpp`

What it does:

- Creates a transaction record from before/after project states.
- Records ID, command, summary, project IDs, and diff.
- Emits deterministic transaction JSON.

Test:

```bash
cmake --build build-qt --target ccad_transaction_tests
ctest --test-dir build-qt -R transaction --output-on-failure
```

Expected result:

- `transaction` test passes.

## Optional Qt Review GUI

Status: implemented as optional target.

Files:

- `src/ccad_gui/main.cpp`
- `CMakeLists.txt`

What it does:

- Builds `ccad_gui` when Qt 6 Widgets is available.
- Provides a native desktop window for human review.
- Opens `.ccad.json` project files.
- Shows project summary.
- Shows board dimensions and layer count when board data exists.
- Renders board outline, pads, vias, and track segments in a native Qt `QGraphicsView` canvas when board data exists.
- Shows ERC diagnostics table.
- Supports reload.

Build with Qt:

```bash
cmake -S . -B build-qt -DCCAD_BUILD_GUI=ON -DCMAKE_PREFIX_PATH=C:\Qt\6.11.1\mingw_64
cmake --build build-qt --target ccad_gui
```

Run:

```bash
build-qt/ccad_gui
```

On Windows:

```powershell
$env:PATH = "C:\Qt\6.11.1\mingw_64\bin;$env:PATH"
.\build-qt\ccad_gui.exe
```

Open a project directly:

```powershell
$env:PATH = "C:\Qt\6.11.1\mingw_64\bin;$env:PATH"
.\build-qt\ccad.exe init --name gui-demo --out .\build-qt\gui-demo.ccad.json
.\build-qt\ccad_gui.exe .\build-qt\gui-demo.ccad.json
```

Board canvas demo:

```powershell
$env:PATH = "C:\Qt\6.11.1\mingw_64\bin;$env:PATH"
.\build-qt\ccad.exe init --name canvas-demo --width-mm 42 --height-mm 28 --out .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad_gui.exe .\build-qt\canvas-demo.ccad.json
```

Or run the demo script:

```powershell
.\scripts\run_gui_demo.ps1
```

If Qt is not installed:

- CMake prints `Qt6 Widgets not found; skipping ccad_gui target`.
- Core, CLI, and tests still build.

Manual test:

1. Build with Qt installed.
2. Run `ccad_gui`.
3. Choose `File > Open`.
4. Open a `.ccad.json` file created by `ccad init`.
5. Confirm summary and diagnostics display.
6. Edit the file externally or regenerate it.
7. Click Reload.

Expected result:

- GUI opens project file.
- Summary counts match the file.
- Clean files show clean status.
- Invalid files show diagnostics.
- Board files show the outline on the dark canvas.
- Board files with primitives show red tracks, pink pads, and yellow vias.

Current limitation:

- The GUI is a review canvas, not a full KiCad/Altium-class PCB editor yet.
- Primitive authoring currently happens through the project JSON/kernel path; command verbs for placement/routing are planned next.

## Research And Architecture Docs

Status: implemented.

Files:

- `docs/research/2026-05-14-llm-native-pcb-tool-report.md`
- `docs/research/2026-05-14-kicad-feature-map.md`
- `docs/superpowers/specs/2026-05-14-kernel-mvp-design.md`
- `docs/superpowers/plans/2026-05-14-kernel-mvp.md`
- `docs/superpowers/specs/2026-05-14-qt-review-gui-design.md`
- `docs/superpowers/plans/2026-05-14-qt-review-gui.md`
- `docs/technical-handover.md`
- `AGENTS.md`

What it does:

- Documents the LLM-native PCB thesis.
- Documents prior art and reuse strategy.
- Documents the current kernel MVP.
- Documents the Qt review GUI design.
- Documents agent handover and sprint process.

Review:

Open the files above in the editor.

