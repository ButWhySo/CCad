# Sprint 30: GUI Inspector Panel

## Sprint Goal

Add a dedicated read-only inspector panel for selected PCB canvas objects.

## Branch

`sprint-30-gui-inspector-panel`

## Progress

Progress: Phase 2/6, Sprint 30, `sprint-30-gui-inspector-panel`, implemented and verified; commit pending.

## Backlog

1. Done: read required handover and GUI context files.
2. Done: inspect current selection metadata and right dock code.
3. Done: add a focused inspector widget test before production code.
4. Done: add `SelectionInspectorPanel`.
5. Done: wire selection changes into the inspector.
6. Done: update feature/codebase docs.
7. Done: focused Qt inspector test.
8. Done: full native Qt build and CTest.
9. Done: demo screenshot.
10. Pending: commit.

## Verification So Far

Focused RED run:

```cmd
cmake --build build-qt --target ccad_gui_inspector_tests
```

- Result: failed because `src/ccad_gui/selection_inspector_panel.cpp` did not exist yet.

Focused inspector test:

```cmd
cmake --build build-qt --target ccad_gui_inspector_tests
```

- Result: passed.

```cmd
set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && set QT_QPA_PLATFORM=offscreen && build-qt\ccad_gui_inspector_tests.exe
```

- Result: passed, exit code 0.

Full branch gate:

```cmd
cmake --build build-qt --clean-first && ctest --test-dir build-qt --output-on-failure
```

- Result: passed, 11/11 tests.

Demo:

```cmd
powershell -ExecutionPolicy Bypass -File .\scripts\run_sprint_demo.ps1 -Name sprint30-gui-inspector-panel -ClickSelection
```

- Result: screenshot captured at `artifacts/screenshots/sprint30-gui-inspector-panel-20260515-205121.png`.
