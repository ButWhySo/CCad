# Sprint 11 Design: CLI Module Split

## Goal

Split the native `ccad` CLI out of the current monolithic `src/ccad_cli/main.cpp` before adding more machine-callable PCB authoring commands.

## Why This Sprint Exists

The CLI is the primary LLM-facing surface. It must stay deterministic, readable, testable, and easy for future agents to modify safely. After Sprint 10, `main.cpp` owns process entry, usage text, option parsing, project I/O, JSON response formatting, top-level command dispatch, PCB mutation commands, and library import commands. That shape creates avoidable merge risk and makes later command additions harder to audit.

## Architecture

Keep `ccad` as one executable, but split ownership:

- `src/ccad_cli/main.cpp`: process entry only.
- `src/ccad_cli/app.hpp/.cpp`: top-level CLI dispatcher and usage text.
- `src/ccad_cli/common.hpp/.cpp`: shared option parsing, project/footprint file I/O, diagnostics JSON, review JSON, geometry validation helpers.
- `src/ccad_cli/project_commands.hpp/.cpp`: project-level commands such as `init`, `validate`, `drc`, `inspect`, and `diff`.
- `src/ccad_cli/pcb_commands.hpp/.cpp`: `pcb` command group and board mutation subcommands.
- `src/ccad_cli/lib_commands.hpp/.cpp`: `lib` command group and footprint import.

The split is behavior-preserving. User-visible command syntax and exit codes must remain unchanged.

## Safety Rules

- Do not shell out from CLI production code.
- Do not execute project or footprint file content.
- Do not weaken option validation or duplicate-ID checks.
- Keep durable geometry as integer nanometers.
- Keep all mutation writeback through deterministic `dumpProjectJson`.

## Test Strategy

Use existing black-box CLI tests as characterization coverage:

```powershell
cmake --build build-qt --target ccad_cli_tests
ctest --test-dir build-qt -R cli --output-on-failure
```

Run the full gate before commit and merge:

```powershell
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
```

## Definition Of Done

- `src/ccad_cli/main.cpp` contains only process entry and delegates to `ccad_cli::run`.
- CLI helper and command ownership is split across named modules.
- Existing CLI tests pass without test relaxation.
- Full native Qt build and CTest pass.
- `docs/codebase-map.md` documents every new CLI module and important public prototypes.
