# Kernel MVP Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the tested base of CCad: repo hygiene, docs, CI, logical project model, deterministic JSON, ERC, and CLI.

**Architecture:** Python package `ccad` with focused modules for model, serialization, ERC, and CLI. JSON is deterministic and schema-versioned. CLI is the first machine-callable surface.

**Tech Stack:** Python 3.11+, pytest, ruff, mypy, GitHub Actions.

---

### Task 1: Repo And Tooling Base

**Files:**
- Create: `pyproject.toml`
- Create: `.gitignore`
- Create: `.github/workflows/ci.yml`
- Create: `README.md`
- Create: `AGENTS.md`
- Create: `docs/technical-handover.md`

- [ ] **Step 1: Add project metadata and tools**

Create `pyproject.toml` with setuptools packaging, pytest, ruff, and mypy settings. Console script: `ccad = ccad.cli:main`.

- [ ] **Step 2: Add CI**

Create GitHub Actions workflow that installs package with dev extras and runs `ruff check .`, `mypy src`, and `pytest`.

- [ ] **Step 3: Add documentation**

Write README, AGENTS handover, and technical handover docs describing phase roadmap, commands, branch rules, testing, and security constraints.

- [ ] **Step 4: Commit**

Run `git add` for docs/tooling and commit `chore: establish project base`.

### Task 2: Model And Serialization

**Files:**
- Create: `src/ccad/__init__.py`
- Create: `src/ccad/model.py`
- Create: `src/ccad/serialize.py`
- Create: `tests/test_serialize.py`

- [ ] **Step 1: Write failing serialization tests**

Tests require deterministic JSON, schema version, project ID/name, component pins, nets, and constraints.

- [ ] **Step 2: Run targeted tests and confirm failure**

Run `pytest tests/test_serialize.py -q`. Expected: import/module failures.

- [ ] **Step 3: Implement dataclasses and JSON functions**

Implement `Pin`, `Component`, `NetMember`, `Net`, `Constraint`, `Project`, `project_to_dict`, `project_from_dict`, `dump_project_json`, and `load_project_json`.

- [ ] **Step 4: Run tests and commit**

Run `pytest tests/test_serialize.py -q`, then commit `feat: add deterministic project model`.

### Task 3: ERC Rules

**Files:**
- Create: `src/ccad/erc.py`
- Create: `tests/test_erc.py`

- [ ] **Step 1: Write failing ERC tests**

Tests cover clean designs, unknown component refs, unknown pin refs, duplicate net membership, and warning for empty projects.

- [ ] **Step 2: Run targeted tests and confirm failure**

Run `pytest tests/test_erc.py -q`. Expected: missing module/functions.

- [ ] **Step 3: Implement typed diagnostics and ERC**

Implement `Diagnostic` and `run_erc(project: Project) -> list[Diagnostic]`.

- [ ] **Step 4: Run tests and commit**

Run `pytest tests/test_erc.py -q`, then commit `feat: add logical erc checks`.

### Task 4: CLI

**Files:**
- Create: `src/ccad/cli.py`
- Create: `tests/test_cli.py`

- [ ] **Step 1: Write failing CLI tests**

Tests cover `ccad init --name demo --out board.json`, `ccad validate board.json`, JSON diagnostics, and nonzero exit for errors.

- [ ] **Step 2: Run targeted tests and confirm failure**

Run `pytest tests/test_cli.py -q`. Expected: missing CLI.

- [ ] **Step 3: Implement CLI**

Implement `init` and `validate` subcommands using `argparse`. `validate` prints JSON diagnostics and exits `1` if any error exists.

- [ ] **Step 4: Run tests and commit**

Run `pytest tests/test_cli.py -q`, then commit `feat: add machine-callable cli`.

### Task 5: Full Verification And Merge

**Files:**
- Modify as needed only for fixes discovered by verification.

- [ ] **Step 1: Run full local verification**

Run `python -m pip install -e .[dev]`, `ruff check .`, `mypy src`, and `pytest`.

- [ ] **Step 2: Fix failures with tests first**

For behavior failures, add or adjust tests before production fixes.

- [ ] **Step 3: Commit final docs/fixes**

Commit with `chore: verify kernel mvp`.

- [ ] **Step 4: Merge locally**

Checkout `main`, merge `phase-0-kernel-base`, rerun full verification, and keep history intact.

