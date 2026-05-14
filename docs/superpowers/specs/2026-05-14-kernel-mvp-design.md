# Kernel MVP Design

## Purpose

Build the base of CCad as a ground-up, machine-callable PCB design kernel. This phase does not attempt GUI, autorouting, KiCad import, or fabrication export. It creates the stable logical-design core that later placement, routing, GUI, and interoperability layers can depend on.

## Scope

Phase 0 establishes repo hygiene, documentation, testing, CI, and agent handover instructions.

Phase 1 establishes a logical circuit kernel with:

- Projects with stable IDs and deterministic JSON serialization.
- Components with typed pins.
- Nets with typed pin memberships.
- Constraints as first-class data.
- ERC diagnostics for missing pins, duplicate component IDs, duplicate pins on a net, unknown pin references, and unconstrained empty projects.
- CLI commands for creating a project, validating a design JSON file, and printing diagnostics as JSON.

## Architecture

The kernel is a native C++20 library named `ccad_core`. C++ is chosen because the long-term product is native desktop CAD software with a durable kernel, not a web application. The command-line executable is the first machine-callable client of that kernel. Python may be used later for automation, tests, bindings, or scripts, but it is not the source-of-truth implementation.

The code is split into small modules:

- `src/ccad_core`: C++ domain model, serialization, and ERC.
- `src/ccad_cli`: native command-line surface for machine callers and CI.
- `tests`: C++ unit tests run through CTest.

The project JSON file is an artifact format, not the eventual full database. It is the first stable interchange layer used for tests, replay, and future RPC.

## Data Flow

1. User or agent creates a JSON project file with `ccad init`.
2. `ccad_core` loads and validates structure.
3. `ccad_core` emits machine-readable ERC diagnostics.
4. `ccad` exits `0` for clean designs and nonzero for errors.

## Error Handling

All domain validation errors become `Diagnostic` values with:

- `severity`: `error` or `warning`
- `code`: stable machine-readable code
- `message`: concise human text
- `object_id`: affected object where available

CLI parse errors use nonzero exit codes and print concise messages to stderr. ERC results are JSON on stdout for automation. Project files are parsed as data only.

## Testing

Development follows TDD. Unit tests cover:

- Deterministic serialization.
- Valid design has no ERC errors.
- Unknown component or pin in a net is reported.
- Duplicate net memberships are reported.
- CLI validates clean and invalid designs with correct exit codes.

Every commit must run the full local test suite.

## Documentation

Docs include:

- `README.md`: project purpose, setup, build, commands.
- `AGENTS.md`: repo working rules and handover for future agents.
- `docs/technical-handover.md`: architecture, phases, testing, CI, security notes.
- `docs/superpowers/plans/2026-05-14-kernel-mvp.md`: implementation plan.

## Security

The CLI treats project files as data only. It does not execute project content, shell commands, plugins, or network calls. JSON parsing is bounded by normal file reads in this phase. CI runs tests without secrets.

## Out of Scope

- GUI.
- Autorouting.
- Physical placement.
- KiCad/Circuit JSON import/export.
- Remote MCP server or web service.
- Package publishing.
