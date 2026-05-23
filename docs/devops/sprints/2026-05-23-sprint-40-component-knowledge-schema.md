# Sprint 40: Component Knowledge Schema

## Goal

Extend the local CCad library catalog so a future component-ingestion pipeline can preserve agent-facing component knowledge, not only raw file provenance. This sprint keeps the work local and deterministic. It does not fetch remote libraries, execute catalog content, or attempt bulk KiCad import.

## Branch

`sprint-40-component-knowledge-schema`

## Scope

The catalog item schema now stores `usage_summary`, `layout_notes`, `source_confidence`, and `review_status`. These fields are intended for local/offline component caches where KiCad-derived footprints, symbols, package data, datasheet summaries, human review notes, and AI-generated usage hints can be searched without repeatedly fetching the internet.

`usage_summary` is a short plain-language explanation of what the item is for. `layout_notes` stores placement, assembly, routing, or manufacturing guidance. `source_confidence` records where the knowledge came from, such as library metadata, datasheet extraction, human review, or generated inference. `review_status` tracks curation state and is currently validated against `generated`, `needs_review`, `reviewed`, and `rejected`.

## Implementation Notes

`LibraryItem` gained the four fields in `src/ccad_core/library_catalog.hpp`. The deterministic catalog JSON reader and writer preserve them in `src/ccad_core/library_catalog.cpp`. Catalog search now indexes `usage_summary`, `layout_notes`, `source_confidence`, and `review_status` so an agent can ask local questions like “find reviewed USB-C connector footprints” or “find items with datasheet-backed knowledge” once richer catalogs exist.

The validator rejects unknown `review_status` values so downstream agents can safely branch on the curation state. `source_confidence` remains a string for now because the exact confidence taxonomy should be designed with the future ingestion and human-review pipeline.

## Verification

Focused red-green test:

```cmd
cmd /c "cmake --build build-qt --target ccad_library_catalog_tests && ctest --test-dir build-qt -R library_catalog --output-on-failure"
```

Result after implementation: passed.

Full sprint gate:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && cmake --build build-qt --clean-first && ctest --test-dir build-qt --output-on-failure"
```

Result: passed. The clean Qt build completed 72 steps and CTest passed 11 of 11 tests.

## Demo

No GUI screenshot is required for this sprint because the work is catalog schema/search behavior. The visible output is deterministic catalog JSON and local search results.

## Definition Of Done

The sprint is done when catalog tests cover round-trip serialization, search over the new knowledge fields, invalid review status diagnostics, documentation updates, full clean Qt build, full CTest pass, and merge back to `main`.
