# Sprint 32: Diagnostic Selection Link

## Sprint Goal

Let diagnostic rows select matching PCB canvas objects by stable object ID.

## Branch

`sprint-32-diagnostic-selection-link`

## Progress

Progress: Phase 2/6, Sprint 32, `main`, merged and verified.

## Backlog

1. Done: inspect diagnostics panel and canvas selection code.
2. Done: add failing focused tests.
3. Done: expose diagnostic row object IDs.
4. Done: add canvas select-by-ID helper.
5. Done: wire diagnostic row clicks to canvas selection.
6. Done: run focused tests and GUI build.
7. Done: full native Qt build and CTest.
8. Done: commit and merge to `main`.

## Verification So Far

```cmd
cmake --build build-qt --target ccad_gui_diagnostics_panel_tests
set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& set QT_QPA_PLATFORM=offscreen&& build-qt\ccad_gui_diagnostics_panel_tests.exe
```

- Result: passed.

```cmd
cmake --build build-qt --target ccad_gui_canvas_selection_link_tests
set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& set QT_QPA_PLATFORM=offscreen&& build-qt\ccad_gui_canvas_selection_link_tests.exe
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
