# Sprint 34 Plan: Diagnostic Overlay Markers

Progress: Phase 2/6, Sprint 34, `sprint-34-diagnostic-overlay-markers`, implementation.

## Steps

1. Done: branch from verified `main`.
2. Done: write failing renderer test for diagnostic marker metadata.
3. Done: add diagnostic marker roles and renderer helper.
4. Done: render markers from `ReviewWindow` after board canvas rendering.
5. Done: make selection highlights derive from object display colors.
6. Done: add theme/plugin/custom component creator compatibility notes to the large-design pipeline.
7. Done: run focused marker and selection-style tests plus GUI build.
8. Done: run full native Qt build and CTest.
9. Pending: commit, merge, and close sprint docs.

## Verification So Far

RED check:

```cmd
cmake --build build-qt --target ccad_gui_diagnostic_markers_tests
```

- Result: failed because marker APIs did not exist.

GREEN checks:

```cmd
cmake --build build-qt --target ccad_gui_diagnostic_markers_tests
set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& set QT_QPA_PLATFORM=offscreen&& build-qt\ccad_gui_diagnostic_markers_tests.exe
```

- Result: passed.

```cmd
cmake --build build-qt --target ccad_gui_canvas_selection_style_tests
set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& set QT_QPA_PLATFORM=offscreen&& build-qt\ccad_gui_canvas_selection_style_tests.exe
```

- Result: passed.

```cmd
cmake --build build-qt --target ccad_gui
```

- Result: passed.

```cmd
cmake --build build-qt --clean-first && ctest --test-dir build-qt --output-on-failure
```

- Result: passed, 11/11 tests.
