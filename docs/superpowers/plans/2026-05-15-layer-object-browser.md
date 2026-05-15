# Sprint 31 Plan: Layer And Object Browser

Progress: Phase 2/6, Sprint 31, `sprint-31-layer-object-browser`, implementation.

## Steps

1. Done: branch from clean `main`.
2. Done: inspect GUI target spec, progress, codebase map, canvas model, and current review window wiring.
3. Done: write failing canvas metadata test.
4. Done: write failing object browser widget test.
5. Done: write failing shape-selection highlight test from user feedback.
6. Done: add canvas layer/net/layer metadata.
7. Done: add `ObjectBrowserPanel` and wire it into the right dock.
8. Done: replace Qt default bounding-box selection with shape-level highlight canvas items.
9. Done: run focused tests and GUI build.
10. Done: run full native Qt build and CTest.
11. Pending: commit, merge, and close sprint docs.

## Verification So Far

RED checks:

```cmd
cmake --build build-qt --target ccad_gui_object_browser_tests
```

- Result: failed because `src/ccad_gui/object_browser_panel.cpp` did not exist yet.

```cmd
cmake --build build-qt --target ccad_gui_canvas_selection_style_tests
```

- Result: failed because `canvasUsesShapeSelectionHighlight` did not exist yet.

Focused GREEN checks:

```cmd
cmake --build build-qt --target ccad_canvas_tests
ctest --test-dir build-qt -R canvas --output-on-failure
```

- Result: passed.

```cmd
cmake --build build-qt --target ccad_gui_object_browser_tests
set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& set QT_QPA_PLATFORM=offscreen&& build-qt\ccad_gui_object_browser_tests.exe
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
