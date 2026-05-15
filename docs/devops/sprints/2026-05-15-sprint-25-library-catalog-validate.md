# Sprint 25: Library Catalog Validation

## Sprint Goal

Add local-only catalog metadata validation before any future bulk library ingestion.

## Branch

`sprint-25-library-catalog-validate`

## Progress

Progress: Phase 2/6, Sprint 25, `main`, merged and verified.

## Backlog

1. Done: add failing catalog validator tests.
2. Done: add failing CLI `catalog-validate` tests.
3. Done: implement core validator diagnostics.
4. Done: expose `lib catalog-validate`.
5. Done: update docs.
6. Done: full native Qt build and CTest.
7. Done: merge to `main`.

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

Full branch gate:

```cmd
cmake --build build-qt --clean-first && ctest --test-dir build-qt --output-on-failure
```

- Result: passed, 11/11 tests.

Main integration gate:

```cmd
cmake --build build-qt --clean-first && ctest --test-dir build-qt --output-on-failure
```

- Result: passed, 11/11 tests after merge to `main`.

## Demo

- No screenshot required. This sprint is CLI/catalog validation only.
