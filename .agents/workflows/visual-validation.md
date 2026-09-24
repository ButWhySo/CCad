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

> [!WARNING]
> **COMPILER CAVEAT**: You MUST build and run the CCad GUI and CLI using the explicit Qt MinGW compiler toolchain (e.g., `C:\Qt\Tools\mingw1310_64\bin`) when running visual validation tests. The `run_sprint_demo.ps1` script executes both the CLI and GUI from the same build directory. Using generic compilers from standard PATH (like CodeBlocks MinGW) will cause toolchain mismatch crashes during GUI load and process errors. Always configure your build appropriately by referring to the README and ensure `scripts/preflight_qt_env.ps1` passes before running visual UI tests.

Wait a few seconds and interact with at least two or three basic GUI elements before assuming the previous state is stable.

### Phase 2: Research and Implementation

Research first. Look up existing projects, standard implementations, and proven approaches before reinventing core behavior. Adapt findings strictly to the project’s LLM-native CAD use-case.Use internet and browser and lookup the existing implementation and best practices for current scope.Use browser automation if needed. I am specifically banning you from mock/stub codes and plumbing. in complete ccad project.Whatever imlpementation/code you write should be complete, end to end , production grade.We also have browser automation setup, which will help you saerch and lookup, we have playwright setup.

Use the local KiCad source checkout, expected around `F:\kicad_src`, as a reference implementation source. Study relevant KiCad patterns and adapt the architectural lessons where appropriate, without blindly copying behavior that does not fit CCAD.

Write or update C++ behavior tests in the `tests/` directory before or alongside implementation. Tests should define expected behavior clearly enough that regressions are detectable.

Implement changes with a decentralized architecture. The GUI must remain thin but solid, at least comparable in reliability expectations to KiCad-style CAD software. Core logic belongs in the `ccad_core` kernel. Maintain single responsibility. Split files when they become too large or mixed in purpose. Remember that this is intended to become an agent-first CAD design system, not merely a manual drawing UI.

### Phase 3: Verification Gate — Proof

Run moderated automated testing before committing. Test both CLI and GUI behavior. Since C++ builds can take time, use judgment during development, but the full CMake build and CTest gate are mandatory before merging to `main` or pushing final changes.

If GUI or visual components are touched, run the official test harness:

```powershell
scripts/run_sprint_demo.ps1
```

The harness must load the board, place the component, wait the current single-preview settle time of 7 seconds, and capture screenshots. For multi-target GUI validation, use the app-owned target harness with a 5-second initial load wait and fast per-action waits around 800 ms, unless the specific feature requires a longer explicit wait. We also have language servers setup.

Intercept both `stdout` and `stderr`. Redirect errors into logs that are inspected by the agent, not merely saved and ignored. Underlying Qt crashes, warnings, failed widget lookups, missing assets, and rendering failures must be visible during verification.

For stricter manual or scripted GUI validation, use the following startup sequence. Start with a beep, wait 2 seconds, start the GUI, force it to open focused, maximized, and fullscreen, then wait 7 seconds for complete loading before interacting.

```powershell
$env:PATH = "C:\Qt\6.11.1\mingw_64\bin;$env:PATH"
[console]::beep(800,300)
Start-Sleep -Seconds 2
.\build-qt\ccad_gui.exe *> gui_stdout.log 2> gui_stderr.log
```

If a more complete PowerShell launcher already exists in the repo, use that script with use case based corrections/modifications instead of duplicating it, but ensure it still satisfies the same requirements: beep, 2-second pre-launch wait, focused maximized/fullscreen launch, 7-second load wait, stderr capture, stdout capture, and screenshot capture.

Interact with the GUI through the live Qt6 GUI map feature already coded in the project. Do not use random coordinates unless there is no mapped alternative and the exception is documented. The GUI map must be used to identify real widgets, menus, actions, buttons, panels, dialogs, and newly changed elements.In running visual validation, you are supposed to ingest the output screenshots and then observe them. Visual validation was meant to simply guide/verify the actual work you did was implemented and working fine without bugs as a user, you need to visually verify the feature that you worked on/bug you fixed not simply perform the fixed steps everytime.

