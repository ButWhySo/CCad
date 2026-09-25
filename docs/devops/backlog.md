# CCad Consolidated Backlog

This file is the single local backlog for scattered CCad feature requests. Sprint files remain the execution record, while this backlog holds the larger map so future agents do not lose roadmap items.

## Verified Work Ledger

- [x] Sprint 993: explicit board-group/schematic-sheet functional-block retrieval with revision provenance; 35/35 focused contracts, Pyright and clangd source diagnostics checked, Qt MinGW Release build, full CTest 115/115, provider-disabled 10-action GUI-map validation, four inspected states, captured logs reviewed, isolated transcript DB assertion, six evidence-manifest contracts, and focused GUI-harness CTest. `artifacts/evidence/sprint993-functional-block-context-publish-ready.json` (SHA-256 `8753A1ABDA9444A08B8B1E41D5D3D34B129A107719D75E8903CB9AE552598400`). Generated logs/screenshots remain workspace-only; only manifest fingerprints enter Git. Semantic/vector retrieval remains open.
- [x] Sprint 992: retrieval includes connected/unconnected serialized schematic pins and bounded annotations, safe net membership, incremental deletion, and bitmap exclusion; 34/34 focused contracts, Pyright clean, Qt MinGW Release, 115/115 CTest, 10-action provider-free UI-map proof, four inspected screenshots, stdout/stderr reviewed. `SchPin.id` is still not serialized; broader C3 remains open.

This ledger is the current single checklist for scattered user-reported GUI, KiCad compatibility, and agent-harness work. Check an item only after code review, focused tests, KiCad/reference comparison where relevant, and visual validation when the behavior is visible.

- [x] Sprint 157: read-only native UI map export through `ccad_gui --dump-ui-map`, with target validation against live Qt hit-testing.
- [x] Sprint 983: production-shaped project retrieval indexes nested `padstack.layer_set`, typed board layer families, route-request preferred layers, bounds, serialized positions, and board-level design-rule scalars; 20/20 focused contracts, zero changed-module Pyright diagnostics, Qt MinGW Release, 113/113 CTest, and seven-action provider-disabled GUI-map proof with inspected screenshots/logs. Per-rule IDs and broader graph/diagnostic coverage remain open.
- [x] Sprint 987: schematic symbol fields and properties plus safe relative sheet names/paths are searchable in bounded project context; exact project identity reaches chat retrieval. 32/32 focused contracts, zero changed-module Pyright diagnostics, Qt MinGW Release, 113/113 CTest, and seven-action provider-disabled GUI-map proof with three inspected screenshots/logs. Kernel-level graph identities and broader schematic/PCB state coverage remain open.
- [x] Sprint 975: truthful Agent memory add/update/delete/reset UI outcomes, safe confirmed deletion across thread switches, isolated GUI-map lifecycle validation, and correction of the visible GUI-map privacy-test fixture.
- [x] Sprint 967: implementation, 98/98 CTest, clean targeted Pyright, and memory Settings GUI-map evidence verified locally; Sprint 969 adds provider-free large-context chat proof. Provider-backed validation was not needed for the local preview contract.
- [x] Sprint 968: explicit task-scoped STM, bounded session/task cache, lexical near-duplicate guard, 99/99 CTest, inspected 8-action GUI-map flow, >20-second GUI stability run, secret scan, commit, and push verified. Later Sprint 970 implements conversation-history compaction; Sprint 974 adds reviewed durable-memory compaction.
- [x] Sprint 969: local `/context [draft]` preview uses persisted provider/model identity, live bounded context, the actual GUI native tool catalog (59 schemas), memory retrieval, system instructions, and conversation accounting, without provider/tool/history side effects. Release build and 100/100 CTest, Pyright, eight mapped interactions, nine inspected screenshots, stdout/stderr review, and safe source-content exclusion verified. Staged secret scan is clean; verified implementation/docs are committed and pushed.
- [x] Sprint 971: durable LTM/episodic activation validates storage, checked preferences persist before runtime changes, failures unload caches and preserve corrupt bytes, reset remains confirmed, and mapped GUI authoring/reopen state verified. Qt MinGW Release build, 104/104 CTest, targeted Pyright, isolated orchestrator contracts, 42 inspected screenshots, and logs passed; durable semantic memory compaction remains open.
- [x] Sprint 973: official provider docs reconciled with actual adapters and presets; safe OpenRouter/Cerebras capability metadata retained; ambiguous Google `RESOURCE_EXHAUSTED` separated from confirmed quota exhaustion. Six provider contracts, changed-module Pyright, Qt MinGW Release, CTest 104/104, 18 inspected Settings screenshots, and stdout/stderr passed. No real model inference was made; live account/model capability remains an explicit opt-in check.
- [x] Sprint 979: split bounded provider model-catalog transport and human-turn handling out of the JSON-RPC orchestrator without suppressing Pyright complexity analysis; 0 changed-module Pyright diagnostics, Qt MinGW Release build, 112/112 CTest, seven-action provider-free GUI-map flow, four inspected distinct screenshots, and reviewed stdout/stderr.
- [ ] Memory follow-up: decide whether explicit capture should include project-scoped episodic storage. Ordinary chat capture remains off; `/cc` compacts conversation history only. Durable LTM/episodic compaction uses a separate explicit plan/send/review/apply flow.
- [ ] Provider follow-up: replace LangChain Google Gemini's pinned adapter retry decorator when upgrading to an API with a supported retry setting; do not patch vendored site-packages.
- [ ] Provider follow-up: add opt-in, explicitly initiated per-provider live model/tool-call verification and report verified capability separately from adapter initialization; normal startup remains no-network.
- [ ] UI follow-up: repair MCP Servers table palette so its empty state, headers, and populated rows follow the dark Settings theme; visual validation exposed a white table body in the current dark dialog.

## Demo-deadline orchestration gate

- [ ] Intake guardrails: normalize intent and reject prompt-injection/secret-bearing tool requests.
- [x] Sprint 815: Python provider boundary blocks known prompt-injection and inline-secret input, with no secret echo; broader document provenance scanning remains open.
- [x] Sprint 816: CI executes intake guard contract; feature and codebase handover expose its exact boundary.
- [x] Sprint 817: CI runs checkpoint accept, denial, and cancellation restart phases explicitly.
- [x] Sprint 818: CI checkpoint setup phases explicitly reset to `first`, preventing inherited phase-variable contamination.
- [x] Sprint 819: checkpoint CI uses supported denial phase; unknown harness phases now fail loudly.
- [x] Sprint 820: intake guard recognizes common provider key prefixes without exposing values.
- [x] Sprint 821: agent method catalog documents intake preflight and redacted response events.
- [x] Sprint 822: context state reports redacted source provenance and forbids memory content emission.
- [x] Sprint 823: context source labels omit empty request context, preventing false provenance.
- [x] Sprint 824: native CLI MCP consumes initialized notification without stdout response; framing regression covered in C++.
- [x] Sprint 825: native CLI MCP silently consumes all notification methods, not only initialized.
- [x] Sprint 826: context events expose previous hash revision for redacted delta tracking.
- [x] Sprint 827: subprocess contract proves previous context revision chaining across messages.
- [x] Sprint 828: native MCP tools/list exposes read-only/destructive/open-world hints.
- [x] Sprint 829: native MCP exposes agent method catalog as read-only tool.
- [x] Sprint 830: Python GUI MCP bridge silently consumes all lifecycle notifications, matching native server.
- [x] Sprint 831: GUI MCP rejects non-string method values with JSON-RPC invalid-params error.
- [x] Sprint 832: GUI MCP rejects non-object JSON roots without child-process crash.
- [x] Sprint 833: GUI MCP rejects non-object params roots before tool dispatch.
- [ ] Context fabric: canonical project snapshot, revision delta, pinned constraints, token budget, and compaction policy. Partial: C++ revision/pins and Python bounded compaction shipped; semantic delta/RAG remains.
- [ ] Provider runtime: real provider-neutral request/response transport with timeout, retry, and truthful connectivity state.
- [ ] LangGraph run loop: pending run state survives tool calls; broker result resumes same run by `call_id`. Sprint 811 fixes thread-scoped result routing and stale-result rejection; protocol-level live broker proof remains open.
- [x] Sprint 862: delay `tool_result_ack` until pending-call or durable checkpoint/call correlation validation; unknown, late, and mismatched results cannot emit false acknowledgement.
- [x] Sprint 863: deduplicate process-local and checkpoint pending-call IDs in recovery count metadata.
- [x] Sprint 864: scope context revision chaining by agent thread so cross-thread context deltas remain truthful.
- [x] Sprint 865: expose read-only opaque `agent.context_state` for harness inspection without context-content disclosure.
- [x] Sprint 866: prove `agent.context_state` through the real offline orchestrator subprocess, not source inspection alone.
- [x] Sprint 867: bound retained per-thread context revision metadata to prevent unbounded harness memory growth.
- [x] Sprint 868: align provider-unavailable message events with stable category/kind/redaction metadata.
- [x] Sprint 869: reset current-thread context revision when `/clear` removes chat history and context.
- [x] Sprint 870: propagate classified provider failure category into the safe user-facing message event.
- [x] Sprint 871: expose explicit approval-required metadata on mutating tool-call events, including dry-run behavior.
- [x] Sprint 872: preserve approval-required metadata inside durable checkpoint interrupts across resume.
- [x] Sprint 873: assert approval-required metadata in the real checkpoint restart fixture.
- [x] Sprint 874: fresh-DB proof covers checkpoint approval accept, denial, and cancellation branches.
- [x] Sprint 875: make optional checkpoint gate execute all decision branches with fresh temporary DBs.
- [x] Sprint 876: centralize tool approval decision so live tool-call metadata uses one deterministic policy.
- [x] Sprint 877: expose explicit approval reason in live and durable tool metadata.
- [x] Sprint 878: align tool-call discovery schema with emitted approval reason metadata.
- [x] Sprint 879: expose redacted approval status/reason in pending-call recovery snapshots.
- [x] Sprint 880: prove pending-call approval metadata through the real offline subprocess boundary.
- [x] Sprint 881: prove nonempty pending approval metadata at the durable checkpoint boundary.
- [x] Sprint 882: prove pending-call approval fields in both runtime response and method discovery.
- [x] Sprint 883: prove the complete redacted pending-call discovery schema.
- [x] Sprint 884: scope process-broker pending-call recovery by agent thread.
- [x] Sprint 885: propagate explicit human-message thread IDs through context and agent execution.
- [x] Sprint 886: advertise optional human-message thread binding in method discovery.
- [x] Sprint 887: include queried thread identity in pending-call recovery state.
- [x] Sprint 888: advertise optional pending-call query thread binding.
- [x] Sprint 889: advertise optional context-state query thread binding.
- [x] Sprint 890: publish thread set/resume controls in agent method discovery.
- [x] Sprint 891: align optional resume query schema with runtime behavior.
- [x] Sprint 892: publish the thread-resumed event response schema.
- [x] Sprint 893: publish redacted provider-secret control schema.
- [x] Sprint 894: publish transient provider-probe discovery schema.
- [x] Sprint 895: publish read-only marketplace catalog discovery schema.
- [x] Sprint 896: publish persisted configuration discovery schema.
- [x] Sprint 897: publish configuration mutation discovery schema.
- [x] Sprint 898: publish component-generator discovery schema.
- [x] Sprint 899: guard runtime agent-method discovery parity.
- [x] Sprint 900: reject secret-like fields from persisted agent configuration.
- [x] Sprint 901: recursively sanitize nested persisted config secrets.
- [x] Sprint 902: execute recursive config sanitizer behavior proof.
- [x] Sprint 812: canonicalize malformed JSON-RPC tool correlation IDs before pending-call lookup; invalid IDs now fail safely.
- [x] Sprint 813: checkpointed tool cancellation resumes matching thread with structured cancellation error; stale cancellation remains rejected.
- [x] Sprint 814: two-process checkpoint test proves cancellation result survives process restart.
- [x] Sprint 810: mock-provider headless contract proves supervisor-to-router-to-tool-node execution and terminal completion; durable broker resume remains open.
- [ ] Approval authority: one pre-side-effect policy gate; denial/error paths stop or re-plan without mutation.
- [ ] Verification: intent-conditioned DRC/ERC/build/visual checks; result becomes signoff evidence before commit/export.
- [ ] Observability: Langfuse/LangSmith redaction tests, run ID display, cost/token/latency fields only when provider reports them.
- [ ] External harness: standards-compatible MCP stdio/HTTP server and interoperability test; bespoke JSON-RPC is not MCP.
- [x] Sprint 914: stdio MCP legacy initialize negotiation accepts published versions through `2025-06-18` and rejects unknown versions; HTTP transport and external-client interoperability remain open.
- [x] Sprint 915: native subprocess MCP stdio interoperability proof covers initialize, silent initialized notification, and tools/list without network access.
- [x] Sprint 916: expose read-only harness-context and workspace-state resources through native MCP stdio, with subprocess read/list coverage.
- [x] Sprint 917: advertise the native MCP resources capability during initialize and assert discovery in the subprocess client proof.
- [x] Sprint 918: reject undeclared native MCP resource URIs and cover the boundary in the external-client proof.
- [x] Sprint 919: expose official Cerebras context-window and speed metadata in the offline model catalog with no network refresh.
- [x] Sprint 920: publish per-model catalog metadata fields in agent method discovery so harnesses need not infer the schema.
- [x] Sprint 922: expose documented Cerebras reasoning-effort choices per model in the offline catalog and discovery schema.
- [x] Sprint 921: add Cerebras' documented LangGraph integration header to provider requests and cover it without a live call.
- [x] Sprint 923: apply documented Cerebras reasoning-effort defaults and session override validation at runtime without a live request.
- [x] Sprint 924: isolate and restore the Cerebras reasoning override during transient provider probes.
- [x] Sprint 925: prove LangChain accepts Cerebras integration headers and reasoning parameters without network access.
- [ ] Product UI: usable schematic editor, complete core PCB edit loop, professional chat/activity presentation.
- [x] Sprint 158: selective `ui.target` queries by semantic ID and PCB board point, including high-DPI physical pixel fields.
- [x] Sprint 159: safe direct UI action triggers for non-destructive view/navigation actions, with unsafe actions explicitly refused.
- [x] Sprint 160: CI fix for KiCad symbol importer shadow warnings and missing local `.tmp` include, verified by focused symbol-import test.
- [x] Sprint 160: DSN export preserves large 64-bit nanometer coordinates during millimeter formatting, verified by focused DSN test.
- [x] Sprint 160: footprint placement left-click no longer reloads from an unclosed project file and no longer hangs the GUI map placement regression.
- [x] Sprint 160: app-owned placement-click smoke can place a footprint on a fresh board and returns `performed:true`.
- [x] Sprint 160: final screenshot must prove KiCad SVG icons are visible in top, left, and right toolbars instead of text-only fallbacks.
- [x] Sprint 161: add a UI-map mouse target harness that beeps, waits two seconds, moves to Select, Measure, Save, File menu, and DRC pane targets, screenshots each target, resizes the window, and repeats the same target checks.
- [x] Sprint 163: keep the UI map available while the app is open through a long-lived local socket query surface instead of one-shot process exits; WebSocket, JSON-RPC, MCP bridging, and streaming diffs remain future expansion.
- [x] Sprint 164: unfinished right-toolbar editor tools no longer behave as silent stubs; they report planned-tool status to humans and `future_tool_not_implemented` to agents.
- [x] Sprint 165: unfinished left-toolbar display and panel tools no longer behave as silent stubs; they report planned-tool status to humans and `future_tool_not_implemented` to agents.
- [x] Sprint 166: Show Layers and Show Properties perform real left-toolbar panel toggles and return `panel_toggled` to agents.
- [x] Sprint 167: native Qt Agent panel shell exists as a bottom dock tab, can refresh UI-map JSON, can trigger safe UI actions, and is addressable as `panel:agent`.
- [x] Sprint 168: remaining left-toolbar display controls now perform real grid, polar, inch-unit, crosshair, ratsnest, net-highlight, and high-contrast display actions, with UI-map checked state and target-sequence proof.
- [x] Sprint 168: selection inspector stale editor overlap during rapid multi-selection is fixed and covered by a GUI inspector regression test.
- [x] Sprint 169: visual validation harness timing is guarded by CTest so single-preview waits stay at 7 seconds and multi-target/action waits stay at 5 seconds initial plus 800 ms per target/action.
- [x] Sprint 170: first right-toolbar PCB edit batch is real for Add Via, Route Track, Add Keepout, and Delete, with safe-trigger JSON and app-owned viewport-click hooks.
- [x] Sprint 171: PCB active-layer context is real in the GUI and agent surface; `control:active_pcb_layer`, active-layer JSON, live socket methods, footprint placement, and Route Track all honor the selected copper layer.
- [x] Sprint 172: PCB active-net context is real in the GUI and agent surface; `control:active_pcb_net`, active-net JSON, live socket methods, Add Via, and Route Track all honor the selected net context.
- [x] Sprint 174: Add Zone is a real kernel-backed first-slice copper-zone tool with `BoardZone`, `pcb add-zone`, DRC/diff/KiCad export, object browser/inspector support, UI-map exposure, and app-owned viewport-click placement.
- [x] Sprint 175: connect the native Agent panel to the live UI-map agent protocol. The panel can issue `ui.find`, `ui.map_delta`, `ui.target`, `ui.target_board_point`, `ui.trigger_safe`, active layer, and active net method calls through the same `ReviewWindow` dispatcher used by the local socket.
- [x] Sprint 175: add the first low-token UI map delta and find slice. `ui.map_delta` returns no nodes when the caller already has the current epoch and returns the current map only for stale epochs; `ui.find` returns compact role/query matches without dumping the whole map.
- [x] Sprint 176: upgrade the first-slice UI-map delta so stale epochs return compact nodes instead of the full map, add `ui.map_compact`, `ui.role_summary`, `ui.hit_test`, and first linear-scan `ui.nearest_canvas_object` methods through the Agent panel and live socket.
- [x] Sprint 179: add high-level Agent PCB workflow methods over the existing GUI event path: `ui.current_tool`, `ui.cancel_tool`, `ui.place_via`, `ui.route_track`, `ui.add_zone`, `ui.add_keepout`, `ui.draw_graphic`, `ui.place_text`, and `ui.delete_object`.
- [x] Sprint 180: add Agent project evidence tools over the same live dispatcher: app-owned `ui.screenshot`, loaded-project `project.context`, compact `project.object_counts`, `project.review`, `project.erc`, `project.drc`, and combined `project.diagnostics`.
- [x] Sprint 181: upgrade Sprint 176's compact UI-map surface into first-slice dirty-node batching, bounded `ui.wait_for_delta`, and indexed `ui.nearest_canvas_object` metadata with a small uniform-grid candidate filter.
- [x] Sprint 182: add cached UI-node indexes by stable ID and role, indexed `ui.index_stats`, `ui.get_node`, `ui.nodes_by_role`, bounded dirty-event history, and `ui.watch_delta` over the shared Agent-panel/live-socket dispatcher.
- [x] Sprint 183: add `agent.methods`, `agent.method_schema`, and `agent.quickstart` so the Agent panel and live socket expose a discoverable protocol catalog with schema-shaped inputs, safety flags, dry-run support, examples, and the recommended UI-map loop.
- [x] Sprint 187: upgrade the right-side Agent dock into a compact workspace surface with live project/view/layer/net/tool context, cached diagnostic counts, result-state summaries, and one-click Context, Diagnostics, Tool Guide, and Clear actions exposed through semantic UI-map targets.
- [x] Sprint 188: add first local Agent task workspace state with `control:agent_goal`, Stage Goal, Pin Evidence, Clear Evidence, bounded local evidence, and read-only `agent.workspace_state` exposed through the shared GUI/live-socket dispatcher.
- [x] Sprint 189: add first local approval lane with `control:agent_approval_request`, Request, Accept, Decline, Cancel, Clear, approval status, and approval fields in `agent.workspace_state`.
- [x] Sprint 190: ingest `docs/req_agentHarness.md` into this backlog as a structured master map, research current agent UI, durable execution, MCP, observability, KiCad CLI, simulation, and Altium automation references, and define the next agent-harness sprint sequence.
- [x] Sprint 194: add deterministic CLI/JSON-RPC Agent policy gates with dry-run classification, approval-required metadata, method discovery, and centralized `agent serve` read/write decisions.
- [x] Sprint 195: add structured native Agent evidence cards, bind screenshot/DRC/ERC/diagnostics outputs to manifest-ready card fields, expose `evidence_cards` through `agent.workspace_state`, and update `agent.evidence_manifest_schema`.
- [x] Sprint 196: redesign the native Agent pane into a command-center workspace with visible header icon actions, model/mode/local-policy chips, Command/Evidence/Approvals selectors, Activity events, workspace-state layout metadata, and UI-map targets for the new passive regions.
- [x] Sprint 197: add no-secret headless BYOK/BYOT configuration surfaces with `agent.provider_config_schema`, `agent.provider_config_template`, and `agent.provider_status`, plus direct CLI commands, method discovery, tool-guide routing, help metadata, and presence-only env status for OpenAI, OpenAI-compatible, Anthropic, Google Gemini, and local model servers.
- [x] Sprint 198: add disabled-by-default headless observability configuration surfaces with `agent.trace_export_schema`, `agent.trace_export_template`, `agent.trace_redaction_policy`, and `agent.trace_export_dry_run`, plus direct CLI commands, method discovery, tool-guide routing, help metadata, no-network dry-run behavior, and redacted OTLP header status.
- [x] Sprint 201: add the fourth native Agent pane visual contract with `visual_style:"agent_reference_panel_v4"`, `workspace_layout_version:4`, compact status rail, command composer, plan deck, evidence lane, approval lane, preserved semantic IDs, visual proof, target proof, and full CTest gate.
- [x] Sprint 202: bind the native Agent pane to local `.ccad-agent-session.json` metadata with targetable session path, load, checkpoint, and status controls; expose durable session fields through `agent.workspace_state`; append metadata-only GUI checkpoints; prove visual and target behavior before commit.
- [x] Sprint 203: bind the native Agent pane to the shared command policy classifier with targetable policy/risk/dry-run controls, `agent.workspace_state` policy fields, approval-lane population for project mutations, GUI UI-map checkbox target support, visual proof, target proof, and full CTest gate.
- [x] Sprint 204: bind the native Agent pane to local trace-link metadata with targetable trace ID, span ID, status, export-disabled labels, semantic new-trace action, `agent.workspace_state` trace fields, UI target scroll-into-view behavior, sampled canvas-object target validation, focused GUI tests, visual proof, and target proof.
- [x] Sprint 205: bind the native Agent pane to no-secret provider readiness metadata with targetable provider family, model hint, refresh action, env-presence labels, `agent.workspace_state` provider fields, GUI UI-map combo/text support, visual proof, target proof, and full CTest gate.
- [x] Sprint 206: add the first native Agent pane run-queue foundation with a first-viewport local queue panel, targetable status/count/current-step labels, cancel and clear actions, `agent.workspace_state` queue metadata, compact queue event JSON, focused GUI tests, visual proof, and target proof while keeping provider execution, worker threads, trace export, external processes, and project mutation disabled.
- [x] Sprint 226: fix the reported silent GUI crash by making `ReviewWindow` cursor and selection callbacks tolerate empty and board-only projects, add a 7-second `gui_ui_map` survival regression, rebuild `ccad_gui`, run the official beep-and-screenshot harness, ingest the screenshot, and directly prove no-argument, empty-project, board-only, and sprint-demo GUI launch modes survive.
- [x] Sprint 226: map KiCad `collectors.cpp/.h` locked-item suppression into CCad by adding optional locked state to board physical items and placed footprints, exposing collector row `locked` metadata, making `pcb collect-items --ignore-locked true` filter locked candidates, and hardening `writeProjectFile()` so serialization completes before opening/truncating the destination project file.
- [x] Sprint 226: map KiCad `convert_shape_list_to_polygon.cpp/.h` into a first Edge.Cuts outline-polygon report with KiCad provenance, segment-chain points, closed/valid state, rectangular inference fallback, diagnostics, and explicit pending shape-feature metadata exposed through `pcb outline-polygon`.
- [x] Sprint 226: map KiCad `cross-probing.cpp` into a first headless cross-probe packet resolver with KiCad provenance, `$NET`, `$NETS`, `$PART`, `$PAD`, `$SELECT`, and `$CLEAR` packet handling, board/schematic target rows, focus metadata, diagnostics, and explicit pending live GUI/Kiway parity gaps exposed through `pcb cross-probe`.
- [x] Sprint 220: add first KiCad PCB API parity query slice from `F:\kicad_src\pcbnew\api`, including headless layer, stackup, rules, outline, same-net, connected-item, and Agent schema surfaces; completed after full sprint gate passed 37 of 37 tests.
- [ ] Upgrade the Sprint 182 UI-map surface into true push-style live coordinate streaming and larger-board spatial indexes such as an R-tree.
- [x] Sprint 184: fix schematic symbol placement persistence so saved symbols reload with real KiCad primitive graphics and visible pins/leads, not placeholders.
- [x] Sprint 232 continuation: restore schematic symbol snapshot persistence after the `SchSymbol` model rename, preserve legacy project JSON compatibility, expand embedded symbol graphics into schematic canvas primitives, pass the full 56-test CTest gate, and visually prove the Schematic tab with `artifacts\screenshots\sprint232-schematic-symbol-snapshot.png`.
- [x] Add footprint placement layer selection in PCB mode so the user and agent can choose F.Cu, B.Cu, or another valid copper layer before committing. Sprint 171 covers footprint placement and Route Track layer context; Sprint 172 covers active-net context for new vias and tracks.
- [ ] Fix pad, paste, mask, and courtyard rendering glitches on hover and ensure zoom does not mask stale render state. Sprint 162 separates copper, mask, and paste layer rendering; courtyard and hover-specific stale render checks remain open.
- [ ] Expand footprint rendering beyond the current supported subset until KiCad library-cache footprints no longer collapse into fixed rectangles.
- [ ] Implement the real kernel-backed behavior behind every left/right tool rail icon; Sprint 170 covers Add Via, Route Track, Add Keepout, and Delete, Sprint 171 covers active PCB layer selection for placement/routing, Sprint 172 covers active PCB net selection for new copper, Sprint 173 covers Draw Graphic and Place Text, and Sprint 174 covers first-slice Add Zone, while remaining schematic/PCB editor actions and richer zone editing remain open.
- [ ] Move detailed selected-object properties into a properties dialog or KiCad-like properties surface when opened, rather than relying only on always-visible right-panel rows.
- [ ] Improve board outline, track, pad, keepout, and placement-region resize/edit workflows for both humans and agents.
- [ ] Add explicit LLM/native command docs next to each GUI feature, including preferred kernel command, UI-map fallback, verification command, and failure recovery loop.
- [ ] Continue full KiCad library import coverage: symbol libraries, footprint libraries, aliases, metadata, filters, 3D models, unsupported-construct diagnostics, and provenance.
- [x] Sprint 185: raw KiCad `.kicad_sym` files expand into one chooser row per top-level symbol, with nested unit symbols kept inside their parent and `extends` metadata carried in the selection.
- [x] Ingest `docs/req_agentHarness.md` into the todo/work ledger and backlog. Sprint 190 covers the first full structured ingestion; future sprints should consume the new master map instead of re-reading scattered raw notes.

