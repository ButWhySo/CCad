# Project Progress Counter

This file is the canonical phase/sprint counter for local CCad agent work.

## Current Position

-New idea/thought, maybe we could implment the functionality of instead of using fixed otel, maybe let user input their otel for whatever otel we use(like langfuse) and instead route the otel to their langfuse, instead of loading our own, this empowers users and orgs?
- Phase: 7 / 7 (Phase 6 complete)
- Phase name: Final Polish & Release
- Sprint: 186
- Branch: `sprint-186-agent-harness-foundation`
- Phase 6 sprint budget: Sprints 136 through 140 planned. (Completed)

Phase 6 focused on Agent protocol: JSON-RPC/MCP over the transaction bus, audit logs, permission gates, benchmark harness.

**Current State**: 
- **Sprint 148 (KiCad GUI Parity)** is complete and merged to `main`. It added KiCad-style icon toolbars, GUI footprint/symbol placement flows, and shape-aware pad/drill rendering.
- **Sprint 149 (Pad Shape and Layer Fidelity)** is complete and merged to `main`. It carries KiCad pad shape metadata through import, placement, project JSON, KiCad export, diffs, and GUI rendering.
- **Sprint 150 (Standard Layer Registry Fidelity)** is complete and merged to `main`. It aligns CCad's standard layer order and KiCad PCB export layer numbers with current KiCad source and exposes canonical layer numbers to agent-facing layer queries.
- **Sprint 151 (KiCad Pad Authoring CLI)** is complete and merged to `main`. It exposes KiCad-style pad type, shape, drill, roundrect ratio, chamfer ratio, and multi-layer authoring through `ccad pcb add-pad` and `ccad pcb set-pad`.
- **Sprint 152 (Rich Pad Query Contract)** is complete and merged to `main`. It exposes KiCad-style pad metadata through `ccad pcb get-object` and `ccad pcb list-objects --type pad` so agents can plan from command output instead of parsing raw project JSON.
- **Sprint 153 (Route-Job Pad Metadata)** is complete and merged to `main`. It carries KiCad-style pad type, shape, drill, ratio, layer, and rotation metadata into `ccad pcb export-route-job` so external routers and AI tools see the same pad geometry intent.
- **Sprint 154 (GUI Actions and Library Cache Placement)** is complete and merged to `main`. It replaced top-toolbar placeholders with real Save, Board Setup, Undo, Redo, Run DRC, and DRC export behavior, made the library chooser more KiCad-like, and fixed converted symbol inheritance so library-cache symbol placement gets real pins.
- **Sprint 155 (KiCad Placement Chooser and Ghost Placement)** is complete and merged to `main`. It removed raw file-preview workflows from the user-facing placement path, routed Add Symbol/Add Footprint by active editor tab, used local `library-cache` chooser data, added cursor-following placement ghosts, and fixed layer color and selected-track visibility problems.
- **Sprint 156 (Lazy Library Chooser Loading)** is complete on `sprint-156-lazy-library-chooser`. It fixes Add Symbol/Add Footprint hangs by indexing `library-cache` entries cheaply and deferring actual symbol or footprint parsing until a row is selected for details/preview or placed.
- **Sprint 157 (Read-only GUI UI Map)** is complete on `sprint-157-readonly-ui-map`. It adds an app-owned semantic UI map dump and live target validation for future LLM/native automation tooling, with stable action IDs, tab and canvas bounds, canvas object metadata, route/layer/net context, and target coordinates proven by Qt hit-testing.
- **Sprint 158 (UI Target Queries)** is complete on `sprint-158-ui-target-queries`. It adds app-owned targeted coordinate queries by semantic ID and board-space point so agents can request one actionable target without dumping the full UI map.
- **Sprint 159 (Safe UI Actions)** is complete on `sprint-159-safe-ui-actions`. It adds app-owned direct triggering for a small allowlist of safe view/navigation actions and explicit refusal for unsafe or mutating actions.
- **Sprint 160 (Placement Crash, CI Fixes, Icon Recovery, and Backlog Ledger)** is complete and merged to `main`. It fixes left-click placement reload crashes, CI compile failures in KiCad symbol import and DSN export, robust KiCad SVG icon lookup, app-owned placement-click regression coverage, and the consolidated backlog ledger.
- **Sprint 161 (UI Map Mouse Target Harness)** is complete and merged to `main`. It adds menu/panel nodes to the semantic UI map and an app-owned mouse-target screenshot harness with beep, fast policy waits, marker overlays, and resize-repeat evidence.
- **Sprint 162 (Pad Layer Rendering Fidelity)** is complete and merged to `main`. It separates copper, solder-mask, and solder-paste pad rendering across the board canvas, footprint chooser preview, and footprint placement ghost.
- **Sprint 163 (Live UI Map Server)** is complete and merged to `main`. It adds a local socket JSON Lines server for repeated `ui.map`, `ui.target`, and `ui.epoch` queries while the GUI stays open.
- **Sprint 164 (Toolbar Action Contracts)** is complete on `sprint-164-toolbar-action-contracts`. It removes silent right-toolbar stubs by giving unfinished editor tools visible planned-tool status and an agent-readable `future_tool_not_implemented` safe-trigger response.
- **Sprint 165 (Left Toolbar Action Contracts)** is complete on `sprint-165-left-toolbar-action-contracts`. It extends the no-silent-stubs contract to unfinished left-toolbar display and panel controls.
- **Sprint 166 (Left Toolbar Real Toggles)** is complete on `sprint-166-left-toolbar-real-toggles`. It turns Show Layers and Show Properties into real panel toggles with `panel_toggled` safe-trigger responses.
- **Sprint 167 (Native Agent Panel Shell)** is complete on `sprint-167-agent-panel-shell`. It adds the first persistent Qt Agent panel for UI-map refreshes, safe UI action triggers, and `panel:agent` targeting without provider or secret integration. It also updates the GUI visual-validation timing policy to 7 seconds for single-preview screenshots and 5 seconds initial plus 800 ms per-target waits for multi-target GUI harness runs.
- **Sprint 168 (Left Toolbar Display Controls)** is complete on `sprint-168-left-toolbar-display-controls`. It turns grid, polar coordinates, inch units, crosshair, ratsnest, net highlight, and display mode into real safe display actions, exposes checked state in the UI map, extends the target-sequence harness to exercise those controls, and fixes stale inspector editor overlap during rapid multi-selection changes.
- **Sprint 169 (Visual Harness Timing Policy)** is complete on `sprint-169-visual-harness-timing`. It enforces the updated 7-second single-preview timing and 5-second initial plus 800 ms per-action multi-target timing through a dedicated CTest policy guard, updates the live interaction wrapper, and fixes chooser-row targeting so live footprint preview checks actually select a catalogue row.
- **Sprint 170 (PCB Edit Tool Entry)** is complete on `sprint-170-pcb-edit-tool-entry`. It promotes Add Via, Route Track, Add Keepout, and Delete from right-toolbar planned-tool status to real kernel-backed GUI edit entries with matching agent/test hooks, while keeping Add Zone, Draw Graphic, and Place Text as explicit planned tools until their durable board primitives exist. Focused GUI-map coverage passed, official visual validation produced a full bridge-rectifier screenshot, app-owned viewport hooks placed a via, track, and keepout before deleting the new via, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 171 (PCB Active Layer Context)** is complete on `sprint-171-pcb-active-layer-context`. It adds a top-toolbar active copper-layer selector, agent-readable/settable active-layer JSON, live UI-map socket active-layer methods, and active-layer-aware footprint placement plus Route Track commits. Focused GUI-map coverage passed, official visual validation produced `artifacts/screenshots/sprint171-pcb-active-layer-context-final-20260603-010814.png`, app-owned active-layer hooks returned `F.Cu`, set `B.Cu`, and resolved `control:active_pcb_layer`, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 172 (PCB Active Net Context)** is complete on `sprint-172-pcb-active-net-context`. It adds a top-toolbar active net selector, agent-readable/settable active-net JSON, live UI-map socket active-net methods, `control:active_pcb_net` targeting, and active-net-aware Add Via plus Route Track commits. Focused GUI-map coverage passed, official visual validation produced `artifacts/screenshots/sprint172-pcb-active-net-context-final-20260603-012426.png`, app-owned active-net hooks returned `AC1`, set `DC_NEG`, and resolved `control:active_pcb_net`, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 173 (PCB Graphics and Text Tools)** is complete on `sprint-173-pcb-graphics-text-tools`. It adds durable `BoardGraphic` and `BoardText` primitives, CLI authoring and compact queries, DRC/diff/KiCad PCB export support, real Draw Graphic and Place Text GUI edit modes, object-browser and inspector support, hidden-layer-safe defaults, and the official bridge-rectifier visual proof at `artifacts/screenshots/sprint173-pcb-graphics-text-tools-final-20260603-021600.png`. Focused tests passed, the official visual screenshot was inspected, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 174 (PCB Zone Tool First Slice)** is complete on `sprint-174-pcb-zone-tool`. It adds durable KiCad-compatible first-slice `BoardZone` primitives with CLI `pcb add-zone`, compact query/list and route-job exposure, DRC/diff/KiCad PCB export support, real Add Zone GUI edit mode, object-browser and inspector support, app-owned zone placement through the Qt viewport event path, and the official bridge-rectifier zone visual proof at `artifacts/screenshots/sprint174-pcb-zone-tool-final-20260603-031321.png`. Focused tests passed, the official visual screenshot was inspected, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 175 (Agent Panel Live UI Map)** is complete on `sprint-175-agent-panel-live-map`. It connects the native Agent panel to the same live UI-map protocol as the local socket, adds `ui.map_delta`, `ui.find`, `ui.target_board_point`, and `ui.trigger_safe` live methods, exposes Agent method/payload/action controls as targetable UI-map nodes, parses live socket requests with Qt JSON APIs, and ingests `docs/req_agentHarness.md` into the local backlog. Focused tests passed, official visual validation produced `artifacts/screenshots/sprint175-agent-panel-live-map-final-20260603-102936.png`, the target harness proved Agent controls in `artifacts/screenshots/sprint175-agent-panel-live-map-targets-final-target-sequence.json`, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 176 (UI Map Compact Deltas)** is complete on `sprint-176-ui-map-compact-deltas`. It adds `ui.map_compact`, `ui.role_summary`, `ui.hit_test`, and `ui.nearest_canvas_object` to the shared Agent-panel/live-socket dispatcher, changes stale `ui.map_delta` responses to compact nodes instead of full nested maps, keeps current-epoch deltas empty, and covers direct plus socket routing in GUI-map tests. Focused tests passed, official visual validation produced `artifacts/screenshots/sprint176-ui-map-compact-deltas-final-20260603-110523.png`, the target harness proved semantic coordinates in `artifacts/screenshots/sprint176-ui-map-compact-deltas-targets-final-target-sequence.json`, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 177 (Agent UI Interaction Tools)** is complete on `sprint-177-agent-ui-interaction-tools`. It adds the first direct semantic interaction tools on top of the shared Agent-panel/live-socket dispatcher: dry-run click targeting, safe action/tab clicks, Agent-control focus and text entry, Escape cancellation, PCB canvas-object selection, selection inspection, and bounded UI epoch waits. Focused `gui_ui_map` and `gui_agent_panel` tests passed, official visual validation produced `artifacts/screenshots/sprint177-agent-ui-interaction-tools-final-20260603-112907.png`, the target harness proved semantic coordinates in `artifacts/screenshots/sprint177-agent-ui-interaction-tools-targets-final-target-sequence.json`, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 178 (Map-Driven Viewport Input)** is complete on `sprint-178-map-driven-viewport-input`. It adds `ui.canvas_click` and `ui.canvas_drag` to the shared Agent-panel/live-socket dispatcher so agents can resolve PCB board-space coordinates and send real Qt viewport mouse events through the same GUI tool event path as human clicks. Focused `gui_ui_map` and `gui_agent_panel` tests passed, official visual validation produced `artifacts/screenshots/sprint178-map-driven-viewport-input-final-20260603-115354.png`, the target harness proved semantic coordinates in `artifacts/screenshots/sprint178-map-driven-viewport-input-targets-final-target-sequence.json`, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 179 (Agent PCB Workflow Tools)** is complete on `sprint-179-agent-pcb-workflow-tools`. It adds higher-level Agent-panel/live-socket workflow methods `ui.current_tool`, `ui.cancel_tool`, `ui.place_via`, `ui.route_track`, `ui.add_zone`, `ui.add_keepout`, `ui.draw_graphic`, `ui.place_text`, and `ui.delete_object`, all composed from safe action activation plus real viewport input or canvas-object selection. Focused `gui_ui_map` and `gui_agent_panel` tests passed, official visual validation produced `artifacts/screenshots/sprint179-agent-pcb-workflow-tools-final-20260603-121454.png`, the target harness proved semantic coordinates in `artifacts/screenshots/sprint179-agent-pcb-workflow-tools-targets-final-target-sequence.json`, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 180 (Agent Project Evidence Tools)** is complete on `sprint-180-agent-project-evidence-tools`. It adds `ui.screenshot`, `project.context`, `project.object_counts`, `project.review`, `project.erc`, `project.drc`, and `project.diagnostics` to the shared Agent-panel/live-socket dispatcher so agents can capture app-owned GUI evidence and query loaded-project health without shelling out or scraping files. Focused `gui_ui_map`, `gui_agent_panel`, and `review` tests passed, official visual validation produced `artifacts/screenshots/sprint180-agent-project-evidence-tools-final-20260603-123654.png`, the target harness proved semantic coordinates in `artifacts/screenshots/sprint180-agent-project-evidence-tools-targets-final-target-sequence.json`, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 181 (UI Map Dirty Deltas and Canvas Index)** is complete on `sprint-181-ui-map-dirty-index`. It upgrades the Agent-panel/live-socket UI-map surface with first-slice dirty semantic IDs, changed roles, bounded `ui.wait_for_delta`, and indexed `ui.nearest_canvas_object` metadata over rendered selectable canvas objects. Focused `gui_ui_map` and `gui_agent_panel` tests passed, official visual validation produced `artifacts/screenshots/sprint181-ui-map-dirty-index-final-20260603-131018.png`, the target harness proved semantic coordinates in `artifacts/screenshots/sprint181-ui-map-dirty-index-targets-final-target-sequence.json`, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 182 (UI Map Index Cache and Delta Watch)** is complete on `sprint-182-ui-map-index-cache`. It adds cached UI-node ID/role indexes, indexed `ui.index_stats`, `ui.get_node`, and `ui.nodes_by_role` agent lookups, bounded dirty-event history, and `ui.watch_delta` so agents can read semantic GUI changes without repeated full map dumps. Focused `gui_ui_map` and `gui_agent_panel` tests passed, official visual validation produced `artifacts/screenshots/sprint182-ui-map-index-cache-final-20260603-133922.png`, the target harness proved semantic coordinates in `artifacts/screenshots/sprint182-ui-map-index-cache-targets-final-target-sequence.json`, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 183 (Agent Protocol Catalog)** is complete on `sprint-183-agent-protocol-catalog`. It adds `agent.methods`, `agent.method_schema`, and `agent.quickstart` over the shared Agent-panel/live-socket dispatcher so agents can discover current GUI/project protocol methods, input-schema shapes, safety flags, dry-run support, and the recommended UI-map operating loop without scraping docs or guessing method names. Focused `gui_ui_map` and `gui_agent_panel` tests passed, official visual validation produced `artifacts/screenshots/sprint183-agent-protocol-catalog-final-20260603-140218.png`, the target harness proved semantic coordinates in `artifacts/screenshots/sprint183-agent-protocol-catalog-targets-final-target-sequence.json`, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 184 (Library Placement Stability)** is complete on `sprint-184-library-placement-stability`. It adds persisted symbol snapshots for placed schematic components, schematic scene rendering from stored symbol primitives and pin leads, `ccad sch place-symbol`, concrete chooser row identity metadata, schematic-only GUI tab selection, and clean `ccad_gui --screenshot-project` visual capture. The full Qt build passed, CTest passed 36 of 36 tests, official visual validation produced `artifacts/screenshots/sprint184-library-placement-stability-shipping-20260603-152917.png`, target-sequence validation produced `artifacts/screenshots/sprint184-library-placement-stability-targets-shipping-target-sequence.json`, and schematic screenshot validation produced `artifacts/screenshots/sprint184-symbol-schematic-visual-shipping.png`.
- **Sprint 185 (KiCad Symbol Catalog Scale)** is complete on `sprint-185-kicad-symbol-catalog-scale`. It expands raw KiCad `.kicad_sym` libraries into top-level symbol chooser rows, carries `extends` metadata in chooser selections, uses the selected item name for lazy preview/final symbol loading, and adds core plus GUI regression tests. The full Qt build passed, CTest passed 36 of 36 tests, official visual validation produced `artifacts/screenshots/sprint185-kicad-symbol-catalog-scale-shipping-20260603-160243.png`, chooser visual validation produced `artifacts/screenshots/sprint185-kicad-symbol-chooser-shipping.png`, and target-sequence validation produced `artifacts/screenshots/sprint185-kicad-symbol-catalog-scale-targets-shipping-target-sequence.json`.
- **Sprint 186 (Agent Harness Foundation)** is complete on `sprint-186-agent-harness-foundation`. It moves the Agent panel to a right-side dock while preserving `panel:agent` and `tab:agent`, adds read-only GUI `agent.*` harness metadata for session state, run profile, safety, provider, observability, evidence manifest, and tool guides, and exposes matching headless CLI metadata commands plus JSON-RPC routes. Focused `gui_ui_map` and `agent_serve` tests passed, official visual validation produced `artifacts/screenshots/sprint186-agent-harness-foundation-rightsplit-20260604-150605.png`, target validation produced `artifacts/screenshots/sprint186-agent-harness-foundation-rightsplit-targets-target-sequence.json`, and the full build plus CTest gate passed 36 of 36 tests.

