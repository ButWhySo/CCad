# Qt Review GUI Design

## Purpose

Add the first human co-working surface for CCad without weakening the machine-first architecture. The GUI is a native desktop review shell: it loads the same `.ccad.json` projects as the CLI, shows project summary and ERC diagnostics, and lets humans inspect agent progress. It does not become the source of truth.

## Scope

This phase adds:

- A tested review view-model in `ccad_core`.
- An optional Qt 6 Widgets executable named `ccad_gui`.
- Documentation for building with and without Qt.

This phase does not add schematic drawing, PCB canvas rendering, placement, routing, editing, or transactions.

## Architecture

`ccad_core` owns all project loading and review state. The GUI calls core functions and renders their output. This keeps GUI behavior testable without running a desktop event loop.

Modules:

- `src/ccad_core/review.hpp/.cpp`: project review summary and diagnostic formatting.
- `src/ccad_gui/main.cpp`: Qt 6 Widgets shell.
- `tests/test_review.cpp`: tests core review behavior.

Qt is optional. If Qt 6 Widgets is installed, CMake builds `ccad_gui`. If not installed, the core, CLI, and tests still build and pass. Linux CI can later install Qt and set `CCAD_BUILD_GUI=ON`.

## User Experience

The first GUI window provides:

- Open project file.
- Reload current file.
- Project metadata: ID, name, component count, net count, constraint count.
- ERC diagnostics table/list with severity, code, object ID, and message.
- Status text showing clean design, warnings, errors, or load failure.

The UI is review-focused. It intentionally avoids editing controls until transactions exist.

## Error Handling

Invalid files show a load error in the status area. ERC errors remain structured diagnostics. The GUI never executes project file content.

## Testing

Tests cover the core review model:

- Valid project summary counts.
- Clean project status.
- Invalid net reference status and diagnostics.
- Empty project warning status.

GUI build is verified by CMake when Qt is available. Core tests remain the correctness gate.

## Security

The GUI opens local project files as data only. It does not shell out, run plugins, load network resources, or execute commands from project content.

