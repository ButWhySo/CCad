# CCad Consolidated Backlog

This file is the single local backlog for scattered CCad feature requests. Sprint files remain the execution record, while this backlog holds the larger map so future agents do not lose roadmap items.

## Verified Work Ledger

This ledger is the current single checklist for scattered user-reported GUI, KiCad compatibility, and agent-harness work. Check an item only after code review, focused tests, KiCad/reference comparison where relevant, and visual validation when the behavior is visible.

- [x] Sprint 157: read-only native UI map export through `ccad_gui --dump-ui-map`, with target validation against live Qt hit-testing.
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

- [ ] Sprint 223 cleanup: after verifying merged branch state, list local and remote sprint branches, delete only branches fully merged into `main`, and never rewrite history or delete unmerged/user work.
- [ ] Sprint 225 backlog and parity control plane: normalize `docs/devops/backlog.md`, `docs/devops/progress.md`, and the active sprint file so the KiCad parity mandate is sprint-bounded, removes duplicated raw text, and records the source-walk ledger format.
- [ ] Sprint 225 repo-state hygiene: record current branch, dirty files, untracked helper files, and ownership assumptions before touching behavior code.
- [ ] Sprint 225 parity ledger schema: define fields for KiCad source path, CCad analogue, external references, behavior decision, tests, GUI visual proof, documentation links, unsupported diagnostics, and status.
- [ ] Sprint 225 phase budget: define the next bounded parity phase budget so PCB editor, schematic editor, external formats, Gerber, 3D, project model, library verification, GUI map, agent harness, and autorouter work cannot drift indefinitely.
- [ ] Sprint 226 PCB editor root model slice: walk `F:\kicad_src\pcbnew` root files in stable order, starting with board/model/settings/commit/connectivity files, and map them to CCad board state, transactions, design settings, object metadata, loader readiness, stackup, statistics, board-item container mutation, text variables, legacy board BOM output, cleanup action discovery, collector behavior, outline polygon conversion, cross-probing, and DRC visibility. First tested sub-slice maps `board_design_settings.cpp` into shared CCad design-rule validation used by DRC. Second tested sub-slice maps `board_item.cpp` and `include\board_item.h` into shared KiCad `BOARD_ITEM` metadata for pads, vias, tracks, graphics, texts, and zones returned by PCB object queries. Third tested sub-slice maps `board_loader.cpp`, `board_loader.h`, and KiCad board-loader tests into `ccad_core/board_loader.hpp/.cpp` plus `pcb load-state` for agent-visible board load readiness. Fourth tested sub-slice maps `board_stackup_manager\board_stackup.cpp`, `board_stackup.h`, dielectric material sources, and stackup reporter sources into `ccad_core/board_stackup.hpp/.cpp` plus a physical `pcb get-board-stackup` report with stackup items and copper layer distances. Fifth tested sub-slice maps `board_statistics.cpp`, `board_statistics.h`, `board_statistics_report.cpp`, and KiCad board-statistics tests into `ccad_core/board_statistics.hpp/.cpp` plus `pcb drill-statistics` for pad/via drill row aggregation. Sixth tested sub-slice maps `board_item_container.h` into `ccad_core/board_item_container.hpp/.cpp` plus KiCad-shaped `pcb remove-object` delete/remove metadata and `--mode normal|bulk`. Seventh tested sub-slice maps `board_statistics_report.cpp/.h` into `BoardStatisticsReport` plus `pcb board-statistics` for agent-visible board dimensions, counts, minimums, board thickness, and drill rows. Eighth tested sub-slice maps `board_text_var_adapter.cpp/.h` and KiCad text-variable API handlers into `ccad_core/board_text_var_adapter.hpp/.cpp`, project `text_variables`, and `project set-text-variable`, `project list-text-variables`, and `pcb expand-text-variables`. Ninth tested sub-slice maps `build_BOM_from_board.cpp` into placed `BoardFootprint` metadata, KiCad footprint `exclude_from_bom` preservation, and `pcb export-board-bom` for KiCad legacy board-side BOM CSV export. Tenth tested sub-slice maps `cleanup_item.cpp/.h` into `ccad_core/cleanup_item.hpp/.cpp` plus `pcb cleanup-actions` for KiCad `CLEANUP_ITEM` action catalog and `VECTOR_CLEANUP_ITEMS_PROVIDER` row semantics. Eleventh tested sub-slice maps `collectors.cpp/.h` locked-item suppression into persisted board-object lock flags, collector row metadata, and `pcb collect-items --ignore-locked true`, with CLI project-write truncation hardening fixed in the same sprint after the serializer failure reproduced. Twelfth tested source-walk sub-slice maps `convert_shape_list_to_polygon.cpp/.h` into `ccad_core/board_outline_polygon.hpp/.cpp` plus `pcb outline-polygon` for first Edge.Cuts line-chain reporting and rectangular inference fallback. Thirteenth tested source-walk sub-slice maps `cross-probing.cpp` into `ccad_core/cross_probing.hpp/.cpp` plus `pcb cross-probe` for first KiCad-style packet resolution across board and schematic targets. Fourteenth tested source-walk sub-slice maps the remaining PCB layout objects (pcb_barcode.cpp, pcb_dimension.cpp, pcb_group.cpp, pcb_reference_image.cpp, pcb_table.cpp, pcb_target.cpp, pcb_text.cpp, pcb_textbox.cpp) into ccad_core/model.hpp structs BoardBarcode, BoardDimension, BoardGroup, BoardReferenceImage, BoardTable, BoardTarget, and BoardText with full JSON serialization and rendering. Fifteenth source-walk sub-slice audited pcb_design_block_utils.cpp and pcb_field.cpp, documenting them as UI abstraction and deferred custom footprint fields respectively. Sixteenth and final source-walk sub-slice audited pcbexpr_evaluator.cpp, pcbexpr_functions.cpp, tracks_cleaner.cpp, zone.cpp, zone_filler.cpp, zone_settings.cpp, zone_utils.cpp, documenting that their complex evaluation algorithms and UI behaviors are deferred since their core data structures (text variables, cleanup items, basic BoardZone) were successfully mapped in prior phases.
- [x] Sprint 227 PCB item geometry slice: map KiCad pads, padstacks, vias, tracks, arcs, graphics, text, zones, keepouts, drills, annular rings, mask, paste, courtyard, fab, and silk behavior to CCad model, render, import, export, and query surfaces.
- [ ] Sprint 228 PCB editor interaction slice: implement KiCad-like select, measure, route, add via, add zone, draw graphic, place text, delete, move, resize, edit properties, context menu, tooltip, Escape-cancel, and placement-ghost behavior through human GUI and agent APIs.
- [ ] Sprint 229 PCB Appearance/Layers/Objects slice: bring layer colors, active layer, visible layer, selected object, selection filters, net filters, right dock, bottom diagnostics, and toolbar/icon behavior closer to KiCad.
- [ ] Sprint 230 PCB footprint/library placement slice: make Add Footprint use cache-backed catalogue loading, lazy selected-footprint parse, KiCad-like chooser layout, footprint preview, layer-aware ghost placement, Escape cancellation, metadata, and future 3D model hooks.
- [ ] Sprint 231 schematic document and symbol model slice: walk `F:\kicad_src\eeschema` root files in stable order and map symbols, units, pins, pin leads, fields, labels, power symbols, wires, sheets, no-connects, and no-pins diagnostics into CCad.
- [ ] Sprint 232 schematic interactions/dialogs slice: implement KiCad-like Add Symbol, power symbol chooser, annotation, ERC, property dialogs, context menus, placement ghost, Escape cancellation, and symbol preview behavior through GUI and agent APIs.
- [ ] Sprint 233 schematic-PCB association slice: implement symbol-footprint links, netlist sync, cross-probing, update-PCB-from-schematic behavior, and agent-visible mismatch diagnostics.
- [ ] Sprint 234 external EDA provider audit: walk KiCad import/export/provider folders and map each provider or file-format capability to a CCad provider registry, including unsupported fields and round-trip fixture needs.
- [ ] Sprint 235 external provider implementation batch: implement the first bounded provider batch with parse/export tests, provenance, compatibility diagnostics, and CLI/agent commands.
- [ ] Sprint 236 manufacturing format expansion: plan and implement Gerber/job, drill, IPC-2581, ODB++, BOM, PnP, assembly drawing, stackup, and approval-gate improvements in tested batches.
- [ ] Sprint 237 Gerber viewer parity: walk `F:\kicad_src\gerbview` file by file and map layers, apertures, drill overlays, measurement, selection, visibility, rendering, CLI evidence, and visual validation into CCad.
- [ ] Sprint 238 3D viewer parity: walk `F:\kicad_src\3d-viewer` file by file and map board stackup, footprints, 3D model paths, transforms, materials, cameras, screenshots, and agent evidence into CCad.
- [ ] Sprint 239 multi-document project model: support one project containing many schematics and many PCBs, with explicit links, unlinked states, project tree UI, CLI/API queries, and transaction metadata.
- [ ] Sprint 240 linked/unlinked DRC/ERC intelligence: run physical-only DRC for unlinked boards, schematic-only ERC for unlinked schematics, and cross-document diagnostics only when project links exist.
- [ ] Sprint 241 project-level transactions and audit: extend diffs, audit logs, rollback, agent context, and evidence manifests across multiple boards and schematics.
- [ ] Sprint 242 library-cache inventory harness: enumerate KiCad symbol, footprint, and 3D model source files and compare them against CCad cache entries with pass/fail/unsupported status.
- [ ] Sprint 243 symbol losslessness harness: verify aliases, inheritance, units, pins, pin electrical types, fields, graphics, footprints, keywords, descriptions, provenance, and LLM-native metadata for every symbol batch.
- [ ] Sprint 244 footprint losslessness harness: verify pads, drills, annular rings, pad shapes, copper layer sets, mask, paste, courtyard, fab, silk, properties, 3D references, and provenance for every footprint batch.
- [x] Sprint 245 KiCad Footprint Oval Drill Support: Support KiCad oval drills in the footprint importer, JSON serialization/deserialization, and footprint losslessness verification. This resolves the dominant footprint import failure.
- [ ] Sprint 246 low-latency live Qt map: replace slow polling paths with push-style map deltas, local socket or WebSocket subscription, semantic ID caches, stale-epoch detection, and nonblocking UI-thread ownership.
- [ ] Sprint 247 canvas spatial indexes: add fast nearest-object and hit-test structures for large boards, including board-space to screen-space transforms under pan, zoom, resize, and device-pixel-ratio changes.
- [ ] Sprint 248 deterministic GUI action tools: implement `ui.double_click`, richer keyboard input, drag, scroll, wait-for-dialog, dialog automation, menu/context-menu automation, properties editing, and target validation.
- [ ] Sprint 249 prompt and tool-guide assets: write feature-scoped prompt, tool, command, verification, retry, and failure-classification guides for the agent layer as each parity feature lands.
- [ ] Sprint 250 durable agent runner: persist queues, own worker threads outside the Qt UI thread, stream tool-loop progress, enforce approvals, support retries, and write rollback or compensation records.
- [ ] Sprint 251 observability and BYOT: add real OpenTelemetry span emission, Langfuse/OTLP export links, redaction, opt-in controls, trace IDs, cost/token metadata, and no-secret configuration handling.
- [ ] Sprint 252 classic autorouter baseline: establish deterministic no-agent autorouter quality metrics, route fixtures, DRC proof, clearance/manufacturing checks, and regression thresholds.
- [ ] Sprint 253 agent-guided autorouter planning: let the agent diagnose constraints, prioritize nets, suggest route plans, propose DRC repairs, and produce evidence without directly degrading board state.
- [ ] Sprint 254 supervised autorouter loop: add approval gates, route-quality regression checks, rollback, evidence artifacts, and acceptance criteria around agent-assisted autorouting.

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

