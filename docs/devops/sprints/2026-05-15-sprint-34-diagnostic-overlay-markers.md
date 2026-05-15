# Sprint 34: Diagnostic Overlay Markers

## Sprint Goal

Draw read-only PCB canvas markers for diagnostics that name a selectable object.

## Branch

`sprint-34-diagnostic-overlay-markers`

## Progress

Progress: Phase 2/6, Sprint 34, `sprint-34-diagnostic-overlay-markers`, implementation in progress.

## Backlog

1. Done: add failing marker renderer test.
2. Done: add marker metadata roles and marker renderer helper.
3. Done: render diagnostic markers from the review window.
4. Done: make selected-object highlights derive from object display colors.
5. Done: record theme/plugin/custom component creator compatibility in the large-design pipeline.
6. Done: run focused marker and selection-style tests plus GUI build.
7. Done: full native Qt build and CTest.
8. Pending: commit and merge to `main`.

## Verification So Far

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

Full branch gate:

```cmd
cmake --build build-qt --clean-first && ctest --test-dir build-qt --output-on-failure
```

- Result: passed, 11/11 tests.

## Demo

No screenshot captured during implementation because the user asked not to use screenshot or mouse control while they use the desktop.
