# Sprint 21 Design: Native Library Catalog Foundation

## Goal

Create the local/offline metadata foundation needed to reuse KiCad and mirror libraries without repeatedly fetching the internet or reparsing raw upstream files during normal design work.

## Requirements

- Define a CCad native library catalog model.
- Store upstream source metadata: source name, kind, URL, commit/hash, mirror, and fetch timestamp.
- Store item metadata: stable ID, kind, display name, source path, native CCad path, checksum, license, provenance, and import warnings.
- Provide deterministic JSON dump/load.
- Provide lookup by stable item ID.
- Keep raw and converted huge catalogs out of the main source repo.

## Non-Goals

- No internet fetching in this sprint.
- No SQLite/search index yet.
- No KiCad symbol importer yet.
- No bulk import of KiCad repositories yet.
- No packaging of a full CCad library catalog yet.

## Test Strategy

- Unit test catalog round-trip JSON.
- Unit test provenance/checksum/license/warnings preservation.
- Unit test strict rejection of missing source/items.
- Unit test lookup by item ID.