## Orchestrator and Copilot UI Mandate (Iteration 467)

The python backend orchestrator (`src/ccad_agent/orchestrator.py`) and CCad Agent C++ stubs must be fully implemented, removing all placeholders. This includes:
- **Copilot UI Parity**: Ensuring the Agent UI matches the exact specifications of the hand-drawn wireframes and Copilot UI style references.
- **Marketplace & Personalizations**: Fully implement the marketplace UI and personalization hooks.
- **Hooks & Workflows**: Proper integration of hooks, workflows, and custom prompts at the correct orchestration phases.
- **Slash Commands**: Implement full backend routing and execution for `/` commands in the chat interface.
- **Observability & Providers**: Complete OpenTelemetry (OTel) integration with real span emission and export links, and fully wire up various model providers behind the no-secret approval gates.

## Deterministic KiCad Parity Sprint Plan

The raw parity directive is now tracked as bounded sprint work instead of a single unbounded paragraph. Every item below must start from current repo state, then read `F:\kicad_src` and current external references before code edits. Each parity sprint must record the KiCad file or folder read, the closest CCad analogue, the missing behavior, the tests or visual proof added, the documentation updated, and any intentional difference from KiCad.

- [x] Sprint 223 cleanup: after verifying merged branch state, list local and remote sprint branches, delete only branches fully merged into `main`, and never rewrite history or delete unmerged/user work.
- [x] Sprint 225 backlog and parity control plane: normalize `docs/devops/backlog.md`, `docs/devops/progress.md`, and the active sprint file so the KiCad parity mandate is sprint-bounded, removes duplicated raw text, and records the source-walk ledger format.
- [x] Sprint 225 repo-state hygiene: record current branch, dirty files, untracked helper files, and ownership assumptions before touching behavior code.
- [x] Sprint 225 parity ledger schema: define fields for KiCad source path, CCad analogue, external references, behavior decision, tests, GUI visual proof, documentation links, unsupported diagnostics, and status.
- [x] Sprint 225 phase budget: define the next bounded parity phase budget so PCB editor, schematic editor, external formats, Gerber, 3D, project model, library verification, GUI map, agent harness, and autorouter work cannot drift indefinitely.
- [x] Sprint 226 PCB editor root model slice: walk `F:\kicad_src\pcbnew` root files in stable order, starting with board/model/settings/commit/connectivity files, and map them to CCad board state, transactions, design settings, object metadata, loader readiness, stackup, statistics, board-item container mutation, text variables, legacy board BOM output, cleanup action discovery, collector behavior, outline polygon conversion, cross-probing, and DRC visibility. First tested sub-slice maps `board_design_settings.cpp` into shared CCad design-rule validation used by DRC. Second tested sub-slice maps `board_item.cpp` and `include\board_item.h` into shared KiCad `BOARD_ITEM` metadata for pads, vias, tracks, graphics, texts, and zones returned by PCB object queries. Third tested sub-slice maps `board_loader.cpp`, `board_loader.h`, and KiCad board-loader tests into `ccad_core/board_loader.hpp/.cpp` plus `pcb load-state` for agent-visible board load readiness. Fourth tested sub-slice maps `board_stackup_manager\board_stackup.cpp`, `board_stackup.h`, dielectric material sources, and stackup reporter sources into `ccad_core/board_stackup.hpp/.cpp` plus a physical `pcb get-board-stackup` report with stackup items and copper layer distances. Fifth tested sub-slice maps `board_statistics.cpp`, `board_statistics.h`, `board_statistics_report.cpp`, and KiCad board-statistics tests into `ccad_core/board_statistics.hpp/.cpp` plus `pcb drill-statistics` for pad/via drill row aggregation. Sixth tested sub-slice maps `board_item_container.h` into `ccad_core/board_item_container.hpp/.cpp` plus KiCad-shaped `pcb remove-object` delete/remove metadata and `--mode normal|bulk`. Seventh tested sub-slice maps `board_statistics_report.cpp/.h` into `BoardStatisticsReport` plus `pcb board-statistics` for agent-visible board dimensions, counts, minimums, board thickness, and drill rows. Eighth tested sub-slice maps `board_text_var_adapter.cpp/.h` and KiCad text-variable API handlers into `ccad_core/board_text_var_adapter.hpp/.cpp`, project `text_variables`, and `project set-text-variable`, `project list-text-variables`, and `pcb expand-text-variables`. Ninth tested sub-slice maps `build_BOM_from_board.cpp` into placed `BoardFootprint` metadata, KiCad footprint `exclude_from_bom` preservation, and `pcb export-board-bom` for KiCad legacy board-side BOM CSV export. Tenth tested sub-slice maps `cleanup_item.cpp/.h` into `ccad_core/cleanup_item.hpp/.cpp` plus `pcb cleanup-actions` for KiCad `CLEANUP_ITEM` action catalog and `VECTOR_CLEANUP_ITEMS_PROVIDER` row semantics. Eleventh tested sub-slice maps `collectors.cpp/.h` locked-item suppression into persisted board-object lock flags, collector row metadata, and `pcb collect-items --ignore-locked true`, with CLI project-write truncation hardening fixed in the same sprint after the serializer failure reproduced. Twelfth tested source-walk sub-slice maps `convert_shape_list_to_polygon.cpp/.h` into `ccad_core/board_outline_polygon.hpp/.cpp` plus `pcb outline-polygon` for first Edge.Cuts line-chain reporting and rectangular inference fallback. Thirteenth tested source-walk sub-slice maps `cross-probing.cpp` into `ccad_core/cross_probing.hpp/.cpp` plus `pcb cross-probe` for first KiCad-style packet resolution across board and schematic targets. Fourteenth tested source-walk sub-slice maps the remaining PCB layout objects (pcb_barcode.cpp, pcb_dimension.cpp, pcb_group.cpp, pcb_reference_image.cpp, pcb_table.cpp, pcb_target.cpp, pcb_text.cpp, pcb_textbox.cpp) into ccad_core/model.hpp structs BoardBarcode, BoardDimension, BoardGroup, BoardReferenceImage, BoardTable, BoardTarget, and BoardText with full JSON serialization and rendering. Fifteenth source-walk sub-slice audited pcb_design_block_utils.cpp and pcb_field.cpp, documenting them as UI abstraction and deferred custom footprint fields respectively. Sixteenth and final source-walk sub-slice audited pcbexpr_evaluator.cpp, pcbexpr_functions.cpp, tracks_cleaner.cpp, zone.cpp, zone_filler.cpp, zone_settings.cpp, zone_utils.cpp, documenting that their complex evaluation algorithms and UI behaviors are deferred since their core data structures (text variables, cleanup items, basic BoardZone) were successfully mapped in prior phases.
- [x] Sprint 227 PCB item geometry slice: map KiCad pads, padstacks, vias, tracks, arcs, graphics, text, zones, keepouts, drills, annular rings, mask, paste, courtyard, fab, and silk behavior to CCad model, render, import, export, and query surfaces.
- [x] Sprint 228 PCB editor interaction slice: implement KiCad-like select, measure, route, add via, add zone, draw graphic, place text, delete, move, resize, edit properties, context menu, tooltip, Escape-cancel, and placement-ghost behavior through human GUI and agent APIs.
- [x] Sprint 229 PCB Appearance/Layers/Objects slice: bring layer colors, active layer, visible layer, selected object, selection filters, net filters, right dock, bottom diagnostics, and toolbar/icon behavior closer to KiCad.
- [x] Sprint 230 PCB footprint/library placement slice: make Add Footprint use cache-backed catalogue loading, lazy selected-footprint parse, KiCad-like chooser layout, footprint preview, layer-aware ghost placement, Escape cancellation, metadata, and future 3D model hooks.
- [x] Sprint 231 schematic document and symbol model slice: walk `F:\kicad_src\eeschema` root files in stable order and map symbols, units, pins, pin leads, fields, labels, power symbols, wires, sheets, no-connects, and no-pins diagnostics into CCad.
- [x] Sprint 232 schematic interactions/dialogs slice: implement KiCad-like Add Symbol, power symbol chooser, annotation, ERC, property dialogs, context menus, placement ghost, Escape cancellation, and symbol preview behavior through GUI and agent APIs.
- [x] Sprint 233 schematic-PCB association slice: implement symbol-footprint links, netlist sync, cross-probing, update-PCB-from-schematic behavior, and agent-visible mismatch diagnostics.
- [x] Sprint 234 external EDA provider audit: walk KiCad import/export/provider folders and map each provider or file-format capability to a CCad provider registry, including unsupported fields and round-trip fixture needs.
- [x] Sprint 235 external provider implementation batch: implement the first bounded provider batch with parse/export tests, provenance, compatibility diagnostics, and CLI/agent commands.
- [x] Sprint 236 manufacturing format expansion: plan and implement Gerber/job, drill, IPC-2581, ODB++, BOM, PnP, assembly drawing, stackup, and approval-gate improvements in tested batches.
- [x] Sprint 237 Gerber viewer parity: walk `F:\kicad_src\gerbview` file by file and map layers, apertures, drill overlays, measurement, selection, visibility, rendering, CLI evidence, and visual validation into CCad.
- [x] Sprint 238 3D viewer parity: walk `F:\kicad_src\3d-viewer` file by file and map board stackup, footprints, 3D model paths, transforms, materials, cameras, screenshots, and agent evidence into CCad.
- [x] Sprint 239 multi-document project model: support one project containing many schematics and many PCBs, with explicit links, unlinked states, project tree UI, CLI/API queries, and transaction metadata.
- [x] Sprint 240 linked/unlinked DRC/ERC intelligence: run physical-only DRC for unlinked boards, schematic-only ERC for unlinked schematics, and cross-document diagnostics only when project links exist.
- [x] Sprint 241 project-level transactions and audit: extend diffs, audit logs, rollback, agent context, and evidence manifests across multiple boards and schematics.
- [x] Sprint 242 library-cache inventory harness: enumerate KiCad symbol, footprint, and 3D model source files and compare them against CCad cache entries with pass/fail/unsupported status.
- [x] Sprint 243 symbol losslessness harness: verify aliases, inheritance, units, pins, pin electrical types, fields, graphics, footprints, keywords, descriptions, provenance, and LLM-native metadata for every symbol batch.
- [x] Sprint 244 footprint losslessness harness: verify pads, drills, annular rings, pad shapes, copper layer sets, mask, paste, courtyard, fab, silk, properties, 3D references, and provenance for every footprint batch.
- [x] Sprint 245 KiCad Footprint Oval Drill Support: Support KiCad oval drills in the footprint importer, JSON serialization/deserialization, and footprint losslessness verification. This resolves the dominant footprint import failure.
- [x] Sprint 246 low-latency live Qt map: replace slow polling paths with push-style map deltas, local socket or WebSocket subscription, semantic ID caches, stale-epoch detection, and nonblocking UI-thread ownership.
- [x] Sprint 247 canvas spatial indexes: add fast nearest-object and hit-test structures for large boards, including board-space to screen-space transforms under pan, zoom, resize, and device-pixel-ratio changes.
- [x] Sprint 248 deterministic GUI action tools: implement `ui.double_click`, richer keyboard input, drag, scroll, wait-for-dialog, dialog automation, menu/context-menu automation, properties editing, and target validation.
- [x] Sprint 249 prompt and tool-guide assets: write feature-scoped prompt, tool, command, verification, retry, and failure-classification guides for the agent layer as each parity feature lands.
- [x] Sprint 250 durable agent runner: persist queues, own worker threads outside the Qt UI thread, stream tool-loop progress, enforce approvals, support retries, and write rollback or compensation records.
- [x] Sprint 251 observability and BYOT: add real OpenTelemetry span emission, Langfuse/OTLP export links, redaction, opt-in controls, trace IDs, cost/token metadata, and no-secret configuration handling.
- [x] Sprint 252 classic autorouter baseline: establish deterministic no-agent autorouter quality metrics, route fixtures, DRC proof, clearance/manufacturing checks, and regression thresholds.
- [x] Sprint 253 agent-guided autorouter planning: let the agent diagnose constraints, prioritize nets, suggest route plans, propose DRC repairs, and produce evidence without directly degrading board state.
- [x] Sprint 254 supervised autorouter loop: add approval gates, route-quality regression checks, rollback, evidence artifacts, and acceptance criteria around agent-assisted autorouting.

Every GUI-visible parity sprint must use `.agents/workflows/visual-validation.md`, including the docs beep, handoff delay, screenshot capture, stdout/stderr interception, image ingestion, useful engineering demo board, and UI-map target proof. Documentation updates must land in the same sprint as behavior changes.

## KiCad File-by-File Parity Mandate

The active KiCad parity workflow is now a deterministic source walk, not an ad hoc feature queue. For PCB editor parity, start in `F:\kicad_src\pcbnew`, take one folder at a time in stable filesystem order, read one source file in full, identify the closest CCad analogue, and either document that the analogue is complete or implement the missing behavior in CCad. Each slice must record the local KiCad files read, the official or industry references checked, the CCad files inspected, the compatibility decision, the tests added, and any deliberate difference from KiCad.

