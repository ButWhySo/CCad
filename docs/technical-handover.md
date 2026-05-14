# Technical Handover

## Vision

CCad is a PCB design kernel built for machine callers first. Agents should interact with stable typed objects, deterministic files, validation results, and eventually transactions/RPC. Human UI should render and review kernel state rather than own it.

## Phase Roadmap

1. Logical kernel: project, components, pins, nets, constraints, ERC, CLI.
2. Physical primitives: board outline, layers, keepouts, placement regions, early DRC.
3. Routing assistance: constrained route requests and external router boundary.
4. GUI/reviewer: render schematic/PCB state, diffs, diagnostics, and transaction review.
5. Interop: KiCad, Circuit JSON, DSN/SES, manufacturing exports.

## Current Architecture

The initial codebase is Python 3.11+:

- Model objects are plain dataclasses with explicit JSON conversion.
- JSON output is sorted and stable for diffs.
- ERC returns typed diagnostics for agent consumption.
- CLI commands are intentionally small and deterministic.

## Development Commands

```bash
python -m pip install -e ".[dev]"
ruff check .
mypy src
pytest
```

## CI/CD

GitHub Actions workflow lives in `.github/workflows/ci.yml`. It runs lint, type checks, and tests on pushes to `main` and `phase-*`, plus pull requests to `main`.

There is no remote configured in this local repo. CI config is present for future GitHub use.

## Test Policy

Use TDD for production behavior. Each new kernel rule or CLI behavior should have a focused test that fails before implementation. Keep tests behavioral and avoid mocking domain code.

## Security Posture

- Project JSON is data only.
- No dynamic import or code execution from design files.
- No network calls in kernel or CLI.
- No secrets needed for local tests or CI.
- Future importers should validate input before mapping into kernel objects.