## Phase Roadmap

1. Logical kernel: project, components, pins, nets, constraints, ERC, CLI.
2. Physical primitives: board outline, layers, pads, vias, tracks, keepouts, placement regions, early DRC.
3. Routing assistance: constrained route requests and external router boundary.
4. Native GUI/editor: schematic and PCB review/edit surfaces, diffs, diagnostics, and transaction review.
5. Interop: KiCad, Circuit JSON, DSN/SES, SPICE/simulation hooks, manufacturing exports.
6. Agent protocol: JSON-RPC/MCP over the transaction bus, audit logs, permission gates, benchmark harness.

## Recently Completed

Sprint 65 completed the Phase 2 hardening batch for board validity and authoring safety. It added stricter ERC identity checks, DRC checks for board outline, net/member integrity, layer identity, physical object identity, pad logical parity, full-geometry board-edge checks, full-geometry keepout checks, and CLI authoring guards for edge overhangs and cross-type physical ID reuse.

Sprint 66 has added physical placement regions as first-class board primitives. They now round-trip through deterministic JSON, can be authored through `ccad pcb add-placement-region`, appear in inspect/review counts, are checked by DRC for identity, kind, size, board bounds, and physical ID reuse, and are visible in the Qt canvas/object browser/demo path.

Sprint 67 has started with board layer authoring. The first target is `ccad pcb add-layer`, giving agents a deterministic way to add layers after board initialization instead of editing JSON manually.