The first execution lane is the PCB editor. The next lanes are schematic editor parity, external EDA format import/export provider support, Gerber viewer parity, 3D viewer parity, and then the multi-document project model where one project can contain many schematics and many PCBs that may or may not be linked. DRC must understand that project relationship and report only relevant diagnostics when a board has no linked schematic, while still escalating cross-document mismatches when links exist.

GUI parity means menu behavior, toolbar/ribbon actions, tabs, dialogs, context menus, tooltips, cursor-following ghosts, placement cancellation, selection, properties editing, and visual themes should converge toward KiCad behavior unless CCad intentionally adds an LLM-native layer. That LLM-native layer must sit beside the human workflow through typed kernel transactions, CLI commands, JSON/agent methods, prompt/tool guides, and visual or structural verification artifacts.

The library-cache audit must become lossless and evidence-based. For KiCad symbols, footprints, and 3D model references, compare source files against imported CCad objects and prove that pins, electrical types, fields, aliases, inheritance, pad shapes, drills, annular rings, masks, paste, courtyard, fab, silk, layers, properties, and provenance are preserved or reported as structured unsupported diagnostics. This audit is intentionally time-consuming and should be split into repeatable comparison harnesses rather than manual spot checks only.

The live Qt interaction map remains a performance backlog item. Scripted mouse and keyboard calls are fast, but live agent interaction must also become low-latency through push-style map deltas, cached semantic IDs, spatial indexes for canvas objects, direct coordinate targeting, and nonblocking UI-thread ownership. The agent does not require the GUI for every operation, so kernel and CLI paths remain primary, but GUI control must be fast when visual workflows are needed.

The routing backlog includes an agent-supervised autorouter lane. CCad should preserve classic deterministic routing quality while adding agent guidance for setup, constraint diagnosis, net prioritization, route review, DRC repair suggestions, and evidence-based acceptance. Agent intervention must not degrade route correctness, clearance, manufacturability, or repeatability.

## KiCad-Compatible GUI Parity

The schematic editor needs KiCad-style Add Symbol behavior: load libraries from `library-cache`, search/filter by symbol name and description, preview the symbol and associated footprint, enter a cursor-following placement ghost, place on left click, and cancel with Escape. The PCB editor needs the matching Place Footprint behavior with a cache-backed chooser, footprint preview, cursor ghost, layer-aware placement, left-click commit, and Escape cancellation.

The main GUI should not expose raw "preview KiCad file" workflows as normal editor actions. Raw import previews can remain internal test or debug helpers, but user-facing authoring should use project/library-backed tools.

The right, left, top, and bottom toolbars should continue moving toward KiCad icon names, icon-only buttons where KiCad uses icons, and text only where KiCad uses text. Toolbar actions that are still placeholders must be wired to kernel/API behavior or moved into a clearly marked future-tool state.

The Add Symbol and Add Footprint dialogs should keep converging toward KiCad chooser layout: filter field, library/category column, item table, description/details region, symbol or footprint preview, footprint filter/selector for symbols, repeated placement options, and remembered selection state.

## Canvas Fidelity

Pads must render by actual KiCad pad shape, size, drill, annular ring, rotation, and layer set. Through-hole pads need visible copper annular rings and drill openings. Tracks must render by actual width and selected tracks must highlight the full visible copper stroke, not just a centerline.

Layer colors should follow KiCad themes closely enough for immediate recognition. `F.Cu` and `B.Cu` must be distinct, inner layers need separate colors, non-copper technical layers need their own visual treatment, and layer visibility must remain live through the Appearance/Layers dock.

Sprint 184 stores a placed symbol snapshot on each schematic component and renders reloaded symbol body primitives plus pin leads in the schematic scene. Remaining schematic work is full KiCad symbol-editor parity, wiring, labels, annotation, property editing, and symbol-footprint assignment.

## Library and Catalog Import

The local `library-cache` should become a full KiCad-compatible catalog cache, not a manually browsed file store. It should index symbol libraries, footprint libraries, aliases, descriptions, keywords, footprint filters, metadata fields, pin counts, pad counts, and provenance.

CCad needs import coverage for the whole KiCad schematic symbol and footprint library surface that matters for authoring. Unsupported constructs should be recorded as structured diagnostics, not silently dropped.

3D model metadata should be preserved in footprint imports and surfaced in the catalog even before 3D preview rendering exists.

Component creators should produce both human-facing KiCad-compatible fields and LLM-native fields such as intended function, electrical role, footprint constraints, simulation model links, manufacturability notes, and preferred routing semantics.

## LLM-Native API and Harness

`docs/req_agentHarness.md` is now treated as the broad requirements inventory for a native CCad agent harness. The file covers more than a chat pane; it describes an agent runtime, prompt/context/memory layer, durable execution engine, tool system, observability plane, EDA-specific automation suite, GUI-control harness, evaluation harness, and safety model. Future work should consume the structured backlog below before starting another agent sprint.

Every human GUI action that mutates a design needs a corresponding kernel transaction and command/API path so agents can perform the same work deterministically. Agent calls should expose small, typed operations rather than requiring raw JSON edits.

The agent harness needs high-loop failure handling with structured retries, audit trails, screenshots, DRC/ ERC evidence, and explicit rollback or correction transactions. Future work should evaluate LangGraph-style orchestration, OpenTelemetry spans, and Langfuse-style trace inspection before implementation.

Prompts and tool docs should be authored alongside each feature so an LLM can learn how to use the new capability, what inputs are safe, how to verify output, and which diagnostics mean retry versus stop.

### Current Agent UI Gap

The sample UIs and the current VS Code Chat view reference show the expected shape: the agent belongs in a full-height side workspace beside the editor, with session controls at the top, a central conversation or activity stream, and input plus model/permission controls at the bottom. Sprint 191 moves CCad from the Sprint 186 through 189 functional skeleton into the first real vertical workspace shell. It now has a full-height Agent column beside Layers / Objects, session header, workspace context, goal/task area, visible approval card with accept/decline decisions, fixed command input, and visible Request Context plus Trigger DRC actions. Sprint 192 adds the first headless CLI workspace-state parity slice with deterministic provider-free state, task, evidence, and approval commands. Sprint 193 adds provider-free local session files with ordered checkpoints and replay manifests. Sprint 194 adds the first headless command policy gate so risky tool calls can be classified before execution. Sprint 195 adds the first visible structured evidence-card surface and manifest-ready `agent.workspace_state` fields. Sprint 196 adds the command-center visual pass with visible header icon actions, model/mode/local-policy chips, tab selectors, and first-viewport Activity events. Sprint 197 adds no-secret headless BYOK/BYOT configuration discovery and env-presence checks. Sprint 198 adds disabled-by-default headless OpenTelemetry/Langfuse trace configuration, redaction policy, and dry-run readiness. Sprint 199 adds the next vertical polish slice: local run controls, trace/session chips, and first-viewport active-plan rows, all exposed through `agent.workspace_state` and semantic UI-map targets. Sprint 201 adds a fourth visual contract with targetable status rail, command composer, plan deck, evidence lane, and approval lane while preserving previous IDs. Sprint 202 binds the Agent pane to local durable session metadata and GUI checkpoint records. Sprint 203 binds the visible command strip to the same command policy classifier used by CLI and JSON-RPC, including dry-run and approval-required state. Sprint 204 binds the visible trace-link widgets to local trace ID, span ID, export-disabled status, and session/thread correlation metadata. Sprint 205 binds visible provider family, model-hint, env-presence, and execution-disabled controls to the no-secret provider metadata contract. Sprint 206 adds the first visible local run-queue foundation with targetable cancel, clear, status, count, and current-step controls. The Agent pane still lacks provider-backed execution behind approvals, actual worker-thread runner ownership, actual span emission, actual observability export wiring, and a streamed tool-loop progress surface.

The next UI target is no longer the dock topology, first-visible run controls, first policy preview, first local trace metadata, provider-readiness controls, or the first queue-state panel; those are now in place. The remaining UI target is to bind the visible controls to real durable execution state: persisted queue files, live tool-loop progress, provider-backed execution behind approvals, worker-thread ownership, real span emission, and trace backend links should be added while preserving `panel:agent`, `tab:agent`, and the existing command/task/evidence/approval/run/queue/trace/provider/plan/policy IDs.

### Agent Workspace UI and CLI Roadmap

CCad should have one first-class agent workspace exposed through both the native Qt GUI and the headless CLI. The GUI surface should present the live session, current project context, active tasks, pinned evidence, approvals, traces, diagnostics, and command input. The CLI surface should expose the same state through deterministic commands so batch jobs, CI, and external orchestrators can run without Qt. The minimum headless command family should become `ccad agent run`, `ccad agent state`, `ccad agent tasks`, `ccad agent evidence`, `ccad agent approvals`, `ccad agent traces`, `ccad agent replay`, and `ccad agent cancel`, with JSON output and stable exit codes.

Sprint 186 moved the first Agent panel shell into a right-side dock and kept `panel:agent` plus `tab:agent` as semantic automation handles. Sprint 187 added the first compact workspace strip, preset actions, result-state summary, and targetable Agent workspace controls. Sprint 188 added the first staged task goal, bounded local evidence queue, and read-only `agent.workspace_state` snapshot. Sprint 189 added the first local approval lane and explicit accept/decline/cancel decisions. Sprint 191 converted the stacked dock into a full-height vertical workspace column, added command staging, visible footer actions, and UI-map geometry tests for the correct dock layout. Sprint 192 exposed matching headless state, task, evidence, and approval JSON through direct CLI commands and JSON-RPC routes. Sprint 193 added local session/checkpoint files and replay manifests. Sprint 194 adds deterministic policy classification, dry-run decisions, and approval-required metadata for command execution. Sprint 195 adds structured GUI evidence cards and schema fields for screenshot, DRC, ERC, diagnostics, and trace-ready references. Sprint 196 adds command-center visual density, Sprint 197 adds provider configuration metadata, Sprint 198 adds trace configuration metadata, Sprint 199 adds local run controls plus trace/session/active-plan UI-map state, Sprint 201 adds the fourth targetable visual contract, Sprint 202 binds the GUI to durable local session metadata, Sprint 203 binds GUI command preview to the shared policy classifier, Sprint 204 binds GUI trace-link widgets to local trace/span metadata, Sprint 205 binds GUI provider-readiness controls to the no-secret provider metadata contract, and Sprint 206 adds first-slice local run-queue metadata and targetable queue controls. The remaining work is CLI run execution, provider-backed execution, real span emission, richer trace surfaces, and observability export.

### Prompt, Context, Memory, and Runbooks

The harness needs explicit prompt assets rather than hidden string constants. Backlog items include system/developer/user prompt layering, prompt templates, prompt libraries, prompt versioning, dynamic prompt injection from project state, feature-scoped tool guides, and prompt regression tests. Context handling needs pinned evidence, compaction, pruning, summaries, layered context windows, file and object citations, commit-hash or transaction-hash anchoring, and failure memory. Runbooks should be generated from successful workflows so later agents can repeat bridge-rectifier placement, DRC repair, route review, footprint verification, symbol placement, BOM review, and fabrication export without rediscovering the sequence.

### Durable Orchestration and State

The orchestration layer should be durable and resumable. LangGraph-style persistence, thread IDs, checkpoints, human-in-the-loop interrupts, streaming updates, and replay match long PCB loops better than a stateless request/response chat. Tool calls with side effects, such as file writes, GUI input, library imports, DRC runs, routing attempts, provider calls, and fabrication exports, should be wrapped as durable tasks with idempotency keys so a resumed run does not repeat destructive actions. The CCad agent state should include the active project path, project revision, transaction ID, board/schematic view, selected object IDs, pending diagnostics, current plan, task budget, provider/model config, approval state, evidence artifacts, and last verified visual artifact.

Sprint 193 creates the first local JSON session file contract with `session_id`, `thread_id`, local resource URIs, ordered checkpoint records, and replay manifests. Sprint 202 binds that local session metadata into the native Agent pane and can append metadata-only GUI checkpoints. This is still not a LangGraph runtime or provider runner yet; it is the durable file substrate that later policy, runner, provider, and observability work can share.

The durable runner still needs queues, timeouts, cancellation, retries, exponential backoff, circuit breakers, stop conditions, token and cost budgets, transaction checkpoints, rollback or compensation actions, dead-letter handling, and partial completion reporting. Long model calls and tool loops must run outside the Qt UI thread and stream incremental status back to the Agent pane.

### Human Approval, Policy, and Safety

Human-in-the-loop approval must be modeled as an interruptible workflow state, not just a final Accept button. Approval gates are required before deleting files, overwriting project data, exporting private board data to remote providers, broad filesystem or network access, fabrication outputs, irreversible layout edits, library-cache mutation, and automated ordering or procurement. The user should be able to accept, decline, edit, cancel, or request more evidence while the run state remains resumable.

The safety model must include provider data policies, local-only mode, read-only mode, dry-run mode, write-limited mode, least-privilege tool permissions, secrets redaction, prompt-injection checks for external documents, MCP/tool trust boundaries, per-run audit logs, RBAC-ready policy hooks, retention controls, and kill switches. API keys belong in OS credential storage or environment variables, never project files.

### Tool System, MCP, Connectors, and Native Actions

The native tool surface should expose deterministic GUI tools next to kernel tools. Current methods cover `ui.map`, compact deltas, target lookup, screenshot, project context, ERC/DRC, and several PCB workflow actions. The backlog still includes `ui.double_click`, richer `ui.key`, `ui.drag`, `ui.scroll`, `ui.wait_for_dialog`, selection editing, properties dialog tools, transaction apply/rollback, dialog automation, KiCad-compatible library chooser tooling, and schematic authoring tools.

MCP support should mature beyond first-slice `tools/list` and `tools/call`. The harness should expose MCP tools, resources, prompts, sampling boundaries, tool schemas, deferred tool loading, tool search, connector manifests, permission prompts, tool-result normalization, tool-result summarization, replay, mocks, hooks, callbacks, and standard resource URIs for projects, boards, schematics, library items, screenshots, DRC reports, ERC reports, traces, and evidence manifests. External connectors such as browser, curl, document/PDF readers, vendor APIs, and future procurement services must go through the same permission and audit surface.

### Observability, BYOT, and Run History

Observability should be built in from the start rather than retrofitted. The agent harness should emit OpenTelemetry spans using current GenAI semantic conventions where possible, then export to Langfuse or another OTEL backend. The user-requested BYOT model is now a backlog requirement: CCad should let a user or organization configure their own OTLP endpoint, Langfuse endpoint, or local collector instead of forcing CCad-owned telemetry. Sprint 198 implements the first headless configuration and redaction contract with local-off defaults, no-secret templates, and no-network dry-run readiness. Remaining observability work must add per-run opt-in, endpoint validation, live trace IDs, span emission, GUI trace links, and actual exporter integration without storing secrets in project files.

Each run should have spans for prompt assembly, model calls, tool calls, GUI map queries, UI clicks, screenshot captures, DRC/ERC checks, library searches, KiCad CLI invocations, ngspice runs, file writes, retries, interrupts, approvals, rollbacks, and final verification. Token usage, cost, provider, model, prompt version, project ID, sprint ID, transaction IDs, artifact paths, and tool risk should be structured metadata. The GUI should show trace status and links in the Agent pane; the CLI should export trace IDs and artifact manifests.

### GUI Map, Coordinate Streaming, and Agent-Control Geometry

The GUI-control layer should not depend primarily on screenshots. Because CCad owns the Qt widgets and canvas renderer, the app can expose a native UI/object map that describes visible menus, toolbar actions, docks, tabs, dialogs, canvas objects, selectable primitives, layer rows, diagnostic rows, property rows, transient placement ghosts, and approval controls. Each entry should include a stable semantic ID, role/type, label or icon action name, enabled/visible state, owner widget, local rectangle, scene rectangle where relevant, z-order, transform, device-pixel ratio, and global screen rectangle computed through Qt coordinate mapping.

Coordinate targeting should behave like an HTML image map but tied to Qt and CAD geometry instead of static pixels. A target query should return local widget coordinates, global screen coordinates, confidence, reason fields, and stale-epoch detection. `ui.target({"id":"action:add_footprint"})` should return the button center, while board-space targeting should return the current screen coordinate after board-origin, zoom, pan, and device-pixel-ratio transforms. If a requested target is hidden, disabled, occluded, outside the viewport, or stale because `ui_epoch` changed, the tool should return a structured failure instead of guessing.

The map should be dynamic and cheap. Every GUI render, layout change, tab switch, dialog open, selection change, canvas pan/zoom, and layer visibility change should increment `ui_epoch`. The agent should cache the last map and request only diffs. Sprint 181 added dirty IDs, changed roles, bounded `ui.wait_for_delta`, and a uniform-grid nearest-object candidate filter. Sprint 182 added dirty-event history and cached semantic node indexes by stable ID and role. CCad still needs true push-style coordinate streaming, a persistent subscription surface such as WebSocket or local socket notifications, and stronger spatial indexes such as R-tree or equivalent for large boards. The implementation must lazily materialize JSON, intern repeated strings or use stable integer IDs internally, and avoid UI-thread stalls on low-end machines.

### EDA-Specific Harness and KiCad/Altium Parity

The EDA harness must treat KiCad compatibility as a product requirement and use `F:\kicad_src` plus official KiCad docs before implementing KiCad-like behavior. Required backlog tracks include KiCad CLI runners, KiCad file-format import/export verification, KiCad symbol and footprint library ingestion, KiCad-like dialog and toolbar parity, ERC and DRC report parsing, schematic-to-PCB parity checks, ngspice simulation setup, simulation source and probe authoring, PCB calculator equivalents, design-rule calculators, and manufacturing outputs.

Altium remains a reference for professional workflow coverage, scripting/API automation, rule-driven online DRC behavior, and industry UI expectations. CCad should not copy proprietary code, but it should study Altium's scripting model, DRC rule organization, documentation structure, and automation boundaries when designing CCad's own safe tool API.

### Library, Datasheet, BOM, Simulation, and Manufacturing Agents

Specialized EDA subagents should be tracked as future durable roles, not ad hoc prompts. The minimum set is datasheet reader, part selector, schematic reviewer, footprint/package verifier, PCB layout reviewer, power-integrity reviewer, RF/high-speed reviewer, EMI/EMC reviewer, SPICE simulation agent, BOM/procurement agent, manufacturing-output agent, DRC/ERC repair agent, visual-evidence agent, and release/verifier agent. Each subagent must use bounded tools, explicit evidence, citations or artifact paths, and a handoff contract to the supervisor run state.

Library work remains central. CCad still needs full KiCad symbol and footprint library import, aliases, inheritance, filters, metadata, pin/electrical types, pad stacks, courtyard/fab/silk/mask/paste details, 3D model references, unsupported-construct diagnostics, provenance, license data, and LLM-native fields such as intended function, electrical role, layout constraints, simulation model links, manufacturability notes, and preferred routing semantics. Component creators should emit both KiCad-compatible fields and AI-friendly semantic fields.

Simulation work needs KiCad/ngspice-compatible netlist behavior, model mapping, source/probe authoring, AC/DC/operating-point/transient modes, result parsing, plot/evidence artifacts, and failure classification. Manufacturing work needs BOM, PnP, drill, Gerber/job outputs, ODB++/IPC-2581 planning, assembly drawings, stackup data, IPC-style checks where feasible, and fabrication approval gates.

### Evaluation, CI, and Regression Harness

The harness needs its own tests. Backlog items include unit tests for agent state schemas, CLI command contracts, policy decisions, trace redaction, tool schemas, and retry logic; integration tests for GUI/live-socket methods; visual regression tests using the official beep-and-screenshot harness; golden trace replay; mock provider runs; LLM-judge-free structural checks; policy tests; load tests for long UI-map sessions; and CI workflows that preserve artifact manifests without leaking secrets.

Every agent-visible feature should define a tool guide, a schema test, a CLI or GUI invocation test, a failure-path test, and a verification artifact. The official visual-validation workflow remains mandatory for GUI-visible changes. Sprint 204 also proves that target validation must scroll targetable controls into view before returning coordinates and must hit-test real canvas points instead of assuming a bounding-box center is a valid click target.

### Agent Harness Sprint Sequence

