# Sprint 22: Library Catalog CLI

## Sprint Goal

Expose local CCad-native library catalogs through deterministic CLI commands for agent use.

## Branch

`sprint-22-library-catalog-cli`

## Progress

Progress: Phase 2/6, Sprint 22, `sprint-22-library-catalog-cli`, implementation.

## Backlog

1. Done: add failing CLI tests.
2. Done: add help metadata for catalog commands.
3. Done: implement `lib catalog-info`.
4. Done: implement `lib catalog-find`.
5. Done: run focused CLI test.
6. In progress: update docs.
7. Pending: run full native Qt build and CTest.
8. Pending: merge to `main`.

## Verification So Far

RED:

```cmd
cmake --build build-qt --target ccad_cli_tests
ctest --test-dir build-qt -R cli --output-on-failure
```

- Result: failed with `test failure: help json describes catalog info`.

GREEN:

```cmd
cmake --build build-qt --target ccad_cli_tests
ctest --test-dir build-qt -R cli --output-on-failure
```

- Result: focused CLI test passed.

## Demo

- No screenshot required. This sprint adds CLI/catalog behavior only.
