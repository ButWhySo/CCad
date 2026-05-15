# Sprint 26 Plan: Library Catalog File Checks

Progress: Phase 2/6, Sprint 26, `sprint-26-library-catalog-file-checks`, implementation.

## Steps

1. Done: add failing core and CLI tests for `--root` file checks.
2. Done: implement local SHA-256 verification in `ccad_core`.
3. Done: expose optional `--root` on `lib catalog-validate`.
4. Done: run focused catalog and CLI tests.
5. In progress: update docs and handover.
6. Pending: run full native Qt build and CTest.
7. Pending: commit, merge to `main`, and rerun full integration gate.

## Verification

RED:

```cmd
cmake --build build-qt --target ccad_library_catalog_tests ccad_cli_tests && ctest --test-dir build-qt -R "library_catalog|cli" --output-on-failure
```

- Result: failed because `validateLibraryCatalog(catalog, root)` and `--root` did not exist.

GREEN:

```cmd
cmake --build build-qt --target ccad_library_catalog_tests ccad_cli_tests && ctest --test-dir build-qt -R "library_catalog|cli" --output-on-failure
```

- Result: focused catalog and CLI tests passed, 2/2.

## Demo

- No screenshot required. This sprint is CLI/catalog validation only.
