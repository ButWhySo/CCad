# Sprint 42: GUI Canvas MVP Review

## Progress

Progress: Phase 2/6, Sprint 42, `main`, merged and verified after merge.

## Goal

Sprint 42 keeps the GUI aligned with the original Phase 2 MVP. The target is not a full PCB editor yet. The target is a more credible native review surface for the board primitives already produced by the kernel and CLI, plus a reliable sprint-end visual QA harness that does not depend on whatever desktop window is currently focused.

## Implemented Scope

The board canvas now has explicit toolbar actions for Fit, Zoom Out, Zoom In, and 100%. This makes review behavior visible and testable instead of hiding zoom only behind the mouse wheel.

The status bar now reports whether cursor coordinates are inside the board outline or merely on the surrounding canvas. This is still read-only, but it starts the coordinate discipline needed for later precise placement and routing work.

The GUI screenshot path now supports `ccad_gui --screenshot <project.ccad.json> <out.png>`. This mode loads the project, shows the native Qt window, processes Qt events for 20 seconds, grabs the window through Qt, saves the PNG, and exits the launched process. It avoids foreground-window capture and avoids killing globally named processes.

The object browser no longer populates placeholder rows in its constructor. Rows are rendered only from `renderScene`, which keeps startup and project-load refreshes within one ownership path.

The sprint demo script now creates the project JSON and reports under `artifacts/demos/`, then uses the app-owned screenshot mode to write the visual artifact under `artifacts/screenshots/`.

## Verification Evidence

Focused GUI build passed:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && cmake --build build-qt --target ccad_gui"
```

Focused object browser and screenshot harness check passed:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && cmake --build build-qt --target ccad_gui && build-qt\ccad_gui.exe --screenshot artifacts\demos\sprint42-gui-canvas-mvp-review-final3.ccad.json artifacts\screenshots\manual-clean.png"
```

Sprint demo passed:

```cmd
cmd /c "powershell -ExecutionPolicy Bypass -File .\scripts\run_sprint_demo.ps1 -Name sprint42-gui-canvas-mvp-review-final4"
```

Demo artifacts from the first passing GUI harness run:

```text
artifacts/demos/sprint42-gui-canvas-mvp-review-final4.ccad.json
artifacts/demos/sprint42-gui-canvas-mvp-review-final4.inspect.json
artifacts/demos/sprint42-gui-canvas-mvp-review-final4.validate.json
artifacts/demos/sprint42-gui-canvas-mvp-review-final4.drc.json
artifacts/screenshots/sprint42-gui-canvas-mvp-review-final4-20260523-194243.png
```

Demo artifacts from the post-clean-build verification run:

```text
artifacts/demos/sprint42-gui-canvas-mvp-review-verified.ccad.json
artifacts/demos/sprint42-gui-canvas-mvp-review-verified.inspect.json
artifacts/demos/sprint42-gui-canvas-mvp-review-verified.validate.json
artifacts/demos/sprint42-gui-canvas-mvp-review-verified.drc.json
artifacts/screenshots/sprint42-gui-canvas-mvp-review-verified-20260523-194845.png
```

Process cleanup check passed:

```cmd
cmd /c "tasklist /FI ""IMAGENAME eq ccad_gui.exe"""
```

Expected result:

```text
INFO: No tasks are running which match the specified criteria.
```

## Full Gate

The full clean build and CTest gate passed before commit:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && cmake --build build-qt --clean-first && ctest --test-dir build-qt --output-on-failure"
```

Result:

```text
100% tests passed, 0 tests failed out of 11
```
