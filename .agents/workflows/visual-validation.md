---
description: This workflow should run everytime.It is a working guide
---

## CCAD Agent Development Workflow

### Phase 0: Repository Ingestion and Current-State Recovery

Before making any implementation decision, fully ingest the project state. Read `agent-methodology`, `README`, `AGENTS.md`, `docs/devops/progress.md`, `backlog.md`, and all other markdown or documentation files that indicate architecture, workflow, sprint state, unfinished work, failed CI/CT/CD, known bugs, or current development direction.

Generate or inspect a directory tree using `DirTree`, `tree`, or an equivalent method so the repo structure is understood before edits begin. Establish what already exists, what is partially implemented, what is broken, and what the current sprint or phase expects.

### Phase 1: Preparation and Branching

Determine a specific, small set of things to work on for the commits in this branch. The scope must be narrow enough to review, test, visually validate, document, and merge cleanly.

Create or switch to the appropriate feature branch. If the branch does not exist, create it from `main` using standard Git/GitHub naming conventions such as `sprint-<number>-<topic>`.

Before writing code, map incoming and outgoing dependencies of the target functions, files, GUI widgets, scripts, and tests. This prevents breaking existing callers or hidden GUI workflows.

Read `docs/devops/progress.md` to establish the current phase and sprint context.

Keep all backlog locations updated. Treat every feature, bug, TODO, missing validation, known weakness, or deferred item as backlog material. Keep `backlog.md` consistent with any other backlog or progress documents. If `backlog.md` is malformed, outdated, duplicated, or inconsistent, fix it as part of the work. If the repo mentions failed CI, CT, CD, GUI crashes, or previous verification failures, account for those while working.

Before starting new changes, verify that the GUI from the previous commit or progress state still runs and does not immediately crash. On Windows, use:

```powershell
$env:PATH = "C:\Qt\6.11.1\mingw_64\bin;$env:PATH"
.\build-qt\ccad_gui.exe
```

Wait a few seconds and interact with at least two or three basic GUI elements before assuming the previous state is stable.

### Phase 2: Research and Implementation

Research first. Look up existing projects, standard implementations, and proven approaches before reinventing core behavior. Adapt findings strictly to the project’s LLM-native CAD use-case.

Use the local KiCad source checkout, expected around `F:\kicad_src`, as a reference implementation source. Study relevant KiCad patterns and adapt the architectural lessons where appropriate, without blindly copying behavior that does not fit CCAD.

Write or update C++ behavior tests in the `tests/` directory before or alongside implementation. Tests should define expected behavior clearly enough that regressions are detectable.

Implement changes with a decentralized architecture. The GUI must remain thin but solid, at least comparable in reliability expectations to KiCad-style CAD software. Core logic belongs in the `ccad_core` kernel. Maintain single responsibility. Split files when they become too large or mixed in purpose. Remember that this is intended to become an agent-first CAD design system, not merely a manual drawing UI.

### Phase 3: Verification Gate — Proof

Run moderated automated testing before committing. Test both CLI and GUI behavior. Since C++ builds can take time, use judgment during development, but the full CMake build and CTest gate are mandatory before merging to `main` or pushing final changes.

If GUI or visual components are touched, run the official test harness:

```powershell
scripts/run_sprint_demo.ps1
```

The harness must load the board, place the component, wait the current single-preview settle time of 7 seconds, and capture screenshots. For multi-target GUI validation, use the app-owned target harness with a 5-second initial load wait and fast per-action waits around 800 ms, unless the specific feature requires a longer explicit wait.

Intercept both `stdout` and `stderr`. Redirect errors into logs that are inspected by the agent, not merely saved and ignored. Underlying Qt crashes, warnings, failed widget lookups, missing assets, and rendering failures must be visible during verification.

For stricter manual or scripted GUI validation, use the following startup sequence. Start with a beep, wait 2 seconds, start the GUI, force it to open focused, maximized, and fullscreen, then wait 7 seconds for complete loading before interacting.

```powershell
$env:PATH = "C:\Qt\6.11.1\mingw_64\bin;$env:PATH"
[console]::beep(800,300)
Start-Sleep -Seconds 2
.\build-qt\ccad_gui.exe *> gui_stdout.log 2> gui_stderr.log
```

If a more complete PowerShell launcher already exists in the repo, use that script instead of duplicating it, but ensure it still satisfies the same requirements: beep, 2-second pre-launch wait, focused maximized/fullscreen launch, 7-second load wait, stderr capture, stdout capture, and screenshot capture.

Interact with the GUI through the live Qt6 GUI map feature already coded in the project. Do not use random coordinates unless there is no mapped alternative and the exception is documented. The GUI map must be used to identify real widgets, menus, actions, buttons, panels, dialogs, and newly changed elements.

During GUI validation, interact with at least 7 elements. Space interactions by roughly 0.5 seconds unless the UI requires longer. The interaction set must include normal existing elements, newly coded or recently fixed elements, menus that open dialog boxes, and controls inside those dialog boxes. After opening a dialog, interact with relevant controls inside it, capture screenshots, then close the dialog cleanly after successful verification.

Capture a screenshot after every interaction. Do not rely on a single final screenshot. The screenshot sequence must prove that the GUI remains alive, focused, visually correct, and responsive over time.

For any currently worked-on, newly coded, fixed, or patched feature, perform targeted interaction through the GUI map and capture screenshots before and after the feature-specific action. Small tests passing is not enough, because the final build may still crash or visually break after running for more than 20 seconds.

Ingest every generated screenshot from the current validation run into image-analysis tools. Use `view_file` or the available image parsing path to inspect each `.png`. Analyze them strictly against expected behavior: layout correctness, element visibility, text readability, widget state, no clipped panels, no broken rendering, no stale state, no unexpected blank regions, no crash dialogs, no frozen windows, and no incorrect modal behavior.

The visual validation is only accepted when the screenshots, GUI-map interactions, stdout logs, stderr logs, and feature-specific behavior all agree that the GUI works as expected.

### Phase 4: Documentation and Cleanup

Update amnesia and handover documentation immediately in the same sprint. At minimum, update `docs/codebase-map.md`, `docs/features/implemented-features.md`, `docs/devops/progress.md`, and any sprint handover notes affected by the work.

Keep backlog documentation synchronized. Any new limitation, unimplemented edge case, skipped visual proof, flaky test, or deferred improvement must be recorded in the backlog.

Purge intermediate `.tmp` logs, obsolete WIP screenshots, stale generated files, and failed-run artifacts. Keep only final verified artifacts that are useful for proof, review, or handover.

### Phase 5: Commit, Merge, and Branch Cleanup

Stage changes carefully and write a detailed multi-line commit message containing:

```text
Why:
Changed:
Behavior:
Verification:
Demo:
```

Provide the user with the required status format:

```text
Progress: Phase X/Y, Sprint N, <branch>, <status>
```

Merge back to `main` only after the branch is green, visually proven, documented, and backed by the required verification artifacts.

After a successful merge, clean up the feature branch once its purpose is served. Also clean up stray branches whose purpose is complete and which are no longer needed.