- [ ] Stub: `TestZoneIntersection` (deferred from `edit_zone_helpers.cpp` until `zone.cpp` is processed)

- [ ] Stub: `net_chain_bridging.cpp` (advanced router bridging length calculation)
- [ ] Stub: `padstack.cpp` (KiCad 8 complex multi-layer padstack models)
- [ ] Stub: `graphics_cleaner.cpp` (duplicate/overlapping shape cleanup)

- [x] Stub: `pcb_barcode.cpp` (Barcode rendering)
- [x] Stub: `pcb_design_block_utils.cpp` (Design blocks; skipped as UI logic)
- [x] Stub: `pcb_dimension.cpp` (Dimensioning objects)
- [x] Stub: `pcb_field.cpp`, `pcb_fields_grid_table.cpp` (Property fields; deferred custom fields to BoardFootprint)
- [ ] Stub: `pcb_generator.cpp` (Plugin generators)
- [x] Stub: `pcb_group.cpp` (Object grouping)
- [x] Stub: `pcb_plot*.cpp`, `plot_*.cpp` (Gerber, PDF, SVG plotting)
- [x] Stub: `pcb_reference_image.cpp` (Reference image overlays)
- [x] Stub: `pcb_table.cpp`, `pcb_tablecell.cpp` (Table objects)
- [x] Stub: `pcb_target.cpp` (Alignment targets)
- [x] Stub: `pcb_text.cpp`, `pcb_textbox.cpp` (Standalone text primitives)
- [x] Stub: pcbexpr_evaluator.cpp, pcbexpr_functions.cpp (Text substitution engine; deferred as CCad basic text var adapter is complete)
- [x] Stub: tracks_cleaner.cpp (Track merging and cleanup logic; algorithmic, deferred)
- [x] Stub: zone.cpp, zone_filler.cpp, zone_settings.cpp, zone_utils.cpp (Copper pours and zone algorithms; basic BoardZone is complete)


