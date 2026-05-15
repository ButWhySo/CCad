# Sprint 25 Plan: Library Catalog Validation

Progress: Phase 2/6, Sprint 25, `sprint-25-library-catalog-validate`, implementation.

## Steps

1. Done: create failing core and CLI tests for catalog validation.
2. Done: add `CatalogDiagnostic` and `validateLibraryCatalog`.
3. Done: add `lib catalog-validate` command and help metadata.
4. Done: run focused catalog and CLI tests.
5. In progress: update docs and handover.
6. Pending: run full native Qt build and CTest.
7. Pending: commit, merge to `main`, and rerun full integration gate.

## Verification

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

- No screenshot required. This sprint adds CLI/catalog validation only.