During GUI validation, interact with at least 7 elements(if you are not aware of what changes u did, otherwise be specific to the feature/bug you solved). Space interactions by roughly 0.5 seconds unless the UI requires longer. The interaction set must include normal existing elements, newly coded or recently fixed elements, menus that open dialog boxes, and controls inside those dialog boxes. After opening a dialog, interact with relevant controls inside it, capture screenshots, then close the dialog cleanly after successful verification.

Record every mapped interaction in the machine-readable action log, but capture screenshots only at visually meaningful checkpoints: the feature's before state, a distinct dialog/control state, the result after an asynchronous action completes, and the restored/final state. Do not save near-identical images for every click or keystroke. Add a screenshot only when it exposes a new state, regression, or feature-specific result that the existing checkpoints cannot prove. Inspect every retained screenshot.

For any currently worked-on, newly coded, fixed, or patched feature, perform targeted interaction through the GUI map and capture screenshots before and after the feature-specific action. Small tests passing is not enough, because the final build may still crash or visually break after running for more than 20 seconds.

Ingest every generated screenshot from the current validation run into image-analysis tools. Use `view_file` or the available image parsing path to inspect each `.png`. Analyze them strictly against expected behavior: layout correctness, element visibility, text readability, widget state, no clipped panels, no broken rendering, no stale state, no unexpected blank regions, no crash dialogs, no frozen windows, and no incorrect modal behavior.

The visual validation is only accepted when the screenshots, GUI-map interactions, stdout logs, stderr logs, and feature-specific behavior all agree that the GUI works as expected.

Agents must use the official PowerShell interaction scripts to test features and must interact with the GUI by actively injecting mouse clicks and keyboard values , or through the ui map and the live commands/tool calls developed in ccad/mouse keyboard computer use. Every feature worked on must be tested by utilizing mouse and keyboard inputs, adopting agent-first methodologies, executing command prompt direct controls, and utilizing the GUI map to the fullest extent. Every generated screenshot from the validation run must be explicitly ingested and visually inspected by the agent to verify layout correctness and specific scoped feature/bug resolution behavior.

Run moderated automated testing before committing. Test both CLI and GUI behavior. Since C++ builds can take time, use judgment during development, but the full CMake build and CTest gate are mandatory before merging to main or pushing final changes.

If GUI or visual components are touched, use scripts/run_sprint_demo.ps1 (or an equivalent existing launcher) only to get the application into a stable, loaded, screenshot-capable state — beep, 2-second pre-launch wait, focused/maximized/fullscreen launch, 7-second settle time, stdout/stderr capture. The script's job ends there. It is scaffolding, not the test.

The actual verification is driven by what changed this sprint, not by the script's built-in steps. Before touching the GUI:

Identify the specific feature added or bug fixed in this branch — name it explicitly (e.g. "trace-width validation on the routing dialog," not "routing").
Identify, via the Qt6 GUI map, which real widgets/menus/dialogs/controls that change actually touches. Do not use random coordinates unless there is no mapped alternative, and document the exception if so.
Build a short interaction plan around that feature/bug specifically — what sequence of clicks/inputs would exercise it, expose the old bug if it regressed, or confirm the new behavior works as intended.

Then execute:

Interact with at least 7 elements total, spaced ~0.5s apart (longer if the UI needs it). This set must include pre-existing elements that prove the app is alive and the newly coded/fixed elements, including opening relevant dialogs and operating their controls. Keep a machine-readable record for every interaction. Capture only distinct visual checkpoints: target before state, relevant dialog/control state, completed feature result, and cleanly restored final state. Capture intermediate states only when they prove behavior that cannot be inferred from adjacent checkpoints; never generate near-identical screenshots for every action.
Intercept stdout/stderr into logs and actually inspect them for warnings, failed widget lookups, missing assets, or rendering failures tied to the touched area.

Explicitly do not treat "the script ran and produced screenshots" as sufficient. The harness proves the app booted; it does not prove the feature works. Running the demo script's default steps without deliberately routing through the sprint's actual change is not valid verification — if the script's built-in flow happens not to touch the feature/bug in question, that's a gap to fill manually via the GUI map, not something to paper over with the script's default screenshots.

