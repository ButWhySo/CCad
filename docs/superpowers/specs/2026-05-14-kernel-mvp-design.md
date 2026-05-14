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

The kernel is a Python package named `ccad`. Python is chosen for this base because it gives fast iteration, strong test tooling, readable models, and a practical path to later CLI/RPC/MCP services.

The code is split into small modules:

- `ccad.model`: immutable-ish dataclasses for project, component, pin, net, and constraints.
- `ccad.serialize`: deterministic JSON load/dump with schema versioning.
- `ccad.erc`: rule checks that return typed diagnostics, never raw strings.
- `ccad.cli`: command-line surface for machine callers and CI.

The project JSON file is an artifact format, not the eventual full database. It is the first stable interchange layer used for tests, replay, and future RPC.

## Data Flow

1. User or agent creates a JSON project file with `ccad init`.
2. `ccad.serialize` loads and validates structure.
3. `ccad.erc` emits machine-readable diagnostics.
4. `ccad.cli` exits `0` for clean designs and nonzero for errors.

## Error Handling

All domain validation errors become `Diagnostic` values with:

- `severity`: `error` or `warning`
- `code`: stable machine-readable code
- `message`: concise human text
- `object_id`: affected object where available

CLI parse errors use nonzero exit codes and print concise messages to stderr. ERC results are JSON on stdout for automation.

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

- `README.md`: project purpose, setup, commands.
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
- Remote MCP server.
- Package publishing.

