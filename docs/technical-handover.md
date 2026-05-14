# Technical Handover

## Vision

CCad is a native desktop PCB design kernel built for machine callers first. Agents should interact with stable typed objects, deterministic files, validation results, and eventually transactions/RPC. Human UI should render and review kernel state rather than own it.

The research basis for this architecture is documented in `docs/research/2026-05-14-llm-native-pcb-tool-report.md`. The key decision is that CCad is not a KiCad wrapper. KiCad, tscircuit/Circuit JSON, SKiDL, Atopile, Freerouting, pcb-rnd, Magic, and OpenROAD are references or interoperability targets. CCad's source of truth remains its own native kernel.

KiCad-style functionality and UI expectations are mapped in `docs/research/2026-05-14-kicad-feature-map.md`.

## Phase Roadmap

1. Logical kernel: project, components, pins, nets, constraints, ERC, CLI.
2. Physical primitives: board outline, layers, keepouts, placement regions, early DRC.
3. Routing assistance: constrained route requests and external router boundary.
4. Native GUI/reviewer: render schematic/PCB state, diffs, diagnostics, and transaction review.
5. Interop: KiCad, Circuit JSON, DSN/SES, manufacturing exports.
6. Agent protocol: JSON-RPC/MCP over the same transaction bus, with audit logs and permission gates.

The first GUI phase is specified in `docs/superpowers/specs/2026-05-14-qt-review-gui-design.md`. It uses Qt 6 Widgets when available and keeps all review logic in `ccad_core`.

Sprint tracking starts in `docs/devops/sprints/2026-05-14-sprint-1-qt-review-gui.md`.

## Current Architecture

The initial codebase is C++20:

- Model objects are plain C++ structs/classes with explicit JSON conversion.
- JSON output is sorted and stable for diffs.
- ERC returns typed diagnostics for agent consumption.
- CLI commands are intentionally small and deterministic.
- `ccad_gui` is optional and builds only when Qt 6 Widgets is available.

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
