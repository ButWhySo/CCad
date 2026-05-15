# Sprint 23: Library Catalog Search

## Sprint Goal

Add local text search over CCad-native library catalogs and expose it through the CLI.

## Branch

`sprint-23-library-catalog-search`

## Progress

Progress: Phase 2/6, Sprint 23, `main`, merged and verified.

## Backlog

1. Done: add failing catalog and CLI tests.
2. Done: implement `searchLibraryItems`.
3. Done: implement `lib catalog-search`.
4. Done: run focused catalog and CLI tests.
5. In progress: update docs.
6. Pending: run full native Qt build and CTest.
7. Done: merge to `main`.

## Verification So Far

RED:

```cmd
cmake --build build-qt --target ccad_library_catalog_tests ccad_cli_tests
ctest --test-dir build-qt -R "library_catalog|cli" --output-on-failure
```

- Result: failed because `searchLibraryItems` and `catalog-search` did not exist.

GREEN:

```cmd
cmake --build build-qt --target ccad_library_catalog_tests ccad_cli_tests
ctest --test-dir build-qt -R "library_catalog|cli" --output-on-failure
```

- Result: focused catalog and CLI tests passed.

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

- Result: 11 / 11 tests passed after merging Sprint 23 to `main`.

## Demo

- No screenshot required. This sprint adds CLI/catalog search behavior only.
