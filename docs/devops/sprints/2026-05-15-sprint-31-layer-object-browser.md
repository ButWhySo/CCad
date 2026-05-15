# Sprint 31: Layer And Object Browser

## Sprint Goal

Replace the placeholder right-dock layer list with a read-only browser for canvas layers and board objects, and make selected primitives highlight their real geometry rather than a bounding box.

## Branch

`sprint-31-layer-object-browser`

## Progress

Progress: Phase 2/6, Sprint 31, `main`, merged and verified.

## Backlog

1. Done: inspect current GUI shell and canvas model.
2. Done: add failing tests for canvas metadata, object browser rows, and shape selection highlight.
3. Done: add canvas layer and primitive metadata for GUI browsing.
4. Done: add `ObjectBrowserPanel`.
5. Done: wire browser into the right dock.
6. Done: replace default selection bounding boxes with shape-level highlight painting.
7. Done: run focused tests and GUI build.
8. Done: full native Qt build and CTest.
9. Done: commit and merge to `main`.

## Verification So Far

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

No screenshot captured during implementation because the user asked not to use screenshot or mouse control while they use the desktop.