- [ ] Stub: `annotate.cpp` (Schematic automatic reference designator annotation)
- [ ] Stub: `autoplace_fields.cpp` (Heuristics to auto-arrange property text fields)
- [ ] Stub: `bus-wire-junction.cpp` (Visual junctions for intersecting wires)
- [ ] Stub: `connection_graph.cpp` (Core connectivity graph algorithm for schematic nets)


- [ ] Stub: `gfx_import_utils.cpp` (Schematic DXF/SVG import utilities)
- [ ] Stub: `junction_helpers.cpp` (Visual junction creation heuristics for schematic intersections)


- [ ] Stub: `sch_symbol.cpp`, `sch_pin.cpp`, `sch_line.cpp`, `sch_junction.cpp`, `sch_label.cpp`, `sch_text.cpp`, `sch_bus_entry.cpp`, `sch_sheet.cpp` (Core schematic primitives)
- [ ] Stub follow-up: extend Sprint 232's embedded symbol snapshot fix into full KiCad `SCH_SYMBOL`/`SCH_PIN` parity, including multi-unit symbols, alternate pin functions, field placement/autoplace, body conversion edge cases, sheet paths, and library-reference refresh without losing local snapshots.
- [ ] Stub: `sch_connection.cpp`, `sch_netchain.cpp`, `net_navigator.cpp` (Hierarchical schematic net connectivity algorithms)


