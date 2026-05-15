# Sprint 24 Design: Library Catalog Kind Filter

## Goal

Allow agents to restrict local catalog search results to one item kind, such as `footprint`, `symbol`, or `model`.

## Requirements

- Add a core search overload that accepts an optional kind filter.
- Match kind case-insensitively.
- Preserve catalog-order result determinism.
- Add `--kind <kind>` to `ccad lib catalog-search`.
- Include the requested kind in search JSON output.

## Non-Goals

- No multi-kind expressions.
- No fuzzy ranking.
- No source/mirror filters yet.
- No network fetches.

## Test Strategy

- Core test for kind-matching and kind-exclusion.
- CLI black-box test for `--kind` returning zero matches when the kind excludes all matches.
