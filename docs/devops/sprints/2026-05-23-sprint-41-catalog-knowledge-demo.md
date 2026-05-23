# Sprint 41: Catalog Knowledge CLI Visibility

## Goal

Make the component-knowledge fields from Sprint 40 visible through the CLI so agents can actually use the local catalog as a decision surface. The schema existed already, but `catalog-find` and `catalog-search` still printed only the older identity/path subset.

## Branch

`sprint-41-catalog-knowledge-demo`

## Scope

`ccad lib catalog-find` now prints `usage_summary`, `layout_notes`, `source_confidence`, and `review_status` for the matching item. `ccad lib catalog-search` now prints the same fields for each search result. This keeps the CLI output useful for local/offline component selection without forcing agents to open raw catalog files.

## Verification

Focused red-green test:

```cmd
cmd /c "cmake --build build-qt --target ccad_cli_tests && ctest --test-dir build-qt -R cli --output-on-failure"
```

The first focused run failed as expected because `catalog-find` did not print `usage_summary`. After implementation, the focused CLI test passed.

Full sprint gate:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && cmake --build build-qt --clean-first && ctest --test-dir build-qt --output-on-failure"
```

Result: passed. The clean Qt build completed 72 steps and CTest passed 11 of 11 tests.

## Demo

No GUI screenshot is required because this sprint changes CLI JSON output only. The demo is the `catalog-find` and `catalog-search` JSON output containing the knowledge fields.