Sprint 67 completed board layer authoring through `ccad pcb add-layer`, including duplicate ID checks, optional visibility parsing, CLI tests, README usage, and demo coverage.

Sprint 68 has started board-level DRC rule configuration. The target is serialized design rules plus `ccad pcb set-rules` so copper clearance, minimum track width, and minimum via annular ring are no longer hardcoded behavior.

Sprint 68 completed board-level DRC rule configuration. Design rules now serialize with the board, `ccad pcb set-rules` updates them deterministically, and DRC obeys configured copper clearance, minimum track width, and via annular ring values.

Sprint 69 has started board outline mutation. The target is `ccad pcb set-outline` with guards that reject outlines which would leave existing board primitives or regions outside the board.

Sprint 69 completed board outline authoring through `ccad pcb set-outline`, including object-containment guards, CLI tests, README usage, and demo coverage.

Sprint 70 has started layer-aware visibility. The target is to make layer `visible` data affect review rendering, expose `ccad pcb set-layer-visibility`, and keep the object/layer browser clear about hidden layers.

Sprint 70 completed layer-aware visibility. CanvasScene now carries layer visibility, the Qt canvas hides pads and tracks on hidden layers while keeping vias visible, the object browser labels layer visibility, `ccad pcb set-layer-visibility` toggles existing layers, and the demo script exercises the command.

