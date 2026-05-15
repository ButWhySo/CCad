# Sprint 24: Library Catalog Kind Filter

## Sprint Goal

Add an optional kind filter to local library catalog search.

## Branch

`sprint-24-library-catalog-kind-filter`

## Progress

Progress: Phase 2/6, Sprint 24, `main`, merged and verified.

## Backlog

1. Done: add failing catalog and CLI tests.
2. Done: implement kind-filtered catalog search.
3. Done: expose `--kind` on `lib catalog-search`.
4. Done: run focused catalog and CLI tests.
5. Done: update docs.
6. Done: run full native Qt build and CTest.
7. Done: merge to `main`.

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

- No screenshot required. This sprint adds CLI/catalog search filtering only.
