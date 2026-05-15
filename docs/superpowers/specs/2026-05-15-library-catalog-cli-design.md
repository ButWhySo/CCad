# Sprint 22 Design: Library Catalog CLI

## Goal

Expose local CCad native library catalogs through deterministic CLI commands so agents can inspect and resolve catalog items without reparsing JSON manually or fetching the network.

## Requirements

- Add `ccad lib catalog-info --catalog <path>`.
- Add `ccad lib catalog-find --catalog <path> --id <id>`.
- Emit deterministic JSON.
- Return `0` for successful summary/found lookup.
- Return `1` for missing item lookup.
- Return `2` for usage, parse, or file failures.
- Do not fetch network sources.

## Non-Goals

- No text search yet.
- No catalog index generation yet.
- No KiCad bulk import yet.
- No SQLite search index yet.

## Test Strategy

- CLI black-box test creates a small local catalog file.
- Verify help metadata includes both commands.
- Verify `catalog-info` emits name/source/item count.
- Verify `catalog-find` emits native path for a found stable ID.
- Verify missing item returns nonzero and emits `found: false`.
