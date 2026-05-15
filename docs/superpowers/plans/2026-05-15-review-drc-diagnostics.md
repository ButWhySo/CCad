# Sprint 33 Plan: Review DRC Diagnostics

Progress: Phase 2/6, Sprint 33, `sprint-33-review-drc-diagnostics`, implementation.

## Steps

1. Done: branch from verified `main`.
2. Done: add failing review test for a physical DRC violation.
3. Done: append DRC diagnostics to project review diagnostics.
4. Done: run focused review test.
5. Pending: update docs.
6. Done: run full native Qt build and CTest.
7. Pending: commit, merge, and close sprint docs.

## Verification So Far

RED check:

```cmd
cmake --build build-qt --target ccad_review_tests
ctest --test-dir build-qt -R review --output-on-failure
```

- Result: failed because `buildReview` ignored DRC diagnostics.

GREEN check:

```cmd
cmake --build build-qt --target ccad_review_tests
ctest --test-dir build-qt -R review --output-on-failure
```

- Result: passed.

```cmd
cmake --build build-qt --clean-first && ctest --test-dir build-qt --output-on-failure
```

- Result: passed, 11/11 tests.
