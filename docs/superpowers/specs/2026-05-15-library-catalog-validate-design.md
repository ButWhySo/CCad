# Sprint 25 Design: Library Catalog Validation

## Purpose

Local CCad catalog files are the safety boundary for future bulk KiCad/library ingestion. Before the project imports large upstream libraries into a native cache, agents need a deterministic command that can reject poisoned, incomplete, or ambiguous catalog metadata without internet access.

## Scope

- Add a core catalog validator that returns typed diagnostics.
- Add `ccad lib catalog-validate --catalog <path.ccad-library.json>`.
- Treat duplicate stable IDs and missing required metadata as errors.
- Keep validation local-only. It does not fetch upstream files or verify hashes on disk yet.

## Required Fields

Each catalog item must have `id`, `kind`, `name`, `source_path`, `native_path`, `sha256`, `license`, and `provenance`.

## CLI Contract

```cmd
ccad lib catalog-validate --catalog <path.ccad-library.json>
```

- Exit `0` when there are no diagnostics.
- Exit `1` when catalog diagnostics exist.
- Exit `2` for command or parse/load errors.
- Output machine-readable JSON with a `diagnostics` array.

## Non-Goals

- No network fetching.
- No raw KiCad bulk import.
- No checksum verification against local files yet.
- No catalog repair command yet.
