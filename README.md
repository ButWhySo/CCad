# CCad

CCad is a ground-up, machine-callable PCB design kernel. The first milestone builds a logical circuit core, deterministic JSON representation, ERC checks, and a CLI that agents can call without driving a GUI.

## Current Phase

Phase 1: kernel MVP.

- Typed project model.
- Deterministic JSON load/dump.
- Logical ERC diagnostics.
- CLI: `ccad init` and `ccad validate`.

Out of scope for this phase: GUI, placement, routing, KiCad import/export, fabrication outputs, and network services.

## Setup

```bash
python -m pip install -e ".[dev]"
```

## Verify

```bash
ruff check .
mypy src
pytest
```

## CLI

Create an empty project:

```bash
ccad init --name demo --out demo.ccad.json
```

Validate a project:

```bash
ccad validate demo.ccad.json
```

`validate` prints JSON diagnostics. Exit code `0` means no ERC errors. Exit code `1` means at least one error.

