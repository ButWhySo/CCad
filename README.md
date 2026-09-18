# CCad

CCad is a ground-up, machine-callable PCB design kernel for native desktop CAD software. The first milestone builds a C++ logical circuit core, deterministic JSON representation, ERC checks, and a native CLI that agents can call without driving a GUI.

## Demo


https://github.com/user-attachments/assets/05717497-df24-4f35-9a38-16e1ddd11aba



## Current Phase

Phase 8 / 8: Agent Runtime and EDA Evidence Expansion.

Progress counter: Phase 9 / 9, Sprint 226 is active on `sprint-226-pcb-root-model` for the file-by-file KiCad PCB root model walk covering board design settings, `BOARD_ITEM` metadata, board-loader state parity, `BOARD_STACKUP` default physical stackup parity, board-statistics drill table and report parity, `BOARD_ITEM_CONTAINER` remove/delete parity, `BOARD_TEXT_VAR_ADAPTER` text-variable expansion parity, KiCad legacy board-BOM export parity, KiCad `CLEANUP_ITEM` action-catalog parity, KiCad `GENERAL_COLLECTOR` locked-item filtering, safe CLI project writes, and KiCad `ConvertOutlineToPolygon` first-slice outline reporting.

Phase 6 is complete. CCad now has an Agent protocol: JSON-RPC/MCP over the transaction bus, audit logs, permission gates, and benchmark harness. Phase 7 closed the first native Agent pane, GUI parity, and visual-validation backlog budget through Sprint 205. Phase 8 focuses on bounded Agent runtime state, EDA evidence expansion, runner queues, observability export wiring, and simulation planning slices.

- Typed project model.
- Deterministic JSON load/dump.
- Logical ERC diagnostics.
- CLI: `ccad help --format json`, `ccad init`, `ccad validate`, `ccad inspect`, and `ccad diff`.
- CLI agent metadata: `ccad agent methods`, `ccad agent harness-context`, and `ccad agent tool-guide --method <name>` expose the headless harness contract without launching Qt.
- CLI agent workspace state: `ccad agent state`, `ccad agent tasks`, `ccad agent evidence`, and `ccad agent approvals` expose deterministic provider-free Agent workspace state for headless tooling, with matching `agent.state`, `agent.workspace_state`, `agent.tasks`, `agent.evidence`, and `agent.approvals` JSON-RPC routes.
- CLI agent local sessions: `ccad agent session-schema`, `ccad agent session-new`, `ccad agent session-state`, `ccad agent checkpoint-add`, and `ccad agent replay` create, inspect, checkpoint, and replay provider-free local Agent session files, with read-only `agent.session_schema`, `agent.session_state`, and `agent.replay_manifest` JSON-RPC routes.
- CLI agent policy gates: `ccad agent policy-schema`, `ccad agent policy-check`, and `ccad agent dry-run` classify command risk, read/write permission needs, approval-required state, and dry-run decisions before execution, with matching `agent.policy_schema` and `agent.policy_check` JSON-RPC routes.
- CLI agent provider configuration: `ccad agent provider-config-schema`, `ccad agent provider-config-template`, and `ccad agent provider-status` expose BYOK/BYOT env-var metadata, no-secret templates, and presence-only readiness checks, with matching `agent.provider_config_schema`, `agent.provider_config_template`, and `agent.provider_status` JSON-RPC routes.

## Gemini BYOK quickstart

Set credentials only in the PowerShell process that launches CCad. Replace the model with any model supported by your Google account; `gemini-3.8-flash` is the current fast smoke-test choice.

```powershell
$env:GEMINI_API_KEY = "PASTE_KEY_HERE"
$env:CCAD_GEMINI_MODEL = "gemini-3.8-flash"
.\build\ccad.exe agent provider-status
$env:PATH = "C:\Qt\6.11.1\mingw_64\bin;$env:PATH"
.\build-qt\ccad_gui.exe
```

In Agent Settings, choose `Google Gemini`, confirm model, paste key, and press `Test Provider`. Test Provider uses the key transiently; Save persists preferences only, never the secret. Do not put keys in project JSON, `.env`, logs, screenshots, or Git. Remove temporary values with `Remove-Item Env:GEMINI_API_KEY,Env:GOOGLE_API_KEY,Env:CCAD_GEMINI_MODEL`.

## Cerebras BYOK quickstart

Install the bundled agent environment once, then launch the application with
the same environment. The current adapter uses Cerebras' OpenAI-compatible
endpoint and defaults to the official production model `gpt-oss-120b`.
`llama3.1-8b` is also production; `zai-glm-4.7` is preview. Cerebras marks
`qwen-3-235b-a22b-instruct-2507` deprecated, so use it only as an explicit
custom ID if the account still serves it.

```powershell
& src/ccad_agent/venv/Scripts/python.exe -m pip install -r src/ccad_agent/requirements.txt
$env:CEREBRAS_API_KEY = "PASTE_KEY_HERE"
$env:CCAD_CEREBRAS_MODEL = "gpt-oss-120b"
$env:CCAD_PROVIDER = "cerebras"
$env:PATH = "C:\Qt\6.11.1\mingw_64\bin;$env:PATH"
.\build-qt\ccad_gui.exe
```

In Agent Settings choose `Cerebras`, select `gpt-oss-120b`, enter the session
key, and press `Test Provider`. The GUI launches the checked-in venv by
absolute path and disables user-site packages, preventing stale global
LangChain versions from breaking the adapter. Remove the session variables
afterward with `Remove-Item Env:CEREBRAS_API_KEY,Env:CCAD_CEREBRAS_MODEL,Env:CCAD_PROVIDER`.

## OpenRouter BYOK quickstart