The next agent-harness work should proceed in bounded sprints rather than endless panel tinkering. Sprint 191 is complete: it redesigned the Agent dock into the full-height vertical workspace shell with header, workspace context, plan, approval, fixed footer command input, visible Request Context and Trigger DRC actions, preserved existing controls, and added target validation for the new semantic IDs. Sprint 192 is complete: it added CLI parity for `agent state`, `agent tasks`, `agent evidence`, and `agent approvals`, with matching JSON-RPC routes and discovery metadata. Sprint 193 is complete: it added a durable local session schema, checkpoint files, and replay metadata without provider calls. Sprint 194 is complete: it adds policy and permission gates for existing agent tools, including dry-run and approval-required flags. Sprint 195 is complete: it adds evidence-card artifacts and screenshot/DRC/ERC/diagnostics manifest binding. Sprint 196 is complete: it adds command-center visual polish, visible Activity events, workspace-state layout metadata, and UI-map targets for Agent regions. Sprint 197 is complete: it adds BYOK/BYOT configuration schema, no-secret templates, and provider env-presence status without project-file secrets. Sprint 198 is complete: it adds OpenTelemetry/Langfuse dry-run trace export configuration, redaction policy, method discovery, and no-network readiness checks. Sprint 199 is complete: it adds local run controls, trace/session chips, first-viewport active-plan rows, workspace-state layout version 3, and UI-map/target-harness coverage for the new Agent regions. Sprint 200 is complete: it adds KiCad CLI evidence schema, planning, dry-run readiness, guarded run, JSON-RPC routes, policy classification, and read-only serve denial for explicit KiCad evidence execution. Sprint 201 is complete: it adds the reference-inspired visual style version and new targetable Agent subregions while preserving the local-only boundary. Sprint 202 is complete: it binds the GUI to local durable Agent session metadata and metadata-only checkpoints. Sprint 203 is complete: it binds command preview to the shared policy classifier and makes the dry-run checkbox map/target/click addressable. Sprint 204 is complete after full gate: it binds trace-link widgets to local trace/span metadata and hardens UI target coordinate validation. Sprint 205 is complete after full gate: it binds provider-readiness widgets to the no-secret provider metadata contract and adds combo/text control support to the UI-map bridge. Sprint 206 is complete after full gate: it adds a first-viewport local run-queue foundation with semantic cancel/clear/status/count/current-step controls and workspace-state queue metadata. Sprint 207 is complete after full gate: it adds the fifth reference-inspired Agent pane contract with compact header action bar, evidence thumbnails, approval preview, one-row footer quick actions, preserved old IDs, UI-map target proof, and full CTest proof. Sprint 208 should continue with datasheet/PDF evidence ingestion, persisted runner queues, live tool-loop progress, actual provider execution behind approvals, actual span emission/export links, and the first ngspice simulation planning and result-artifact slice.

### Native Agent Pane, BYOK Models, and GUI Map Harness

The model layer should be provider-agnostic. The preferred shape is a small CCad model-provider interface with adapters for BYOK API keys and local/subscription-backed providers where legally and technically possible. BYOK should support normal provider keys for OpenAI-compatible APIs, Anthropic, Google Gemini, and local model servers. Existing paid user subscriptions or Gmail/browser sessions should be treated cautiously: do not automate consumer web UIs or account sessions unless the provider explicitly allows that access path. The clean path is official APIs, local models, enterprise connectors, or a future user-installed connector that clearly documents terms, secrets, rate limits, and data exposure.

Sprint 205 binds the native Agent pane to this provider metadata as a visible, targetable, no-secret readiness surface. It exposes provider family, model hint, env-var names, presence-only readiness, and execution-disabled state through `agent.workspace_state` and the UI map. It does not add model calls, provider routing, account automation, token/cost accounting, prompt submission, or any secret-value storage.

Sprint 206 adds the first native Agent run-queue foundation. It exposes `panel:agent_run_queue`, `label:agent_run_queue_status`, `label:agent_run_queue_counts`, `label:agent_run_queue_current_step`, `action:agent_cancel_run_queue`, and `action:agent_clear_run_queue` in the UI map, and it publishes local queue metadata through `agent.workspace_state`. It is still not a model runner, not a worker thread, not a LangGraph runtime, and not an observability exporter.

## Simulation, Manufacturing, and DRC

Simulation work needs KiCad/ngspice-compatible netlist behavior, model mapping, source/probe authoring, simulation run commands, and result inspection. It should not be added as a GUI-only feature.

Manufacturing export must keep expanding from BOM, PnP, drill, DSN, and KiCad export into Gerber/job outputs, assembly drawings, layer stackup data, IPC-style checks where relevant, and stronger DRC coverage.

DRC needs industry-style constraints for copper clearance, mask/paste behavior, courtyard/collision, annular ring, drill, edge clearance, differential pairs, length matching, and return-path rules.

Zone work after Sprint 174 needs KiCad-style refill behavior, thermal relief geometry, island removal, cutouts, priority interactions, zone manager UI, per-zone property editing, polygon vertex editing, and stronger DRC interactions with clearance classes and pad connection rules.

## Visual Validation and Process

Every GUI sprint must use the official visual validation workflow, including the docs beep, two-second hand-off delay, screenshot capture, stdout/stderr interception, and screenshot inspection by the agent. The current timing policy is 7 seconds for a single preview screenshot. Multi-target GUI validation uses a 5-second initial load wait, then fast per-action screenshots around 800 ms unless a feature needs a documented longer wait.

GUI test boards should be useful engineering examples such as bridge rectifiers, regulators, connectors, small MCU breakouts, and mixed top/bottom routing. Placeholder-only visual demos are not acceptable sprint proof.

Each sprint must ship a meaningful batch of at least eight to ten visible/user-meaningful features, fixes, documented capabilities, or verification improvements unless the user explicitly approves a smaller safety sprint.

- [x] Stub: `TestZoneIntersection` (deferred from `edit_zone_helpers.cpp` until `zone.cpp` is processed)

- [x] Stub: `net_chain_bridging.cpp` (advanced router bridging length calculation)
- [x] Stub: `padstack.cpp` (KiCad 8 complex multi-layer padstack models)
- [x] Stub: `graphics_cleaner.cpp` (duplicate/overlapping shape cleanup)

- [x] Stub: `pcb_barcode.cpp` (Barcode rendering)
- [x] Stub: `pcb_design_block_utils.cpp` (Design blocks; skipped as UI logic)
- [x] Stub: `pcb_dimension.cpp` (Dimensioning objects)
- [x] Stub: `pcb_field.cpp`, `pcb_fields_grid_table.cpp` (Property fields; deferred custom fields to BoardFootprint)
- [x] Stub: `pcb_generator.cpp` (Plugin generators)
- [x] Stub: `pcb_group.cpp` (Object grouping)
- [x] Stub: `pcb_plot*.cpp`, `plot_*.cpp` (Gerber, PDF, SVG plotting)
- [x] Stub: `pcb_reference_image.cpp` (Reference image overlays)
- [x] Stub: `pcb_table.cpp`, `pcb_tablecell.cpp` (Table objects)
- [x] Stub: `pcb_target.cpp` (Alignment targets)
- [x] Stub: `pcb_text.cpp`, `pcb_textbox.cpp` (Standalone text primitives)
- [x] Stub: pcbexpr_evaluator.cpp, pcbexpr_functions.cpp (Text substitution engine; deferred as CCad basic text var adapter is complete)
- [x] Stub: tracks_cleaner.cpp (Track merging and cleanup logic; algorithmic, deferred)
- [x] Stub: zone.cpp, zone_filler.cpp, zone_settings.cpp, zone_utils.cpp (Copper pours and zone algorithms; basic BoardZone is complete)


- [x] Stub: `annotate.cpp` (Schematic automatic reference designator annotation)
- [x] Stub: `autoplace_fields.cpp` (Heuristics to auto-arrange property text fields)
- [x] Stub: `bus-wire-junction.cpp` (Visual junctions for intersecting wires)
- [x] Stub: `connection_graph.cpp` (Core connectivity graph algorithm for schematic nets)


- [x] Stub: `gfx_import_utils.cpp` (Schematic DXF/SVG import utilities) (Sprint 265)
- [x] Stub: `junction_helpers.cpp` (Visual junction creation heuristics for schematic intersections) (Sprint 264)


- [x] Stub: `sch_symbol.cpp`, `sch_pin.cpp`, `sch_line.cpp`, `sch_junction.cpp`, `sch_label.cpp`, `sch_text.cpp`, `sch_bus_entry.cpp`, `sch_sheet.cpp` (Core schematic primitives) (Sprints 255-264)
- [x] Stub follow-up: extend Sprint 232's embedded symbol snapshot fix into full KiCad `SCH_SYMBOL`/`SCH_PIN` parity, including multi-unit symbols, alternate pin functions, field placement/autoplace, body conversion edge cases, sheet paths, and library-reference refresh without losing local snapshots. (Sprint 280)
- [x] Stub: `sch_connection.cpp`, `sch_netchain.cpp`, `net_navigator.cpp` (Hierarchical schematic net connectivity algorithms) (Sprint 266)


- [x] Stub: `am_param.cpp`, `am_primitive.cpp`, `aperture_macro.cpp` (Gerber aperture macros and parameters) (Sprint 267)
- [x] Stub: `dcode.cpp` (Gerber D-Code tool definitions) (Sprint 268)
- [x] Stub: `evaluate.cpp` (Expression evaluation for aperture variables) (Sprint 268)
- [x] Stub: `excellon_read_drill_file.cpp` (Excellon drill file parsing) (Sprint 268)
- [x] Stub: `gbr_layout.cpp` (Gerber layer and layout models) (Sprint 268)


- [x] Stub: `gerber_draw_item.cpp`, `gerber_file_image.cpp`, `gerber_file_image_list.cpp` (Gerber graphic primitives and document image model) (Sprint 269)
- [x] Stub: `readgerb.cpp`, `rs274x.cpp`, `rs274d.cpp`, `X2_gerber_attributes.cpp`, `job_file_reader.cpp` (Gerber RS-274X syntax and attributes parsers) (Sprint 270)
- [x] Stub: `gerber_to_polyset.cpp` (Translating Gerber flashes to geometric polygons) (Sprint 271)
- [x] Stub: `gerber_collectors.cpp`, `gerber_diff.cpp` (Hit testing and file comparison) (Sprint 271)


- [x] Stub: `3d_fastmath.cpp`, `3d_math.cpp` (3D coordinate transformations and fast approximations) (Sprint 272)
- [x] Stub: `3d_cache.cpp`, `3d_resolver.cpp` (3D model asset caching and path resolution) (Sprint 272)
- [x] Stub: `3d_canvas/`, `3d_rendering/` (OpenGL scene graph, materials, and raytracing engine) (Sprint 273)


- [x] Stub: `cvpcb/read_netlist.cpp`, `cvpcb/listboxes.cpp` (Parsing schematic netlists for footprint assignment) (Sprint 274)
- [x] Stub: `cvpcb/auto_associate.cpp` (Heuristics for automatically assigning footprints based on symbol properties) (Sprint 275)


- [x] Stub: `pcbnew/autorouter/ar_autoplacer.cpp`, `pcbnew/autorouter/spread_footprints.cpp` (Heuristics for packing and spreading footprints) (Sprint 276)
- [x] Stub: `pcbnew/router/pns_algo_base.cpp`, `pcbnew/router/pns_index.cpp`, `pcbnew/router/pns_node.cpp` (Push and Shove graph theory and spatial indexes) (Sprint 277)
- [x] Stub: `pcbnew/router/pns_dragger.cpp`, `pcbnew/router/pns_diff_pair_placer.cpp`, `pcbnew/router/pns_meander_placer.cpp` (Algorithms for shoving tracks, routing differential pairs, and length matching) (Sprint 277)


- [x] Stub: `pcbnew/pcb_io/allegro`, `altium`, `cadstar`, `eagle`, `easyeda`, `easyedapro`, `fabmaster`, `geda`, `pads`, `pcad`, `sprint_layout` (Importers for external 3rd-party EDA formats) (Sprint 278)


- [x] Stub: `pagelayout_editor` (Custom drawing sheet and title block editor) (Sprint 279)
- [x] Stub: `pcb_calculator` (RF transmission line, E-series, and track current calculators) (Sprint 279)
- [x] Stub: `bitmap2component` (Raster image to PCB geometric footprint converter) (Sprint 279)

- [x] Stub: Agent Orchestration Settings UI and backend logic to store and reflect user settings in appropriate fields, and tweak base agent-orchestrator behavior according to these settings. (Sprint 279)

## Schematic Netlist Reader & Component Association
- **legacy_netlist_reader.cpp**: Parses older format netlists to construct internal NETLIST structures. CCad natively serializes netlists as JSON, but legacy netlist reading is critical for backwards compatibility with pre-2015 KiCad tools and external capture utilities.
- **netlist_reader.h/cpp**: The base class NETLIST_READER defines the contract for streaming in pin-to-net connectivity and footprint assignments (CMP_READER). The parsing logic is token-based. CCad will need a modernized ccad::NetlistReader abstract base class to handle incoming netlist streams.
- **netlist.cpp, pcb_netlist.cpp/h**: NETLIST and PCB_NETLIST encapsulate the in-memory graph of components and their nets before they are resolved into the ccad::Board model. 
- **pcb_component.cpp/h**: Represents a parsed component from the netlist (COMPONENT), bridging the gap between the schematic symbol and the physical footprint. It stores the reference designator, value, and intended footprint string. CCad represents this natively in ccad::SchematicComponent.

## PCB IO & Plugin Manager
- **pcb_io.cpp/h, pcb_io_mgr.cpp/h**: Defines the PCB_IO abstract base class and PCB_IO_MGR factory. This architecture allows dynamically loading or instantiating format-specific importers at runtime. CCad relies on a much simpler JSON/binary internal format and uses stateless function endpoints for conversions, eliminating the need for a heavy object-oriented plugin manager.

## Allegro Importer
- **allegro_builder.cpp/h, pcb_io_allegro.cpp/h, allegro_db_utils.cpp/h**: The BOARD_BUILDER class translates an Allegro BRD_DB into a KiCad BOARD. The PCB_IO_ALLEGRO wrapper maps this back to the standard plugin interface. CCad will need a robust layer-mapping system (LAYER_MAPPER / LAYER_MAPPABLE_PLUGIN) to safely import foreign layer stacks into the unified CCad oard_stackup_manager.

## Component Classes (Cache Proxy & Manager)
- **component_class_cache_proxy.cpp/h**: Implements COMPONENT_CLASS_CACHE_PROXY. Creating dynamic component classes from class generators is an expensive operation. This class acts as a cache-aware proxy for a FOOTPRINT's component class to optimize the runtime overhead. In CCad, footprint component classifications are cached directly inside ccad::SchematicComponent or ccad::Footprint during board ingestion, effectively achieving O(1) lookup without needing a dedicated proxy object.
- **component_class_manager.cpp/h**: Implements COMPONENT_CLASS_MANAGER. This manager operates within the BOARD context. It owns generated COMPONENT_CLASS objects and ensures that pointers to managed classes remain valid. For CCad, this logic maps seamlessly into the ccad::Board data structure where design rules and component groupings are centrally evaluated during DRC.

## Core Connectivity Algorithms
- **connectivity_algo.cpp/h**: Defines CN_CONNECTIVITY_ALGO and CN_EDGE. This is the spatial and logical intersection solver that groups distinct layout primitives (tracks, vias, pads) into electrically connected clusters. CCad's connectivity_algo handles this directly inside the board model using spatial R-trees, avoiding decoupled calculation passes.
- **connectivity_data.cpp/h**: CONNECTIVITY_DATA manages the live cache of net graphs and detached net clusters (CN_DISJOINT_NET_ENTRY). It is responsible for calculating unrouted paths (RN_DYNAMIC_LINE). In CCad, connectivity_data has been mapped to ccad::Board's internal node sets, ensuring memory-efficient footprint association.
- **connectivity_items.cpp**: Maps basic primitives into generic CN_ITEM structs for the algorithm to process blindly. CCad uses its typed C++ structures directly, skipping the secondary mapping overhead.

## Connectivity Auxiliaries (Topology & Cache)
- **connectivity_items.h**: Defines CN_ANCHOR, CN_ITEM, CN_CLUSTER. Maps basic layout primitives into connectivity-aware generic structures. As noted previously, CCad avoids this secondary mapping step by building connectivity natively into the objects themselves.
- **connectivity_rtree.h**: Spatial index used specifically by the connectivity algorithm for finding nearest neighboring elements. CCad leverages a unified SpatialIndex structure across both rendering and connectivity queries.
- **from_to_cache.cpp/h**: Defines FROM_TO_CACHE for accelerating netlist-to-pad mappings. In CCad, mapping from netlist endpoints to footprint pads is solved during footprint auto-placement using the native ccad::Schematic context.
- **topo_match.cpp/h**: Contains CONNECTION_GRAPH and an isomorphic backtracking algorithm (BACKTRACK_STAGE) used for identifying partially matched topology structures (e.g. diff pairs, multi-gate packages). This logic is immensely valuable; CCad will port this exact backtracking algorithm into ccad_core/net_tie_drc.cpp and multi_channel_probe for robust pattern matching.

## Dialogs: Barcodes & Reannotation (UI Domain Logic)
- **dialog_barcode_properties.cpp/h/base**: Manages the properties of a PCB_BARCODE object. While the dialog itself is wxWidgets, the data binding maps to barcode rendering attributes which CCad needs to replicate in its native Qt property inspectors.
- **dialog_board_reannotate.cpp/h/base**: Contains REFDES_CHANGE, REFDES_INFO, and REFDES_PREFIX_INFO. This perfectly illustrates why the UI cannot be skipped! KiCad has hidden core business logic (UUID tracking for RefDes changes, footprint prefix tracking, and collision sets) inside the dialog headers. CCad MUST extract these structures out of the GUI and place them into a standalone ccad_core/reannotation_engine.h so that the CLI and Qt GUI can both access this logic symmetrically.

## Dialogs: Board Setup & Statistics (Master State Hubs)
- **dialog_board_setup.cpp/h**: This single dialog is the master hub for board initialization, linking together massive subsets of business logic: constraints, stackups, layers, defaults, netclasses, rules, and tuning profiles (PANEL_SETUP_CONSTRAINTS, PANEL_SETUP_BOARD_STACKUP, etc.). 
  - **CCad Architectural Decision**: CCad will strip this monolithic setup dialog pattern. Instead of a mega-dialog, these properties map to discrete ccad::Board kernel settings. The GUI will use decentralized QDockWidget inspectors that modify these properties via atomic kernel transactions.
- **dialog_board_statistics.cpp/h/base, dialog_board_stats_job.cpp/h**: Aggregates board-level statistics (components, pads, vias, tracks) and manages background jobs (JOB_EXPORT_PCB_STATS) for exporting them. CCad's kernel natively exposes these counts through the CLI (e.g., ccad pcb stats), removing the need for a GUI-bound statistics job tracker.

## Dialogs: Cleanup Jobs (Graphics, Tracks, Vias)
- **dialog_cleanup_graphics.cpp/h/base**: Configuration UI for cleaning up degenerate graphics (e.g., zero-length segments, overlapping items). 
- **dialog_cleanup_tracks_and_vias.cpp/h/base**: Configuration UI for deleting dangling tracks, merging co-linear segments, and removing redundant vias. 
  - **CCad Architectural Decision**: In KiCad, these dialogs often directly invoke cleanup routines. CCad explicitly isolates cleanup algorithms into the ccad_core transaction engine. The CLI (ccad pcb cleanup --all) and any future Qt cleanup dialogs will merely pass configuration parameters (flags) to the stateless kernel cleanup endpoint, keeping the UI entirely devoid of geometry mutation logic.

## Dialogs: Editing Tools & DRC (Modal Logic)
- **dialog_copper_zones_base, dialog_dimension_properties**: Basic wxWidgets forms configuring physical primitives. CCad completely replaces these with dynamic QPropertyWidget panels tied directly to the selected ccad::Zone or ccad::Dimension object.
- **dialog_create_array.cpp/h/base**: This dialog derives from PCB_PICKER_TOOL::RECEIVER, revealing tight coupling between the modal UI and the canvas mouse picker (for defining array origins). 
  - **CCad Architectural Decision**: CCad breaks this coupling. We rely on the CLI (ccad pcb array) which natively pauses for coordinate inputs (via the active tool state machine) without needing a persistent modal dialog to own the picker callback.