## Active Sprint

Sprint 71 has started origin-aware canvas rendering. The target is to make the GUI-independent canvas scene preserve board outline origin and make the Qt renderer draw pads, tracks, vias, keepouts, and placement regions relative to that origin.

Sprint 71 completed origin-aware canvas rendering. CanvasScene carries board outline origin, the Qt renderer draws board primitives and regions relative to that origin, tests verify origin metadata and rendered item coordinates, and the demo uses a non-zero-origin outline.

Sprint 72 has started layer-aware DRC hardening. The target is to keep copper clearance and connectivity checks faithful to physical layer semantics: pads and tracks connect/collide only on shared copper layers, while vias remain cross-layer copper.

Sprint 72 completed layer-aware DRC hardening. Track-to-pad endpoint contact now requires a shared copper layer, different-layer pads/tracks can overlap without clearance diagnostics, and vias still check clearance across layers.

Sprint 73 has started copper-layer authoring guards. The target is to reject pads, tracks, and placed footprint pads on non-copper layers through both DRC diagnostics and CLI mutation guards.

Sprint 73 completed copper-layer authoring guards. DRC now reports pads and tracks on non-copper layers, and the CLI rejects add-pad, add-track, and place-footprint requests that target non-copper layers.

Sprint 74 has started board-origin review reporting. The target is for ProjectReview and `ccad inspect` to report the full board outline rectangle, including origin and size, so agents can reason about shifted board coordinates without opening raw project JSON.

Sprint 74 completed board-origin review reporting. ProjectReview now carries board origin coordinates, and `ccad inspect` emits `x_nm`, `y_nm`, `width_nm`, and `height_nm` in board metadata.

Sprint 75 completed GUI board-origin summary display. The Qt project summary now shows non-zero board outline origin next to board size, keeps zero-origin boards compact, wraps long summary title/subtitle text inside the dock, and has a dedicated CTest panel target.

Sprint 76 completed review/inspect design-rule reporting. ProjectReview and `ccad inspect` now expose active board-level physical DRC rules, including copper clearance, minimum track width, and minimum via annular ring, so agents can reason from inspect JSON instead of raw project JSON.

Sprint 77 completed GUI design-rule summary display. The Qt project summary now shows active copper clearance, minimum track width, and minimum via annular ring values from ProjectReview near the top of the dock, so human review sees the same physical-rule context that agents get from inspect JSON.

Sprint 78 completed origin-aware cursor status. The Qt status bar coordinate readout now classifies Board versus Canvas positions using the actual board outline origin and max point, matching the origin-aware renderer and summary.

