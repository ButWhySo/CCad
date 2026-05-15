# Sprint 32 Plan: Diagnostic Selection Link

Progress: Phase 2/6, Sprint 32, `sprint-32-diagnostic-selection-link`, implementation.

## Steps

1. Done: branch from verified `main`.
2. Done: inspect diagnostics panel, review window, review model, and DRC tests.
3. Done: write failing diagnostics panel row object ID test.
4. Done: write failing canvas select-by-ID test.
5. Done: add diagnostics row object ID accessor.
6. Done: add canvas select-by-ID helper.
7. Done: wire diagnostic row clicks to canvas selection.
8. Done: run focused Qt tests and GUI build.
9. Done: run full native Qt build and CTest.
10. Pending: commit, merge, and close sprint docs.

## Verification So Far

RED checks:

```cmd
cmake --build build-qt --target ccad_gui_diagnostics_panel_tests
```

- Result: failed because `DiagnosticsPanel::objectIdForRow` did not exist.

```cmd
cmake --build build-qt --target ccad_gui_canvas_selection_link_tests
```

- Result: failed because `selectCanvasObjectById` did not exist.

Focused GREEN checks:

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

```cmd
cmake --build build-qt --clean-first && ctest --test-dir build-qt --output-on-failure
```

- Result: passed, 11/11 tests.