- **dialog_drc.cpp/h/base**: The DRC control dialog. In KiCad, this triggers the DRC routines and displays violations. CCad shifts the entire DRC loop to the headless ccad_core/drc_engine. The Qt GUI will only feature a passive "DRC Violations" dock widget that listens to standard kernel reporting events.

## Dialogs: Footprint Exchange & Exporters
- **dialog_enum_pads.cpp/h/base**: UI for pad numbering (auto-incrementing pad names during footprint creation). CCad maps this to a dedicated CLI command ccad footprint pad-renumber and a standalone Qt property action.
- **dialog_exchange_footprints.cpp/h/base**: Handles replacing a FOOTPRINT with a new LIB_ID. In KiCad, this UI directly manipulates the board items. CCad strictly separates this: the UI just collects the mapping (Old_LIB_ID -> New_LIB_ID) and passes it to the ccad_core/footprint_manager endpoint to execute the atomic swap. This ensures CLI scripts can exchange footprints without invoking the GUI.
- **dialog_export_2581.cpp/h/base, dialog_export_idf.cpp/h/base**: Configuration dialogs binding to JOB_EXPORT_PCB_IPC2581 and JOB_EXPORT_IDF. CCad implements these exporters headlessly; the Qt UI merely serializes the form data into the exact same JSON configuration structure used by ccad pcb export ipc2581.

## Dialogs: Manufacturing Exporters (ODB++, STEP, VRML)
- **dialog_export_odbpp.cpp/h/base**: UI form binding to JOB_EXPORT_PCB_ODB.
- **dialog_export_step.cpp/h/base**: UI form binding to JOB_EXPORT_PCB_3D.
- **dialog_export_vrml.cpp/h/base**: VRML legacy exporter.
  - **CCad Architectural Decision**: Like IPC2581/IDF, these dialogs primarily act as serialization boundaries, mapping checkboxes to job properties. CCad will handle ODB++, STEP, and VRML through headless stateless executors (ccad pcb export step) driven by JSON rule files, decoupling the manufacturing pipeline entirely from the Qt presentation layer.

## Dialogs: Search, Filter, and Associations
- **dialog_filter_selection.cpp/h/base**: Configuration for what types of objects the mouse picker is allowed to select. CCad natively supports CLI-driven selection masks (e.g., ccad pcb select --type Pad) and uses Qt event filters for GUI canvas picking, entirely superseding this dialog.
- **dialog_find.cpp/h/base, dialog_find_by_properties.cpp/h/base**: Implements text search and advanced property-based querying (PROPERTY_MATCH_MODE, PROPERTY_ROW_DATA). 
  - **CCad Architectural Decision**: Advanced querying is a core feature of the CLI (e.g., ccad pcb find "layer == F.Cu and type == Track"). The expression parser and property matching engine MUST be decoupled from the UI and placed into ccad_core/query_engine.h so that both the Qt Search dock and the CLI can use the exact same AST evaluator.
- **dialog_footprint_associations.cpp/h/base**: Displays library and symbol link provenance for a FOOTPRINT. In CCad, this is a passive read-only view in the dynamic Qt Property Inspector.

## Dialogs: Footprint Editing & Validation
- **dialog_footprint_checker.cpp/h/base**: UI that triggers the footprint validation logic (PCB_MARKER generation). Like DIALOG_DRC, CCad pushes this entirely to the kernel (e.g., ccad footprint validate) which emits standard diagnostic messages.
- **dialog_footprint_properties.cpp/h/base**: A monolithic properties window aggregating PANEL_FP_PROPERTIES_3D_MODEL, PANEL_EMBEDDED_FILES, and PCB_FIELDS_GRID_TABLE. 
  - **CCad Architectural Decision**: CCad decentralizes footprint properties. Text fields, 3D models, and custom embedded parameters are exposed natively to the standard Qt Property Inspector via the ccad::Footprint object model, entirely negating the need for a custom C++ dialog form.
- **dialog_footprint_wizard_list.cpp/h/base**: Displays available python footprint generators. CCad's CLI handles generator listing natively via ccad footprint generate --list.

## Dialogs: Pad Tables, Drilling & GenCAD
- **dialog_fp_edit_pad_table.cpp/h/base**: Advanced footprint pad property editor.
- **dialog_gendrill.cpp/h/base, dialog_gencad_export_options.cpp/h**: These bind to JOB_EXPORT_PCB_DRILL and GenCAD exporters. 
- **dialog_generators.cpp/h/base**: Manages dynamic design generators (e.g., Python scripts for arrays/teardrops) by hooking into BOARD_LISTENER. 
  - **CCad Architectural Decision**: CCad will use a headless event bus for BOARD_LISTENER triggers, ensuring generator scripts can react to CLI manipulation (e.g. ccad pcb route) without requiring a GUI context. Drill generation will be standardized under the ccad pcb export drill headless executor.

## Dialogs: Placement Export, Fetchers & Bulk Mutations
- **dialog_gen_footprint_position.cpp/h/base**: Binds to JOB_EXPORT_PCB_POS to export component centroid (placement) files for Pick & Place machines. 
  - **CCad Architectural Decision**: Like the other manufacturing exporters, CCad strictly offloads this to a headless CLI exporter (ccad pcb export pos). 
- **dialog_get_footprint_by_name.cpp/h/base**: A modal fetcher to grab a footprint from the library by string name. CCad achieves this via direct CLI parameter insertion (ccad pcb place-footprint [lib:name]) without a blocking modal.
- **dialog_global_deletion.cpp/h/base**: UI form to bulk delete items by type or layer (e.g., "Delete all tracks"). CCad natively handles this via standard querying and mutation commands (e.g., ccad pcb delete --type Track).
- **dialog_global_edit_teardrops**: Bulk teardrop modifier. CCad executes teardrop logic natively in the kernel, so this UI is superseded by direct CLI commands.

## Dialogs: Global Mutators & Settings Importers
- **dialog_global_edit_text_and_graphics.cpp/h/base, dialog_global_edit_tracks_and_vias.cpp/h/base**: Form views for bulk manipulating tracks, vias, text, and graphic properties. dialog_global_edit_tracks_and_vias employs a VIA_PROTECTION_UI_MIXIN. 
  - **CCad Architectural Decision**: CCad shifts this entire paradigm. We do not use modal dialogs for global mutation. CCad relies on the ccad pcb set command combined with selection masks (e.g., ccad pcb set --type Via --tented true --mask "net == GND"). The GUI simply exposes this batching mechanism through a multi-select property inspector.
- **dialog_import_netlist.cpp/h/base**: The UI for applying a NETLIST to the current board. CCad isolates this logic into ccad_core/board_netlist_updater and exposes it via ccad pcb import-netlist, completely decoupling the synchronization engine from the view layer.
- **dialog_import_settings.cpp/h**: UI for importing constraints and design rules from another board. Superseded in CCad by ccad pcb import-settings [file].

## Dialogs: Layer Mapping & Precision Transforms
- **dialog_map_layers.cpp/h/base**: Collects a mapping array of INPUT_LAYER_DESC when importing foreign formats (DXF/Allegro). CCad handles this natively via headless configuration maps (ccad pcb import-layers --map mapping.json), completely decoupling the import layer resolver from the UI.
- **dialog_migrate_3d_models.cpp/h/base**: Utility to convert legacy 3D path macros.
- **dialog_move_exact.cpp/h/base**: A modal dialog for applying precise VECTOR2I translation and EDA_ANGLE rotation to a selection using a ROTATION_ANCHOR.
  - **CCad Architectural Decision**: CCad rejects modal transform dialogs. We expose exact transformation through the native CLI (ccad pcb move --dx 10 --dy 5 --rot 45). The Qt GUI's "Move Exact" action simply pipes values directly to this unified endpoint, enforcing identical behavior across scripted and manual workflows.

## Dialogs: Multi-Channel Layouts & Offsets
- **dialog_multichannel_repeat_layout.cpp/h/base, dialog_multichannel_generate_rule_areas.cpp/h/base**: Binds to MULTICHANNEL_TOOL and RULE_AREA to automate the replication of hierarchical schematic sheets (rooms) in the PCB layout. 
  - **CCad Architectural Decision**: Hierarchical replication is a cornerstone of modern CAD. In CCad, this logic must be decoupled from the MULTICHANNEL_TOOL UI action and implemented as a kernel transaction (ccad_core/multichannel_replicator). The CLI will expose this via ccad pcb replicate-layout --source RoomA --target RoomB.
- **dialog_non_copper_zones_properties**: Standard property editor for keepouts and non-copper fills. CCad uses QPropertyWidget bindings.
- **dialog_offset_item.cpp/h/base**: A modal to offset an item by a specific vector. Completely superseded in CCad by the unified ccad pcb move transform pipeline.

## Dialogs: Exporters & PNS Router Settings
- **dialog_outset_items.cpp/h/base**: Configuration for the geometry outset operation (expanding shapes).
- **dialog_pad_properties.cpp/h/base**: Complex pad property editor. CCad replaces this entire UI surface with the standard Qt QPropertyWidget mapped natively to ccad::Pad parameters.
- **dialog_plot.cpp/h/base**: Binds to JOB_EXPORT_PCB_PLOT. This is the Gerber, SVG, and PDF plot generator form. 
  - **CCad Architectural Decision**: CCad shifts all plotting to the headless CLI (ccad pcb export plot --config plot.json). The Qt UI serves only to construct the JSON job configuration.
- **dialog_pns_diff_pair_dimensions.cpp/h/base**: Modifies PNS::SIZES_SETTINGS for the Push and Shove Router. CCad decouples PNS settings from modal dialogs, mapping them directly to active design rules within the ccad::Board context so the router can adapt in real-time based on the local spatial area.

## Dialogs: Routing Settings & Property Broadcast
- **dialog_pns_settings.cpp/h/base**: The UI for configuring PNS::ROUTING_SETTINGS (shove rules, smoothing). In CCad, these settings are exposed natively in the Qt QDockWidget for the Active Tool, avoiding a blocking modal so the user can tune shoving behaviors while actively routing.
- **dialog_position_relative.cpp/h/base**: Modal for moving items relative to another anchor. Fully superseded by ccad pcb move --relative-to [anchor_id].
- **dialog_print_pcbnew, dialog_produce_pcb_base**: UI bindings for the legacy print system.
- **dialog_push_pad_properties.cpp/h/base**: A specific UI for copying one pad's properties to other pads in the footprint or board. 
  - **CCad Architectural Decision**: CCad completely avoids building custom C++ "property broadcaster" dialogs. Property broadcast is natively handled by the CLI selection engine. A user or GUI can simply execute ccad pcb set --type Pad --shape Round --mask "parent == U1" to broadcast properties. The native Qt property inspector natively supports multi-select broadcasting, making this custom dialog obsolete.

## Dialogs: Images, Renderer & Rule Areas
- **dialog_reference_image_properties.cpp/h/base**: Configuration for reference background images on the canvas. 
- **dialog_render_job.cpp/h/base**: Configures a 3D raytracing render job. CCad handles 3D rendering headlessly (e.g., via standard OpenGL/Vulkan contexts bound directly to ccad::Board3D), meaning we do not use a modal dialog to configure raytracing pipelines.
- **dialog_router_save_test_case.cpp/h/base**: A developer utility that saves the current board state as a PNS test case. CCad replicates this natively in the CLI via ccad debug save-router-test.
- **dialog_rule_area_properties.cpp/base**: Configures Keepouts (Rule Areas). 
  - **CCad Architectural Decision**: Keepouts in CCad are primary primitives (ccad::RuleArea). CCad completely ditches the modal dialog in favor of binding ccad::RuleArea instances directly to standard QPropertyWidget inspectors, where layers, track constraints, and via constraints are toggled natively.

## Dialogs: Shape & Table Properties, Layer Swapper
- **dialog_shape_properties.cpp/h/base**: Manages base graphics (lines, circles). CCad uses the ccad::Shape property inspector natively without a separate modal window.
- **dialog_swap_layers.cpp/h/base**: A modal dialog constructing a std::map<PCB_LAYER_ID, PCB_LAYER_ID> to batch-move items between layers. CCad handles this completely through the CLI: ccad pcb swap-layers --map "F.Cu=B.Cu".
- **dialog_table_properties.cpp/h/base, dialog_tablecell_properties.cpp/h/base**: UI forms for editing PCB_TABLE dimensions and cell content. CCad merges these directly into the Qt Inspector dock when a ccad::Table object is selected.

