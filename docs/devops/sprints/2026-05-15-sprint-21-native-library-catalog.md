# Sprint 21: Native Library Catalog Foundation

## Sprint Goal

Add the metadata foundation for local/offline CCad-native library catalogs with provenance, checksum, and license fields.

## Branch

`sprint-21-native-library-catalog`

## Progress

Progress: Phase 2/6, Sprint 21, `main`, merged and verified.

## Backlog

1. Done: add failing catalog unit test.
2. Done: add `LibraryCatalog`, `LibrarySource`, and `LibraryItem` model.
3. Done: add deterministic catalog JSON dump/load.
4. Done: add lookup by stable item ID.
5. Done: ignore local huge cache paths.
6. In progress: update docs.
7. Pending: run full native Qt build and CTest.
8. Done: merge to `main`.

## Verification So Far

RED:

```cmd
cmake --build build-qt --target ccad_library_catalog_tests
```

- Result: failed because `src/ccad_core/library_catalog.cpp` did not exist.

GREEN:

```cmd
cmake --build build-qt --target ccad_library_catalog_tests
ctest --test-dir build-qt -R library_catalog --output-on-failure
```

- Result: focused catalog test passed.

Full feature-branch gate:

```cmd
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
```

- Result: 11 / 11 tests passed before commit.

Main integration gate:

```cmd
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
```

- Result: 11 / 11 tests passed after merging Sprint 21 to `main`.

## Demo

- No screenshot required. This sprint is a kernel metadata foundation for future local library catalogs.
