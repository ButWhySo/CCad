# CCad

CCad is a ground-up, machine-callable PCB design kernel for native desktop CAD software. The first milestone builds a C++ logical circuit core, deterministic JSON representation, ERC checks, and a native CLI that agents can call without driving a GUI.

## Current Phase

Phase 1: kernel + native review GUI foundation.

- Typed project model.
- Deterministic JSON load/dump.
- Logical ERC diagnostics.
- CLI: `ccad init`, `ccad validate`, `ccad inspect`, and `ccad diff`.
- Physical board outline, layers, pads, vias, and track segments in project JSON.
- Optional Qt 6 native GUI for human review and board canvas viewing.

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
.\build-qt\ccad.exe inspect .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe validate .\build-qt\canvas-demo.ccad.json
$env:PATH = 'C:\Qt\6.11.1\mingw_64\bin;' + $env:PATH
.\build-qt\ccad_gui.exe .\build-qt\canvas-demo.ccad.json
```