Sprint 79 completed design-rule DRC validation. DRC now reports non-positive copper clearance, minimum track width, and minimum via annular ring values when they arrive from hand-edited or imported project JSON.

Sprint 80 completed configured clearance diagnostics. `COPPER_CLEARANCE` messages now name the active board-level clearance value, so agents and humans can interpret findings without separately looking up the rule.

Sprint 81 completed rule-threshold diagnostic hardening. Minimum track-width and via annular-ring diagnostics now name the active configured threshold, matching the copper-clearance diagnostic behavior.

Sprint 82 completed via-ring diagnostic priority. DRC avoids reporting a derived annular-ring violation when the via drill already exceeds the via diameter or the via size is otherwise invalid.

Sprint 83 completed keepout diagnostic priority. DRC avoids reporting derived keepout geometry violations when the checked pad, via, or track has invalid dimensions.

Sprint 84 completed clearance diagnostic priority. DRC avoids reporting derived copper-clearance violations when the checked pad, via, or track has invalid dimensions.

Sprint 85 completed diagnostic JSON summaries. `validate` and `drc` include total, error, and warning counts next to the diagnostics array, so agents can read status without recounting the list.

Sprint 86 completed catalog diagnostic summaries. `lib catalog-validate` uses the same summary shape as `validate` and `drc`, keeping agent-facing diagnostic JSON consistent across project and catalog checks.

Sprint 87 completed catalog search summaries. `lib catalog-search` includes summary metadata with match counts while preserving its existing output fields.

Sprint 88 completed board-layer project diffs. `ccad diff` and transaction diff JSON report board layer additions, removals, and changes instead of only logical components, nets, and constraints.

Sprint 89 completed board-outline project diffs. `ccad diff` and transaction diff JSON report board outline additions, removals, and changes.

Sprint 90 completed board-pad project diffs. `ccad diff` and transaction diff JSON report board pad additions, removals, and changes.

Sprint 91 completed board route-primitive project diffs. `ccad diff` and transaction diff JSON now report board via and track additions, removals, and changes.

Sprint 140 completed Agent MCP Support.

Sprint 141 completed KiCad Component Ingestion and Rendering. It successfully ingested KiCad footprints and symbols, parsed their graphic primitives (Lines, Arcs, Circles, Polygons, Text), and natively rendered them in the CCad Qt Canvas, fully verified by automated screenshot pipelines.

Sprint 142 completed BOM Export. It emitted a CSV Bill of Materials from the `Project` model, providing a basic manufacturing output containing designators and parts.

Sprint 143 completed Pick and Place (PnP) Export. It emitted a CSV file containing component centroids and rotations from the PCB layout, facilitating board assembly.

Sprint 144 completed Drill Export. It emitted an Excellon NC Drill file from the PCB layout's vias and through-hole pads, enabling bare-board fabrication.

Sprint 92 completed board region project diffs. `ccad diff` and transaction diff JSON now report board keepout and placement-region additions, removals, and changes.

Sprint 93 completed board design-rule project diffs. `ccad diff` and transaction diff JSON now report board-level DRC rule changes.

Sprint 94 completed CLI board-diff coverage. The `ccad diff` executable test now verifies board-level physical entries for design rules, layers, placement regions, keepouts, pads, vias, and tracks.

Sprint 95 completed physical object removal authoring. `ccad pcb remove-object --file <path> --id <id>` removes pads, vias, tracks, keepouts, and placement regions by stable ID.

Sprint 96 completed board layer removal authoring. `ccad pcb remove-layer --file <path> --id <id>` removes unused board layers and rejects missing or referenced layers.

Sprint 97 completed physical object movement authoring. `ccad pcb move-object --file <path> --id <id> --x-mm <n> --y-mm <n>` moves pads, vias, keepouts, and placement regions with board-boundary guards.

Sprint 98 completed physical object resizing authoring. `ccad pcb resize-object --file <path> --id <id> --width-mm <n> --height-mm <n>` resizes pads, keepouts, and placement regions with board-boundary guards.

Sprint 99 completed track geometry editing. `ccad pcb set-track --file <path> --id <id> --start-x-mm <n> --start-y-mm <n> --end-x-mm <n> --end-y-mm <n> --width-mm <n>` updates existing track endpoints and width with board-boundary guards.

Sprint 100 completed via geometry editing. `ccad pcb set-via --file <path> --id <id> --diameter-mm <n> --drill-mm <n>` updates existing via diameter and drill with drill and board-boundary guards.

Sprint 101 completed pad metadata and rotation editing. `ccad pcb set-pad --file <path> --id <id> --component <id> --pin <name> --net <id> --layer <id> --rotation-deg <n>` updates existing pad binding, layer, and rotation with copper-layer and board-boundary guards.

Sprint 102 completed region kind editing. `ccad pcb set-region-kind --file <path> --id <id> --kind <kind>` updates existing keepout and placement-region kind values by stable ID.

Sprint 103 has started layer metadata editing. The target is `ccad pcb set-layer --file <path> --id <id> --name <name> --kind <kind> --visible true|false`, with guards that prevent referenced copper layers from being changed into non-copper metadata.

Sprint 103 completed layer metadata editing. `ccad pcb set-layer --file <path> --id <id> --name <name> --kind <kind> --visible true|false` updates existing layer name, kind, and visibility while rejecting referenced non-copper kind changes.

Sprint 104 has started route primitive metadata editing. The target is to extend `ccad pcb set-via` with optional net updates and `ccad pcb set-track` with optional net and copper-layer updates, without breaking existing geometry-only command usage.

Sprint 104 completed route primitive metadata editing. `ccad pcb set-via` accepts optional `--net`, and `ccad pcb set-track` accepts optional `--net` and `--layer`, with copper-layer validation for track layer changes.

Sprint 105 has started compact PCB object lookup. The target is `ccad pcb get-object --file <path> --id <id>`, emitting one board layer, pad, via, track, keepout, or placement region as JSON so agents can inspect stable objects without rereading the whole project file.

