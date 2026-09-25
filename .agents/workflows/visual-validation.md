---
description: Evidence-gated CCad development and feature-specific visual validation.
---

# CCad development and visual-validation workflow

Use this workflow at the start of each new CCad work slice. Read `AGENTS.md`,
`docs/agent-methodology.md`, `README.md`, `docs/codebase-map.md`,
`docs/devops/progress.md`, the active sprint checklist, and relevant current
feature docs. Inspect repository structure and existing tests before editing.
Treat the active sprint TODO as the source of truth; update it in the same
change set, and tick an item only after its stated evidence exists. Do not invent
or silently expand backlog locations that the repository does not use.

## Scope, dependencies, and implementation

Work on the current feature branch and keep one coherent, reviewable feature
slice. Before changing a public function, widget, IPC payload, or transaction,
trace its callers and effects, then update all affected callers and contracts.
Write behavior tests before or alongside the change. Implement real end-to-end
behavior; do not add demo-only success, stubs, fabricated data, or fake fallbacks.
When a capability cannot be completed safely, return a truthful failure and leave
the corresponding TODO open.

Research current standards and project-local prior art when relevant. Use the
KiCad source checkout only as a reference, respecting its license. Keep kernel
logic in `ccad_core`, with Qt as a client. For C++, Python, and CMake changes,
use clangd with the real compile database, Pyright, and cmake-language-server
respectively when they can catch dependency/type/configuration mistakes sooner.
Language-server results are advisory and never replace builds, tests, or runtime
proof.

## Feature-specific GUI validation

First state the exact changed behavior and identify its real widgets/actions via
the live Qt GUI map. Build an interaction plan that can expose regression in
that behavior, not a generic tour. Prefer the app-owned target-sequence harness
(`scripts/run_ui_map_mouse_target_demo.ps1`) and extend it only for the scoped
feature. Do not call fictional GUI wrappers or use unmapped coordinates; if a
mapped operation is unavailable, document the specific reason and use the
nearest truthful test boundary.

Run `scripts/preflight_qt_env.ps1`, use the pinned Qt 6.11.1 and MinGW 13.1
toolchain, then build and run the relevant tests. Before a final merge or push,
run the full Release build and full CTest suite. For GUI validation, launch the
app focused/maximized, wait for the specified load interval, capture stdout and
stderr, and use mapped mouse/keyboard actions. Exercise at least seven mapped
interactions overall when the feature supports them; count feature-specific
controls and meaningful existing controls that establish the application is
responsive. Open relevant dialogs, exercise their controls, and close cleanly.
The single-preview harness settles for 7 seconds; the multi-target harness uses
a 5-second initial load wait and about 800 ms per action unless the feature needs
longer.

Record every mapped interaction in the harness's machine-readable report. Capture
screenshots only for distinct visual states: before, relevant dialog/control,
completed result, and restored final state. Inspect every retained screenshot
with image analysis. Check the specific behavior, legibility, clipping, stale
state, rendering, focus, responsiveness, and crash dialogs. Inspect stdout and
stderr; saved-but-unread logs are not verification. The launch/demo harness only
proves startup unless it also exercises the changed feature.

## Evidence gate and honest CI status

Use `scripts/verify_sprint.ps1` when a feature target-sequence and interaction
plan exist. It runs preflight, Release build, full CTest, the official mapped
target harness, and writes a hash-bearing manifest under
`artifacts/evidence/<sprint-id>.json`, alongside the exact logs, interaction
report, and retained screenshots. The verifier may use only an existing
app-owned target sequence; it must fail if the requested interaction contract
or output artifact is absent. Never hand-edit a passing manifest. Read every
retained image and log before claiming the evidence gate passed.

For a slice that demonstrably changes no GUI behavior, run the same verifier
with `-NonVisual` and no interaction plan. That mode still runs Qt/MinGW
preflight, Release build, and full CTest, and produces a hash-bearing manifest
with `visual_validation: not_applicable_no_gui_behavior_changed`. Do not use it
to avoid mapped interaction or screenshots for a user-visible change; the
default GUI mode still requires the app-owned plan and all its interaction and
image checks.

Pass `-WorkspaceOnlyEvidence` for GUI screenshots, captured logs, and other
local-only evidence. Do not commit screenshots or logs. The manifest remains
committed with hashes for those workspace-only payloads. The local commit hook
must find and hash-check every payload before accepting the commit; hosted CI
can verify the committed manifest and its fingerprint metadata, but cannot
independently inspect workspace-only payload bytes. Source, tests, docs,
interaction plans, and the manifest remain the reviewable commit contents.

The commit-msg hook can enforce a referenced manifest's existence, hash, and
`pass` status when installed. It checks evidence integrity, not screenshot
quality, code correctness, or that an artifact was produced by trusted CI. CI
must run on configured hosted/self-hosted runners and report its actual result.
Do not claim branch protection is active unless repository settings confirm it.
Local success is not CI success; a pending/unavailable check stays pending.
Complete the full local verification and commit the passing evidence manifest,
then push the verified branch to trigger its independent CI run. Confirm the
workflow ran against the pushed commit SHA and inspect its actual results; never
describe a missing, pending, or unavailable check as green. Open or update the
branch's pull request as appropriate, and merge only after the actual required
CI checks pass. Verify the resulting main SHA before cleaning up the merged
branch. Do not claim branch protection is configured unless repository settings
confirm it.

## Documentation, commit, and handoff

Update `docs/codebase-map.md`, `docs/features/implemented-features.md`,
`docs/devops/progress.md`, and the active sprint TODO in the same change set.
Record unresolved edges and skipped evidence in the TODO. Do not commit secrets,
vault/config data, local logs, unrelated files, or generated project boards.
Stage explicit paths only. Use a descriptive multi-line message with `Why`,
`Changed`, `Behavior`, `Verification`, and `Demo`. Push only after required
local gates pass; then wait for the independent CI result before merging. Verify
the resulting commit SHA. Clean up only the feature branch confirmed merged and
no longer needed.

For user-facing progress, use:

```text
Progress: Phase X/Y, Sprint N, <branch>, <status> <work in plain language>
```

## Open-ended language-server use cases

Use clangd for declarations, include/type diagnostics, Qt signal/slot references,
generated MOC-facing changes, and caller discovery. Use Pyright for provider,
orchestration, memory, telemetry, and IPC types. Use cmake-language-server for
target/dependency wiring and test registration. These are examples, not an
exclusive list: apply the appropriate server wherever it can cheaply detect a
real issue before the authoritative build and runtime checks.
