# Sprint 23: Library Catalog Search

## Sprint Goal

Add local text search over CCad-native library catalogs and expose it through the CLI.

## Branch

`sprint-23-library-catalog-search`

## Progress

Progress: Phase 2/6, Sprint 23, `sprint-23-library-catalog-search`, implementation.

## Backlog

1. Done: add failing catalog and CLI tests.
2. Done: implement `searchLibraryItems`.
3. Done: implement `lib catalog-search`.
4. Done: run focused catalog and CLI tests.
5. In progress: update docs.
6. Pending: run full native Qt build and CTest.
7. Pending: merge to `main`.

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

## Demo

- No screenshot required. This sprint adds CLI/catalog search behavior only.