Sprint 105 completed compact PCB object lookup. `ccad pcb get-object --file <path> --id <id>` emits one board layer, pad, via, track, keepout, or placement region as JSON and rejects missing IDs.

Sprint 106 has started compact PCB object listing. The target is `ccad pcb list-objects --file <path> [--type <type>]`, emitting stable IDs and light metadata for board layers and physical objects without requiring agents to parse the full project file.

Sprint 106 completed compact PCB object listing. `ccad pcb list-objects --file <path> [--type <type>]` emits stable IDs and light metadata for board layers and physical objects, with optional type filters.

Sprint 107 has started compact PCB net listing. The target is `ccad pcb list-nets --file <path>`, emitting physical net usage counts for pads, vias, and tracks so agents can reason about board connectivity without parsing full project JSON.

Sprint 107 completed compact PCB net listing. `ccad pcb list-nets --file <path>` emits physical net usage counts for pads, vias, and tracks, ignoring empty net IDs.

Sprint 108 has started a CLI module split. The target is to move PCB object query JSON helpers out of `pcb_commands.cpp` into `pcb_object_queries.*` without changing behavior.

Sprint 108 completed the CLI module split. PCB object and net query JSON helpers now live in `src/ccad_cli/pcb_object_queries.hpp/.cpp`, keeping `pcb_commands.cpp` focused on command dispatch and mutation flow.

Sprint 109 has started the Phase 2 completion gate. Phase 2 is being closed because the board model, early authoring commands, DRC, diffs, query commands, and GUI review hooks are present and verified. Phase 3 opens next with constrained routing assistance and external router boundary work.

Sprint 109 completed the Phase 2 completion gate. README now marks Phase 3 as the active phase, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 110 planning.

Sprint 110 has started route-request model groundwork. The target is a deterministic board-level route-request record that captures routing intent without generating trace geometry yet.

Sprint 110 completed route-request model groundwork. `ccad::Board` now preserves deterministic `route_requests` JSON records, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 111 planning.

Sprint 111 has started route-request DRC validation. The target is to reject malformed route intent before route commands or solver integration consume it.

Sprint 111 completed route-request DRC validation. Route requests now receive identity, net, layer, endpoint, and width diagnostics, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 112 planning.

Sprint 112 has started CLI route-request authoring. The target is `ccad pcb add-route-request`, creating route intent records by stable endpoint object IDs without generating tracks.

Sprint 112 completed CLI route-request authoring. `ccad pcb add-route-request` now appends deterministic route intent records, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 113 planning.

Sprint 113 has started compact route-request listing. The target is `ccad pcb list-route-requests`, giving agents route intent summaries without parsing full project JSON.

Sprint 113 completed compact route-request listing. `ccad pcb list-route-requests` now emits route intent summaries, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 114 planning.

Sprint 114 has started route-request project diffs. The target is to make `ccad diff` and transaction diff summaries expose route intent changes.

Sprint 114 completed route-request project diffs. `ccad diff` now reports route_request additions, changes, and removals, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 115 planning.

Sprint 115 has started route-request editing. The target is `ccad pcb set-route-request`, letting agents update routing intent by stable ID without deleting and recreating records.

Sprint 115 completed route-request editing. `ccad pcb set-route-request` now updates existing route intent records by stable ID, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 116 planning.

Sprint 116 has started route-request removal. The target is `ccad pcb remove-route-request`, completing the add/list/update/remove lifecycle for route intent records before external router handoff work.

Sprint 116 completed route-request removal. `ccad pcb remove-route-request` now removes route intent records by stable ID, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 117 planning.

Sprint 117 has started external-router boundary groundwork. The target is `ccad pcb export-route-job`, emitting deterministic route-job JSON that a later router process can consume without scraping full project JSON.

Sprint 117 completed external-router boundary groundwork. `ccad pcb export-route-job` now emits compact deterministic route-job JSON, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 118 planning.

Sprint 118 has started route-job obstacle export. The target is to include keepouts and placement regions in `ccad pcb export-route-job` so future routers see board constraints, not just copper objects.

Sprint 118 completed route-job obstacle export. `ccad pcb export-route-job` now includes keepouts and placement regions, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 119 planning.

Sprint 119 has started scoped route-job export. The target is `ccad pcb export-route-job --request-id <id>`, allowing agents to hand external routers one route request at a time while preserving the same board context envelope.

Sprint 119 completed scoped route-job export. `ccad pcb export-route-job --request-id <id>` now exports one selected request or rejects missing IDs, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 120 planning.

Sprint 120 has started route-job metadata hardening. The target is to make `ccad pcb export-route-job` self-describing with schema version and unit metadata for external router consumers.

Sprint 120 completed route-job metadata hardening. `ccad pcb export-route-job` now declares schema version and units, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 121 planning.

Sprint 121 has started route-result application groundwork. The target is `ccad pcb apply-route-segment`, a minimal external-router return path that converts one satisfied route request into a board track segment.

Sprint 121 completed route-result application groundwork. `ccad pcb apply-route-segment` now converts one satisfied route request into a board track segment, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 122 planning.

Sprint 122 has started multi-segment route application groundwork. The target is `ccad pcb apply-route-segment --complete false`, letting external route results append intermediate track segments while keeping the request open until the final segment.

Sprint 122 completed multi-segment route application groundwork. `ccad pcb apply-route-segment --complete false` now leaves route requests open for later segments, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 123 planning.

Sprint 123 has started route-segment default-layer handling. The target is for `ccad pcb apply-route-segment` to use the route request's preferred layer when the external result does not provide an explicit layer.

Sprint 123 completed route-segment default-layer handling. `ccad pcb apply-route-segment` now defaults to the request preferred layer when `--layer` is omitted, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 124 planning.

Sprint 124 has started route-segment provenance. The target is for tracks created from route requests to preserve `source_route_request_id` through project JSON and route-job exports.