## Dialogs: Primitives & Board Properties
- **dialog_target_properties, dialog_textbox_properties, dialog_text_properties**: Simple properties configuration for drafting primitives. Replaced in CCad by Qt's native QPropertyWidget.
- **dialog_track_via_properties.cpp/h/base**: A monolithic UI for configuring PCB_SELECTION (tracks, vias). Like global tracking, it uses a VIA_PROTECTION_UI_MIXIN. 
  - **CCad Architectural Decision**: CCad completely discards this dialog. The Qt object inspector (similar to Altium's Properties panel) dynamically parses the selected ccad::Track or ccad::Via structure using C++ introspection/Qt Meta-Object system, rendering fields natively without a bespoke dialog window.

## Dialogs: Track Tuning, Pads & Schematic Sync
- **dialog_track_via_size.cpp/h/base**: Manages global track and via sizes. CCad manages this via rule stacks rather than floating dialogs.
- **dialog_tuning_pattern_properties.cpp/h/base**: Modifies length-tuning/meander properties. In CCad, meander settings are bound directly to the active tuning tool state and can be modified via the CLI (e.g., ccad pcb route tune --amplitude 2mm).
- **dialog_unused_pad_layers.cpp/h/base**: Triggers the algorithm that removes unused inner layers from pads to reduce capacitance. CCad makes this a headless CLI optimization step (ccad pcb optimize --remove-unused-pads).
- **dialog_update_pcb.cpp/h/base**: The main dialog for synchronizing a schematic NETLIST to the board layout. 
  - **CCad Architectural Decision**: Synchronization is a core headless transaction. CCad separates the UI preview log from the actual transaction engine. The engine lives in ccad_core/board_netlist_updater and is invoked seamlessly via ccad pcb import-netlist or ccad pcb sync.

## Dialog Panels: Application Settings
- **panel_assign_component_classes.cpp/base**: Configuration pane for mapping footprints to classes.
- **panel_display_options.cpp/h/base**: Modifies PCBNEW_SETTINGS and APP_SETTINGS_BASE (e.g., rendering modes, grid colors). 
- **panel_edit_options.cpp/h/base**: Modifies FOOTPRINT_EDITOR_SETTINGS and PCBNEW_SETTINGS (e.g., magnetic snapping, rotation angles).
- **panel_fp_editor_color_settings.cpp/h, panel_fp_editor_field_defaults.cpp/h**: Footprint editor specific configurations.
  - **CCad Architectural Decision**: In KiCad, these panels directly modify global application structs. CCad centralizes all application and editor settings into ccad_core/settings_manager.h, serialized to JSON profiles. The Qt QSettings UI panes will bind directly to these schema paths via Model-View-Controller patterns, keeping the UI panels completely devoid of domain configuration structs.

## Dialog Panels: Footprint Editor & Libraries
- **panel_fp_editor_graphics_defaults.cpp/h**: Manages default geometry dimensions for footprint creation (e.g., default silk width). In CCad, this is serialized directly to the active ccad_core/settings_manager.h footprint profile.
- **panel_fp_lib_table.cpp/h/base**: Edits the FP_LIB_TABLE (which libraries are active). CCad handles the catalog metadata natively. The GUI wraps ccad_core/library_manager through Qt's QAbstractTableModel.
- **panel_fp_properties_3d_model.cpp/h**: Binds a PANEL_PREVIEW_3D_MODEL to a footprint. As established, CCad uses the ccad::Footprint property inspector to manage 3D paths, completely decoupling the UI panel from the model data.
- **panel_fp_user_layer_names.cpp/h**: Edits custom layer names for the footprint. CCad manages this via ccad pcb set-layer-name and standard Qt layer views.

## Dialog Panels: Plugins, Origins & Sub-Panels
- **panel_pcbnew_action_plugins.cpp/h/base**: Configuration UI for Python Action Plugins. 
  - **CCad Architectural Decision**: CCad shifts plugin architecture completely to the backend. The core kernel provides a generic WASM/Python embedding interface. The CLI discovers plugins (ccad plugin list) and executes them (ccad plugin run <name>). A Qt UI panel is strictly optional and merely reflects the CLI's plugin manifest.
- **panel_pcbnew_display_origin.cpp/h/base**: Modifies display origin coordinates (e.g., relative vs absolute). CCad maps this to settings_manager.h.
- **panel_rule_area_properties_keepout_base/placement_base**: UI forms for rule area properties. As documented earlier, CCad replaces all rule area UI dialogs with native QPropertyWidget integrations.

## Dialog Panels: Board Design Settings
- **panel_setup_constraints.cpp/h/base**: The core form for editing design rules (minimum track width, clearance, via size). Modifies BOARD_DESIGN_SETTINGS. 
  - **CCad Architectural Decision**: CCad maps design rules into the ccad::DesignRules AST within the ccad_core kernel. The Qt UI will use a unified generic rule editor (similar to Altium's query-based rule editor) rather than hardcoded C++ forms for specific constraints. The CLI accesses this via ccad pcb rule set --rule TrackWidth --value 0.2mm.
- **panel_setup_defaults, panel_setup_dimensions, panel_setup_formatting**: Forms for defaults and text formatting.
- **panel_setup_layers.cpp/h/base**: Modifies active layers and stackup bindings via PANEL_SETUP_LAYERS_CTLs. CCad moves the stackup definition into an independent engine (ccad_core/stackup_manager.h), modified via ccad pcb stackup.

## Dialog Panels: Custom Rules & Solder Mask
- **panel_setup_mask_and_paste.cpp/h/base**: Modifies global mask expansions in BOARD_DESIGN_SETTINGS. 
- **panel_setup_rules.cpp/h/base**: The UI for the custom Design Rule syntax (s-expression like syntax). Manipulates DRC_RULE. 
  - **CCad Architectural Decision**: CCad parses design rules natively into the ccad_core/rule_evaluator AST. The Qt UI uses standard text editing (like SCINTILLA_TRICKS) to author the rules, but the rules are evaluated completely independently by the kernel when ccad pcb drc is invoked.

## Design Rule Check (DRC) Engine: Core
- **panel_zone_properties.cpp/h/base**: Finally out of dialogs/. Standard zone properties, superseded by native Qt property inspector.
- **drc_engine.cpp/h**: The beating mechanical heart of the CAD validator. Contains DRC_ENGINE, DRC_RULE, DRC_CONSTRAINT, and the DRC_VIOLATION_HANDLER callback type.
  - **CCad Architectural Decision**: CCad perfectly mirrors this headless engine structure. ccad_core/drc/drc_engine parses the AST, calculates physical intersections, and emits violations. The GUI operates merely as a consumer of this output stream (e.g., parsing the JSON output of ccad pcb drc).
- **drc_cache_generator.cpp/h**: Subclasses DRC_TEST_PROVIDER to build spatial caches (R-Trees/Grids) before running collision checks.
- **drc_chain_topology.cpp/h**: Analyzes connectivity graphs via CHAIN_TOPOLOGY to find stubs and branches. Used heavily by the High-Speed routing constraint checkers.

## Design Rule Check (DRC) Engine: Parsers & Test Providers
- **drc_rule_parser.cpp/h**: Implements the S-expression parser DRC_RULES_PARSER generating a std::vector<std::shared_ptr<DRC_RULE>>. 
  - **CCad Architectural Decision**: CCad retains the S-Expression grammar for backward compatibility but implements the parser strictly within ccad_core/drc_parser. The CLI executes ccad pcb rule compile to ingest these text files.
- **drc_test_provider.cpp/h**: Defines the DRC_TEST_PROVIDER interface and DRC_TEST_PROVIDER_REGISTRY. This is a classic plugin registry pattern for physical tests. 
  - **CCad Architectural Decision**: CCad will use this exact registry pattern. Specific geometrical tests (annular width, connection width, connectivity, courtyard) will be compiled as independent headless DRC_TEST_PROVIDER modules in ccad_core/drc/providers. This allows agents to write new custom DRC checks simply by implementing a new provider class without touching the core drc_engine.cpp.

## Design Rule Check (DRC) Engine: Rule Editor UI
- **rule_editor/dialog_drc_rule_editor.cpp/h**: The main container for the visual rule editor (the new visual rule builder in KiCad 8+). It translates visual constraints into the text-based S-expressions.
- **rule_editor/drc_re_condition_group_panel.cpp/h**: Manages logic gates (AND, OR, AND NOT, OR NOT) between different rule conditions.
- **rule_editor/drc_re_*_overlay_panel.cpp/h**: Various input widgets for absolute lengths, boolean inputs, and allowed orientations for specific rule parameters.
  - **CCad Architectural Decision**: CCad separates the rule authoring UI completely from the rule execution engine. The Qt Inspector will host a visual query builder (similar to Altium's query helper) that outputs standard CCad CLI queries (e.g., 
et_class == "HighSpeed"), serializing them to JSON profiles instead of S-expressions. The dialog_drc_rule_editor logic is replaced by standard Qt Model-View controllers over ccad::DesignRules.

## Design Rule Check (DRC) Engine: Rule Editor Overlay Panels
- **rule_editor/drc_re_*_overlay_panel.cpp/h**: (e.g. drc_re_abs_length_two_overlay_panel, drc_re_bool_input_overlay_panel, drc_re_permitted_layers_overlay_panel). Dozens of custom C++ Qt widgets explicitly coded to handle specific types of rule data inputs.
  - **CCad Architectural Decision**: CCad leverages the Qt Meta-Object system to automatically generate these property editors. By subclassing ccad::DRCConstraint into types like ccad::NumericConstraint or ccad::LayerConstraint and decorating them with Q_PROPERTY macros, the generic Qt Property Inspector completely replaces these 50+ bespoke drc_re_ UI files.

## Exporters: Base Interfaces & Legacy Formats
- **board_exporter_base.h**: The abstract interface BOARD_EXPORTER_BASE for all serialization engines. It consumes REPORTER and PROGRESS_REPORTER.
- **export_d356, export_gencad, export_hyperlynx, export_idf, export_vrml**: Various legacy and simulation format generators.
- **gendrill_excellon_writer.cpp/h**: EXCELLON_WRITER parses the board and emits standard N/C Drill files.
  - **CCad Architectural Decision**: CCad will directly preserve these writer classes in the ccad_core/exporters/ kernel. Since they already depend solely on BOARD and not on Qt UI contexts, they can be flawlessly wired into the headless CLI pipeline (e.g., ccad pcb export excellon --board myboard.kicad_pcb).
## Exporters: Drill, Gerber, POS & STEP
- **gendrill_writer_base.cpp/h**: Base class for Drill writers.
- **gerber_jobfile_writer.cpp/h, gerber_placefile_writer.cpp/h**: Writers for Gerber X2/X3 attributes and pick & place files.
- **place_file_exporter.cpp/h**: Base centroid generator for SMD placements.
- **step/exporter_step.cpp/h, step/step_pcb_model.cpp/h**: STEP 3D MCAD export subsystem. Converts 2D BOARD primitives to OpenCASCADE-compatible 3D B-reps.
  - **CCad Architectural Decision**: All of these exporters (place_file_exporter, exporter_step, gerber_jobfile_writer) are inherently headless in KiCad's source. CCad seamlessly ports them to ccad_core/exporters/ without modification, directly mounting them to the CLI endpoints (ccad pcb export gerber/pos/step).

## Exporters (U3D) & Generators & Git Merging
- **exporters/u3d/**: Legacy Universal 3D format exporters. CCad natively supports 3D export via headless CLI (ccad pcb export u3d).
- **generators/pcb_tuning_pattern.cpp/h**: Implements PCB_TUNING_PATTERN and TUNING_STATUS_VIEW_ITEM. This is the engine that calculates meander geometry for length matching. 
  - **CCad Architectural Decision**: CCad decouples the physical generator from the GUI view items. The geometry is generated strictly in the kernel ccad_core/generators/ and returned to the UI as immutable ccad::Shape buffers.
- **git/kigit_pcb_merge.cpp/h**: Implements KIGIT_PCB_MERGE which acts as a custom git_merge_driver_source for resolving S-expression merge conflicts automatically. CCad provides a dedicated CLI binary (ccad-git-merge) to act as the .gitconfig merge driver for .kicad_pcb files.
- **import_gfx/dialog_import_graphics.cpp/h**: A UI for importing DXF/SVG images onto the board. Replaced by ccad pcb import-graphics [file].

## High-Speed Analytics, Microwave Tools & Input Nav
- **length_delay_calculation/length_delay_calculation.cpp/h**: Implements LENGTH_DELAY_CALCULATION and LENGTH_DELAY_STATS. Analyzes electrical delay across paths considering via spans and pad clips. 
  - **CCad Architectural Decision**: This is a pure analytical kernel. CCad will migrate this directly into ccad_core/analytics/length_delay. It runs headlessly and feeds both the DRC engine and the GUI property inspectors.
- **microwave/microwave_tool.cpp/h**: MICROWAVE_TOOL generates RF patterns (e.g. MICROWAVE_INDUCTOR_PATTERN). In CCad, RF pattern generators are abstracted as standard headless script generators, executable via ccad footprint generate rf-inductor.
- **navlib/**: 3DConnexion SpaceMouse library bindings. CCad abstracts spatial navigation through generic Qt HID event interceptors rather than bespoke CAD-level implementations.

## Netlist Synchronization & I/O Architecture
- **netlist_reader/board_netlist_updater.cpp/h**: BOARD_NETLIST_UPDATER takes a BOARD* and a NETLIST* and performs the synchronization algorithms (Update PCB from Schematic). 
  - **CCad Architectural Decision**: This is a pure transaction. CCad migrates this to ccad_core/board_netlist_updater and executes it directly via the CLI (ccad pcb import-netlist). It completely avoids UI coupling, returning a structured JSON diff of operations performed.
- **pcb_io/pcb_io.cpp/h/mgr**: PCB_IO inheriting from IO_BASE. This represents the plugin interface for native CAD serialization (reading/writing .kicad_pcb or .kicad_mod). 
  - **CCad Architectural Decision**: CCad replicates this exact abstract syntax. IO loaders (Eagle, Altium, KiCad) will be registered dynamically so the CLI can uniformly execute ccad pcb convert altium_file.PcbDoc output.kicad_pcb.

## Unrouted Nets (Ratsnest) & Push-and-Shove (PNS) Router
- **ratsnest/ratsnest_data.cpp/h**: RN_NET models the topological graph of unconnected pad-to-pad vectors. 
  - **CCad Architectural Decision**: CCad maps this to ccad_core/ratsnest. It calculates minimal spanning trees via headless graph algorithms. The Qt Canvas merely requests oard->GetUnconnectedVectors() to draw the ratsnest lines.
- **router/pns_algo_base.cpp/h**: ALGO_BASE serves as the foundation for the Push and Shove router algorithms (Walkaround, Shove, Drag). 
  - **CCad Architectural Decision**: The PNS router in KiCad is famously well-isolated from the GUI. CCad moves the entire 
outer/ directory into ccad_core/router/. It operates on headless ROUTER contexts. The UI layer simply feeds coordinate events to the algorithm, and the algorithm returns the modified geometric diffs.

## Freerouting Export & Teardrop Generation
- **specctra_import_export/specctra.cpp/h**: SPECCTRA_DB lexes and emits the .dsn format used by external auto-routers (like Freerouting or Electra). 
  - **CCad Architectural Decision**: CCad maps this seamlessly to ccad_core/exporters/specctra. It is a pure serialization endpoint and runs natively from ccad pcb export dsn.
- **teardrop/teardrop.cpp/h**: TEARDROP_MANAGER generates smoothed teardrop zones at via/pad intersections.
  - **CCad Architectural Decision**: This is a pure mathematical generator. CCad moves it to ccad_core/generators/teardrops and executes it completely headlessly. The GUI sees teardrops merely as ccad::Zone primitives with the teardrop boolean flag set.

## Interactive Tools (GAL)
- **tools/**: Includes lign_distribute_tool, drawing_tool, oard_editor_control, etc. These files inherit from TOOL_INTERACTIVE and define the state machines for mouse clicks (e.g., clicking to start a line, moving the mouse to draw the line, clicking to end).
  - **CCad Architectural Decision**: CCad fundamentally abandons the stateful TOOL_INTERACTIVE paradigm. Instead, the GUI emits stateless JSON commands into the kernel, such as ccad pcb draw-track --points "[...]". The kernel verifies the action and mutates the ccad::Board. This ensures 100% parity between what an LLM Agent can execute via CLI and what a human can execute via mouse.

## General UI Widgets
- **widgets/**: Contains ppearance_controls, 
et_inspector_panel, and panel_selection_filter. These are the docking panels in the KiCad GUI (e.g., the layer selection panel on the right).
  - **CCad Architectural Decision**: CCad will rewrite these explicitly in ccad_gui/widgets/. They will bind directly to the ccad::Board data structures and the ccad_core/settings_manager profile. The backend logic for layer visibility and selection filtering remains purely in the core.

## Zone Manager (Copper Pours)
- **zone_manager/**: UI and backend logic for managing and prioritizing overlapping Copper Pours (ZONE).
  - **CCad Architectural Decision**: CCad shifts zone conflict resolution and hatching completely into the kernel (ccad_core/zone_manager). The GUI simply presents the zone boundaries, and the core calculates the final poured geometry.

---
# MODULE 2: EESCHEMA (Schematic Editor)
*Exploration Transition: Successfully exited pcbnew, entering eeschema.*

## EESchema Core: Annotations, Netlists & BOM
- **eeschema/connection_graph.cpp/h**: CONNECTION_GRAPH computes the global electrical netlist by traversing CONNECTION_SUBGRAPH elements across hierarchical sheets.
  - **CCad Architectural Decision**: Like the DRC engine, schematic connectivity is completely headless in CCad (ccad_core/schematic/connection_graph). It computes dynamically as wires are placed and returns the net graph for ERC (Electrical Rules Check) and netlist export.
- **eeschema/bom_plugins.cpp/h**: BOM_GENERATOR_HANDLER manages the execution of external scripts (usually Python) to generate Bill of Materials. CCad manages this natively via the ccad plugin CLI system, rendering BOM generation identical to Action Plugins.
- **eeschema/annotate.cpp, autoplace_fields.cpp, cross-probing.cpp**: Legacy C++ files for annotation and IPC cross-probing. CCad implements annotation as a synchronous AST visitor over the schematic tree (ccad sch annotate).

## EESchema Core: Fields Data Model & Junction Helpers
- **eeschema/fields_data_model.cpp/h**, **eeschema/lib_fields_data_model.cpp/h**: These provide the underlying wxGridTableBase implementations for bulk editing component fields (Reference, Value, Footprint, Datasheet).
  - **CCad Architectural Decision**: CCad abandons wxGridTableBase completely. The CLI provides a headless spreadsheet CSV import/export (ccad sch export-bom --format csv and ccad sch import-attributes map.csv). The UI maps this natively into QAbstractTableModel.
- **eeschema/junction_helpers.cpp/h**: Algorithms for automatically generating wire intersections (SCH_JUNCTION) when wires cross at valid terminals.

## Library Symbols & Project Rescue
- **eeschema/lib_symbol.cpp/h**: LIB_SYMBOL represents a parsed schematic component from a .kicad_sym or legacy .lib file.
  - **CCad Architectural Decision**: CCad maps this seamlessly into ccad_core/library/symbol. It functions identically to KiCad 6+ where symbols are fully isolated objects with Pins and Fields.
- **eeschema/project_rescue.cpp/h**: Implements the RESCUER tool which detects when a schematic symbol is missing from active libraries and attempts to recover it from the project cache.
  - **CCad Architectural Decision**: CCad leverages KiCad 6+ embedded symbol paradigm, meaning the RESCUER is strictly a legacy migration tool. CCad implements this as a one-time migration hook during ccad sch migrate rather than an interactive UI dialog.

## EESchema Core: Schematic Root & UI Frame
- **eeschema/schematic.cpp/h**: Defines SCHEMATIC : public EDA_ITEM, which acts as the root data structure for an entire schematic project (analogous to BOARD in pcbnew).
  - **CCad Architectural Decision**: CCad perfectly mirrors this structure in ccad_core/schematic/schematic.h (defined as ccad::Schematic). It holds the abstract syntax tree of all sheets and symbols.
- **eeschema/sch_base_frame.cpp/h**: Defines SCH_BASE_FRAME : public EDA_DRAW_FRAME. This is the wxWidgets monolithic UI window that binds directly to the SCHEMATIC data.
  - **CCad Architectural Decision**: CCad fully decouples the SCHEMATIC data model from the UI. The Qt GUI wrapper (ccad_gui/schematic_editor) binds via Model-View and mutates the ccad::Schematic purely through stateless CLI/kernel transactions.

## EESchema Core: Connections & Edit Frame
- **eeschema/sch_connection.cpp/h**: SCH_CONNECTION represents an active electrical node or bus connection generated during the traversal of the CONNECTION_GRAPH.
- **eeschema/sch_edit_frame.cpp/h**: SCH_EDIT_FRAME is the main interactive canvas for EESchema. It holds the active SCHEMATIC, manages tools, and handles the hierarchical navigation tree (HIERARCHY_PANE).
  - **CCad Architectural Decision**: Like PCB_EDIT_FRAME, the entire interactive logic in SCH_EDIT_FRAME is fully decoupled in CCad. ccad_gui/schematic_editor binds via Model-View to the headless ccad::Schematic and executes modifications purely via JSON CLI transactions, enabling 100% agent parity.

## EESchema Core: Abstract Syntax Tree (AST) Primitives
- **eeschema/sch_item.cpp/h**: Defines SCH_ITEM : public EDA_ITEM, the foundational geometric and logical primitive for every entity drawn in the schematic editor (Symbols, Wires, Pins, Text).
  - **CCad Architectural Decision**: CCad maps this exactly to ccad_core/schematic/item.h (ccad::SchItem). This ensures that the AST for schematic files matches the physical constraints of EDA_ITEM without inheriting Qt/wxWidgets rendering baggage.
- **eeschema/sch_line, sch_junction, sch_label, sch_marker, sch_no_connect**: The specific implementations of SCH_ITEM representing wires, dots, net-names, DRC markers, and X-marks.
- **eeschema/sch_netchain.cpp/h**: Implements SCH_NETCHAIN, which groups SCH_CONNECTION instances that are separated only by passive zero-ohm/passive bridge components (essential for Net Ties).

## EESchema Graphics & Annotation
- **eeschema/sch_painter.cpp/h**: Implements SCH_PAINTER, the Graphics Abstraction Layer (GAL) rendering engine for EESchema. It handles drawing SCH_SYMBOL, SCH_LINE, etc.
  - **CCad Architectural Decision**: CCad discards GAL entirely. The ccad_gui/schematic_editor will implement a native Qt QGraphicsScene painter that directly consumes the abstract bounding boxes and vector geometry provided by ccad::SchItem.
- **eeschema/sch_reference_list.cpp/h**: SCH_REFERENCE_LIST is the core data structure used during schematic annotation to ensure that R1, C1, etc., are uniquely assigned across hierarchical sheet paths.

## EESchema Core: Hierarchical AST (Sheets and Screens)
- **eeschema/sch_sheet.cpp/h**: SCH_SHEET : public SCH_ITEM represents a hierarchical sheet block instantiated within a schematic.
- **eeschema/sch_screen.cpp/h**: SCH_SCREEN : public BASE_SCREEN represents the actual canvas/container holding the items inside a specific sheet. 
  - **CCad Architectural Decision**: CCad maps this topology directly to ccad_core/schematic/. ccad::Schematic holds the tree, ccad::SchSheet is the node, and ccad::SchScreen is the item container. This is identical to KiCad's data structure but natively isolated from any GUI/Qt elements, enabling headless hierarchical traversals.
- **eeschema/sch_sheet_path, sch_sheet_pin**: Utilities for resolving paths through complex, multi-instantiated hierarchical sheets.

## EESchema Symbols & Library Manager
- **eeschema/symbol.cpp/h**: SYMBOL : public SCH_ITEM is the abstract representation of a schematic part. 
- **eeschema/symbol_library_manager.cpp/h**: Implements SYMBOL_LIBRARY_MANAGER and LIB_BUFFER. This caches symbols loaded from the sym-lib-table.
  - **CCad Architectural Decision**: CCad unifies the Symbol and Footprint library managers. The exact same ccad_core/library/manager natively caches both .kicad_sym and .kicad_mod files, eliminating the duplicated logic between eeschema and pcbnew.
- **eeschema/sch_view.cpp/h**: The graphics View (part of GAL). Replaced by QGraphicsScene.

## EESchema API & Headless Validation
- **eeschema/api/api_handler_sch.cpp/h**: API_HANDLER_SCH provides the RPC (Remote Procedure Call) endpoints for the new KiCad 8 Python API.
- **eeschema/api/headless_sch_context.cpp/h**: HEADLESS_SCH_CONTEXT provides an execution environment for schematic queries and mutations without spawning a SCH_EDIT_FRAME UI window.
  - **CCad Architectural Validation**: The existence of HEADLESS_SCH_CONTEXT in upstream KiCad completely validates CCad's thesis! CCad takes this architecture to its logical extreme: ccad_core/schematic *is* the headless context. There is no alternative. The GUI (ccad_gui) is just another consumer of the exact same API endpoints that the CLI Python/WASM agents use.

## EESchema Dialogs
- **eeschema/dialogs/**: Contains wxWidgets popup dialogs (e.g., dialog_annotate, dialog_bom, dialog_change_symbols). 
  - **CCad Architectural Decision**: Similar to pcbnew/dialogs, all business logic currently housed in these UI files (like annotation traversal or BOM script invocation) MUST be entirely decoupled into ccad_core/schematic executors. The CCad GUI (ccad_gui/dialogs) will just be thin QDialog wrappers that gather options and dispatch headless commands like ccad sch annotate --scope all.

## EESchema Dialogs: Change Symbols & Net Chains
- **eeschema/dialogs/dialog_change_symbols.cpp/h**: Provides the UI and execution logic for "Update Symbols from Library" (refreshing cached footprints/symbols from the source library).
  - **CCad Architectural Decision**: CCad extracts the update logic into ccad_core/schematic/executors/symbol_updater.cpp. It runs via CLI (ccad sch update-symbols) and the UI simply invokes this API.
- **eeschema/dialogs/dialog_create_net_chain.cpp/h**: Provides UI logic for explicitly defining Net Ties or connecting distinct nets through a passive bridge.

## EESchema Dialogs: Database Libraries & ERC
- **eeschema/dialogs/dialog_database_lib_settings.cpp/h**: Manages ODBC/SQL database library connections.
  - **CCad Architectural Decision**: Database library definitions in CCad are handled natively via the .ccad_pro project configuration file using JSON, and queries run headlessly in the ccad_core/library subsystem.
- **eeschema/dialogs/dialog_erc.cpp/h**: Provides the UI for the Electrical Rules Check, but critically, it also houses the execution loop that gathers the errors and generates the .erc report file.
  - **CCad Architectural Decision**: CCad moves the *entire* ERC execution loop into ccad_core/erc_engine. The GUI simply asks the kernel to evaluate the rules (ccad sch erc) and displays the returned ccad::Violation objects in a standard Model-View table.

## EESchema Dialogs: Netlist Export & Global Edits
- **eeschema/dialogs/dialog_export_netlist.cpp/h**: Manages the netlist export process, again coupling execution logic (writing the .net file via XSLT) to the UI.
  - **CCad Architectural Decision**: Moved to ccad_core/exporters/netlist_exporter and executed via ccad sch export netlist.
- **eeschema/dialogs/dialog_global_edit_text_and_graphics.cpp/h**: The bulk property editor UI. In CCad, this logic is executed by a headless AST AST Visitor (ccad sch mutate --match "type=text" --set "size=2mm").

## EESchema Dialogs: Annotation & Pin Tables
- **eeschema/dialogs/dialog_increment_annotations**: UI for Paste Special (incrementing reference designators on paste).
- **eeschema/dialogs/dialog_lib_edit_pin_table**: UI for the bulk pin editor in the symbol library editor.
  - **CCad Architectural Decision**: CCad moves the bulk pin editor logic out of wxWidgets and into a unified Qt Model-View spreadsheet that allows CSV import/export of pin maps, drastically speeding up high-pin-count component creation.

## EESchema Dialogs: Properties and Pin Tables
- **eeschema/dialogs/dialog_lib_edit_pin_table.cpp/h**, **dialog_lib_fields_table.cpp/h**, **dialog_lib_symbol_properties.cpp/h**, **dialog_line_properties.cpp/h**: These represent dozens of bespoke C++ wxDialogs used solely to map UI text boxes to C++ member variables (e.g., Line Thickness, Symbol Value, Pin Name).
  - **CCad Architectural Decision**: CCad eliminates these entirely in favor of a universal ccad_gui/QPropertyWidget inspector that reflects on the ccad::SchItem AST directly, providing a unified properties panel on the right side of the screen (similar to modern CAD tools like Altium or Figma).

## EESchema Dialogs: Plotting & Rescue
- **eeschema/dialogs/dialog_plot_schematic.cpp/h**: Provides UI for exporting schematics to PDF/SVG/DXF. 
  - **CCad Architectural Decision**: CCad extracts the plotting loop into ccad_core/exporters/plotter, executing via ccad sch plot --format pdf. The GUI merely feeds parameters to the command.
- **eeschema/dialogs/dialog_rescue_each.cpp/h**: The interactive wizard for rescuing symbols from the cache. CCad runs rescue headlessly during project migration, removing the interactive element.

## EESchema Dialogs: Setup & Properties
- **eeschema/dialogs/dialog_schematic_setup**: Manages project-level schematic settings (Net Classes, ERC constraints, Formatting).
  - **CCad Architectural Decision**: Mapped directly to ccad_core/settings_manager. The GUI renders the JSON config via a generic QPropertyTreeWidget, eliminating the need for custom C++ dialogs.
- **eeschema/dialogs/dialog_sch_find**: Replaced in CCad by the universal ccad sch find --query "xxx" API endpoint.
- **eeschema/dialogs/dialog_shape_properties**, **dialog_sheet_pin_properties**, **dialog_sheet_properties**: These are absorbed into the unified ccad_gui properties side-panel.

## EESchema Dialogs: Simulation (SPICE)
- **eeschema/dialogs/dialog_sim_*.cpp/h**: These dialogs manage the configuration of ngspice simulation models, parameters, and analysis commands (AC, DC, TRAN).
  - **CCad Architectural Decision**: CCad shifts the entire simulation engine binding (ngspice) to ccad_core/simulation. The GUI will rely on a generic inspector, and simulation graphs will be handled by a dedicated Qt charting widget, decoupling the SPICE netlist generator from the wxWidgets UI.

## EESchema Dialogs: Migration & Properties
- **eeschema/dialogs/dialog_symbol_remap.cpp/h**: Provides the UI and execution for migrating legacy KiCad 4 schematics to KiCad 5+ (mapping symbols to their specific library tables).
  - **CCad Architectural Decision**: Legacy schematic parsing and rescue is executed headlessly by ccad sch migrate. No UI wizard is needed.
- **eeschema/dialogs/dialog_text_properties**, **dialog_table_properties**: More UI wrappers for AST mutations that CCad replaces with the universal QPropertyWidget inspector.

## EESchema Dialogs: PCB Sync & Bus Properties
- **eeschema/dialogs/dialog_update_from_pcb.cpp/h**: Provides the UI for back-annotation (importing pin swaps, footprint assignments, and reference changes from the PCB).
  - **CCad Architectural Decision**: Back-annotation in CCad is a pure kernel transaction executed by ccad_core/synchronizer. The CLI invokes it via ccad sch update-from-pcb --source myboard.kicad_pcb.
- **eeschema/dialogs/dialog_wire_bus_properties**: More UI wrappers mapped to CCad's unified properties panel.

## EESchema Dialogs: Preferences Panels
- **eeschema/dialogs/panel_*.cpp/h**: These represent the sub-panels injected into the main Preferences dialog (BOM Presets, Colors, Display, Editing).
  - **CCad Architectural Decision**: CCad's Settings Manager drives this purely through JSON schemas. The UI dynamically builds the preferences tree from the schema, entirely eliminating the need for hardcoded C++ preference panel layouts.

## EESchema Dialogs: Setup Panels
- **eeschema/dialogs/panel_setup_*.cpp/h**: These panels configure project-specific schematic setup options (Net Chains, Pin Maps, Formatting, Buses, Data Sources). 
  - **CCad Architectural Decision**: Like the preference panels, these setup pages are entirely replaced by the unified JSON Settings Manager and dynamically generated Qt Property Trees.

## EESchema Dialogs: Symbol Editor Panels
- **eeschema/dialogs/panel_sym_*.cpp/h**: Panels specifically for configuring the Symbol Editor (Colors, Display Options, Editing Options, Lib Table, Template Fields).
  - **CCad Architectural Decision**: Driven purely by JSON schemas in ccad_core/settings_manager, eliminating bespoke wxWidgets C++ classes.

## EESchema ERC (Electrical Rules Check)
- **eeschema/erc/erc.cpp/h**: Defines ERC_TESTER which traverses the CONNECTION_GRAPH to validate pin conflicts, unconnected pins, and missing drivers.
- **eeschema/erc/erc_item.cpp/h**: ERC_ITEM : public RC_ITEM represents a single electrical violation.
  - **CCad Architectural Decision**: CCad maps this seamlessly to ccad_core/erc_engine. It runs entirely headlessly, evaluating the AST and outputting violations as JSON data via the CLI (ccad sch erc --format json), guaranteeing parity across environments.

## EESchema Legacy Libraries & Exporters
- **eeschema/libraries/legacy_symbol_library.cpp/h**: Parser for the legacy KiCad 4/5 .lib symbol format.
  - **CCad Architectural Decision**: CCad does not natively load legacy .lib files into memory during operation. Instead, ccad sch migrate converts them headlessly to modern .kicad_sym formats for the unified library manager to consume.
- **eeschema/netlist_exporters/**: Contains third party netlist exporters (Allegro, Cadstar, IPC-D-356).
  - **CCad Architectural Decision**: All netlist generators are purely headless kernel plugins in ccad_core/exporters/netlist.

## EESchema IO Plugins & Printing
- **eeschema/printing/**: UI dialogs and wxWidgets wxPrintout wrappers for hardware printing.
  - **CCad Architectural Decision**: Removed. Printing is handled headlessly via the Plotter exporter to PDF, which the user can then print via their system viewer.
- **eeschema/sch_io/sch_io.cpp/h**: SCH_IO defines the abstract interface for loading/saving schematic files and libraries.
- **eeschema/sch_io/sch_io_mgr.cpp/h**: SCH_IO_MGR is the plugin factory that registers parsers (e.g., KiCad S-Expr, Legacy, Altium, Eagle).
  - **CCad Architectural Decision**: CCad replicates this exact structure natively in ccad_core/schematic_io. This guarantees that CCad can leverage KiCad's mature third-party schematic importers simply by porting the parser plugins.

## EESchema Simulation (SPICE)
- **eeschema/sim/ngspice.cpp/h**, **simulator.h**: The core bindings to the libngspice shared library.
- **eeschema/sim/simulator_frame.cpp/h**: The wxWidgets window for plotting SPICE waveforms.
  - **CCad Architectural Decision**: CCad isolates the ngspice engine bindings into ccad_core/simulation. The simulation engine is fully headless (ccad sch simulate --type AC). The plotting UI (simulator_frame) is rebuilt natively in ccad_gui using QtCharts, completely decoupled from the netlist generation loop.

## EESchema Symbol Editor
- **eeschema/symbol_editor/symbol_edit_frame.cpp/h**: SYMBOL_EDIT_FRAME : public SCH_BASE_FRAME manages the UI and interactive tools for drawing library symbols. 
  - **CCad Architectural Decision**: CCad leverages its decoupled architecture here beautifully. Because ccad_core/library natively understands LIB_SYMBOL and the ccad_core/settings_manager unified the library formats, the ccad_gui/symbol_editor will literally just reuse the ccad_gui/schematic_editor drawing canvas, constrained to a single ccad::SchSymbol AST node instead of a full ccad::Schematic AST tree.

## EESchema Hierarchical Sheet Synchronization
- **eeschema/sync_sheet_pin/**: Contains sheet_synchronization_agent and sheet_synchronization_model. This logic is responsible for ensuring that a parent sheet's SCH_SHEET_PIN symbols match the SCH_HIERLABEL (Hierarchical Labels) defined inside the child schematic file.
  - **CCad Architectural Decision**: This is a pure AST transformation and validation task. CCad moves this completely into ccad_core/schematic/synchronizer, which executes headlessly via ccad sch sync-pins. The UI dialogs are replaced by standard Model-View diff tables in ccad_gui.


## EESchema Tools: Footprint Assignment Back-Annotation
- **eeschema/tools/assign_footprints.cpp**: SCH_EDITOR_CONTROL::AssignFootprints() parses the cvpcb_netlist S-expression (via KiCad DSNLEXER + Boost.PropertyTree) and back-annotates footprint fields on SCH_SYMBOL objects. Also contains processCmpToFootprintLinkFile() for legacy .cmp files and ImportFPAssignments() which pops a wxFileDialog + wxSingleChoiceDialog for field-visibility preference.
  - **CCad Architectural Decision**: CCad separates back-annotation into a pure kernel transaction: ccad sch back-annotate-footprints <netlist.json> . The kernel reads the footprint map, resolves symbol references from the flat SCH_REFERENCE_LIST equivalent in ccad_core, and issues a batch Modify transaction. No GUI dialog needed at the kernel level. The ccad_gui wraps this as a menu action that shows a simple visibility-toggle checkbox before calling the kernel command.


## EESchema Tools: Back-Annotation Engine
- **eeschema/tools/backannotate.cpp**: The BACK_ANNOTATE class is the full schematic-from-PCB sync engine. Key methods:
  - getPcbModulesFromString(): Parses KiCad's pcb_netlist S-expression (DSNLEXER + Boost.PropertyTree) into PCB_FP_DATA structs with pin-to-net maps.
  - getChangeList(): Matches PCB footprints to SCH_REFERENCE_LIST entries (by timestamp path or reference), building a changelist of (SCH_REFERENCE, PCB_FP_DATA) pairs.
  - applyChangelist(): Applies reference, value, footprint, DNP, excludeFromBOM, and other-fields changes to each SCH_SYMBOL. Handles SKIP_STRUCT flag to prevent double-applying after unit swaps.
  - PlanBackannotateUnitSwaps(): Standalone function that solves the unit-swap problem for multi-unit symbols (e.g. logic gate arrays) using a cycle-detection algorithm on pin-net matching.
  - applyPinSwaps(): For symbols with swappable pins (e.g. differential pairs), physically swaps LIB_PIN geometry using SwapPinGeometry(), then rebuilds wires via SCH_LINE_WIRE_BUS_TOOL.
  - processNetNameChange(): When a net rename is needed, finds the highest-priority driver (SCH_LABEL > SCH_GLOBAL_LABEL > power pin) via graph walk and renames or adds a label.
  - FetchNetlistFromPCB(): Fetches live netlist from open pcbnew via Kiway ExpressMail (MAIL_PCB_GET_NETLIST). Requires non-standalone mode.
  - **CCad Architectural Decision**: CCad separates this into two pure kernel commands: (1) ccad sch back-annotate --netlist <file> --options <json> which performs all field/ref/net changes as an AST transaction with dry-run support; (2) ccad sch back-annotate --unit-swap which runs the PlanBackannotateUnitSwaps cycle-detection algorithm. The KIWAY ExpressMail IPC is replaced by a ccad ipc fetch-pcb-netlist command. All reporting goes through the structured REPORTER pattern already in ccad_core.
### Sprint 565 follow-up: provider and end-to-end agent gate

Implement provider-neutral request/response transport, explicit approval-aware tool loop, async queue worker, context snapshot plus revision delta, and redacted provider error handling. Add tests using a fake provider transport before any live-key test. Then validate one real human prompt through context assembly, provider response, approved CCad mutation, returned tool result, revision refresh, and DRC. Do not call the bespoke newline JSON-RPC surface MCP until a standards-compatible stdio/HTTP MCP server and external-client interoperability test exist.

- [ ] Sprint 566 follow-up: reduce orchestrator cold-start latency and expose startup/provider adapter readiness separately from verified network connectivity; add timeout/error state visible in AgentPanel.

- [ ] Sprint 567 follow-up: add provider-backed response test and approved tool-loop response-result continuation; local fallback is not AI completion.

- [ ] Sprint 568 follow-up: make mock provider emit deterministic approved tool call, feed tool result back into graph, then assert project revision and DRC in one end-to-end harness test.

- [ ] Sprint 569 follow-up: route mock tool_call through AgentPanel broker, return tool result to Python with request correlation, and verify approved mutation plus DRC.

- [ ] Sprint 570 follow-up: consume 	ool_result in Python, correlate by call ID, resume graph only after broker response, and require approval for low-mutation tools.

- [ ] Sprint 571 follow-up: retain pending run state by call_id and resume graph from tool_result instead of only acknowledging it; add approval deny/error continuation tests.

- [ ] Sprint 572 follow-up: add redaction tests for Langfuse/LangSmith callback configs, capture run IDs in AgentPanel, and verify opt-in traces against a local fake collector before external export.

- [ ] Sprint 573 follow-up: add provider request timeout/retry policy and redacted failure classification; verify with local fake transport before real API call.

- [ ] Sprint 575 follow-up: add broker wait timeout/cancellation and preserve unrelated inbound requests; enforce approval before waiting/executing mutation; add real provider integration test.

- [ ] Sprint 576 follow-up: refactor Windows pipe protocol into one reader/dispatcher with bounded broker wait, cancellation, and queued inbound messages; test timeout and late result.

- [ ] Sprint 577 follow-up: add cancellation token and pending-call registry; test late result, unrelated request preservation, approval denial, and broker crash.
- [x] Sprint 582: approval Cancel now returns correlated `approval_canceled`; durable LangGraph checkpoint/resume and cancellation tokens remain open.

- [ ] Sprint 578 follow-up: connect AgentPanel accept/decline decision to a scoped approval token/config, then test allowed-after-approval and denied mutation without bypassing core policy.

- [ ] Sprint 579 follow-up: AgentPanel accept creates one-shot scoped approval token; consume token on exactly one matching mutation; decline/cancel clears it.
- [x] Sprint 583: broker wait preserves unrelated inbound messages; pending-call registry and late-result/cancellation tests remain open.
- [x] Sprint 584: unique per-invocation broker call IDs prevent retry/late-result cross-wiring; durable pending-call registry remains open.
- [x] Sprint 585: track and zone agent tools now await correlated broker results; footprint/schematic mutation tools and durable pending-call registry remain open.
- [x] Sprint 586: footprint and schematic mutation tools await correlated broker results; durable pending-call registry and real provider integration remain open.
- [x] Sprint 587: process-local pending-call registry routes exact tool results and cleans up terminal calls; durable cross-process persistence remains open.
- [x] Sprint 588: propagate stable LangGraph thread identity; durable checkpointer and cross-process state persistence remain open.
- [x] Sprint 589: pin compatible LangGraph SQLite checkpoint dependencies; compile-time/runtime checkpointer wiring and restart proof remain open.
- [x] Sprint 590: opt-in SQLite LangGraph checkpointer wired with two-process restart proof; GUI session binding and approval interrupt resume remain open.
- [x] Sprint 591: GUI session thread ID propagates to Python graph; checkpoint DB selection, resume UI, and approval interrupt restoration remain open.
- [x] Sprint 592: GUI Resume queries checkpoint state with explicit disabled response; interrupted graph continuation and approval restoration remain open.
- [x] Sprint 593: GUI surfaces checkpoint resume result; actual interrupted-tool continuation and approval interrupt restoration remain open.
- [x] Sprint 594: wire checkpointed graph resume through protocol and GUI; interrupt-producing tool adapter coverage remains open.
- [x] Sprint 595: checkpoint-enabled mutating tools use durable LangGraph interrupts; add restart integration fixture and approval decision UI automation next.
- [x] Sprint 596: add two-process interrupt/restart/resume proof; approval UI automation and live provider contract test remain open.
- [x] Sprint 597: expose semantic GUI targets for agent resume status; automate live approval interaction next.
- [x] Sprint 598: regression-test approval pending/decline/cancel UI states; live provider approval round-trip remains open.
- [x] Sprint 599: surface redacted provider adapter initialization failures; live network/provider round-trip remains open.
- [x] Sprint 600: report successful BYOK provider readiness; authenticated live request and cost/latency trace remain open.
- [x] Sprint 601: document current MCP stdio boundary; MCP exposure of selected GUI-map tools remains future work.
- [x] Sprint 602: expose read-only harness/workspace context through MCP; approved GUI-map mutation bridge remains future work.
- [x] Sprint 603: add read-only stdio MCP bridge to live GUI-map socket; approval-mediated GUI mutation tool remains future work.
- [x] Sprint 604: declare read-only MCP GUI safety metadata; approval-mediated mutation bridge remains future work.
- [x] Sprint 605: MCP can stage native approval card without execution; add live pipe round-trip and accepted mutation result test next.
- [x] Sprint 606: live-pipe MCP query verified and approval staging made truthful; expose approval controls in live GUI-map next.
- [x] Sprint 607: expose live approval controls and verify MCP staging; human decision roundtrip for a checkpointed mutation remains next.
- [x] Sprint 608: verify live MCP staging plus native human Decline and authoritative GUI-map state; checkpointed mutation execution remains next.
- [x] Sprint 610: prove provider-backed chat execution with mock adapter; live external-provider call and approved tool execution remain environment-dependent.
- [x] Sprint 611: empty API-key save clears process-held provider secret; live external-provider call remains environment-dependent.
- [x] Sprint 612: add explicit session-only Test Provider action; real network validation remains user-environment dependent.
- [x] Sprint 613: replace hardcoded context counter with client-side estimate; provider token accounting remains unavailable until a real adapter call.
- [x] Sprint 614: prevent premature tool-result acknowledgement before human approval; live checkpointed mutation proof remains next.
- [x] Sprint 615: checkpoint restart fixture proves accepted and denied outcomes; full GUI-to-checkpoint mutation integration remains next.
- [x] Sprint 618: GUI-map `ui.type_text` supports Agent chat QTextEdit; live checkpointed mutation proof remains next.
- [x] Sprint 616: CI runs provider mock and checkpoint acceptance/denial fixtures; external-provider network test remains intentionally absent.
- [x] Sprint 619: live harness types into Agent chat through GUI map; provider-backed send and approved mutation remain environment-dependent.
- [x] Sprint 620: live harness sends through mock provider; visible response inspection and approval-gated mutation remain next.
- [x] Sprint 621: GUI map exposes confirmed chat response state; approval-gated provider tool mutation remains next.
- [x] Sprint 622: approval card contextual visibility and scoped contrast fixed; richer quick replies and approval mutation proof remain next.
- [x] Sprint 623: MCP approval staging adapted to contextual card visibility; approved mutation execution remains next.
- [x] Sprint 624: slash palette entries now match executable parser syntax and popup styling is readable; quick-action semantics remain next.
- [x] Sprint 625: guided PCB chat actions insert editable commands; provider-backed approved mutation remains next.
- [x] Sprint 626: offline provider tool call now traverses supervisor/router, approval, and real via mutation; authenticated external-provider proof remains environment-dependent.
- [x] Sprint 628: native screenshot rendering is authoritative; exact-window capture is fallback-only and the behavior is covered by the harness policy test.
- [ ] Sprint 629: connect authenticated provider tool calls to the same scoped approval-token contract and prove one live request in a user-owned environment.
- [x] Sprint 630: provider initialization contract check passes with local-compatible env config, correct model, enabled adapter, and redacted state; full local stub tool-call test remains next.
- [x] Sprint 631: localhost OpenAI-compatible stub verifies model/base URL, tool schema, supervisor/router request path, and CCad tool-call boundary; CI wired.
- [ ] Sprint 632 follow-up: make local provider GUI approval fixture deterministic; current provider-boundary and offline mock proofs remain green.
- [x] Sprint 633: Agent panel surfaces tool-result acknowledgment and live provider proof asserts accepted status; richer trace/cost metrics remain open.
- [x] Sprint 634: run/trace telemetry reaches semantic Agent labels with honest unavailable cost/token markers; external Langfuse/LangSmith export remains opt-in.
- [x] Sprint 635: remove visible telemetry chips; retain hidden harness diagnostics and verify dark composer through native visual harness.
- [x] Sprint 636: dry-run tool previews bypass mutation approval; full CTest 91/91 and native screenshot proof pass.
- [x] Sprint 637: provider settings use stable adapter IDs and expose compatible/local providers; GUI settings test 9/9 and native harness proof pass.
- [x] Sprint 641: deterministic live harness waits on approval/run state and proves combined mock provider chat-to-mutation flow.
- [x] Sprint 642: localhost OpenAI-compatible provider GUI fixture proves provider-to-approval-to-mutation flow locally; CI wiring deferred until native Windows GUI runner exists.
- [x] Sprint 644: MCP bridge read-only policy contract runs in cross-platform agent CI; live Windows GUI pipe remains local-only.
- [x] Sprint 646: branch audit confirms no stale branches; provider harness terminal-state variants validated.
- [x] Sprint 648: checkpointed tool-result resume rejects mismatched call IDs before graph continuation.
- [x] Sprint 649: checkpoint dependency/runtime versions aligned; interrupt, resume, and denial fixtures pass.
- [x] Sprint 651: separate all-zero-position schematic symbols for readable review rendering; preserve explicit positions; full Qt CTest and visual proof pass.
- [x] Sprint 652: approval card appears only for real mutating commands; read-only/dry-run policy clears stale card; full Qt CTest and visual proof pass.
- [x] Sprint 653: ContextBuilder emits versioned project envelope with explicit mutation/secret constraints; focused and full Qt verification pass.
- [x] Sprint 654: IntakeLayer risk scan blocks risky goals before decomposition/provider execution; full Qt verification pass.
- [x] Sprint 655: intake rejects prompt-injection and credential-bearing goal markers; full Qt verification pass.
- [x] Sprint 656: revalidate hardened intake gate with official harness, screenshot inspection, and full Qt CTest 91/91.
- [x] Sprint 657: provider chat emits opaque context revision/change state without content telemetry; boundary test and visual harness pass.
- [x] Sprint 658: bounded redacted provider retries recover from transient HTTP failure without retrying tools; fixture and full Qt verification pass.
- [x] Sprint 659: terminal provider errors stay inside chat protocol with redacted failure state; no tool execution; full Qt verification pass.
- [x] Sprint 660: CI Python syntax gate added and CMake builds parallelized across platforms; workflow YAML and focused tests verified.
- [x] Sprint 725: bound LangGraph agent tool-loop recursion to protect provider quota; GUI validation paused by explicit user instruction.
- [x] Sprint 726: bound interactive agent history to limit context/token growth; durable memory compaction remains future work.
- [x] Sprint 727: bound incoming project-context size and mark truncation; semantic context compaction remains future work.
- [x] Sprint 728: wire agent context/loop contracts into CI Python gate; authenticated provider and GUI validation remain separate.
- [x] Sprint 730: report project-context truncation state without emitting content; active semantic compaction remains next.
- [x] Sprint 732: stop broad PNS candidate rejection from falsely blocking clear routes; exact geometry remains authoritative.
- [x] Sprint 736: expose active context budget beside truncation metadata; no content leakage.
- [x] Sprint 737: add bounded provider request timeout; live connectivity remains user-environment dependent.
- [x] Sprint 738: make `/cc`/`/compact` truthful local history compaction without provider quota use; GUI validation remains paused.
- [x] Sprint 739: wire compaction and timeout contracts into agent CI gate; no GUI/provider network execution.
- [x] Sprint 740: add deterministic C++ context revision plus pinned constraints and compaction metadata; GUI validation remains paused.
- [x] Sprint 741: add local memory CRUD and explicit `/memory` commands with secret rejection; no provider/GUI execution.
- [x] Sprint 743: inject enabled project STM memories through the bounded context gate; cap eight entries/1000 chars; GUI paused.
- [x] Sprint 745: ignore local memory/session files so custom-path private notes cannot enter commits.
- [x] Sprint 746: require explicit scope/all for memory clearing; bare clear is safe no-op.
- [x] Sprint 749: expose scope/title prefixes on memory add so scoped CRUD is usable.
- [x] Sprint 750: add scoped memory listing and synchronize command help surfaces.
- [x] Sprint 751: add update-by-ID to complete local memory CRUD.
- [x] Sprint 752: distinguish provider tool approval wait from completed telemetry state; no GUI/network run.
- [x] Sprint 753: emit approval telemetry at the mutating-tool broker boundary before waiting for client result; headless provider proof remains quota-safe and GUI validation stays paused.
- [x] Sprint 754: revalidate accepted and denied durable LangGraph checkpoint resumes with fresh SQLite fixtures; GUI and external-provider execution remain paused.
- [x] Sprint 755: add correlated headless broker cancellation and explicit late-result rejection; GUI and external-provider execution remain paused.
- [x] Sprint 756: expose opaque `agent.pending_calls` recovery metadata for process/checkpoint state without tool arguments or secrets.
- [x] Sprint 758: repair pinned provider dependencies, absolute venv discovery, single-stream selectable chat, and current-account key reveal semantics; GUI runtime validation remains paused.
- [x] Sprint 760: add CI smoke coverage for Cerebras OpenAI-compatible adapter initialization without network or quota use.
- [x] Sprint 761: rerun the full agent-python contract subset inside the bundled venv, including checkpoint accept/deny restart proof.
- [x] Sprint 763: document the quota-safe Cerebras BYOK setup and bundled-venv launch path.
- [x] Sprint 764: add no-GUI regression coverage for selectable chat stream, venv activation, and current-account password reveal.
- [x] Sprint 765: harden Windows current-account password validation and clear the temporary password buffer.
- [x] Sprint 766: reconcile provider/model presets with first-party catalogs and remove the stale Cerebras `qwen-3-32b` fallback; update compatibility docs and no-network contracts in the same change.
- [x] Sprint 767: classify provider failures into actionable safe categories while preserving quota and secret protections; add no-network contract coverage.
- [x] Sprint 768: remove stale Anthropic, Gemini, and OpenAI runtime fallback IDs; enforce current defaults with no-network contract coverage.
- [x] Sprint 769: synchronize GUI preset tests and Gemini quickstart with current documented IDs; remove retired Gemini 2.5 presets.
- [x] Sprint 770: explicitly disable submodules and enable clean checkout in every CI job to prevent stale submodule cleanup failures.
- [x] Sprint 771: add fake-client execution proof for bounded transient retries and immediate quota/rate-limit stop.
- [x] Sprint 772: wire provider safety and clean-checkout contracts into CI agent gate.
- [x] Sprint 773: add OpenRouter first-class provider wiring with custom model IDs and quota-safe no-network contract.
- [x] Sprint 774: document OpenRouter BYOK launch, custom model IDs, and quota-safe environment cleanup.
- [x] Sprint 775: add explicit bounded OpenRouter model catalog refresh with redacted metadata/failures and no-network contract.
- [x] Sprint 776: harden catalog refresh against malformed timeout environment and invalid JSON shape.
- [x] Sprint 777: expose explicit model refresh in the discoverable agent method catalog.
- [x] Sprint 778: make unsupported-provider model catalog capability responses explicit and quota-safe.
- [x] Sprint 779: preserve delimiter-containing custom model IDs in `/set` parsing.
- [x] Sprint 780: publish discoverable parameter and response schema for model refresh.
- [x] Sprint 781: wire model-selector boundary regression into the CI agent gate.
- [x] Sprint 782: reconcile Cerebras model presets with the official supported-model catalog.
- [x] Sprint 783: gate provider catalog/runtime/documentation consistency in CI.
- [x] Sprint 784: restore officially documented Gemini 2.5 text presets.
- [x] Sprint 785: connect explicit model catalog refresh to Settings dropdown.
- [x] Sprint 786: exercise model catalog protocol branches without network access.
- [x] Sprint 787: scope Settings model refresh action to supported dynamic catalogs.
- [x] Sprint 788: repair Windows password-dialog compilation and synchronize the
  physical GUI harness with the current Cerebras provider row; GUI verification
  paused again by user request after targeted execution.
- [x] Sprint 789: add value/text-based UI-map combo selection and update the
  physical provider/model harness; GUI verification remains user-paused.
- [x] Sprint 790: publish semantic combo selectors in UI input schemas; GUI
  verification remains user-paused.
- [x] Sprint 791: return redacted terminal provider state for Test Provider
  missing-key/adapter failures; GUI verification remains user-paused.
- [x] Sprint 792: make Settings save preferences through one atomic config
  message; GUI verification remains user-paused.
- [x] Sprint 793: extend restart persistence proof across security, memory, and
  personalisation settings; GUI verification remains user-paused.
- [x] Sprint 794: make curated model selectors read-only while preserving custom
  IDs for endpoint-backed providers; GUI verification remains user-paused.
- [x] Sprint 795: align Anthropic GUI preset ordering with runtime default and
  remove stale `claude-fable-5-1`; GUI verification remains user-paused.
- [x] Sprint 796: guard model-catalog callback against torn-down provider/model
  controls; GUI verification remains user-paused.
- [x] Sprint 797: clear model-catalog callback during Settings teardown to stop
  late responses invoking a destroyed dialog; GUI verification remains paused.
- [x] Sprint 798: remove obsolete per-message chat bubble fallback and enforce
  one selectable Agent stream; GUI verification remains paused.
- [x] Sprint 799: remove dead per-message user/tool bubble CSS after single
  stream migration; GUI verification remains paused.
- [x] Sprint 800: remove duplicate dead bubble/tool CSS from review-window
  stylesheet and cover both UI sources; GUI verification remains paused.
- [x] Sprint 801: preserve repeated user prompts/assistant responses while
  deduplicating only repeated backend warning notices; GUI remains paused.
- [x] Sprint 802: move activity events into single chat stream and remove
  per-event checklist widget boxes; GUI verification remains paused.
- [x] Sprint 803: remove redundant outer chat scroll wrapper and make the single
  QTextBrowser own scrolling; GUI verification remains paused.
- [x] Sprint 804: remove stale include left by chat renderer cleanup; GUI
  verification remains paused.
- [x] Sprint 805: remove duplicate QTextBrowser include after stream
  consolidation; GUI verification remains paused.
- [x] Sprint 806: expose separate Python backend readiness from provider
  adapter readiness for harnesses; GUI verification remains paused.
- [x] Sprint 807: bridge backend/provider readiness into native Agent workspace
  state without visible telemetry chips; GUI verification remains paused.
- [x] Sprint 808: add subprocess proof for backend readiness payload and
  no-network redaction; GUI verification remains paused.
- [x] Sprint 809: wire backend readiness subprocess proof into CI agent gate;
  GUI verification remains paused.
- [x] Sprint 834: add bundled-venv offline agent contract runner plus static
  exclusion contract; headless MCP bridge included, GUI/live-provider tests
  excluded, user pause/quota boundaries active.
- [x] Sprint 835: wire offline gate exclusion contract into CI; no GUI or live
  provider execution added.
- [x] Sprint 836: reconcile Cerebras runtime/UI presets with current public
  catalog and remove deprecated Qwen default; no network or GUI execution.
- [x] Sprint 837: expose quota-safe Cerebras model snapshot through explicit
  catalog refresh; OpenRouter remains network-fetched only.
- [x] Sprint 838: publish provider-specific model-catalog network capabilities
  for external harnesses; no GUI or provider request.
- [x] Sprint 839: clarify unsupported model-catalog provider error and cover
  unknown-provider response; no GUI or provider request.
- [x] Sprint 840: synchronize stale OpenRouter catalog contract with the
  unsupported-provider diagnostic; no GUI or network execution.
- [x] Sprint 841: publish model-catalog provider enum for harness discovery;
  no GUI or provider request.
- [x] Sprint 842: synchronize OpenRouter catalog contract with provider enum;
  no GUI or network execution.
- [x] Sprint 843: align the Cerebras adapter contract test with the current
  production default; no GUI or provider request.
- [x] Sprint 844: expose initial/changed/unchanged context transition kind
  alongside the existing opaque revision; no GUI or provider request.
- [x] Sprint 845: publish context-state response fields through method
  discovery; no GUI or provider request.
- [x] Sprint 846: verify context-state method discovery at runtime, not only
  through source inspection; no GUI or provider request.
- [x] Sprint 847: publish intake-state response fields through method
  discovery; no GUI or provider request.
- [x] Sprint 848: publish provider-state response fields through method
  discovery; no GUI or provider request.
- [x] Sprint 849: publish tool-call response fields through method discovery;
  no GUI or provider request.
- [x] Sprint 850: align tool-call discovery with the actual emitted `tool`
  field; no GUI or provider request.
- [x] Sprint 851: align provider-state discovery with emitted readiness/error
  keys; no GUI or provider request.
- [x] Sprint 852: align tool-call response fields with actual emitted payload;
  no GUI or provider request.
- [x] Sprint 853: parse mock tool-call wire events and assert exact payload
  shape; no GUI or provider request.
- [x] Sprint 854: emit actionable redacted provider error categories; no GUI
  or provider request.
- [x] Sprint 855: guard provider_state error-category emission directly; no
  GUI or provider request.
- [x] Sprint 856: capture a runtime provider failure event and prove
  classification/redaction; no GUI or provider request.
- [x] Sprint 857: advertise provider_state in human_message responses; no GUI
  or provider request.
- [x] Sprint 858: synchronize intake guardrail contract with response-list
  discovery; no GUI or provider request.
- [x] Sprint 859: advertise backend-state response schema; no GUI or provider
  request.
- [x] Sprint 860: advertise message response schema; no GUI or provider
  request.
- [x] Sprint 861: advertise tool-canceled response schema; no GUI or provider
  request.
- [x] Sprint 903: isolate session provider keys and prove provider-switch cleanup without network calls.
- [x] Sprint 904: restore active adapter after transient provider probe.
- [x] Sprint 905: refresh Cerebras model presets from first-party public catalog docs.
- [x] Sprint 906: expose model-catalog provenance to agent consumers.
- [x] Sprint 907: expose OpenRouter catalog provenance and endpoint in agent responses.
- [x] Sprint 909: make track tool dry-run behavior match approval policy.
- [x] Sprint 910: make polygon/zone tool dry-run behavior match approval policy.
- [x] Sprint 911: prove track/zone dry-run fields reach provider tool schemas.
- [x] Sprint 912: guard approval pane visibility so idle chat has no approval card.
- [x] Sprint 913: expose dry-run preview for footprint and schematic-symbol placement.
- [x] Sprint 926: add provider/model selector concatenation regression coverage; GUI execution deferred until authorized.
- [x] Sprint 927: add explicit native core CTest allow-list; prevent accidental GUI test execution.
- [x] Sprint 928: document fast native gate and rebuild boundary.
- [x] Sprint 929: self-check native gate allow-list against GUI/provider test prefixes.
- [x] Sprint 930: canonicalize provider IDs for model-catalog requests.
- [x] Sprint 931: advertise provider-ID normalization in model-catalog discovery.
- [x] Sprint 932: reject non-string model-catalog provider parameters explicitly.
- [x] Sprint 933: label static OpenAI/Anthropic/Gemini model presets as non-live.
- [x] Sprint 934: label Cerebras model list as official curated snapshot.
- [x] Sprint 935: suppress known LangGraph checkpoint import warning narrowly.
- [x] Sprint 938: restore canonical `drill_nm` pad fields during project JSON
  loading; serializer round-trip and official GUI harness verified.
- [x] Sprint 939: make explicit Cerebras model refresh use the authenticated
  provider catalog instead of returning a static snapshot.
- [x] Sprint 939: align the native Cerebras settings hint with the explicit
  refresh behavior so users are not told the live catalog is offline.
- [x] Sprint 940: emit a provider-specific terminal validation result so a
  restored background adapter cannot overwrite Settings feedback.
- [ ] Add a separate explicitly confirmed, budget-capped live connection probe.
  It must not change `Validate Provider Setup`, which intentionally makes no
  network request and consumes no quota.
- [x] Extend explicit model refresh beyond OpenRouter/Cerebras to the documented
  OpenAI, Anthropic, and Gemini catalog APIs; retain manual model entry for
  unknown OpenAI-compatible and local endpoints.
- [x] Validate every first-party catalog parser against controlled provider-shaped
  responses, including its authentication header and malformed payload handling.
- [x] Add an explicit Set key action that persists the selected provider key
  safely and activates the selected provider/model without requiring dialog close.
- [x] Report a terminal key-activation result instead of optimistic Settings
  success text, keeping provider readiness truthful.
- [x] Prevent module import from initializing a provider or emitting backend IPC;
  initialize the adapter only in the launched agent process.
- [x] Update Gemini credential regression coverage for the selection-scoped
  `provider_test_result` protocol introduced by provider validation.
- [x] Sprint 972: preserve quota exhaustion separately from rate limits,
  expose bounded retry timing in explicit provider-probe status, and verify the
  local no-network validation flow through mapped Settings controls.
- [ ] Provider follow-up: remove the pinned Gemini adapter's implicit retries
  only through a supported dependency/API upgrade; do not patch installed SDK
  files. Keep live-provider verification explicitly initiated and quota-aware.
- [ ] Sprint 976 follow-up: migrate compatible checkpoint-only history into the
  canonical thread store without losing thread identity or tool-call edges.
- [ ] Sprint 976 follow-up: connect durable conversation listing/resume UI and
  verify checkpoint restoration against the per-thread model projection.
- [x] Sprint 989: replace overlap-only memory/TurnRecord ranking with bounded
  BM25, search same-project compact recaps before source-linked turns, preserve
  exact conversation pointers, and reject weak matches. Offline contracts and
  full build/CTest evidence are tracked in the Sprint 989 TODO/log.
- [x] Sprint 1000: add opt-in bounded semantic PCB/schematic context retrieval
  through the existing local Ollama backend, with lexical candidate priority,
  process-only signature-keyed vectors, exact/graph/spatial provenance, safe
  context status, and exact/lexical fallback. Qt Release/full CTest passed
  115/115; real-model relevance validation remains open because Ollama had no
  installed model.
- [ ] Memory follow-up: preference/correction evidence, richer extraction, and
  provider-tokenizer budgeting. Memory semantic retrieval and deterministic
  field fusion/diversity are implemented; this remaining item does not imply
  BM25 alone provides semantic recall.
- [ ] Sprint 1000 follow-up: validate opt-in semantic project retrieval against
  a real installed Ollama model and representative PCB/schematic paraphrase
  queries; generated summaries, durable vectors, and retrieval-quality metrics
  remain unimplemented. Offline backend-contract tests do not establish model
  relevance quality.
- [x] Sprint 988: add project-scoped durable LTM storage/retrieval; safe project
  identity, isolation, manifest counts, namespace-aware compaction invalidation,
  and reset semantics are covered by offline contracts.
- [ ] Continue the revision-aware exact/lexical/graph/spatial project index
  before claiming full PCB/schematic state retrieval.
- [ ] Sprint 986 follow-up: finish source-supported project-graph identities,
  transaction-delta index maintenance, and representative retrieval-quality
  gates. Bounded bounding-box queries, linked DRC/ERC region retrieval, all
  current typed PCB layer-bearing collections, unchanged-snapshot reuse, and a
  10k-track benchmark are implemented. The benchmark still shows full snapshot
  extraction on one-object edits; rule IDs and other absent kernel objects are
  not invented.
- [x] Sprint 978: join context assembly, retrieval, package, and LangGraph
  observations under one thread-session-scoped `agent.turn` root; verify actual
  Langfuse SDK parent links with a local in-memory exporter.
- [ ] Langfuse follow-up: verify a fresh opt-in production-account trace/readback
  and preserve trace identity across approval interruption and resume.
- [ ] Sprint 977 follow-up: add explicit preference/correction weighting and
  provider-tokenizer-based global budget allocation; current memory ranking and
  token counts are deterministic lexical/character estimates.
