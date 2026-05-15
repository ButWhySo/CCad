# Sprint 24: Library Catalog Kind Filter

## Sprint Goal

Add an optional kind filter to local library catalog search.

## Branch

`sprint-24-library-catalog-kind-filter`

## Progress

Progress: Phase 2/6, Sprint 24, `sprint-24-library-catalog-kind-filter`, implementation.

## Backlog

1. Done: add failing catalog and CLI tests.
2. Done: implement kind-filtered catalog search.
3. Done: expose `--kind` on `lib catalog-search`.
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

- Result: failed because the kind-filter overload did not exist.

GREEN:

```cmd
cmake --build build-qt --target ccad_library_catalog_tests ccad_cli_tests
ctest --test-dir build-qt -R "library_catalog|cli" --output-on-failure
```

- Result: focused catalog and CLI tests passed.

## Demo

- No screenshot required. This sprint adds CLI/catalog search filtering only.