Sprint 124 completed route-segment provenance. Tracks now preserve `source_route_request_id` through project JSON, route-job export, and diffs, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 125 planning.

Sprint 125 has started compact track provenance queries. The target is for `ccad pcb get-object` and `ccad pcb list-objects --type track` to expose `source_route_request_id` without requiring agents to parse full project JSON.

Sprint 125 completed compact track provenance queries. Track `get-object` and `list-objects` output now include `source_route_request_id`, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 126 planning.

Sprint 126 completed Phase 3 routing assistance as a real batch rather than another tiny counter increment. The batch added route-status reporting, completed/partial/open route progress counts, stricter route-request DRC for endpoint net mismatch, same endpoint, empty policy, and too-narrow route width, multi-segment `pcb apply-route-polyline`, route-status test coverage, DRC coverage, README command documentation, and a clean scripted GUI demo that exports route-job JSON, applies a routed polyline, writes route-status JSON, waits through the beep-and-20-second screenshot harness, and shows a clean board with completed route provenance. Phase 3 is now complete because CCad has typed route intent, route-job export, route-result application, route provenance, route progress reporting, DRC validity checks, CLI tests, and visible demo evidence. The sprint-end gate passed with a clean 87-step Qt build and 21 of 21 CTest tests passing.

Sprint 127 completed the first Phase 4 native GUI/editor route-review batch. The batch adds route-request and route-progress fields to the shared review model, carries route intent and track route provenance through the GUI-independent canvas model, shows route-request counts and progress in the project summary, adds route-request rows and route provenance to the object browser, lets route rows highlight tracks created from that request, and exposes the same route progress through `ccad inspect`. The GUI screenshot harness produced `artifacts/screenshots/sprint127-gui-route-review-rebuilt-20260527-145138.png`, where routed tracks `RT.1` and `RT.2` show `route RR1` provenance. The full clean Qt build and CTest gate passed with 21 of 21 tests passing.

Sprint 128 completed the KiCad layer compatibility foundation. The batch used current KiCad and industry layer references before implementation, added a canonical KiCad named-layer registry, exposed idempotent `ccad pcb add-standard-layers`, updated the sprint demo into a clean bridge-rectifier board, and kept README/progress/sprint documentation in the same sprint. The GUI screenshot harness produced `artifacts/screenshots/sprint128-bridge-rectifier-layer-foundation-clean2-20260527-155104.png`, where the board shows 59 layers, a clean status, four diode bridge legs, AC/DC pads, routed positive and negative buses, and no diagnostics.

Sprint 129 completed layer review summaries. The batch kept the KiCad layer-display reference rule active and added review-model, inspect JSON, and GUI project-summary visibility for layer categories and layer visibility state. The GUI screenshot harness produced `artifacts/screenshots/sprint129-layer-review-summary-visible-20260527-160424.png`, where the bridge board stays clean and the project summary shows the layer breakdown card. Inspect JSON reports `layer_summary` with 32 copper layers, 27 non-copper layers, 9 visible layers, and 50 hidden layers.

Sprint 130 completed interactive layer visibility. The right-dock layer browser now renders checkable list items for each layer. Signal-blocking prevents recursion during list population, and an `itemChanged` callback updates the in-memory board model in `ReviewWindow` and immediately triggers canvas re-rendering. The GUI screenshot harness produced `artifacts/screenshots/sprint-130-demo-20260530-225412.png` showing interactive checkable layer rows. The clean build and CTest gate passed with all 22 tests passing.

Sprint 131 completed coordinate inspection helpers in the Selection Inspector Panel to display unit-converted physical properties (position, dimension, net, layer, rotation, drill, track length, etc.) for selected PCB objects (pads, vias, tracks, keepouts, placement regions) based on the kernel-mediated Board model. It displays all physical lengths in both millimeters (mm) and mils (mil). The GUI screenshot harness produced `artifacts/screenshots/sprint-demo-20260530-233238.png` demonstrating coordinate inspection. The clean build and CTest gate passed with all 22 tests passing.

Sprint 132 completed interactive physical DRC design rule and object property editing in the Selection Inspector Panel, with callback updates to mutate the `ccad::Board` model, project file auto-save, view refreshes, and selection state preservation. The GUI screenshot harness produced `artifacts/screenshots/sprint-demo-20260530-235551.png` showing interactive inspector row fields. The clean build and CTest gate passed with all 22 tests passing.

Sprint 133 completed the KiCad S-expression PCB export serialization engine (`.kicad_pcb` format) in the CCad core kernel, exposed it via the `ccad pcb export-kicad` subcommand in the CLI, added help metadata, and verified formatting parity and CLI behavior through comprehensive unit and integration tests. The clean build and CTest gate passed with all 23 tests passing.

Sprint 134 completed KiCad Footprint S-expression export (`.kicad_mod`). The batch added `exportKiCadFootprint` serialization, floating point coordinate translation, and `ccad lib export-footprint` CLI command. The clean build and CTest gate passed with all 24 tests passing.

Sprint 135 completed KiCad/Specctra DSN Export functionality and `ccad pcb export-dsn`. The clean build and CTest gate passed with all 25 tests passing.

Sprint 136 completed Agent JSON-RPC Protocol foundation. It introduced `ccad agent serve` that runs a JSON-RPC 2.0 loop on stdin/stdout, routing commands and returning execution status and redirected stdout/stderr.

Sprint 137 completed Agent Audit Logs. `ccad_cli::writeProjectFile` now seamlessly captures `Transaction` records and appends them to `<project>.audit.jsonl` whenever the CLI mutates the board.

Sprint 138 completed Agent Permission Gates. The JSON-RPC `agent serve` now enforces explicit `--allow-read` and `--allow-write` boundaries before executing commands, returning standard JSON-RPC error `-32604` for unauthorized calls.