Ingest every generated screenshot into image-analysis tools (view_file or equivalent). Judge them specifically against what the feature/bug was supposed to do — not just generic "no crash, no clipping" checks, though those still apply. Ask: does this screenshot sequence actually demonstrate the thing I built or fixed, in a way a human reviewer could verify without reading the diff?

Visual validation is only accepted when the screenshots, GUI-map interactions, stdout/stderr logs, and the specific feature/bug behavior all agree — and the interaction plan can be pointed to as evidence of that sprint's change, not just evidence the binary launches.

Do not mistake a local "green" status for a CI/CD/CT "green" status anymore.

### Phase 4: Documentation and Cleanup

Update amnesia and handover documentation immediately in the same sprint. At minimum, update `docs/codebase-map.md`, `docs/features/implemented-features.md`, `docs/devops/progress.md`, and any sprint handover notes affected by the work.

Keep backlog documentation synchronized. Any new limitation, unimplemented edge case, skipped visual proof, flaky test, or deferred improvement must be recorded in the backlog.

Purge intermediate `.tmp` logs, obsolete WIP screenshots, stale generated files, and failed-run artifacts. Keep only final verified artifacts that are useful for proof, review, or handover.

### Phase 5: Commit, Merge, and Branch Cleanup

Stage changes carefully and write a detailed multi-line commit message containing all code, tests, visual validation, and documentation updates. 
**DO NOT spam separate commits** for documentation, `progress.md` updates, or handover notes. They must be bundled into the *same* logical commit as the code and verification they describe to form one cohesive unit of work.

Merge back to `main` only after the branch is green, visually proven, documented, and backed by the required verification artifacts.

After a successful merge, clean up the feature branch once its purpose is served, i.e delete them after merging. Also clean up stray branches whose purpose is complete and which are no longer needed.
```text
Why:
Changed:
Behavior:
Verification:
Demo:
```

Provide the user with the required status format:

```text
Progress: Phase X/Y, Sprint N, <branch>, <status> <worked_on> <importance_from_user_pov_no_dev_lang_layman_lang_only>
```
## Language-server-assisted validation

When the feature touches C++, Qt, Python, or CMake, use the available language server before launching the GUI to catch declaration, include, type, and configuration errors quickly. For C++/Qt, prefer clangd with the repository's generated `compile_commands.json`; for Python orchestration, use Pyright with the agent virtual environment; for CMake, use cmake-language-server. These checks are advisory and must not replace the actual build, CTest, provider calls, GUI-map interactions, screenshots, or log inspection.

Useful open-ended checks include asking clangd to inspect changed translation units, checking symbol references and generated Qt MOC-visible declarations, running Pyright on provider/graph/memory/telemetry modules, validating CMake target and dependency edits, and using the server diagnostics to choose targeted GUI-map scenarios. Repeat the server check after edits that change public interfaces, IPC payloads, tool schemas, settings persistence, or renderer contracts. Record the exact server version, database/configuration path, changed files, and diagnostic result in the validation evidence when it materially affects the feature.
## Language-server-assisted validation

Use the installed language servers actively when they can shorten diagnosis before or after visual validation. `clangd` is useful for C++ symbol navigation, include and type diagnostics, Qt signal/slot call-site tracing, and finding all callers before changing a public GUI or kernel API. `Pyright` is useful for Python orchestrator/provider/telemetry type flow, JSON-RPC payload shape checks, missing members, and catching unsafe `Any` expansion around LangGraph callbacks. `cmake-language-server` is useful for target/dependency discovery, option and install-rule diagnostics, and checking that a newly added test or executable is actually connected to the configured build.

These are advisory accelerators, not proof. Use them wherever beneficial for dependency mapping, provider adapter changes, context and memory payloads, GUI-map IPC, typed transaction APIs, subprocess boundaries, telemetry configuration, or test-target wiring. Keep the use cases open-ended: apply the relevant server to any changed language or build file, record actionable diagnostics, and resolve them or document why they remain before the authoritative build, CTest, real-provider checks, and GUI-map screenshot inspection. A clean language-server report never replaces those runtime gates.
