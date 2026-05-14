# CCad

CCad is a ground-up, machine-callable PCB design kernel for native desktop CAD software. The first milestone builds a C++ logical circuit core, deterministic JSON representation, ERC checks, and a native CLI that agents can call without driving a GUI.

## Current Phase

Phase 1: kernel MVP.

- Typed project model.
- Deterministic JSON load/dump.
- Logical ERC diagnostics.
- CLI: `ccad init` and `ccad validate`.

Out of scope for this phase: GUI, placement, routing, KiCad import/export, fabrication outputs, and network services.

## Setup

Install CMake and a C++20 compiler, then build:

```bash
cmake -S . -B build
cmake --build build
```

## Verify

```bash
ctest --test-dir build --output-on-failure
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
