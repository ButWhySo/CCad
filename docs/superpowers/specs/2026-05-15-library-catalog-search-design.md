# Sprint 23 Design: Library Catalog Search

## Goal

Add local text search over CCad native library catalog items so agents can find reusable symbols/footprints by stable ID fragments, names, kinds, or paths without network access.

## Requirements

- Add `searchLibraryItems` in `ccad_core`.
- Match query case-insensitively against item ID, kind, name, source path, and native path.
- Return deterministic matches in catalog order.
- Add `ccad lib catalog-search --catalog <path> --query <text>`.
- Emit deterministic JSON with query, count, and matched item summaries.
- Do not fetch network sources.

## Non-Goals

- No fuzzy ranking.
- No SQLite/full-text index.
- No synonym/parametric search yet.
- No KiCad bulk import yet.

## Test Strategy

- Core unit test for case-insensitive source-path search.
- Core unit test for multiple results and empty query behavior.
- CLI black-box test for command discovery and JSON output.