OpenRouter uses its official OpenAI-compatible endpoint. Its model catalog is
dynamic; use `openrouter/auto` or enter an exact current model ID from
[OpenRouter's model catalog](https://openrouter.ai/models).

```powershell
& src/ccad_agent/venv/Scripts/python.exe -m pip install -r src/ccad_agent/requirements.txt
$env:OPENROUTER_API_KEY = "PASTE_KEY_HERE"
$env:CCAD_OPENROUTER_MODEL = "openrouter/auto"
$env:CCAD_PROVIDER = "openrouter"
$env:PATH = "C:\Qt\6.11.1\mingw_64\bin;$env:PATH"
.\build-qt\ccad_gui.exe
```

In Agent Settings choose `OpenRouter`, enter the session key, and press
`Refresh models` when you explicitly want the current catalog in the model
dropdown; then select a model and press `Test Provider`. Startup and contract
tests do not fetch the catalog or consume quota. Clear temporary values afterward with
`Remove-Item Env:OPENROUTER_API_KEY,Env:CCAD_OPENROUTER_MODEL,Env:CCAD_PROVIDER`.

Provider safety limits are process-local and optional: `CCAD_PROVIDER_RETRIES`
caps transient retries at 0..2, `CCAD_AGENT_RECURSION_LIMIT` caps graph cycles
at 4..32 (default 12), `CCAD_AGENT_HISTORY_LIMIT` caps retained messages at
4..64 (default 24), and `CCAD_AGENT_CONTEXT_LIMIT` caps provider-bound project
context at 4096..131072 characters (default 32768). `CCAD_PROVIDER_TIMEOUT_SECONDS`
caps OpenAI-compatible adapter requests at 1..120 seconds (default 60). These
limits never store keys and never make network calls by themselves. `/cc` and
`/compact` compact session history locally: recent turns remain intact, older
turns become opaque count/size metadata, and no provider quota is consumed.
Agent memory is local JSON at `%APPDATA%/CCad/agent_memory.json` (override with
`CCAD_AGENT_MEMORY_PATH`). Use `/memory list`, `/memory list scope:x`, `/memory add <text>`,
`/memory update <id> <text>`, `/memory delete <id>`, `/memory clear all`, or `/memory clear scope:<name>`;
`memory.stm` controls bounded project-memory injection into provider context;
`add` accepts optional leading `scope:x` and `title:y` flags.
- CLI agent observability configuration: `ccad agent trace-export-schema`, `ccad agent trace-export-template`, `ccad agent trace-redaction-policy`, and `ccad agent trace-export-dry-run` expose disabled-by-default OpenTelemetry/Langfuse trace-export metadata, redaction policy, and no-network dry-run status, with matching `agent.trace_export_schema`, `agent.trace_export_template`, `agent.trace_redaction_policy`, and `agent.trace_export_dry_run` JSON-RPC routes.
- CLI agent KiCad evidence integration: `ccad agent kicad-evidence-schema`, `ccad agent kicad-evidence-plan`, `ccad agent kicad-evidence-dry-run`, and guarded `ccad agent kicad-evidence-run --execute` expose structured `kicad-cli` DRC/ERC/export command plans, readiness checks, artifact manifests, JSON-RPC routes, tool-guide discovery, and policy-gated execution for local KiCad evidence.
- Native Agent pane visual refinement: the right-side Agent pane now reports `visual_style:"agent_reference_panel_v4"` and `workspace_layout_version:4`, with targetable status rail, command composer, plan deck, evidence lane, and approval lane subregions while preserving the existing local-only controls and UI-map IDs.
- Native Agent pane durable session binding: the right-side Agent pane can bind a local `.ccad-agent-session.json` file, display session ID, thread ID, checkpoint count, latest checkpoint ID, replayable state, and append a metadata-only GUI checkpoint without provider execution, telemetry export, secrets, or project mutation.
- Native Agent pane command policy binding: the right-side Agent pane uses the shared core command-policy classifier to preview read/write risk, approval reason, dry-run-only decisions, and would-execute state for staged CCad CLI-shaped commands, with targetable policy widgets and UI-map checkbox support while still performing no provider execution, telemetry export, or hidden project mutation.
- Native Agent pane trace-link binding: the right-side Agent pane exposes local trace ID, span ID, trace status, export-disabled state, and a targetable new-trace action through `agent.workspace_state` and semantic UI-map IDs, without provider execution, OpenTelemetry export, Langfuse network calls, prompt/tool payload capture, secrets, or project mutation.
- Native Agent pane provider controls: the right-side Agent pane exposes targetable provider family, model-hint, environment-readiness, and execution-disabled controls through `agent.workspace_state` and semantic UI-map IDs, without provider execution, network probes, browser-account automation, secret display, or project mutation.
- Native Agent pane run queue foundation: the right-side Agent pane exposes a first-viewport local run queue, cancel and clear actions, queue counts, current-step text, queue status, and `agent.workspace_state` fields such as `run_queue_available`, `run_queue_id`, `run_queue_status`, `run_queue_depth`, `run_steps_total`, `run_step_current`, `run_queue_provider_execution_enabled:false`, `run_queue_worker_thread_enabled:false`, and `run_queue_trace_export_enabled:false`. Sprint-end verification passed the full Qt build plus 36 of 36 CTest tests.
- Native Agent pane reference polish v5: the right-side Agent pane reports `visual_style:"agent_reference_panel_v5"` and `workspace_layout_version:5`, adds a compact targetable header action bar, reference-style evidence thumbnail strip, approval preview artifact and delta labels, one-row footer quick actions, and first-viewport evidence visibility while preserving every existing Agent control ID and keeping provider execution, worker threads, telemetry export, external processes, and project mutation disabled. Sprint-end verification passed the full Qt build plus 36 of 36 CTest tests.
- CLI PCB authoring: `ccad pcb load-state`, `ccad pcb outline-polygon`, `ccad pcb drill-statistics`, `ccad pcb board-statistics`, `ccad pcb export-board-bom`, `ccad pcb expand-text-variables`, `ccad pcb list-nets`, `ccad pcb list-objects`, `ccad pcb get-object`, `ccad pcb route-status`, `ccad pcb set-outline`, `ccad pcb set-rules`, `ccad pcb add-layer`, `ccad pcb set-layer`, `ccad pcb remove-layer`, `ccad pcb set-layer-visibility`, `ccad pcb add-pad`, `ccad pcb set-pad`, `ccad pcb add-via`, `ccad pcb set-via`, `ccad pcb add-track`, `ccad pcb set-track`, `ccad pcb add-graphic-line`, `ccad pcb add-text`, `ccad pcb add-zone`, `ccad pcb add-keepout`, `ccad pcb add-placement-region`, `ccad pcb set-region-kind`, `ccad pcb remove-object`, `ccad pcb move-object`, `ccad pcb resize-object`, `ccad pcb autoplace-footprint`, and `ccad pcb spread-footprints`.
- CLI PCB KiCad API parity queries: `ccad pcb load-state`, `ccad pcb outline-polygon`, `ccad pcb drill-statistics`, `ccad pcb board-statistics`, `ccad pcb export-board-bom`, `ccad pcb expand-text-variables`, `ccad pcb remove-object`, `ccad pcb list-enabled-layers`, `ccad pcb list-visible-layers`, `ccad pcb get-layer-name`, `ccad pcb get-board-stackup`, `ccad pcb get-rules`, `ccad pcb get-outline`, `ccad pcb list-by-net`, and `ccad pcb list-connected` expose the first headless analogues for KiCad PCB API handler, loader, physical stackup, statistics, report summaries, legacy board BOM export, text-variable expansion, board-item container behavior, and Edge.Cuts outline polygon reporting.
- CLI project text variables: `ccad project set-text-variable` and `ccad project list-text-variables` persist and inspect the first KiCad-style project text variable map used by PCB text expansion.
- KiCad PCB layer-set utility parity: footprint placement resolves KiCad wildcard pad selectors such as `*.Cu` and `*.Mask` against the active board layers, board-context PCB queries expose `resolved_layers` and `kicad_layer_numbers`, and unresolved concrete selectors remain visible to validation instead of being silently discarded.
- Agent PCB API metadata: `ccad agent pcb-api-schema` and JSON-RPC `agent.pcb_api_schema` document the KiCad handler, enum, board-context, and headless-context ledgers, plus CCad command mapping, parity scope, and remaining unsupported handler gaps for automation.
- CLI footprint placement preserves logical net IDs when component pins already appear in project nets.
- CLI schematic authoring can place a converted KiCad/CCad symbol snapshot through `ccad sch place-symbol`.
- Placed schematic components persist symbol geometry in project JSON so pins and primitives survive reload.
- CLI PCB pad authoring supports KiCad-style pad type, shape, drill, roundrect ratio, chamfer ratio, and multi-layer metadata.
- Agent-facing PCB pad queries expose KiCad-style pad type, shape, drill, roundrect ratio, chamfer ratio, and layer metadata through `pcb get-object` and `pcb list-objects --type pad`.
- Native library catalog metadata, lookup, and search for local/offline component-library caches.
- Physical board outline, layers, copper zones, rectangular placement regions, rectangular keepouts, pads, vias, track segments, board graphic lines, and board text in project JSON.
- Project review and `ccad inspect` report the full board outline rectangle and active board-level DRC rules.
- Physical DRC for geometry, connectivity metadata, layer-aware copper connectivity and clearance, copper-zone identity/layer/net/outline validity, rectangular keepout occupancy, track crossing violations, board-level copper clearance, minimum track width, minimum via annular ring, design-rule value validation, stable physical object IDs, logical pad/net parity, route-request validity, route-request endpoint net parity, and route-request width policy.
- Physical DRC rejects pads and tracks placed on non-copper layers.
- Physical DRC track-endpoint connectivity now accepts geometric copper contact with same-net pads and vias, not only exact center-point matches.
- Optional Qt 6 native GUI for human review and board canvas viewing.
- Native GUI selection inspector for stable object type and ID.
- Native GUI layer/object browser and shape-level selection highlight.
- Native GUI diagnostic rows can select matching PCB canvas objects by stable ID.
- Project review and GUI diagnostics include physical DRC findings.
- Native GUI diagnostic markers are drawn over matching PCB canvas objects.
- Native GUI object browser rows can select matching PCB canvas objects by stable ID.
- Native GUI canvas rendering has explicit presentation-theme groundwork.
- Native GUI has read-only transaction timeline panel groundwork.
- Native GUI canvas items expose net metadata for net highlight groundwork.
- Native GUI object browser has net rows for net-selection workflow.
- Native GUI supports interactive footprint placement through the Add Footprint dialog, which calls `ccad_core` directly.
- Native GUI supports schematic symbol placement through the Add Symbol dialog, populating `Project::components`.
- Native GUI supports independent PCB and Schematic Canvas views through a tabbed editor interface.
- Native GUI toolbar layout follows KiCad's pcbnew organization (Top, Left, Right toolbars) for visual parity.
- Native GUI left and right PCB toolbars resolve KiCad SVG icons from `CCAD_KICAD_SRC` or a sibling `kicad_src` checkout, with tooltips preserving action names.
- Native GUI top toolbar actions now resolve KiCad-style icons and perform real Save, Board Setup, Undo, Redo, and Run DRC behavior instead of placeholder buttons.
- Native GUI can export a DRC report from the Tools menu.
- Native GUI library chooser now has KiCad-style chooser context with filtered library/name rows, details, and visual symbol/footprint previews.
- Native GUI library chooser indexes local `library-cache` entries without parsing every symbol or footprint on dialog open; selected rows are parsed lazily for details, preview, and final placement.
- Native GUI library chooser returns concrete row identity metadata for accepted symbols and footprints, including source path, library name, item name, file name, and source kind.
- Native GUI symbol chooser expands raw KiCad `.kicad_sym` library files into one row per top-level symbol, while nested unit symbols remain inside their parent.
- Native GUI footprint previews and the board canvas use a shared KiCad-inspired layer palette so copper, silkscreen, fabrication, courtyard, edge, and user graphics do not collapse into one generic color.
- Native GUI pad rendering separates copper from solder-mask and solder-paste aperture overlays, so visible mask or paste layers no longer force hidden copper to render.
- Native GUI can dump a read-only semantic UI map as JSON for LLM/automation tooling, including stable action IDs, tab/canvas bounds, canvas-object IDs, net/layer metadata, route provenance, and click target coordinates.
- Native GUI has app-owned placement-click regression automation so left-click footprint placement can be tested through the real Qt viewport event path without manual mouse control.
- Native GUI has app-owned UI-map mouse-target automation that moves to semantic targets, captures marked screenshots, resizes the window, and repeats to prove scalable coordinates.
- Native GUI can keep a local live UI-map socket open while the GUI runs, serving repeated JSON Lines agent catalog requests (`agent.methods`, `agent.method_schema`, `agent.quickstart`, `agent.harness_context`, `agent.run_profile`, `agent.safety_policy`, `agent.provider_policy`, `agent.observability_config`, `agent.evidence_manifest_schema`, `agent.tool_guide`, and `agent.workspace_state`), `ui.map`, `ui.map_compact`, `ui.role_summary`, dirty-node `ui.map_delta`, `ui.wait_for_delta`, `ui.watch_delta`, indexed `ui.index_stats`, `ui.get_node`, `ui.nodes_by_role`, `ui.find`, `ui.hit_test`, `ui.target`, `ui.target_board_point`, indexed `ui.nearest_canvas_object`, `ui.canvas_click`, `ui.canvas_drag`, `ui.current_tool`, `ui.cancel_tool`, high-level PCB workflow methods, `ui.trigger_safe`, `ui.epoch`, active-layer, and active-net requests for agents.
- Native GUI right-toolbar Add Via, Route Track, Add Zone, Add Keepout, Draw Graphic, Place Text, and Delete tools now enter real PCB edit modes and create or remove durable board primitives through the same Qt viewport event path used by app-owned tests.
- Native GUI has a PCB active-layer selector, exposes it as `control:active_pcb_layer`, serves active-layer query/set methods to agents, and uses the selected copper layer for footprint placement and Route Track commits.
- Native GUI unfinished toolbar tools now report visible planned-tool status and `future_tool_not_implemented` through the safe UI trigger contract instead of acting as silent stubs.
- Native GUI left toolbar Show Layers and Show Properties actions now toggle their existing panels and report `panel_toggled` through the safe UI trigger contract.
- Native GUI has a right-side Agent dock shell that can refresh the current UI-map JSON, trigger allowlisted safe UI actions by semantic ID, run live UI-map protocol queries from method/payload inputs, and expose itself as `panel:agent` plus `tab:agent` for automation targeting.
- Native GUI Agent dock is now a full-height right-side workspace column beside Layers / Objects, with a session header, workspace context cards, goal/task area, visible approval card with Accept/Decline controls, fixed command prompt, and visible Request Context / Trigger DRC actions.
- Native GUI Agent dock now has a command-center visual pass with icon header actions, model/mode/local-policy chips, Command/Evidence/Approvals selectors, a first-viewport Activity stream, structured activity events in `agent.workspace_state`, and semantic UI-map targets for passive regions such as `panel:agent_session_strip`, `panel:agent_mode_strip`, and `panel:agent_activity_stream`.
- Native GUI Agent dock now has a denser vertical command surface with local run controls, trace/session chips, and first-viewport active-plan rows. These controls update local `agent.workspace_state` fields such as `visual_style:"agent_command_center_dense"`, `workspace_layout_version:3`, `run_state`, `trace_label`, `session_label`, and `plan_items` without starting provider execution or remote telemetry export.
- `ccad agent serve` exposes a genuine MCP stdio subset (`initialize`, `ping`, `tools/list`, and `tools/call`) with one guarded `ccad_execute` tool; it is not yet a full MCP server for every GUI-map method.
- Agent-harness planning now treats the current dock as the first real vertical workspace shell; headless `ccad agent` commands expose workspace-state parity, local session checkpoint files, deterministic policy gates, and no-secret BYOK/BYOT configuration metadata, while the native Agent pane renders pinned screenshot/DRC/ERC/diagnostics outputs as structured evidence cards and binds durable session, policy, trace-link, provider-readiness, and local run-queue metadata. The next backlog targets are live tool-loop progress, actual span emission, observability export links, and provider-backed execution behind explicit approvals.
- Native GUI Agent-panel and live-socket protocol can now perform semantic UI interactions: dry-run click targeting, safe action/tab clicks, Agent-control focus and text entry, Escape cancellation, PCB canvas-object selection, selection inspection, bounded UI epoch waits, map-driven PCB board-point click/drag gestures, and high-level PCB workflows through the real Qt viewport event path.
- Native GUI left toolbar display controls now perform real view-state actions for grid visibility, polar cursor coordinates, inch units, full-window crosshair, ratsnest guides, net highlighting, and high-contrast display mode. Agent safe triggers return `display_state_toggled`, and UI-map action nodes expose checked state.
- Native GUI selection inspector cleans up stale editor rows immediately during rapid multi-selection changes, so net highlight and display-mode workflows do not stack old pad or track editors in the properties panel.
- Native GUI visual-validation harness timing is policy-guarded: single-preview screenshots settle for 7 seconds, while multi-target GUI validation uses a 5-second initial load and 800 ms per target/action.
- Native GUI resolves KiCad SVG icons from `CCAD_KICAD_SRC`, `F:\kicad_src`, and executable/current-directory candidates, and the build now links Qt SVG explicitly for KiCad-style toolbar icon rendering.
- Native GUI symbol placement resolves locally converted KiCad `extends` inheritance from `library-cache`, so derived symbols such as diode variants inherit parent pins before placement.
- Native GUI footprint placement accepts raw KiCad `.kicad_mod` files from the chooser as well as converted CCad footprint JSON.
- Native GUI Add behavior is editor-tab aware: PCB opens cache-backed footprint placement, while Schematic opens cache-backed symbol placement.
- Native GUI placement uses mouse-following footprint and symbol ghosts; left click commits through the core placement API, and Escape cancels before commit.
- Native GUI schematic-only projects open directly on the Schematic tab and render placed symbol body graphics plus pin leads instead of generic placeholder boxes.
- Native GUI no longer exposes raw KiCad symbol/footprint file preview entries as normal File menu placement actions.
- Native GUI renders front and back copper with distinct KiCad-inspired default colors, and selected tracks highlight across their visible copper width.
- Native GUI canvas renders circular, oval, ratio-controlled round-rect, trapezoid, and chamfered pads as shape-aware geometry and shows through-hole drill openings as visible annular rings.
- Native GUI interactive footprint placement and movement use shape-aware, layer-colored ghost previews and convert canvas scene coordinates back to board millimeters before calling the core placement APIs.
- Native GUI canvas toolbar has Fit, Zoom Out, Zoom In, and 100% review controls.
- Sprint demo automation now has a robust screenshot fallback path that captures only the spawned CCad window by PID when `ccad_gui --screenshot` fails.
- Native GUI has a clean `--screenshot-project` mode for screenshotting a loaded project without entering a measurement overlay.
- Native GUI canvas supports CAD-style pan with middle-drag, right-drag, or Shift+left-drag, plus clamped wheel zoom and expanded scene navigation bounds.
- GUI panel/canvas test binaries are now registered in CTest and run in the default `ctest` gate.
- Native GUI canvas supports keyboard navigation: `+`, `-`, `0`, `F`, `Home`, arrow-key panning, and `W/A/S/D` panning.
- Native GUI canvas supports hold-space hand-pan mode with left-drag.
- Native GUI status bar tool state now reflects pan mode transitions (`Select`, `Pan Ready`, `Pan Drag`).
- Native GUI includes in-app navigation controls help (`Help > Navigation Controls`, shortcut `F1`).
- Native GUI project summary shows route request count, open/partial/done route progress, and routed segment count.
- Native GUI object browser lists route-request intent rows and shows route provenance on routed track rows.
- Native GUI route-request rows can select routed tracks that were generated from that request.
- `ccad inspect` reports board route progress derived from route requests and track provenance.
- GitHub CI now uses `actions/checkout@v5` for Node 24 runner compatibility.
- Native GUI starts with a larger default window size derived from desktop available bounds.
- Placement regions are serialized, authored through the CLI, inspected in review JSON, checked by DRC, rendered in the Qt canvas, and listed in the object browser.
- Board-level physical DRC rules are serialized and configurable through `ccad pcb set-rules`.
- Native GUI startup crash (status `0xC0000005`) fixed by ordering status-label initialization before pan-mode callback wiring.
- Native GUI canvas rendering now respects board layer visibility for pads and tracks, while vias remain visible because they are cross-layer objects.
- Native GUI canvas rendering is origin-aware when the board outline uses a non-zero origin.
- Native GUI cursor status uses board-origin-aware coordinates for shifted board outlines.
- Native GUI project summary shows non-zero board outline origin next to board size.
- Native GUI project summary shows active board-level DRC rule values.
- `COPPER_CLEARANCE` DRC diagnostics include the configured clearance value that triggered the finding.
- Track-width and via annular-ring DRC diagnostics include the configured threshold value that triggered the finding.
- Via annular-ring DRC diagnostics are suppressed when via drill geometry is already invalid.
- Keepout geometry DRC diagnostics are suppressed when the checked pad, via, or track has invalid dimensions.
- Copper-clearance DRC diagnostics are suppressed when the checked pad, via, or track has invalid dimensions.
- CLI diagnostic JSON includes summary counts for total diagnostics, errors, and warnings.
- Catalog validation JSON includes summary counts for total diagnostics, errors, and warnings.
- Catalog search JSON includes a summary match count for agent workflows.
- Project diffs include board layer additions, removals, and changes.
- Project diffs include board outline additions, removals, and changes.
- Project diffs include board pad additions, removals, and changes.
- Project diffs include board via, track, zone, graphic-line, and text additions, removals, and changes.
- Project diffs include route-request intent additions, removals, and changes.
- Project diffs include board keepout and placement-region additions, removals, and changes.
- Project diffs include board design-rule changes.
- CLI diff tests cover board-level physical object entries in executable JSON output.
- CLI PCB authoring can list physical board net usage counts as compact JSON.
- CLI PCB authoring can report a KiCad-style first-slice physical board stackup with stackup items, derived thickness, and copper layer distances.
- CLI PCB authoring can emit KiCad-style first-slice drill statistics for pads and vias as compact JSON.
- CLI PCB authoring can list route-request intent records and route completion status as compact JSON.
- CLI PCB authoring can export compact route-job JSON for external router handoff, including KiCad-style pad type, shape, drill, ratio, layer, and rotation metadata.
- CLI PCB authoring can list board layer and physical object IDs as compact JSON, with optional type filtering.
- CLI PCB authoring can inspect one board layer or physical object by stable ID as compact JSON, including route provenance for tracks, zone copper-layer/net/fill metadata, and layer/text metadata for board graphics and board text.
- CLI PCB authoring can remove physical board objects by stable ID.
- CLI PCB authoring can remove unused board layers by stable ID.
- CLI PCB authoring can update board layer name, kind, and visibility by stable ID.
- CLI PCB authoring can move pads, vias, keepouts, and placement regions by stable ID.
- CLI PCB authoring can resize pads, keepouts, and placement regions by stable ID.
- CLI PCB authoring can update existing track endpoints and width by stable ID.
- CLI PCB authoring can update existing via diameter and drill by stable ID.
- CLI PCB authoring can update existing track net/layer metadata and via net metadata by stable ID.
- CLI PCB authoring can update existing pad metadata, layer, and rotation by stable ID.
- CLI PCB authoring can update keepout and placement-region kind values by stable ID.
- Board JSON now has typed route-request records for early routing-assistance intent.
- CLI PCB authoring can add typed route-request records by stable endpoint object IDs.
- CLI PCB authoring can update typed route-request records by stable ID.
- CLI PCB authoring can remove typed route-request records by stable ID.
- CLI PCB authoring can apply one routed segment or one routed polyline from a route request into board tracks while preserving source request provenance.
- Core PCB metadata includes the canonical KiCad named layer set used by `.kicad_pcb` files, including 32 copper layers, paired fabrication/assembly layers, board geometry layers, user layers through `User.9`, and agent-visible KiCad layer numbers.
- CLI PCB authoring can append missing KiCad standard layers with `ccad pcb add-standard-layers --file <path>` while preserving existing and custom layers.
- `ccad inspect` and the native GUI project summary report layer breakdown counts for copper/non-copper and visible/hidden layers.

Out of scope for the Phase 3 MVP: full interactive editing, automatic placement, a production autorouter, KiCad import/export, fabrication outputs, and network services.

## Git History Policy

For public GitHub publishing, internal markdown guidance files are kept locally for human and agent workflow but are removed from Git tracking; only `README.md` stays tracked as markdown.

When publishing a clean snapshot to an empty remote, never orphan long-term `main` history without reconnecting lineage. If a root snapshot commit is used, immediately attach the prior lineage with a non-rewriting merge, for example:

```bash
git merge --allow-unrelated-histories -s ours <full-history-branch> -m "merge: restore historical lineage onto public main"
```

This keeps the current public tree intact while restoring commit-log continuity for `git log`, debugging, and agent context retrieval.

## Setup

Install CMake and a C++20 compiler, then build the core/CLI/test targets:

```bash
cmake -S . -B build
cmake --build build
```

Use this core build when you are changing kernel, CLI, serialization, ERC, diff, transaction, or other non-GUI code.

## Verify

```bash
ctest --test-dir build --output-on-failure
```

If your local setup still has internal docs from private iterations, treat this README as the source of truth for the public GitHub branch.

## Library Cache Direction

CCad should reuse KiCad's symbol, footprint, and 3D model ecosystem through local source caches and CCad-native catalogs.

For large designs, CCad should use semantic batches, enriched component knowledge, local catalog search, and checkpoint-based visual review instead of thousands of primitive commands.

Policy:

- Before implementing PCB/CAD features or bugfixes, check current external references for that feature domain. KiCad official docs and developer file-format docs are the first compatibility source; use Altium, IPC-style manufacturing references, ngspice, OpenTelemetry, Langfuse, or LangGraph docs when those domains are affected.
- Do not fetch the internet repeatedly during normal design work.
- Do not vendor huge KiCad/Gitee/GitHub library dumps into this source repo.
- Keep raw upstream checkouts and converted catalogs in ignored local paths such as `library-cache/` or `catalog-cache/`.
- Store runtime/search data in CCad's own catalog format.
- Preserve source URL, mirror, commit/hash, source path, checksum, license, provenance, component-knowledge fields, and import warnings for each item.
- Use `usage_summary`, `layout_notes`, `source_confidence`, and `review_status` so agents can search local curated knowledge before using web research.
- Later, a separate `ccad-libraries` repo/package can distribute curated prebuilt catalogs.

Inspect a local catalog:

```powershell
.\build-qt\ccad.exe lib catalog-info --catalog .\catalog-cache\kicad.ccad-library.json
```

Find a catalog item by stable ID:

```powershell
.\build-qt\ccad.exe lib catalog-find --catalog .\catalog-cache\kicad.ccad-library.json --id footprint:Resistor_SMD:R_0603_1608Metric
```

Search a catalog by text:

```powershell
.\build-qt\ccad.exe lib catalog-search --catalog .\catalog-cache\kicad.ccad-library.json --query 0603
```

Limit search to one item kind:

```powershell
.\build-qt\ccad.exe lib catalog-search --catalog .\catalog-cache\kicad.ccad-library.json --query 0603 --kind footprint
```

## Windows Qt Development Loop

Run these commands from `F:\CCad` in PowerShell when you need the native desktop GUI too.

Mandatory precheck before configure/build/run:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\preflight_qt_env.ps1
```

This fails fast when the first `Qt6Core.dll` in `PATH` is not from `C:\Qt\6.11.1\mingw_64\bin`, or when `build-qt\CMakeCache.txt` is using the wrong compiler toolchain.

Configure the Qt build directory:

```powershell
cmake -S . -B build-qt `
  -DCCAD_WARNINGS_AS_ERRORS=ON `
  -DCCAD_BUILD_GUI=ON `
  -DCMAKE_PREFIX_PATH=C:\Qt\6.11.1\mingw_64
```

What it does:

- `cmake -S .` tells CMake the source tree is the current repo.
- `-B build-qt` writes generated build files into `build-qt`.
- `CCAD_WARNINGS_AS_ERRORS=ON` makes compiler warnings fail the build.
- `CCAD_BUILD_GUI=ON` requests the Qt desktop GUI target.
- `CMAKE_PREFIX_PATH=...` tells CMake where the local Qt install lives.

When to run:

- First time after cloning/creating the repo.
- After changing CMake files.
- After changing Qt install path or compiler/toolchain.
- When CMake says the cache is stale or misconfigured.

Build everything from scratch:

```powershell
cmake --build build-qt --clean-first
```

What it does:

- Deletes previous compiled outputs in `build-qt`.
- Rebuilds core library, CLI, tests, and GUI.

When to run:

- Before committing or merging.
- After broad refactors.
- After changing headers used by many targets.
- When you need confidence there are no stale object files hiding a build issue.

Run all tests:

```powershell
ctest --test-dir build-qt --output-on-failure
```

What it does:

- Runs the full CTest suite from `build-qt`.
- Prints failing test output if a test fails.

When to run:

- After every behavior change.
- Before every commit that claims working code.
- Before merging a sprint branch to `main`.

For agents or shells using `cmd.exe`, keep the Qt runtime path in the same one-line command:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && ctest --test-dir build-qt --output-on-failure"
```

GitHub Actions CI now runs three lanes by default: Linux core (`CCAD_BUILD_GUI=OFF`), Linux GUI (`CCAD_BUILD_GUI=ON` + Qt install + `QT_QPA_PLATFORM=offscreen` for headless GUI tests), and Windows core (`CCAD_BUILD_GUI=OFF`), and triggers on pushes to `main`, `phase-*`, and `sprint-*`, plus pull requests to `main`.

If CTest fails before any CCad test output with Windows status `0xc0000139`, assume a DLL loader problem first. Check that the intended Qt DLL is found before any other Qt install:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && where Qt6Core.dll"
```

## CLI

Create an empty project:

```bash
build/ccad init --name demo --out demo.ccad.json
```

Validate a project:

```bash
build/ccad validate demo.ccad.json
```

`validate` prints JSON diagnostics. Exit code `0` means no ERC errors. Exit code `1` means at least one error.

Discover commands as machine-readable JSON:

```bash
build/ccad help --format json
```

What it does:

- Emits deterministic command metadata with command names, summaries, and usage strings.
- Gives agents a stable command discovery surface without scraping human usage text.

When to run:

- Before an agent chooses a CLI command.
- When documenting or testing command availability.
- After adding a new CLI command to confirm it is discoverable.

Create a starter board project on Windows:

```powershell
.\build-qt\ccad.exe init --name canvas-demo --width-mm 42 --height-mm 28 --out .\build-qt\canvas-demo.ccad.json
```

What it does:

- Creates a deterministic `.ccad.json` project file.
- Adds a `42 mm x 28 mm` board outline.
- Adds default front/back copper layers.

When to run:

- When you need a quick board file for GUI review.
- When testing the board canvas path.
- When starting a small manual experiment.

Inspect a project:

```powershell
.\build-qt\ccad.exe inspect .\build-qt\canvas-demo.ccad.json
```

What it does:

- Loads a project.
- Emits machine-readable review JSON: project summary, counts, status, diagnostics, and board metadata including board origin and size.

When to run:

- When an agent needs project state without opening the GUI.
- When debugging whether counts/status match the file.
- Before handing a design state to another tool.

Validate a project on Windows:

```powershell
.\build-qt\ccad.exe validate .\build-qt\canvas-demo.ccad.json
```

What it does:

- Runs ERC diagnostics.
- Prints diagnostics and summary counts as JSON.
- Exits `0` if there are no ERC errors, `1` if ERC errors exist, `2` for usage/file/parse failures.

When to run:

- After editing project JSON.
- Before opening a project for review.
- In CI or agent workflows as a correctness gate.

Run physical DRC:

```powershell
.\build-qt\ccad.exe drc .\build-qt\canvas-demo.ccad.json
```

What it does:

- Runs kernel-level physical checks on board primitives.
- Emits JSON diagnostics and summary counts.
- Exits `0` when no DRC errors exist, `1` when DRC errors exist, `2` for usage/file/parse failures.
- Reports `COPPER_CLEARANCE` when different-net copper is closer than the configured board-level copper clearance, including that configured value in the diagnostic message.
- Reports invalid rule values when board-level copper clearance, minimum track width, or minimum via annular ring are non-positive in project JSON.
- Reports minimum track-width and via annular-ring violations with the configured threshold value in the diagnostic message.
- Prioritizes invalid via drill geometry over derived annular-ring violations to keep diagnostics focused.
- Prioritizes invalid pad, via, and track dimensions over derived keepout geometry violations.
- Prioritizes invalid pad, via, and track dimensions over derived copper-clearance violations.
- Treats layer-bound pad/track copper as colliding only when they share a copper layer; vias still interact with copper across layers.
- Reports `PAD_NON_COPPER_LAYER` or `TRACK_NON_COPPER_LAYER` when copper primitives reference a valid layer whose kind is not `copper`.

When to run:

- After adding pads, vias, or tracks.
- Before opening the GUI for review.
- Before treating a generated board as a valid intermediate artifact.

Plan KiCad CLI evidence for an external KiCad project:

```cmd
cmd /c "build-qt\ccad.exe agent kicad-evidence-schema"
cmd /c "build-qt\ccad.exe agent kicad-evidence-plan --kind pcb-drc --input board.kicad_pcb --output artifacts\kicad\drc.json --format json --units mm --severity all --exit-code-violations"
cmd /c "build-qt\ccad.exe agent kicad-evidence-dry-run --kind pcb-export-gerbers --input board.kicad_pcb --output artifacts\fab\gerbers --layers F.Cu,B.Cu"
cmd /c "build-qt\ccad.exe agent kicad-evidence-run --kind sch-erc --input root.kicad_sch --output artifacts\kicad\erc.json --format json"
```

What it does:

- Reports the supported `kicad-cli` evidence kinds for DRC, ERC, Gerbers, drill, position, IPC-2581, and ODB++ export.
- Emits structured command arrays and artifact manifests instead of asking agents to build shell strings.
- Checks whether the KiCad executable and input file are ready without running KiCad.
- Refuses to execute until `kicad-evidence-run` receives `--execute`; read-only JSON-RPC `agent serve` also rejects `execute:true` with approval reason `external_process_file_write`.

When to run:

- When an agent needs KiCad DRC/ERC/export evidence for a referenced KiCad design.
- Before creating fabrication artifacts from KiCad input.
- When CI or an external runner needs deterministic KiCad command planning without launching the CCad GUI.

Import a KiCad footprint:

```powershell
.\build-qt\ccad.exe lib import-footprint --in .\path\to\R_0805_2012Metric.kicad_mod --out .\build-qt\R_0805_2012Metric.ccad-footprint.json
```

What it does:

- Reads a KiCad `.kicad_mod` footprint file as data.
- Imports the footprint name and basic pad geometry.
- Writes deterministic CCad footprint JSON.

When to run:

- When reusing existing KiCad footprint library assets.
- When building a CCad package/footprint catalog.
- Before future placement commands consume library footprints.

Current limitation:

- The importer handles a narrow pad subset: number, type, shape, position, size, simple drill, and layers.
- Advanced KiCad footprint constructs are skipped safely for now.

Place an imported footprint on a board:

```powershell
.\build-qt\ccad.exe pcb place-footprint --file .\build-qt\canvas-demo.ccad.json --footprint .\build-qt\R_0805_2012Metric.ccad-footprint.json --component R1 --at-x-mm 16 --at-y-mm 14 --layer F.Cu --rotation-deg 90
```

What it does:

- Loads CCad footprint JSON.
- Places each footprint pad onto the board as a board pad.
- Generates stable pad IDs such as `R1.1` and `R1.2`.
- Preserves `net_id` from logical project nets when a matching component/pin net member exists.
- Rotates footprint-local pad centers and pad orientation when `--rotation-deg` is provided.

When to run:

- After importing a KiCad footprint.
- When generating board geometry from reusable library data.
- Before GUI review of placed package pads.

Current limitation:

- Pads without a matching logical net member keep an empty `net_id` and DRC reports that as a warning.
- Flipping, courtyard checks, automatic symbol-footprint assignment, and complete schematic-layout parity remain later work.

Place a schematic symbol through the CLI:

```powershell
.\build-qt\ccad.exe init --name schematic-demo --out .\build-qt\schematic-demo.ccad.json
.\build-qt\ccad.exe sch place-symbol --file .\build-qt\schematic-demo.ccad.json --symbol .\library-cache\symbols\1N4007.json --component D1 --at-x-mm 20 --at-y-mm 15 --rotation-deg 0
```

What it does:

- Loads one converted CCad/KiCad symbol JSON file, resolving local converted-symbol inheritance when needed.
- Adds one logical component at the requested schematic position.
- Stores a `symbol` snapshot inside that component so the schematic canvas can render body primitives and pin leads after save and reload.
- Rejects duplicate component IDs, missing symbol files, empty symbols, and malformed symbol JSON.

Current limitation:

- `sch place-symbol` places symbol geometry and pins only. It does not yet create wires, labels, hierarchical sheets, automatic reference annotation, or symbol-footprint assignment.

Add PCB primitives through the CLI:

```powershell
.\build-qt\ccad.exe pcb list-nets --file .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe pcb list-objects --file .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe pcb list-objects --file .\build-qt\canvas-demo.ccad.json --type track
.\build-qt\ccad.exe pcb get-object --file .\build-qt\canvas-demo.ccad.json --id F.Cu
.\build-qt\ccad.exe pcb list-enabled-layers --file .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe pcb list-visible-layers --file .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe pcb get-layer-name --file .\build-qt\canvas-demo.ccad.json --id F.Cu
.\build-qt\ccad.exe pcb get-board-stackup --file .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe pcb get-rules --file .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe pcb get-outline --file .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe pcb list-by-net --file .\build-qt\canvas-demo.ccad.json --net GND
.\build-qt\ccad.exe pcb list-connected --file .\build-qt\canvas-demo.ccad.json --id U1.1
.\build-qt\ccad.exe pcb set-outline --file .\build-qt\canvas-demo.ccad.json --x-mm 0 --y-mm 0 --width-mm 44 --height-mm 30
.\build-qt\ccad.exe pcb set-rules --file .\build-qt\canvas-demo.ccad.json --copper-clearance-mm 0.20 --min-track-width-mm 0.15 --min-via-annular-ring-mm 0.10
.\build-qt\ccad.exe pcb add-layer --file .\build-qt\canvas-demo.ccad.json --id In1.Cu --name "Inner 1 copper" --kind copper --visible false
.\build-qt\ccad.exe pcb add-standard-layers --file .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe pcb set-layer --file .\build-qt\canvas-demo.ccad.json --id In1.Cu --name "Inner signal copper" --kind copper --visible true
.\build-qt\ccad.exe pcb set-layer-visibility --file .\build-qt\canvas-demo.ccad.json --id In1.Cu --visible true
.\build-qt\ccad.exe pcb remove-layer --file .\build-qt\canvas-demo.ccad.json --id In1.Cu
.\build-qt\ccad.exe pcb add-pad --file .\build-qt\canvas-demo.ccad.json --id P1 --component U1 --pin 1 --net N1 --layers F.Cu --x-mm 5 --y-mm 6 --width-mm 1.5 --height-mm 1.0
.\build-qt\ccad.exe pcb set-pad --file .\build-qt\canvas-demo.ccad.json --id P1 --component U1 --pin 1 --net N1 --layers F.Cu --rotation-deg 90
.\build-qt\ccad.exe pcb add-via --file .\build-qt\canvas-demo.ccad.json --id V1 --net N1 --x-mm 8 --y-mm 9 --diameter-mm 0.8 --drill-mm 0.4
.\build-qt\ccad.exe pcb set-via --file .\build-qt\canvas-demo.ccad.json --id V1 --diameter-mm 1.0 --drill-mm 0.5 --net N2
.\build-qt\ccad.exe pcb add-track --file .\build-qt\canvas-demo.ccad.json --id T1 --net N1 --layer F.Cu --start-x-mm 5 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9 --width-mm 0.25
.\build-qt\ccad.exe pcb add-route-request --file .\build-qt\canvas-demo.ccad.json --id RR1 --net N1 --from P1 --to V1 --preferred-layer F.Cu --policy shortest_safe --width-mm 0.25
.\build-qt\ccad.exe pcb set-route-request --file .\build-qt\canvas-demo.ccad.json --id RR1 --net N1 --from V1 --to T1 --preferred-layer B.Cu --policy prefer_back --width-mm 0.30
.\build-qt\ccad.exe pcb export-route-job --file .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe pcb apply-route-segment --file .\build-qt\canvas-demo.ccad.json --request-id RR1 --track-id RT1 --layer F.Cu --start-x-mm 5 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9
.\build-qt\ccad.exe pcb apply-route-polyline --file .\build-qt\canvas-demo.ccad.json --request-id RR1 --track-prefix RTP --points-mm "5,6;6.5,7.5;8,9"
.\build-qt\ccad.exe pcb route-status --file .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe pcb autoplace-footprint --file .\build-qt\canvas-demo.ccad.json --footprint .\build-qt\R_0805_2012Metric.ccad-footprint.json --component R_AUTO --layer F.Cu --grid-mm 1
.\build-qt\ccad.exe pcb spread-footprints --file .\build-qt\canvas-demo.ccad.json --components R1,R_AUTO --target-x-mm 12 --target-y-mm 4 --component-gap-mm 1
.\build-qt\ccad.exe pcb export-board-bom --file .\build-qt\canvas-demo.ccad.json --output .\build-qt\canvas-demo-board-bom.csv
.\build-qt\ccad.exe pcb cleanup-actions
.\build-qt\ccad.exe pcb outline-polygon --file .\build-qt\canvas-demo.ccad.json --infer true
.\build-qt\ccad.exe pcb set-track --file .\build-qt\canvas-demo.ccad.json --id T1 --start-x-mm 6 --start-y-mm 7 --end-x-mm 9 --end-y-mm 10 --width-mm 0.30 --net N2 --layer B.Cu
.\build-qt\ccad.exe pcb set-region-kind --file .\build-qt\canvas-demo.ccad.json --id K1 --kind routing
.\build-qt\ccad.exe pcb move-object --file .\build-qt\canvas-demo.ccad.json --id V1 --x-mm 9 --y-mm 10
.\build-qt\ccad.exe pcb resize-object --file .\build-qt\canvas-demo.ccad.json --id P1 --width-mm 2.0 --height-mm 1.2
.\build-qt\ccad.exe pcb remove-object --file .\build-qt\canvas-demo.ccad.json --id T1
```

What these do:

- `pcb list-nets` emits compact physical net usage counts for pads, vias, and tracks.
- `pcb route-status` emits compact route progress counts for open, partial, and completed route requests, derived from current route requests and track provenance.
- `inspect` also reports board route progress in review JSON so GUI and agents share the same route-review contract.
- `pcb export-route-job` emits a compact deterministic route-job JSON envelope with schema/version metadata, units, board outline, design rules, layers, pads, vias, tracks, keepouts, placement regions, and route requests; pass `--request-id <id>` to export one request. Pad entries include KiCad-style `pad_type`, `shape`, `layers`, optional `drill_nm`, optional `roundrect_rratio`, optional `chamfer_ratio`, position, rotation, and size so external routers and AI tools can reason about actual pad geometry.
- `pcb load-state` emits a KiCad `BOARD_LOADER`-shaped load summary for a CCad project, including source format, initialization mode, board attachment, DRC readiness, connectivity readiness, layer/object/net counts, and remaining KiCad loader parity gaps. Pass `--initialize false` to mirror KiCad's raw load path before project attachment and derived initialization.
- `pcb drill-statistics` emits a KiCad `board_statistics`-referenced first-slice drill table for a CCad project, including unique row count, total drill count, and per-row count, shape, x/y size, plated state, source, and nullable copper start/stop layers for pads and vias.
- `pcb board-statistics` emits a KiCad `board_statistics_report`-referenced first-slice report for a CCad project, including board dimensions, rectangular board area, current object counts, minimum track width, minimum drill diameter, board thickness, and the same drill rows used by `pcb drill-statistics`.
- `pcb export-board-bom` emits a KiCad legacy board-side BOM CSV from placed footprint metadata. It rejects boards with no placed footprints, skips footprints marked excluded from BOM, groups rows by value plus footprint name, naturally sorts references such as `C2` before `C10`, and uses KiCad's legacy board-BOM columns: `Id`, `Designator`, `Footprint`, `Quantity`, `Designation`, and `Supplier and ref`.
- `pcb cleanup-actions` emits the KiCad `CLEANUP_ITEM` action catalog and `VECTOR_CLEANUP_ITEMS_PROVIDER` row semantics for cleanup planning. The current scope is discovery and agent/tool guidance for tracks/vias and graphics cleanup actions; actual redundant-track, dangling-via, duplicate-graphic, and merge algorithms remain future cleanup-engine work.
- `pcb outline-polygon` emits a KiCad `convert_shape_list_to_polygon`-referenced first-slice report for `Edge.Cuts` line chains, including source graphic IDs, ordered points, closed and valid state, bounding box, optional rectangular inference from the durable board outline, diagnostics, and pending shape-conversion features.
- `pcb cross-probe` resolves KiCad-style PCB/schematic cross-probing packets such as `$NET`, `$NETS`, `$PART`, `$PAD`, `$SELECT`, and `$CLEAR` into board and schematic target rows. The current scope is headless packet resolution for agents and tests; live GUI highlight, flash, zoom, Kiway/socket mail, and full sheet-path synchronization remain future editor integration work.
- `project set-text-variable` and `project list-text-variables` store and report the KiCad-style project text variable map used by board text expansion.
- `pcb expand-text-variables` emits a KiCad `ExpandTextVariables`-referenced first-slice expansion report for explicit text passed with `--text` or for every current board text when `--text` is omitted. Known `${NAME}` tokens are replaced from project `text_variables`, and unknown tokens remain visible with `resolved:false` metadata.
- `pcb list-objects` emits compact board layer and physical object rows as JSON, with optional type filtering for `layer`, `pad`, `via`, `track`, `keepout`, or `placement_region`. Pad rows include KiCad-style `pad_type`, `shape`, optional `drill_nm`, optional `roundrect_rratio`, and optional `chamfer_ratio`.
- `pcb get-object` emits one board layer, pad, via, track, keepout, or placement region by stable ID as compact JSON. Pad objects include their KiCad-style type, shape, drill, ratio, layer, position, rotation, and size metadata.
- `pcb list-enabled-layers`, `pcb list-visible-layers`, and `pcb get-layer-name` expose KiCad-like layer query surfaces without mutating the board.
- `pcb get-board-stackup` emits a KiCad `BOARD_STACKUP`-shaped first-slice physical stackup report derived from current board layers and board thickness, including stackup items, default copper/mask/dielectric thicknesses, FR4 material defaults, computed stackup thickness, copper layer distances, and compatible enabled-layer rows.
- `pcb get-rules` and `pcb get-outline` expose the current board design-rule and bounding-box slices used by KiCad API analogues such as `GetBoardDesignRules` and `GetBoundingBox`.
- `pcb list-by-net` and `pcb list-connected` expose the first same-net connectivity query slice for pads, vias, tracks, and zones. The current scope is declared as net-equivalent inspection, not KiCad's full connectivity graph solver.
- `pcb autoplace-footprint` loads CCad footprint JSON, scans legal board origins on the requested copper layer, uses a first KiCad-inspired matrix occupancy and distance-cost field, places the footprint through the same core placement API as manual placement, and returns the selected origin and score.
- `pcb spread-footprints` groups existing board pads by component ID, naturally sorts references such as `R1`, `R2`, and `R10`, and moves selected component groups into a deterministic non-overlapping placement lane while preserving each group's internal pad offsets.
- `pcb set-outline` replaces the rectangular board outline while rejecting outlines that would leave existing pads, vias, tracks, keepouts, or placement regions outside the board.
- `pcb set-rules` updates board-level DRC defaults for copper clearance, minimum track width, and minimum via annular ring.
- `pcb add-layer` appends a board layer with stable ID, display name, kind, and optional visibility.
- `pcb add-standard-layers` appends any missing KiCad standard named layers without removing existing custom layers.
- `pcb set-layer` updates an existing layer's display name, kind, and visibility while rejecting non-copper kind changes for layers referenced by pads or tracks.
- `pcb remove-layer` removes an unused board layer by stable ID.
- `pcb set-layer-visibility` updates an existing layer's visibility flag for review surfaces.
- `pcb add-pad` appends a KiCad-style pad to an existing board project. Optional metadata includes `--type`, `--shape`, `--drill-mm`, `--roundrect-rratio`, and `--chamfer-ratio`.
- `pcb set-pad` updates an existing pad's component, pin, net, layers, type, shape, roundrect ratio, chamfer ratio, and rotation.
- `pcb add-via` appends a plated via with diameter and drill size.
- `pcb set-via` updates an existing via diameter, drill size, and optional net ID.
- `pcb add-track` appends a straight copper track segment.
- `pcb set-track` updates an existing straight track segment's endpoints, width, optional net ID, and optional copper layer ID.
- `pcb set-route-request` updates an existing route-request intent record's net, endpoint object IDs, preferred copper layer, policy, and width.
- `pcb remove-route-request` removes an existing route-request intent record by stable ID.
- `pcb apply-route-segment` appends one track segment from a route request's net and width, records `source_route_request_id`, and defaults to the request's preferred layer when `--layer` is omitted; use `--complete false` for intermediate route segments, otherwise the satisfied request is removed.
- `pcb apply-route-polyline` expands `--points-mm "x,y;x,y;..."` into multiple track segments with generated IDs from `--track-prefix`, preserves route request provenance on every segment, and follows the same completion behavior as `pcb apply-route-segment`.
- `pcb set-region-kind` updates an existing keepout or placement region kind.
- `pcb move-object` moves a pad or via center, or a keepout or placement-region origin, by stable ID.
- `pcb resize-object` resizes a pad, keepout, or placement region by stable ID.
- `pcb remove-object` removes one physical board object by stable ID and emits a KiCad `BOARD_ITEM_CONTAINER`-shaped delete result with object kind, removal mode, item index, and `Delete()`-through-`Remove()` metadata. Pass `--mode bulk` when an agent is performing a batch-style container mutation rather than an ordinary user delete.
- These commands rewrite the same `.ccad.json` file using deterministic JSON.

When to run:

- When an agent needs to mutate a board without manual JSON editing.
- When building a small reproducible PCB test case.
- Before launching the GUI to review generated primitive geometry.

Current command guards:

- The project must already have a board.
- Outline width and height must be positive.
- Rule dimensions must be positive.
- Primitive IDs must be unique within their primitive type.
- Layer IDs must be unique.
- Removed layers must exist and must not be referenced by pads, tracks, zones, board graphics, or board text.
- Dimensions must be positive.
- Referenced layers must exist for pads, tracks, and zones.
- Pad, track, placed-footprint, and zone layers must be copper layers.
- Updated pad layers must be copper layers, and updated pad rotation must remain inside the board outline.
- Positions and track endpoints must be inside the board outline.
- Updated track endpoints and copper width margin must remain inside the board outline.
- Via drill must be less than or equal to via diameter.
- Updated via diameter and drill must stay valid and fully inside the board outline.
- Moved pads, vias, keepouts, and placement regions must remain fully inside the board outline.
- Resized pads, keepouts, and placement regions must remain fully inside the board outline.
- Removal IDs must match an existing pad, via, track, zone, keepout, placement region, board graphic, or board text.

Copper zones:

- Zones are represented in project JSON, can be authored through the CLI, are visible in the Qt board canvas as translucent layer-colored copper polygons, are listed in compact object queries, are checked by DRC, and export to KiCad PCB S-expressions as `zone`, `polygon`, and deterministic first-slice `filled_polygon` records.
- A zone has `id`, optional `name`, optional `net_id`, ordered copper `layer_ids`, outline points, priority, clearance, minimum thickness, fill enabled state, and pad connection mode.
- Current zone authoring is a first slice. It creates deterministic rectangular or explicit-point outlines and filled previews, but it does not yet implement KiCad's full refill solver, thermal spokes, island removal, cutouts, or zone manager UI.

Add a rectangular copper zone through the CLI:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad.exe pcb add-zone --file build-qt\canvas-demo.ccad.json --id Z1 --name DC_NEG_FILL --net DC_NEG --layers B.Cu --x-mm 6 --y-mm 20 --width-mm 30 --height-mm 6 --priority 0 --clearance-mm 0.20 --min-thickness-mm 0.15 --pad-connection solid"
```

Example JSON fragment:

```json
"zones": [
  {
    "id": "Z1",
    "name": "DC_NEG_FILL",
    "net_id": "DC_NEG",
    "layer_ids": ["B.Cu"],
    "outline": [
      { "x_nm": 6000000, "y_nm": 20000000 },
      { "x_nm": 36000000, "y_nm": 20000000 },
      { "x_nm": 36000000, "y_nm": 26000000 },
      { "x_nm": 6000000, "y_nm": 26000000 }
    ],
    "priority": 0,
    "clearance_nm": 200000,
    "min_thickness_nm": 150000,
    "fill_enabled": true,
    "pad_connection": "solid"
  }
]
```

Rectangular keepouts:

- Keepouts are represented in project JSON, can be authored through the CLI, are visible in the Qt board canvas, and are checked by DRC.
- A keepout has `id`, `kind`, and rectangular `area`.
- DRC errors if a pad center, via center, track endpoint, or track segment lies inside or crosses a keepout.

Add a keepout through the CLI:

```powershell
.\build-qt\ccad.exe pcb add-keepout --file .\build-qt\canvas-demo.ccad.json --id K1 --kind placement --x-mm 20 --y-mm 10 --width-mm 4 --height-mm 3
```

Example JSON fragment:

```json
"keepouts": [
  {
    "id": "keepout-mounting-hole",
    "kind": "placement",
    "area": {
      "x_nm": 20000000,
      "y_nm": 20000000,
      "width_nm": 4000000,
      "height_nm": 3000000
    }
  }
]
```

Current limitation:

- These commands create raw primitives only. They do not run full DRC, enforce schematic parity, solve placement, or route nets automatically yet.

Placement regions:

- Placement regions are represented in project JSON, can be authored through the CLI, are visible in the Qt board canvas, are listed in review/object browser surfaces, and are checked by DRC.
- A placement region has `id`, `kind`, and rectangular `area`.
- Current accepted kinds are `component` and `module`.
- Placement regions are guidance geometry for future placement engines and human/agent review. They do not automatically move components yet.

Add a placement region through the CLI:

```powershell
.\build-qt\ccad.exe pcb add-placement-region --file .\build-qt\canvas-demo.ccad.json --id PR1 --kind component --x-mm 8 --y-mm 6 --width-mm 12 --height-mm 8
```

Example JSON fragment:

```json
"placement_regions": [
  {
    "id": "power-cluster",
    "kind": "component",
    "area": {
      "x_nm": 8000000,
      "y_nm": 6000000,
      "width_nm": 12000000,
      "height_nm": 8000000
    }
  }
]
```

## GUI

CCad can build an optional native Qt 6 Widgets review GUI named `ccad_gui`. It is intended for human co-working and review while the kernel and CLI remain the source of truth.

If Qt 6 Widgets is installed, CMake builds it automatically:

```bash
cmake -S . -B build-qt -DCCAD_BUILD_GUI=ON -DCMAKE_PREFIX_PATH=C:\Qt\6.11.1\mingw_64
cmake --build build-qt
```

If Qt is not installed, core, CLI, and tests still build.

On Windows, add Qt DLLs to `PATH` before launching the GUI:

```powershell
$env:PATH = "C:\Qt\6.11.1\mingw_64\bin;$env:PATH"
.\build-qt\ccad_gui.exe
```

Open a project directly:

```powershell
$env:PATH = 'C:\Qt\6.11.1\mingw_64\bin;' + $env:PATH
.\build-qt\ccad_gui.exe .\build-qt\canvas-demo.ccad.json
```

Capture a clean project screenshot without entering a measurement overlay:

```powershell
$env:PATH = 'C:\Qt\6.11.1\mingw_64\bin;' + $env:PATH
.\build-qt\ccad_gui.exe --screenshot-project .\build-qt\canvas-demo.ccad.json .\artifacts\screenshots\canvas-demo.png
```

Dump a read-only semantic UI map for agent tooling:

```powershell
$env:PATH = 'C:\Qt\6.11.1\mingw_64\bin;' + $env:PATH
.\build-qt\ccad_gui.exe --dump-ui-map .\artifacts\demos\sprint156-layer-color-final.ccad.json .\artifacts\demos\ui-map.json
```

The map reports stable IDs such as `action:add_footprint`, `control:active_pcb_layer`, `tab:pcb`, `canvas:pcb`, and `canvas_object:JAC1.1`, plus screen-space target coordinates and CAD metadata. The top-level map also reports `active_pcb_layer_id`.

Validate the exported target coordinates against the live Qt window:

```powershell
$env:PATH = 'C:\Qt\6.11.1\mingw_64\bin;' + $env:PATH
.\build-qt\ccad_gui.exe --validate-ui-map-targets .\artifacts\demos\sprint156-layer-color-final.ccad.json .\artifacts\demos\ui-map-target-validation.json
```

The validator moves the cursor to every visible/enabled exported target and verifies that Qt hit-testing resolves the expected widget, tab, canvas, or canvas item. Hidden or disabled nodes are reported as skipped.

Query one target by semantic ID:

```powershell
$env:PATH = 'C:\Qt\6.11.1\mingw_64\bin;' + $env:PATH
.\build-qt\ccad_gui.exe --ui-target-id .\artifacts\demos\sprint156-layer-color-final.ccad.json action:add_footprint .\artifacts\demos\ui-target-add-footprint.json
```

The native GUI also exposes its right-side Agent dock as `panel:agent`, with `tab:agent` kept as a semantic alias that raises the dock:

```powershell
$env:PATH = 'C:\Qt\6.11.1\mingw_64\bin;' + $env:PATH
.\build-qt\ccad_gui.exe --ui-target-id .\artifacts\demos\sprint156-layer-color-final.ccad.json panel:agent .\artifacts\demos\ui-target-agent-panel.json
```

The panel itself can refresh the current UI-map JSON, trigger allowlisted safe actions such as `action:zoom_in`, focus and type into its own agent controls through semantic IDs, select rendered PCB canvas objects, capture an app-owned `ui.screenshot`, stage a local task goal, pin or clear bounded structured evidence cards, create and resolve one local approval request, and run live protocol queries such as `agent.methods`, `agent.method_schema`, `agent.quickstart`, `agent.harness_context`, `agent.run_profile`, `agent.safety_policy`, `agent.provider_policy`, `agent.observability_config`, `agent.evidence_manifest_schema`, `agent.tool_guide`, `agent.workspace_state`, `ui.find`, `ui.map_compact`, `ui.index_stats`, `ui.get_node`, `ui.nodes_by_role`, `ui.hit_test`, `ui.click`, `ui.canvas_click`, `ui.canvas_drag`, `ui.current_tool`, `ui.cancel_tool`, `ui.place_via`, `ui.route_track`, `ui.add_zone`, `ui.add_keepout`, `ui.draw_graphic`, `ui.place_text`, `ui.delete_object`, `ui.type_text`, `ui.get_selection`, `project.context`, `project.object_counts`, `project.review`, `project.erc`, `project.drc`, `project.diagnostics`, `ui.watch_delta`, `ui.wait_for_delta`, or `ui.map_delta` through method/payload inputs. `agent.workspace_state` now includes both legacy summaries and `evidence_cards` with card ID, kind, title, summary, method, artifact path, counts, size metadata, trace ID, span ID, and source; it also reports command-center fields such as `visual_style`, `workspace_layout_version`, `visible_sections`, `permission_label`, structured `activity_events`, local `run_state`, trace/session labels, bounded `plan_items`, local trace-link metadata, no-secret provider readiness fields, and local run-queue fields such as `run_queue_available`, `run_queue_status`, `run_queue_depth`, `run_steps_total`, `run_step_current`, `run_queue_cancelable`, `run_queue_provider_execution_enabled:false`, `run_queue_worker_thread_enabled:false`, and `run_queue_trace_export_enabled:false`. The UI map exposes command-center passive and control regions such as `panel:agent_session_strip`, `panel:agent_mode_strip`, `panel:agent_trace_strip`, `panel:agent_run_controls`, `panel:agent_run_queue`, `label:agent_run_queue_status`, `label:agent_run_queue_counts`, `label:agent_run_queue_current_step`, `action:agent_cancel_run_queue`, `action:agent_clear_run_queue`, `panel:agent_active_plan`, `panel:agent_plan_row_1`, `panel:agent_provider_controls`, `control:agent_provider_family`, `control:agent_provider_model`, `action:agent_provider_refresh_status`, `tab:agent_command`, `tab:agent_evidence`, `tab:agent_approvals`, `action:agent_pause_run`, `action:agent_resume_run`, and `action:agent_stop_run`. `agent.evidence_manifest_schema` documents evidence-card fields for GUI and headless callers. Headless CLI now exposes no-secret provider configuration methods and disabled-by-default trace export configuration metadata, while the GUI binds trace, provider readiness, and first-slice run-queue state as local metadata. It is a local shell only; provider execution, BYOK routing, LangGraph orchestration, background runner threads, and actual OpenTelemetry/Langfuse export remain future integration work.

Query one PCB board point:

```powershell
$env:PATH = 'C:\Qt\6.11.1\mingw_64\bin;' + $env:PATH
.\build-qt\ccad_gui.exe --ui-target-board-point .\artifacts\demos\sprint156-layer-color-final.ccad.json 8 17 .\artifacts\demos\ui-target-board-point.json
```

Target responses include logical Qt pixels, physical pixels, and the device-pixel ratio so OS-level input tools do not have to infer high-DPI scaling.

Trigger a safe non-destructive GUI action directly by semantic ID:

```powershell
$env:PATH = 'C:\Qt\6.11.1\mingw_64\bin;' + $env:PATH
.\build-qt\ccad_gui.exe --ui-trigger-safe .\artifacts\demos\sprint156-layer-color-final.ccad.json action:zoom_in .\artifacts\demos\ui-action-zoom-in.json
```

Only a small allowlist of view/navigation actions is executable this way. Mutating, dialog-opening, or file-writing actions return `performed:false` with a reason field.

Left-toolbar display controls such as `action:grid`, `action:polar_coord`, `action:unit_inch`, `action:cursor_shape`, `action:show_ratsnest`, `action:net_highlight`, and `action:contrast_mode` are implemented safe display actions. They return `performed:true`, `reason:"display_state_toggled"`, and their current state, while the UI map exposes checked state for action nodes. Left-toolbar panel controls `action:layers_manager` and `action:part_properties` are implemented safe actions and return `reason:"panel_toggled"`. Right-toolbar PCB editor entries `action:add_tracks`, `action:add_via`, `action:add_zone`, `action:add_keepout_area`, `action:add_graphical_segments`, and `action:text` now return `performed:true`, `reason:"editor_tool_selected"`, and a concrete mode value, while `action:delete_cursor` removes the selected board primitive when the model supports that object type. Remaining editor tools are not silently ignored; unfinished actions return `performed:false`, `reason:"future_tool_not_implemented"`, and the user-facing label while the GUI status bar shows the same planned-tool state.

Query or set the GUI PCB active layer:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --ui-active-layer artifacts\demos\sprint171-pcb-active-layer-context-final.ccad.json artifacts\demos\sprint171-active-layer.json"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --ui-set-active-layer artifacts\demos\sprint171-pcb-active-layer-context-final.ccad.json B.Cu artifacts\demos\sprint171-set-active-layer.json"
```

The active layer is GUI/editor state, not extra board JSON. It lists and accepts board copper layers only. Footprint placement and Route Track use the selected active copper layer; via placement remains through-board but uses the active copper color for its placement ghost.

Query or set the GUI PCB active net:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --ui-active-net artifacts\demos\sprint172-pcb-active-net-context-final.ccad.json artifacts\demos\sprint172-active-net.json"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --ui-set-active-net artifacts\demos\sprint172-pcb-active-net-context-final.ccad.json DC_NEG artifacts\demos\sprint172-set-active-net.json"
```

The active net is GUI/editor state, not extra board JSON. It derives available nets from top-level project nets and existing board copper net IDs, exposes `control:active_pcb_net` in the UI map, and is used by Add Via and Route Track when they create new copper. Footprint placement still maps pad nets from component-pin connectivity.

Run the app-owned placement-click regression harness:

```powershell
$env:PATH = 'C:\Qt\6.11.1\mingw_64\bin;' + $env:PATH
.\build-qt\ccad_gui.exe --test-place-footprint-click .\artifacts\demos\sprint160-placement-click-smoke.ccad.json .\artifacts\demos\sprint159-safe-ui-actions-final-R_0805_2012Metric.ccad-footprint.json 18 12 .\artifacts\demos\sprint160-placement-click-result.json
```

This loads the project in the native GUI, enters footprint placement mode, sends the placement click through the real Qt viewport event path, writes compact result JSON, and exits. It is intended for regression and agent harness work, not for normal user authoring.

The same app-owned viewport-click path can exercise first-batch PCB edit tools:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --test-place-via-click artifacts\demos\sprint170-pcb-edit-tool-entry-final.ccad.json 15 11 artifacts\demos\sprint170-via-result.json"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --test-route-track-click artifacts\demos\sprint170-pcb-edit-tool-entry-final.ccad.json 8 9 15 11 artifacts\demos\sprint170-track-result.json"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --test-place-zone-click artifacts\demos\sprint174-pcb-zone-tool-final.ccad.json 6 20 38 28 artifacts\demos\sprint174-zone-result.json"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --test-place-keepout-click artifacts\demos\sprint170-pcb-edit-tool-entry-final.ccad.json 20 10 25 14 artifacts\demos\sprint170-keepout-result.json"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --test-delete-board-object artifacts\demos\sprint170-pcb-edit-tool-entry-final.ccad.json V1 artifacts\demos\sprint170-delete-result.json"
```

These modes enter the same Add Via, Route Track, Add Zone, Add Keepout, and Delete paths that humans invoke from the right toolbar, then return compact JSON such as `reason:"placed"` or `reason:"deleted"` with the updated object count or deleted type.

Run the app-owned UI-map mouse-target harness:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_ui_map_mouse_target_demo.ps1 -Name sprint161-ui-map-mouse-targets -ProjectPath .\artifacts\demos\sprint160-placement-crash-ci-final.ccad.json
```

The script plays the docs beep, waits two seconds, launches the GUI target-sequence mode, lets the window settle for five seconds, moves the cursor to semantic targets such as Select, Measure, Save, File, the properties/DRC panel, and the Agent tab, waits about 800 ms per target before screenshots, saves marked PNGs, resizes the window, repeats the same targets, and writes a JSON report.

For an explicit model refresh, send `{"method":"agent.list_models","params":{"provider":"openrouter"}}`.
This requires the provider key already held by the agent process; it is never
performed automatically and other providers report unsupported catalog status.

Run the live UI-map local socket server:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --serve-ui-map artifacts\demos\sprint162-pad-layer-rendering-fidelity-final.ccad.json ccad-ui-map-demo artifacts\demos\ccad-ui-map-demo.ready.txt"
```

While the GUI stays open, connect to the named local socket and send one JSON request per line. Current methods are `{"method":"agent.methods"}`, `{"method":"agent.method_schema","method_name":"ui.watch_delta"}`, `{"method":"agent.quickstart"}`, `{"method":"agent.harness_context"}`, `{"method":"agent.run_profile"}`, `{"method":"agent.safety_policy"}`, `{"method":"agent.provider_policy"}`, `{"method":"agent.observability_config"}`, `{"method":"agent.evidence_manifest_schema"}`, `{"method":"agent.tool_guide","method_name":"ui.route_track"}`, `{"method":"agent.workspace_state"}`, `{"method":"ui.map"}`, `{"method":"ui.screenshot","dry_run":true}`, `{"method":"ui.map_compact","role":"action","limit":20}`, `{"method":"ui.role_summary"}`, `{"method":"ui.index_stats"}`, `{"method":"ui.get_node","id":"menu:file"}`, `{"method":"ui.nodes_by_role","role":"action","limit":20}`, `{"method":"ui.map_delta","since_epoch":0}`, `{"method":"ui.wait_for_delta","since_epoch":0,"timeout_ms":20}`, `{"method":"ui.watch_delta","since_epoch":0,"timeout_ms":20,"max_events":4}`, `{"method":"ui.find","query":"add","role":"action","limit":8}`, `{"method":"ui.hit_test","x":120,"y":80}`, `{"method":"ui.target","id":"menu:file"}`, `{"method":"ui.target_board_point","x_mm":8,"y_mm":9}`, `{"method":"ui.nearest_canvas_object","canvas":"canvas:pcb","x_mm":8,"y_mm":9}`, `{"method":"ui.click","id":"action:zoom_in","dry_run":true}`, `{"method":"ui.canvas_click","x_mm":15,"y_mm":11}`, `{"method":"ui.canvas_drag","start_x_mm":8,"start_y_mm":9,"end_x_mm":18,"end_y_mm":12}`, `{"method":"ui.current_tool"}`, `{"method":"ui.cancel_tool"}`, `{"method":"ui.place_via","x_mm":14,"y_mm":10}`, `{"method":"ui.route_track","start_x_mm":8,"start_y_mm":9,"end_x_mm":18,"end_y_mm":12}`, `{"method":"ui.add_zone","start_x_mm":2,"start_y_mm":2,"end_x_mm":20,"end_y_mm":12}`, `{"method":"ui.add_keepout","start_x_mm":22,"start_y_mm":8,"end_x_mm":28,"end_y_mm":14}`, `{"method":"ui.draw_graphic","start_x_mm":4,"start_y_mm":24,"end_x_mm":18,"end_y_mm":24}`, `{"method":"ui.place_text","x_mm":8,"y_mm":22,"text":"FLOW TEXT"}`, `{"method":"ui.delete_object","id":"V1"}`, `{"method":"ui.double_click","id":"menu:file","dry_run":true}`, `{"method":"ui.type_text","id":"control:agent_live_method","text":"ui.role_summary"}`, `{"method":"ui.type_text","id":"control:agent_goal","text":"Route DC bridge evidence"}`, `{"method":"ui.click","id":"action:agent_stage_goal"}`, `{"method":"ui.click","id":"action:agent_pin_evidence"}`, `{"method":"ui.click","id":"action:agent_clear_evidence"}`, `{"method":"ui.type_text","id":"control:agent_approval_request","text":"Approve before DRC repair"}`, `{"method":"ui.click","id":"action:agent_request_approval"}`, `{"method":"ui.click","id":"action:agent_approve_next"}`, `{"method":"ui.click","id":"action:agent_decline_next"}`, `{"method":"ui.click","id":"action:agent_cancel_approval"}`, `{"method":"ui.click","id":"action:agent_clear_approvals"}`, `{"method":"ui.key","key":"Escape"}`, `{"method":"ui.select_canvas_object","id":"U1.1"}`, `{"method":"ui.get_selection"}`, `{"method":"ui.wait_for_epoch","minimum_epoch":1,"timeout_ms":20}`, `{"method":"ui.trigger_safe","id":"tab:agent"}`, `{"method":"ui.epoch"}`, `{"method":"ui.active_layer"}`, `{"method":"ui.set_active_layer","layer_id":"B.Cu"}`, `{"method":"ui.active_net"}`, `{"method":"ui.set_active_net","net_id":"DC_NEG"}`, `{"method":"project.context"}`, `{"method":"project.object_counts"}`, `{"method":"project.review"}`, `{"method":"project.erc"}`, `{"method":"project.drc"}`, and `{"method":"project.diagnostics"}`. Responses are newline-delimited JSON values.

What it does:

- Starts the native Qt review GUI.
- Loads the selected `.ccad.json` project.
- Shows project counts, route progress, status, diagnostics, board outline, and current PCB primitives.

When to run:

- When a human wants to review what the kernel/CLI generated.
- When checking visual canvas behavior.
- After `init`, `inspect`, or JSON edits when visual confirmation is needed.

Typical local loop:

```powershell
cmake -S . -B build-qt `
  -DCCAD_WARNINGS_AS_ERRORS=ON `
  -DCCAD_BUILD_GUI=ON `
  -DCMAKE_PREFIX_PATH=C:\Qt\6.11.1\mingw_64
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
.\build-qt\ccad.exe init --name canvas-demo --width-mm 42 --height-mm 28 --out .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe pcb load-state --file .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe pcb set-outline --file .\build-qt\canvas-demo.ccad.json --x-mm 0 --y-mm 0 --width-mm 44 --height-mm 30
.\build-qt\ccad.exe pcb set-rules --file .\build-qt\canvas-demo.ccad.json --copper-clearance-mm 0.20 --min-track-width-mm 0.15 --min-via-annular-ring-mm 0.10
.\build-qt\ccad.exe pcb add-layer --file .\build-qt\canvas-demo.ccad.json --id In1.Cu --name "Inner 1 copper" --kind copper --visible false
.\build-qt\ccad.exe pcb set-layer --file .\build-qt\canvas-demo.ccad.json --id In1.Cu --name "Inner signal copper" --kind copper --visible true
.\build-qt\ccad.exe pcb set-layer-visibility --file .\build-qt\canvas-demo.ccad.json --id In1.Cu --visible true
.\build-qt\ccad.exe pcb add-pad --file .\build-qt\canvas-demo.ccad.json --id P1 --component U1 --pin 1 --net N1 --layers F.Cu --x-mm 5 --y-mm 6 --width-mm 1.5 --height-mm 1.0
.\build-qt\ccad.exe pcb add-via --file .\build-qt\canvas-demo.ccad.json --id V1 --net N1 --x-mm 8 --y-mm 9 --diameter-mm 0.8 --drill-mm 0.4
.\build-qt\ccad.exe pcb set-via --file .\build-qt\canvas-demo.ccad.json --id V1 --diameter-mm 1.0 --drill-mm 0.5 --net N2
.\build-qt\ccad.exe pcb drill-statistics --file .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe pcb board-statistics --file .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe pcb outline-polygon --file .\build-qt\canvas-demo.ccad.json --infer true
.\build-qt\ccad.exe pcb cross-probe --file .\build-qt\canvas-demo.ccad.json --packet "$NET: N1"
.\build-qt\ccad.exe project set-text-variable --file .\build-qt\canvas-demo.ccad.json --key REV --value A1
.\build-qt\ccad.exe project list-text-variables --file .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe pcb expand-text-variables --file .\build-qt\canvas-demo.ccad.json --text "Revision ${REV}"
.\build-qt\ccad.exe pcb add-track --file .\build-qt\canvas-demo.ccad.json --id T1 --net N1 --layer F.Cu --start-x-mm 5 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9 --width-mm 0.25
.\build-qt\ccad.exe pcb add-route-request --file .\build-qt\canvas-demo.ccad.json --id RR1 --net N1 --from P1 --to V1 --preferred-layer F.Cu --policy shortest_safe --width-mm 0.25
.\build-qt\ccad.exe pcb set-route-request --file .\build-qt\canvas-demo.ccad.json --id RR1 --net N1 --from V1 --to T1 --preferred-layer B.Cu --policy prefer_back --width-mm 0.30
.\build-qt\ccad.exe pcb export-route-job --file .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe pcb apply-route-segment --file .\build-qt\canvas-demo.ccad.json --request-id RR1 --track-id RT1 --layer F.Cu --start-x-mm 5 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9
.\build-qt\ccad.exe pcb set-track --file .\build-qt\canvas-demo.ccad.json --id T1 --start-x-mm 6 --start-y-mm 7 --end-x-mm 9 --end-y-mm 10 --width-mm 0.30 --net N2 --layer B.Cu
.\build-qt\ccad.exe pcb add-layer --file .\build-qt\canvas-demo.ccad.json --id F.SilkS --name "Front silkscreen" --kind silkscreen --visible true
.\build-qt\ccad.exe pcb add-layer --file .\build-qt\canvas-demo.ccad.json --id Dwgs.User --name "User drawings" --kind user --visible true
.\build-qt\ccad.exe pcb add-graphic-line --file .\build-qt\canvas-demo.ccad.json --id G1 --layer Dwgs.User --start-x-mm 3 --start-y-mm 4 --end-x-mm 16 --end-y-mm 4 --width-mm 0.15
.\build-qt\ccad.exe pcb add-text --file .\build-qt\canvas-demo.ccad.json --id BT1 --layer F.SilkS --text "Bridge rectifier" --x-mm 8 --y-mm 22 --size-x-mm 1.5 --size-y-mm 1.5 --rotation-deg 0
.\build-qt\ccad.exe pcb add-zone --file .\build-qt\canvas-demo.ccad.json --id Z1 --name DC_NEG_FILL --net N2 --layers B.Cu --x-mm 6 --y-mm 20 --width-mm 30 --height-mm 6 --priority 0 --clearance-mm 0.20 --min-thickness-mm 0.15 --pad-connection solid
.\build-qt\ccad.exe pcb add-placement-region --file .\build-qt\canvas-demo.ccad.json --id PR1 --kind component --x-mm 11 --y-mm 4 --width-mm 12 --height-mm 8
.\build-qt\ccad.exe pcb add-keepout --file .\build-qt\canvas-demo.ccad.json --id K1 --kind placement --x-mm 20 --y-mm 10 --width-mm 4 --height-mm 3
.\build-qt\ccad.exe inspect .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe validate .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad.exe drc .\build-qt\canvas-demo.ccad.json
$env:PATH = 'C:\Qt\6.11.1\mingw_64\bin;' + $env:PATH
.\build-qt\ccad_gui.exe .\build-qt\canvas-demo.ccad.json
```

Generate demo artifacts and a GUI screenshot:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_sprint_demo.ps1 -Name sprint42-gui-canvas-mvp-review
```

What it does:

- Creates `artifacts/demos/sprint42-gui-canvas-mvp-review.ccad.json`.
- Sets a non-zero-origin board outline and board-level DRC rules, adds KiCad standard layers, toggles layer visibility, adds a full bridge rectifier PCB demo with pads, vias, tracks, a visible copper zone, board graphic lines, board text, a rectangular placement region, logical demo nets, and a rectangular keepout.
- Writes inspect, validate, and DRC JSON reports.
- Writes a sample KiCad `.kicad_mod` file and imports it to CCad footprint JSON.
- Places the imported footprint onto the demo board.
- First attempts Qt internal screenshot mode (`ccad_gui --screenshot`).
- If that path fails on the host, it launches the GUI normally, waits for window readiness, captures the exact CCad window bounds, and closes only the spawned GUI process.
- Uses a 7-second single-preview settle window by default.
- Default script behavior uses the native `ccad_gui --screenshot-measure` renderer first; exact-window capture is fallback-only when native rendering fails.

When to run:

- At sprint end.
- After GUI-visible commits.
- When handing off progress for visual review.
