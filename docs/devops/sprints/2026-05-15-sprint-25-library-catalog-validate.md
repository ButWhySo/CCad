# Sprint 25: Library Catalog Validation

## Sprint Goal

Add local-only catalog metadata validation before any future bulk library ingestion.

## Branch

`sprint-25-library-catalog-validate`

## Progress

Progress: Phase 2/6, Sprint 25, `sprint-25-library-catalog-validate`, implementation.

## Backlog

1. Done: add failing catalog validator tests.
2. Done: add failing CLI `catalog-validate` tests.
3. Done: implement core validator diagnostics.
4. Done: expose `lib catalog-validate`.
5. In progress: update docs.
6. Pending: full native Qt build and CTest.
7. Pending: merge to `main`.

## Verification So Far

RED:

```cmd
cmake --build build-qt --target ccad_library_catalog_tests ccad_cli_tests && ctest --test-dir build-qt -R "library_catalog|cli" --output-on-failure
```

- Result: failed because `CatalogDiagnostic` and `validateLibraryCatalog` did not exist.

GREEN:

```cmd
cmake --build build-qt --target ccad_library_catalog_tests ccad_cli_tests && ctest --test-dir build-qt -R "library_catalog|cli" --output-on-failure
```

- Result: focused catalog and CLI tests passed, 2/2.

## Demo

- No screenshot required. This sprint is CLI/catalog validation only.
