# Sprint 26: Library Catalog File Checks

## Sprint Goal

Verify local native catalog artifacts by existence and checksum when a catalog root is supplied.

## Branch

`sprint-26-library-catalog-file-checks`

## Progress

Progress: Phase 2/6, Sprint 26, `main`, merged and verified.

## Backlog

1. Done: add failing catalog file-check tests.
2. Done: add failing CLI `--root` tests.
3. Done: implement SHA-256 file verification.
4. Done: expose `lib catalog-validate --root`.
5. Done: update docs.
6. Done: full native Qt build and CTest.
7. Done: merge to `main`.

## Verification So Far

RED:

```cmd
cmake --build build-qt --target ccad_library_catalog_tests ccad_cli_tests && ctest --test-dir build-qt -R "library_catalog|cli" --output-on-failure
```

- Result: failed because the root-aware validator overload and CLI option were missing.

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