- [ ] Stub: `am_param.cpp`, `am_primitive.cpp`, `aperture_macro.cpp` (Gerber aperture macros and parameters)
- [ ] Stub: `dcode.cpp` (Gerber D-Code tool definitions)
- [ ] Stub: `evaluate.cpp` (Expression evaluation for aperture variables)
- [ ] Stub: `excellon_read_drill_file.cpp` (Excellon drill file parsing)
- [ ] Stub: `gbr_layout.cpp` (Gerber layer and layout models)


- [ ] Stub: `gerber_draw_item.cpp`, `gerber_file_image.cpp`, `gerber_file_image_list.cpp` (Gerber graphic primitives and document image model)
- [ ] Stub: `readgerb.cpp`, `rs274x.cpp`, `rs274d.cpp`, `X2_gerber_attributes.cpp`, `job_file_reader.cpp` (Gerber RS-274X syntax and attributes parsers)
- [ ] Stub: `gerber_to_polyset.cpp` (Translating Gerber flashes to geometric polygons)
- [ ] Stub: `gerber_collectors.cpp`, `gerber_diff.cpp` (Hit testing and file comparison)


- [ ] Stub: `3d_fastmath.cpp`, `3d_math.cpp` (3D coordinate transformations and fast approximations)
- [ ] Stub: `3d_cache/` (STEP/IGES/WRL model parsing and caching engine)
- [ ] Stub: `3d_canvas/`, `3d_rendering/` (OpenGL scene graph, materials, and raytracing engine)


- [ ] Stub: `cvpcb/read_netlist.cpp`, `cvpcb/listboxes.cpp` (Parsing schematic netlists for footprint assignment)
- [ ] Stub: `cvpcb/auto_associate.cpp` (Heuristics for automatically assigning footprints based on symbol properties)


- [ ] Stub: `pcbnew/autorouter/ar_autoplacer.cpp`, `pcbnew/autorouter/spread_footprints.cpp` (Heuristics for packing and spreading footprints)
- [ ] Stub: `pcbnew/router/pns_algo_base.cpp`, `pcbnew/router/pns_index.cpp`, `pcbnew/router/pns_node.cpp` (Push and Shove graph theory and spatial indexes)
- [ ] Stub: `pcbnew/router/pns_dragger.cpp`, `pcbnew/router/pns_diff_pair_placer.cpp`, `pcbnew/router/pns_meander_placer.cpp` (Algorithms for shoving tracks, routing differential pairs, and length matching)


- [ ] Stub: `pcbnew/pcb_io/allegro`, `altium`, `cadstar`, `eagle`, `easyeda`, `easyedapro`, `fabmaster`, `geda`, `pads`, `pcad`, `sprint_layout` (Importers for external 3rd-party EDA formats)


- [ ] Stub: `pagelayout_editor` (Custom drawing sheet and title block editor)
- [ ] Stub: `pcb_calculator` (RF transmission line, E-series, and track current calculators)
- [ ] Stub: `bitmap2component` (Raster image to PCB geometric footprint converter)

- [ ] Stub: Agent Orchestration Settings UI and backend logic to store and reflect user settings in appropriate fields, and tweak base agent-orchestrator behavior according to these settings.