Sprint 139 completed Agent Benchmark Harness. Introduced `scripts/benchmark.py` which runs a specified agent command against a directory of `.ccad.json` files and executes `ccad drc` to validate correctness of the agent's work.

Sprint 140 completed Agent MCP Support and closed Phase 6. The `agent serve` loop now natively speaks the Model Context Protocol (MCP) by handling `initialize`, `tools/list`, and `tools/call`, allowing seamless integration with modern LLM-driven tooling.

Sprint 147 completed Interactive Authoring. It introduced `ccad_core/placement.hpp` for core footprint placement logic, replacing inline CLI logic. It also added `FootprintPlacementDialog` to the GUI, allowing users to interactively place footprints on the board canvas through the 'Add Footprint' toolbar action.

## Reporting Rule


Every commit/merge status update should include:

```text
Progress: Phase X/6, Sprint N, <branch-or-main>, <short status> <next sprint target> <expected sprint count to complete this phase>
```

## Sprint Integrity Rule

A sprint number is not a counter for tiny isolated commits. A sprint is a real scoped batch with a maintained sprint file under `docs/devops/sprints/`, and that file must describe the sprint goal, branch, tasks, bugs or risks, documentation updates, verification commands, verification results, and closure status. Do not start or close Sprint N unless the Sprint N file exists.

Documentation changes belong to the same sprint as the behavior they describe. README updates, progress counter updates, feature inventory updates, codebase-map or handover updates, and sprint notes must be completed before the sprint is closed. They must not be pushed into a later cleanup sprint unless the user explicitly approves that exception.

Sprint closure requires focused tests during development and the full configured build plus CTest gate at the end. GUI-visible work also needs a visible artifact or app-owned screenshot unless the user explicitly blocks visual capture for that run. Record the exact verification command and result in the sprint file before marking the sprint verified.

Each CCad sprint must deliver a moderate product batch, not a tiny one-commit increment. The minimum target is 8 to 10 visible or user-meaningful features, fixes, workflow improvements, or documented capabilities per sprint, unless the user explicitly approves a smaller safety sprint. A phase must also have a limited sprint budget before work starts, with the expected sprint count reported in progress updates, so a phase cannot continue indefinitely without a deliberate scope decision.

Because full CMake and CTest verification is heavy and token-consuming in this repository, do not run the full gate after every small implementation step. Use targeted checks only when they are cheap and necessary during development. At sprint end, before committing completion work or shipping the sprint, run the full configured build and CTest gate, fix any failures, rerun the failing or full gate as appropriate, then ship only after the gate passes.

## External Reference Rule

Every CCad feature or bugfix sprint must start with current external references for the feature domain before code edits begin. For KiCad-compatible PCB/CAD behavior, official KiCad documentation and KiCad developer file-format documentation are mandatory. Manufacturing, layer-stack, DRC, and fabrication work should add Altium or IPC-style industry references when relevant. Simulation work should add KiCad ngspice and ngspice references. AI harness and observability work should add OpenTelemetry, Langfuse, LangGraph, or equivalent primary references. The sprint file must include a "References Checked" section that records the sources, the behavior they imply, CCad's compatibility decision, and the verification evidence tied to that behavior. If internet lookup fails, record the failure and use local cached docs only as a temporary fallback.

Sprint 148 is verified on branch `sprint-148-gui-kicad-parity`. The batch adds KiCad-style icon toolbars sourced from the local KiCad checkout, shape-aware pad and drill rendering for annular-ring visibility, core-owned interactive placement and movement, GUI symbol and footprint placement entry points, legacy pad `layer_id` JSON compatibility, and the official visual screenshot proof. The sprint file is `docs/devops/sprints/2026-06-01-sprint-148-gui-kicad-parity.md`. The final clean rebuild passed all 177 build steps, and CTest passed 33 of 33 tests.

Sprint 154 is complete on branch `sprint-154-gui-actions-library-cache`. The batch replaced visible GUI top-toolbar placeholders with real Save, Board Setup, Undo, Redo, Run DRC, and DRC export behavior; made the local library chooser more KiCad-like with filter, library/name rows, detail metadata, and preview notes; resolved converted symbol `extends` inheritance from sibling `library-cache` JSON so derived symbols inherit pins before placement; and accepted raw KiCad `.kicad_mod` files from the footprint chooser.

Sprint 155 is complete and merged to `main`. The batch delivered KiCad-style cache-backed symbol and footprint choosing, tab-aware Add behavior, cursor-following placement ghosts with Escape cancellation, distinct top/bottom copper colors, full-width track selection highlighting, and a consolidated backlog for GUI, KiCad compatibility, library, simulation, manufacturing, LLM, and visual-validation work. The sprint-end full clean Qt build passed all 177 build steps and CTest passed 33 of 33 tests. The official visual harness produced `artifacts/screenshots/sprint155-kicad-placement-chooser-final-20260601-211645.png`.

Sprint 156 is complete on branch `sprint-156-lazy-library-chooser`. Add Symbol and Add Footprint now open from a cheap cache catalogue, parse only the selected item for detail metadata and a visual preview, and preserve the existing final placement import path. The GUI now has app-owned chooser screenshot modes and a live mouse/keyboard harness for placement-dialog checks. Footprint previews share the actual board canvas layer palette so PCB layer intent colors remain distinct. The sprint-end full clean Qt build passed all 177 build steps and CTest passed 33 of 33 tests. The official visual harness produced `artifacts/screenshots/sprint156-lazy-library-chooser-preview-final-20260602-004023.png`. The live mouse/keyboard harness produced `artifacts/screenshots/sprint156-live-footprint-preview-focus-20260602-004533.png` and `artifacts/screenshots/sprint156-live-symbol-preview-20260602-004647.png`, confirming the chooser previews through real GUI interaction. The layer-color rerun produced `artifacts/screenshots/sprint156-app-footprint-chooser-layer-colors.png` and `artifacts/screenshots/sprint156-app-symbol-chooser-layer-colors.png`.
