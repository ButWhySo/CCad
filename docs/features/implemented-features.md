# Implemented Features

## Sprint 996 evidence-gated visual validation

The visual-validation guide now points to the repository's actual app-owned
Qt GUI-map harness, manifest verifier, commit hook, and hosted CI rather than
copying illustrative generic wrappers. It documents the one end-of-slice
Release/CTest gate, provenance-checked reuse for unchanged code, and the rule
that generated screenshots/logs remain workspace-only when requested. A CTest
policy contract protects the canonical paths and evidence behavior. This does
not claim branch protection or a self-hosted runner; those require confirmation
from repository settings and actual CI runs. Verification: Qt MinGW Release and
full CTest 115/115; manifest `artifacts/evidence/sprint996-validation-reconciliation.json`
(SHA-256 `06AD438359BB59D8D8B5D1B6E52A366CE9EBC2F2370CE9871268AD374382D45F`).

## Sprint 995 stable schematic pin identity persistence

The project JSON reader accepts an optional `id` on each placed symbol-declared schematic pin, and the writer emits non-empty IDs while omitting empty ones. Existing project files without pin IDs remain readable and continue to receive explicitly derived index identities; native IDs use the `native_pin_id` provenance label. Serializer round-trip and legacy-format tests plus project-index provenance tests cover both paths. This does not add library-definition pin records or claim complete schematic graph coverage. Qt MinGW Release and full CTest pass 115/115. Official non-visual evidence manifest: `artifacts/evidence/sprint995-schematic-pin-identities.json` (SHA-256 `21c857fa71413cdb262e3018cdd56db0485d46b6caae1383664b148e46254561`); logs remain workspace-only. clangd parsed the translation unit but did not complete indexing, and is not counted as a pass.

## Sprint 994 evidence-to-CI handoff

The current visual-validation workflow treats the passing local evidence manifest as the boundary before commit and push. The evidence hook validates the staged manifest digest and any committed artifacts; for generated screenshots and logs kept local by sprint policy, it validates the workspace-only payload digests before commit. Hosted CI independently validates the manifest committed for the pushed revision, but cannot verify workspace-only file bytes and does not imply repository branch protection. The verifier invokes CCad's existing Qt UI-map target-sequence harness; generic wrapper scripts from external examples are not part of this project. The workflow requires the actual CI result for the pushed SHA before merge. The visual-harness CTest locks down the timing, evidence, and CI handoff wording.

The verifier also supports `-NonVisual` for process or documentation changes that do not alter GUI behavior. This mode still captures Qt/MinGW preflight, Release-build, and full-CTest results in its generated evidence manifest, and explicitly labels visual validation as not applicable; it cannot be used for changes with visible GUI effects. Sprint 994's verified manifest is `artifacts/evidence/sprint994-process-gate.json` (SHA-256 `2d9a776277b623c81a95939c41aca1e89d6cfbe21e84ecca12eb73aa82bb0dc2`); its three log payloads remain local and are checked by the commit hook.

## Sprint 993 explicit functional-block context

The project index now derives functional-block search records solely from serialized user-created board/schematic groups and existing child schematic sheets. Records retain the source group/sheet, bounded original member IDs, currently resolved typed members, native net IDs, and a board-space bounding region only when physical member geometry is available. PCB and schematic coordinates are never combined. Blocks are rebuilt against the current typed project revision, expand only through direct typed member references, and are removed/updated incrementally. The same bounded block metadata survives the provider context allowlist, and Agent context status reports the number of included blocks. Contracts cover natural-language lookup, current members/nets/bounds, unresolved references, context forwarding, revision provenance, and deletion. Validation: 35/35 focused contracts, Pyright clean, Qt MinGW Release build, full CTest 115/115, and the official provider-disabled GUI-map target with 10 mapped actions, four inspected meaningful screenshots, isolated conversation DB verification, and reviewed stdout/stderr (stderr empty). The live Agent turn displayed one matching functional block. Six evidence-manifest policy tests and the focused GUI-harness CTest pass. Evidence fingerprints: `artifacts/evidence/sprint993-functional-block-context-publish-ready.json`; screenshot/log payloads remain workspace-only. This is deterministic lexical retrieval, not vector/semantic project retrieval; no guessed or model-generated labels are treated as design truth.

## Sprint 992 schematic pin and annotation retrieval

The revision-aware typed project index now adds serialized symbol-declared pins as searchable schematic entities, including unconnected pins that have no net membership. Connected declarations retain their source pin number, electrical type, symbol/unit identity, and resolved net membership; unresolved net records remain explicitly separate rather than being attached by guesswork. Textboxes, graphics, junctions, no-connect markers, bus entries, rule areas, and tables are indexed from their typed schematic collections. Table cell text is bounded, bitmap payload bytes are excluded, and incremental snapshots remove deleted pins and annotations. Validation: 34/34 focused project-index contracts, changed-module Pyright (0 diagnostics), Qt MinGW Release build, and CTest 115/115. The provider-disabled official GUI-map run completed 10 mapped interactions, verified the serialized unconnected PGOOD pin and bounded context retrieval, inspected four meaningful screenshots, and reviewed stdout/stderr; no provider request was sent. Evidence: `artifacts/evidence/sprint992-schematic-pin-retrieval-verified-final.json`. CCad's project JSON currently does not preserve `SchPin.id`, so file-loaded pin identity is explicitly derived from symbol/unit/number-or-name. The broader C3 graph remains open.

Sprint 995 supersedes the Sprint 992 note about missing JSON pin IDs: native non-empty `SchPin.id` values now round-trip, while legacy ID-less project files continue to load with derived retrieval identities.

## Sprint 991 opt-in semantic memory retrieval

Memory retrieval can optionally augment existing title/content/tag BM25+RRF ranking with local Ollama embeddings. The HTTP client accepts numeric loopback addresses only, disables proxy routing, checks that the selected model is already installed, uses the same exact model identity for query and records, requests `truncate=false`, validates finite normalized vectors, and bounds request sizes/timeouts/cache. Query/document cache keys include model digest, tier, opaque namespace, record ID, and content fingerprint; changing model/scope/content, disabling a tier, deleting/expiring memory, or compacting invalidates process-held vectors. Embedding failures return safe categories and lexical search remains available. Personalisation → Memory exposes opt-in enable, loopback endpoint, installed model and live ready/unavailable status. No model is installed or downloaded by CCad. Contracts: `scripts/test_semantic_retrieval.py`, `scripts/test_memory_manager.py`, `scripts/test_lexical_retrieval.py`, `scripts/test_memory_context_contract.py`; GUI proof uses `scripts/run_ui_map_mouse_target_demo.ps1 -Name sprint991-semantic-memory-...` with a disposable profile.

## Sprint 990 fielded lexical memory ranking

Enabled memory records are retrieved through separate BM25 title, content, and tag fields after tier, project namespace, expiry, and minimum-match filters. Weighted reciprocal-rank fusion combines field rankings without conflating their BM25 score scales, then bounded deterministic maximal-marginal-relevance selection reduces near-duplicate context. Safe diagnostics report channel ranks and opaque scope identity, never the memory payload. This is lexical retrieval, not semantic embedding search; history ranking, preference/correction weighting, importance/recency adjustments, and provider tokenizer counts remain open. Offline contracts: `scripts/test_lexical_retrieval.py` and `scripts/test_memory_manager.py`.

## Sprint 989 bounded BM25 memory and history retrieval

Enabled task, conversation, project, and local-user memory now ranks through a shared deterministic BM25 scorer and returns rank/score/matched-term provenance. Conversation history search ranks compact thread recaps from the exact active native project before loading only the matching bounded TurnRecords; project mismatch, the active thread, and weak multi-term matches are excluded. At most eight retrieved prior turns enter context, with their exact thread, turn, and source-message IDs. The existing memory/context budget still governs memory injection. Offline contracts cover relevance, weak/no-match rejection, project isolation, provenance, and the memory lifecycle; `scripts/test_lexical_retrieval.py`, `scripts/test_memory_manager.py`, `scripts/test_conversation_store.py`, and `scripts/test_context_budget.py`.

## Sprint 988 project-scoped durable memory

Project facts can be stored with `/memory add tier:ltm scope:project ...`. CCad derives an opaque namespace from the active native project ID, keeps these records separate from thread-scoped LTM and user-global episodic memory, and automatically retrieves them across chats for the same project. Project switching reloads the bounded LTM runtime; missing project identity rejects project writes and cannot fall back to a shared placeholder. Existing memory list/update/delete/clear operations respect the current namespace; explicit reset remains intentionally global. The context manifest reports project-scope availability and the safe durable-record count, not memory contents. Pending project compaction plans expire on project switch. Offline coverage: `scripts/test_memory_manager.py`, `scripts/test_memory_command_contract.py`, `scripts/test_context_broker.py`, `scripts/test_agent_memory_search_tool.py`, `scripts/test_agent_memory_compaction.py`, and `scripts/test_memory_context_contract.py`. Semantic retrieval, preference/correction ranking, and provider-tokenizer budgets remain open.

## Sprint 987 schematic field and relative sheet retrieval

The bounded typed project index now searches authoritative schematic symbol `fields[]` by visible field name/text while preserving the visibility flag, and indexes schematic sheet titles with project-relative `file_path` values. These properties and paths survive the provider context allowlist; absolute host paths are dropped and secret-like property names/values are filtered. The Agent turn IPC also carries the native project ID so project-scoped retrieval and memory use the correct project namespace. `python scripts/test_project_index.py` passes 32 contracts and changed-module Pyright reports zero diagnostics. Qt MinGW Release and full CTest pass 113/113. The provider-disabled official GUI-map scenario verifies serialized and retrieved `ACME-42`/`sheets/power_stage.kicad_sch`, non-empty project matches, and `/clear` transcript retention; three distinct screenshots and both logs were inspected. The typed model still lacks individual rule IDs, durable project-artifact IDs, and authoritative generated functional-block summaries or annotations.

## Sprint 986 bounded project spatial and diagnostic retrieval

Project context now accepts explicit PCB bounding-box/rectangle requests and retrieves typed objects whose axis-aligned bounds intersect the requested area. The bounded region also includes live DRC/ERC records only when their explicit affected-object identity points to an entity in that region. Typed descriptions and diagnostic details participate in deterministic BM25 search; all currently serialized PCB collections' layer fields and cross-domain schematic/board identity links have regression coverage. Unchanged typed snapshots reuse their indexed documents even when UI-only selection/layer state changes, while changed snapshots continue to refresh incrementally. `python scripts/benchmark_project_index.py --objects 10000` measures the real cold, cached, and one-entity-update paths without imposing hardware-specific timing thresholds. It demonstrates that cached retrieval is much cheaper after the unchanged-snapshot fingerprint, but single-object edits still scan/re-extract the full authoritative snapshot; transaction-delta updates remain open. Rule IDs, schematic library pins absent from the project snapshot, and other non-serialized graph links are not fabricated.

## Sprint 985 typed project relationships and live diagnostic retrieval

`project.context` carries bounded native DRC/ERC records, preserving diagnostic engine, code, severity, message, and affected-object identity. Sprint 986 adds region-based selection of only diagnostics linked to geometries intersecting the requested bounding box. Explicit groups, route endpoints, teardrop anchors, diagnostic targets, and serialized schematic page membership remain revision-aware and bounded. See Sprint 986 for remaining model boundaries and benchmark findings.

## Sprint 984 exact native PCB-net context

The bounded project index derives one `board_net` retrieval record for each native net ID present on indexed PCB objects and links those records to objects carrying the same ID. This makes board-only net queries discoverable and preserves member IDs through the normal context allowlist. The relationship means only that CCad objects share their typed net assignment; it does not assert physical copper continuity. Exact colon-delimited schematic-pin identities now take precedence over embedded net-name tokens even when the query includes natural-language wording. Safe per-turn metadata counts only board-net records that survive context packaging, and the Agent activity line displays that count. The 23 focused project-index contracts pass; Pyright 1.1.414 reports zero diagnostics. Qt MinGW Release and 113/113 CTest pass. The provider-disabled seven-action UI-map query used the real `agent-catalog` project, visibly reported the PCB-net count, verified transcript persistence and `/clear`, and generated three distinct screenshots; stdout/stderr were reviewed. Wider diagnostic, sheet-hierarchy, arbitrary bbox, and C3 benchmark coverage remains open.

## Sprint 983 production-shaped PCB context

Project retrieval now indexes the serialized `padstack.layer_set` used by real CCad pads, as well as via spans, explicit object layers, zones, and preferred route-request layers. Supported typed layer-bearing board objects include footprints, pads, tracks/arcs, vias, zones, graphics, text, dimensions, barcodes, reference images, tables, targets, teardrops, and route requests. Spatial retrieval uses footprint courtyard geometry and typed keepout/placement-region Rects; reference images and tables retain their serialized positions and sizes without forwarding reference-image bytes. Scalar board design-rule settings are queryable as one board-level value object and are preserved when the project snapshot is compacted. CCad currently has no per-rule identity objects, so this does not claim stable individual rule IDs. Exact C3 completion remains open for diagnostic/object graph links, missing durable identities, hierarchy, arbitrary bbox queries, DRC-marker search, and retrieval benchmarks. Sprint 983 validation passed 20/20 focused project-index contracts, zero changed-module Pyright diagnostics, Qt MinGW Release build, and 113/113 CTest; the seven-action provider-disabled GUI-map test verified multi-layer context and transcript clearing, with all three generated screenshots and both process logs inspected.

## Sprint 982 multilayer PCB project context

Typed project retrieval now indexes every declared layer membership on an entity, including a via's start and end copper layers and a pad's layer set. Either exact via endpoint retrieves the same via with both endpoints intact; changing an endpoint updates the layer relationship incrementally and removes the stale association. Bounded project context retains the exact layer IDs that survived packaging and reports the number of distinct IDs in the real per-turn Agent activity (the count is across all included entities, not a via-only count). Both normal canvas-click via placement and the mapped automation placement derive the through-via span from the current board's first and last copper layers instead of leaving endpoints empty or hardcoding a two-layer board. Through-via placement fails before mutation when the board does not define two distinct copper layers. Focused index/context contracts pass 16/16 and changed-module Pyright reports zero diagnostics. The official provider-disabled GUI-map scenario completed nine mapped actions on an isolated project and verified a saved F.Cu/B.Cu via plus non-empty bounded layer context; all five distinct screenshots and stdout/stderr were inspected. Qt MinGW Release build and full CTest pass 113/113. Complete project-graph and all-entity layer coverage remain open.

## Sprint 981 schematic net-member retrieval

Agent project retrieval now exposes each bounded typed schematic net-member pin as an individual entity tied to its source net, component, and schematic symbol. Querying a net can retrieve its member pins and symbols; querying a connected pin can also surface the associated schematic wire, labels, and matching board-net objects. Turn metadata and the Agent context activity line count only pin and symbol results that actually survived context packaging. Graph signatures track membership changes incrementally, and bounded extraction avoids rescanning the entire multi-sheet netlist for every net. Netlist assignment remains distinct from electrical/geometric copper continuity, and unconnected library pins are not fabricated as placed schematic pins. All 13 focused project-index/context contracts pass; the provider-disabled GUI-map run completed seven mapped actions, showed 10 results (3 schematic pins, 1 symbol), verified transcript persistence, and had four inspected screenshots and reviewed stdout/stderr. Full Qt MinGW Release build and CTest passed 113/113. The dedicated UI-map scenario is `sprint981-schematic-project-graph`.

## Sprint 980 typed project retrieval foundation

Every Agent turn can now retrieve bounded, relevant objects from CCad's authoritative typed PCB/schematic snapshot before its provider request. Exact identifiers, reference designators and layers are resolved separately from deterministic BM25 text ranking; exact references can expand one hop across component, layer, or shared-net associations, and coordinate/proximity requests use an incremental grid index over typed object bounds. The context package retains these source-backed results if a large full-project snapshot must be compacted, and safe state/trace metadata reports the project-content revision, result counts, indexing mode and omissions without exporting design payloads as telemetry. The Agent panel displays the actual project-match count in its per-turn context event. Net association is explicitly not claimed as proven copper continuity, and spatial distance is explicitly a bounding-box measure rather than a clearance calculation. Contracts are registered with CTest in `agent_project_index` and cover exact/lexical/relationship/spatial retrieval, revision updates, broker reuse and compacted-context delivery. Qt MinGW Release and CTest pass 113/113; the app-owned GUI-map flow visibly retrieved 10 matches for `U_DEMO` with provider disabled, persisted the turn, and passed seven mapped interactions with four inspected screenshots and reviewed logs. This is a foundation, not full C3 completion: rule/diagnostic/sheet/artifact identities, complete graph coverage, arbitrary query bounding boxes, DRC-marker search, and retrieval benchmarks remain open.

## Sprint 979 Agent runtime analysis boundaries

Provider catalog requests and provider-specific response parsing now live in `src/ccad_agent/model_catalog.py`; the existing orchestration facade preserves the six provider refresh entry points and delegates failures to the canonical safe classifier. The request handler for one human turn is a separate `handle_human_message(req)` routine rather than code nested in the JSON-RPC dispatcher loop. This keeps protocol control flow independent from context assembly, memory handling, conversation persistence, commands, provider execution, and tool-call emission, while retaining no-provider local behavior. The boundary and catalog contracts are registered with CTest; provider parser tests use controlled local HTTP responses and do not contact providers. Pyright remains enabled without complexity-limit suppressions.

## Agent memory management outcomes (Sprint 975)

The memory manager now reports add, update, delete, reset, and backend-error outcomes from the Python memory runtime to the GUI. The settings panel disables write controls while a mutation is pending, displays operation-specific success or safe error text, refreshes records only after authoritative success, rejects empty content visibly, and requires confirmation before deleting a selected record or resetting all persisted records. Delete targets the record ID selected before the confirmation dialog, even if the active conversation changes while that dialog is open. Reset reports the backend's removed-record count or failure and refreshes an open manager list; it no longer relies on a fixed re-enable timer. Validation used an isolated profile. The mapped harness records every interaction but retains only distinct screenshots for dialogs and completed outcomes, avoiding redundant per-click captures.

## Provider status and failure categories

Provider model discovery preserves OpenRouter `supported_parameters` and Cerebras' allowlisted capability flags and context limits. A catalog capability means the provider advertises that feature; it is not a live request test. Generic/local OpenAI-compatible servers and models without capability metadata remain explicitly unverified. Google `RESOURCE_EXHAUSTED` without structured quota/rate details is reported as `quota_or_rate_limit`, not falsely as payment or exhausted quota, and does not trigger an automatic retry.

Provider configuration state means the adapter initialized; it is not a network/model verification claim. Provider failures use nested HTTP and SDK error metadata to distinguish payment, quota/credits, rate limits, credentials, permissions, missing models, timeouts, dependencies, and connectivity without exposing raw error bodies or secrets. Valid `Retry-After` guidance is bounded before display. OpenAI-compatible and Anthropic adapters use zero SDK retries when supported so CCad controls retry behavior; the pinned Gemini adapter currently hard-codes two retries internally, which remains an upstream dependency limitation. Offline contracts live in `scripts/test_provider_failure_event.py` and `scripts/test_provider_error_classification.py`.

This document tracks user-visible and agent-visible features that exist in the repo, how to use them, how to test them, and where they are implemented.

## Sprint 976 durable Agent conversation projection

The Python Agent now stores sanitized canonical messages in a per-thread SQLite database at `%APPDATA%/CCad/agent_conversations.sqlite3` (override with `CCAD_AGENT_CONVERSATION_DB`). User, assistant, and tool messages retain stable message/tool-call IDs; recognized credential-like values are redacted before persistence. Provider input history is a bounded projection that keeps recent complete turns under `CCAD_AGENT_HISTORY_TOKENS` and `CCAD_AGENT_HISTORY_LIMIT`. `/cc` updates only that projection, while `/clear` empties active model context; neither deletes the canonical transcript. Completed and terminally unavailable turns produce a structured, source-message-linked TurnRecord and a recap of up to six recent turns. Historical TurnRecords are retrieved per thread by deterministic lexical token overlap and included in the versioned provider context with source IDs; this is not semantic/vector search.

The current boundary is deliberate: old LangGraph checkpoint-only sessions are not migrated automatically, the new durable thread list/resume UI is not yet wired, and retrieval does not cross thread boundaries or use embeddings. Verification is through `scripts/test_conversation_store.py`, `scripts/test_conversation_runtime.py`, context-package contracts, and the isolated seven-action GUI-map scenario `sprint976-conversation-20260924`; the latter validates one persisted user/assistant pair, one TurnRecord, preserved canonical rows after `/clear`, and an empty model projection without making a provider request.

## Sprint 967 request context and memory lifecycle

The Python agent assembles bounded project/design context plus records retrieved only from enabled memory tiers. Conversation messages, system instructions, and tool schemas are distinct provider-request inputs; their character-based token counts are explicitly estimates, and unknown model limits or multimodal sizes are not invented. Above the configurable large-context threshold, chat receives a source/count breakdown and development logs receive only privacy-safe counts and opaque identifiers. If the envelope is too large, CCad replaces the complete project snapshot with a revision/count summary and drops lowest-ranked memories until valid JSON fits.

Memory is managed through one tier-aware runtime manager shared by Settings and `/memory`: STM is task-scoped and process-only, LTM is durable per conversation thread, and episodic memory is durable per local user across projects on this device. Capture is explicit, disabled tiers unload from the current process without deleting stored records, secret-like writes are rejected, unsafe legacy entries are hidden, expiry is enforced, exact normalized duplicates are avoided, and durable storage is capped at 64 newest entries per namespace. The UI provides live tier state, memory management, and separately confirmed reset/delete operations. Scope labels are explicit management filters, not semantic retrieval filters. Sprint 970 adds real selected-model semantic compaction for chat history through `/cc` and `/compact`; durable memory-record semantic compaction, semantic duplicate matching, and automatic capture remain incomplete.

## Sprint 970 semantic conversation-history compaction

`/cc` and `/compact` send only a bounded, sanitized transcript of older conversation messages to the selected provider with tools disabled. The command tells the user before sending that provider quota may be used, preserves the newest four message objects unchanged, rejects empty/unsafe/unbounded/non-compacting summaries, updates the real LangGraph checkpoint, and verifies the saved replacement. Pending graph work is refused; checkpoint failures trigger restoration and explicit failure reporting. Safe telemetry reports provider/model, counts, estimated input tokens, returned usage when available, thread presence, and whether the request was sent, but never exports conversation contents or tool arguments. The local no-history path sends no provider request.

The durable memory store is not modified by `/cc`; it still uses explicit user capture and newest-record retention. Use `/context [draft]` or wait for the configured large-context threshold to inspect the separate context/memory assembly breakdown, which reports counts and estimates without exposing source content.

## Sprint 974 durable-memory semantic compaction

`/memory compact plan tier:ltm|episodic scope:<scope>` prepares a bounded, same-tier and same-namespace group without contacting a provider or changing storage. The response identifies the selected provider/model and estimated input size, then requires the separate `/memory compact send:<id>` command before memory text is sent. That request uses the selected raw model with tools disabled; its bounded, secret-checked summary stays process-only and is displayed for review. Only `/memory compact apply:<id>` performs an atomic replacement, and only while all source records remain unchanged in relevant fields and the tier/namespace remain active. Cancel, expiry, thread changes, and tier disable discard the in-process plan. Original records remain untouched on provider failure, stale source, or cancellation.

Offline contracts cover bounds, unsafe output, storage freshness, atomic replacement, cache invalidation, and real JSON-RPC plan/send-refusal/cancel behavior. The official Agent UI-map scenario exercises planning and cancellation against an isolated profile and confirms the durable fixture is unchanged. Qt MinGW Release build and full CTest pass 106/106; Pyright reports zero diagnostics in changed agent modules. Seven mapped GUI interactions and screenshots were inspected with empty stdout/stderr. No real provider request was sent; a live summary-and-apply remains opt-in.

Sprint 967 verification: no-network memory/context contracts passed, Pyright reported 0 diagnostics for touched Python modules, Qt MinGW Release build succeeded, and its full CTest passed 98/98. Its app-owned GUI-map validation performed 10 actions per pass and inspected all 26 screenshots; report and logs are in ignored `artifacts/screenshots/sprint967-memory-final*` files. The large-context breakdown is emitted to chat before provider requests without exposing source content; a provider-backed render of this breakdown remains pending.

Sprint 970 verification: semantic-history and real LangGraph checkpoint contracts passed, changed-module Pyright reported 0 diagnostics, Qt MinGW Release build succeeded, and full CTest passed 103/103. The official app-owned GUI-map flow performed seven mapped actions and all ten generated screenshots were ingested and inspected; stdout showed `safe_noop_visible:true` and `provider_request_sent:false`, while stderr was empty. The broader bundled Python sweep remains incomplete after unrelated source-contract failures and an independently hung local provider-boundary test. A provider-backed compaction turn and live Langfuse readback remain unverified.

## Sprint 968 task-scoped STM and duplicate safety

STM now uses a distinct runtime task UUID for each explicit `/task start` to `/task end` interval within a durable Agent session. `/task status` reports whether that scope is active, and each task command is handled locally without model invocation. Unstaged turns receive non-retained task identities, so an STM write is rejected instead of being orphaned. Starting a replacement, ending a task, or evicting an old session clears its process-only STM. Active session/task identities are capped at 32, and each task retains at most 64 records.

Memory add/update return the existing record for normalized exact duplicates and reject high lexical Jaccard overlap (minimum five unique terms, threshold 0.88) with that record's ID. This is deliberately reported as lexical matching, not semantic equivalence. The slash palette opens from both typed and UI-map-set input, and exposes `/task start`, `/task status`, and `/task end`.

Verification: task-scope and duplicate contracts pass; Qt MinGW Release build and full CTest pass 99/99. The app-owned GUI-map flow completed eight mapped interactions in two runs; all 24 screenshots and both runs' stdout/stderr were inspected, including one >20-second stability run. Semantic compaction and semantic duplicate detection remain open.

## Sprint 950 Agent execution truthfulness

`/drc` now dispatches the authoritative native `project.drc` method and reports its returned error, warning, and diagnostic counts instead of leaving a chat turn at a progress-only message. Context assembly emits a bounded, secret-safe `context_state` event with its schema revision and aggregate counts. Pending mutations remain one-call approval boundaries: duplicate calls are rejected, and an approval is shown as applied only when the broker reports a performed successful result. A failed broker/gesture result is returned to the model as an error and remains visibly unapplied. `ui.canvas_drag` now preserves reasons such as `outside_board` rather than reducing every target failure to `start_target_not_found`.

Verification uses `gui_agent_panel`, `gui_ui_map`, and `agent_preview` CTest targets; no-network Python contract tests for DRC, context package, provider failure classification, provider failure events, and approval-result truth; the full 92-test Qt CTest gate; and an inspected GUI screenshot from `scripts/run_sprint_demo.ps1`. Provider startup wording remains explicitly tracked as unfinished when it claims that a configured provider was not contacted.

Gemini import-time dependency warnings are now suppressed from the user-visible activity stream because they do not describe provider readiness or request execution. Real provider state remains emitted only through provider-state events. The no-network warning contract and inspected Sprint 951 GUI screenshot verify that a configured Gemini provider no longer receives a fabricated “Provider was not contacted” card.

The visible `Run DRC` quick action now calls the same authoritative read-only `project.drc` live-query method as the slash command, rather than inserting `/drc` into the composer. Its GUI test proves one exact `project.drc {}` request, while the inspected live UI-map click records `performed:true` and the rendered `Result OK project.drc` activity event.

PNS meander placement now records a deduplicated routed polyline, updates its active item to each destination, and clears state on finish. Length matching and serpentine optimization remain future work.

Meander placement also reports current length, non-negative remaining target distance, and target-reached state for adaptive agent control.

`meanderToTarget()` can add a perpendicular bend when direct travel is shorter than requested target length, giving agents a concrete first target-reaching geometry primitive.

`meanderToTarget()` accepts optional bend count and generates alternating multi-bend paths for longer targets.

PNS nodes can remove owned items while keeping spatial queries synchronized.

PNS nodes can also answer clearance-aware obstacle queries through `hasObstacle()`.

Repeated insertion of the same PNS item is safely ignored at node level.

PNS spatial queries now include segment-versus-item obstacle detection with clearance expansion.

Segment obstacle queries can filter by active net and copper layer.

`PnsBoardObstacleIndex` builds a filtered PNS obstacle view from board pads/vias for agent and router integration.

The adapter can return blocker identities for actionable routing diagnostics.

Interactive routing now has PNS-backed pad/via fallback obstacle protection with an explicit `pns_pad_via` blocked reason.

PNS differential-pair placement now supports distinct endpoints, configurable non-negative gap, and coupled endpoint updates during start/route. This is an endpoint-placement primitive, not yet a complete KiCad-equivalent differential-pair track router.

## Build System And DevOps

Status: implemented.
Files:
- `CMakeLists.txt`
- `.github/workflows/ci.yml`
- `.gitignore`
- `docs/devops/sprints/2026-05-14-sprint-1-qt-review-gui.md`

What it does:
- Builds a native C++20 project with CMake.
- Builds `ccad_core`, `ccad`, tests, and optional `ccad_gui`.
- Runs tests through CTest.
- GitHub Actions has a core job and a Linux Qt GUI compile job.

Use:
```bash
cmake -S . -B build -DCCAD_WARNINGS_AS_ERRORS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Core-only build:
```bash
cmake -S . -B build -DCCAD_WARNINGS_AS_ERRORS=ON -DCCAD_BUILD_GUI=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected result:
- CMake configure succeeds.
- Build succeeds.
- CTest reports all tests passing.

## Native GUI UI Map For Agent Targeting

Status: implemented.
Files:
- `src/ccad_gui/agent_panel.hpp`
- **Agent Command Center**: The `src/ccad_gui/agent_panel.cpp` provides a fully realized Copilot-style side panel. It supports live `/` commands (`/explain`, `/settings`, `/marketplace`) routed directly through `src/ccad_agent/orchestrator.py`, which executes LangGraph nodes and triggers JSON-RPC actions in the core.
  - Verification: `ccad_gui --screenshot` shows the agent panel. Running commands outputs JSON-RPC.
- `src/ccad_gui/agent_panel.cpp`
- `src/ccad_gui/review_window.hpp`
- `src/ccad_gui/review_window.cpp`
- `src/ccad_gui/main.cpp`
- `tests/test_gui_agent_panel.cpp`
- `tests/test_gui_ui_map.cpp`

What it does:
- Exports a read-only semantic map of the native Qt GUI through `ccad_gui --dump-ui-map <project.ccad.json> <out.json>`.
- Reports stable action IDs, tab IDs, canvas IDs, and canvas-object IDs.
- Includes screen-space target coordinates, widget rectangles, scene-space CAD object rectangles, net IDs, layer IDs, and route-request provenance.
- Validates targets through `ccad_gui --validate-ui-map-targets <project.ccad.json> <out.json>`, which moves the cursor over every visible/enabled exported target and verifies live Qt hit-testing.
- Reports hidden or disabled targets as skipped instead of pretending they are clickable.
- Resolves one semantic target through `ccad_gui --ui-target-id <project.ccad.json> <id> <out.json>`.
- Resolves one PCB board-space coordinate through `ccad_gui --ui-target-board-point <project.ccad.json> <x-mm> <y-mm> <out.json>`.
- Reports logical Qt pixels, physical pixels, and device-pixel ratio for high-DPI-aware automation.
- Triggers a small allowlist of safe GUI actions through `ccad_gui --ui-trigger-safe <project.ccad.json> <id> <out.json>`.
- Executes view/display actions directly, enters first-batch PCB edit modes for Add Via, Route Track, and Add Keepout, and deletes selected board primitives through `action:delete_cursor`.
- Refuses still-unsupported, dialog-opening, or file-writing actions with `performed:false` and a structured reason.
- Provides a native bottom `Agent` panel in the Qt GUI. The panel displays the loaded project and UI-map epoch, refreshes the current semantic UI map into read-only JSON output, and triggers safe non-destructive UI actions by semantic ID.
- Exposes the Agent panel itself as `panel:agent` so automation can target it through the same UI map and `--ui-target-id` contract.
- Provides an Agent-panel live-query row with method and JSON payload inputs. It calls the same `ReviewWindow` live-query dispatcher as the long-lived local socket instead of maintaining a second protocol.
- Exposes Agent-panel controls as UI-map targets: `control:agent_action_id`, `control:agent_live_method`, `control:agent_live_payload`, `action:agent_refresh_map`, `action:agent_trigger_safe`, and `action:agent_live_query`.
- Keeps the live UI-map socket available while the GUI is open through `ccad_gui --serve-ui-map <project.ccad.json> <server-name> <ready-file>`.
- Supports live socket and Agent-panel methods `agent.methods`, `agent.method_schema`, `agent.quickstart`, `agent.workspace_state`, `ui.map`, `ui.map_compact`, `ui.role_summary`, `ui.index_stats`, `ui.get_node`, `ui.nodes_by_role`, `ui.map_delta`, `ui.wait_for_delta`, `ui.watch_delta`, `ui.find`, `ui.hit_test`, `ui.target`, `ui.target_board_point`, `ui.nearest_canvas_object`, `ui.canvas_click`, `ui.canvas_drag`, `ui.current_tool`, `ui.cancel_tool`, `ui.place_via`, `ui.route_track`, `ui.add_zone`, `ui.add_keepout`, `ui.draw_graphic`, `ui.place_text`, `ui.delete_object`, `ui.trigger_safe`, `ui.active_layer`, `ui.set_active_layer`, `ui.active_net`, `ui.set_active_net`, and `ui.epoch`.
- Uses `ui.map_delta` as a low-token polling path. A current epoch returns `changed:false` and no nodes; a stale epoch returns dirty compact nodes and changed roles when exact dirty tracking is available, with a full compact fallback when the caller is too stale.
- Uses `ui.find` for compact role/query lookups so agents can find controls such as Add Footprint without dumping the whole map.
- Uses `ui.map_compact` and `ui.role_summary` for cheap map shape inspection, `ui.hit_test` for logical-coordinate node lookup, and `ui.nearest_canvas_object` for indexed nearest rendered CAD object lookup over a small uniform-grid candidate set.
- Uses `ui.index_stats`, `ui.get_node`, and `ui.nodes_by_role` for cached semantic node lookup by ID and role, and `ui.watch_delta` for bounded dirty-event history newer than a caller's cached epoch.
- Uses `agent.methods` for method discovery, `agent.method_schema` for one-method schema lookup, and `agent.quickstart` for the recommended UI-map agent loop. These catalog calls describe categories, safety flags, dry-run support, input schema shapes, output summaries, and example payloads so future agents do not have to scrape README text or guess dispatcher method names.
- Starts Python only after native catalog acceptance; generated provider tools preserve native method IDs and broker results.
- Uses `ui.canvas_click` and `ui.canvas_drag` for map-driven board-point gestures. These methods map PCB millimeter coordinates through the current board outline and `QGraphicsView` viewport, send real Qt mouse events to the PCB viewport, and report interaction mode plus object counts after the existing GUI tool event path finishes.
- Uses high-level workflow methods such as `ui.place_via`, `ui.route_track`, `ui.add_zone`, `ui.add_keepout`, `ui.draw_graphic`, `ui.place_text`, and `ui.delete_object` for agent convenience. These methods activate the same GUI tools and then delegate to viewport input or canvas-object selection rather than mutating the board directly.

Test:
```powershell
$env:PATH = 'C:\Qt\6.11.1\mingw_64\bin;' + $env:PATH
ctest --test-dir build-qt -R gui_agent_panel --output-on-failure
ctest --test-dir build-qt -R gui_ui_map --output-on-failure
.\build-qt\ccad_gui.exe --dump-ui-map .\artifacts\demos\sprint156-layer-color-final.ccad.json .\artifacts\demos\sprint157-ui-map.json
.\build-qt\ccad_gui.exe --validate-ui-map-targets .\artifacts\demos\sprint156-layer-color-final.ccad.json .\artifacts\demos\sprint157-ui-map-target-validation.json
.\build-qt\ccad_gui.exe --ui-target-id .\artifacts\demos\sprint156-layer-color-final.ccad.json action:add_footprint .\artifacts\demos\sprint158-target-add-footprint.json
.\build-qt\ccad_gui.exe --ui-target-board-point .\artifacts\demos\sprint156-layer-color-final.ccad.json 8 17 .\artifacts\demos\sprint158-target-board-point.json
.\build-qt\ccad_gui.exe --ui-trigger-safe .\artifacts\demos\sprint156-layer-color-final.ccad.json action:zoom_in .\artifacts\demos\sprint159-safe-zoom-in.json
.\build-qt\ccad_gui.exe --ui-trigger-safe .\artifacts\demos\sprint170-pcb-edit-tool-entry-final.ccad.json action:add_via .\artifacts\demos\sprint170-safe-add-via.json
```

Live protocol focused test:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& ctest --test-dir build-qt -R gui_agent_panel --output-on-failure"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& ctest --test-dir build-qt -R gui_ui_map --output-on-failure"
```

Expected result:
- `gui_ui_map` passes.
- UI map JSON contains `action:add_footprint`, `tab:pcb`, `canvas:pcb`, and `canvas_object:*` nodes.
- Target validation reports zero failures for visible/enabled nodes.
- Target queries return `found:true` for known IDs and inside-board points.
- Safe trigger returns `performed:true` for allowlisted view actions and first-batch editor mode entries, and `performed:false` for unsafe or unsupported actions.
- The Agent panel can refresh a UI-map JSON document, summarize node count, invoke the safe action callback, and run live protocol queries without owning project state.
- Live protocol requests return structured `ok`, `method`, and `result` fields for map, delta, find, target, board-point target, safe trigger, active-layer, and active-net methods.
- Live protocol requests return structured catalog data for `agent.methods`, `agent.method_schema`, and `agent.quickstart`, including JSON-schema-shaped input descriptions and safety flags.
- Live protocol requests return `agent.workspace_state` with the current Agent-panel staged goal, visible task state, bounded evidence count, local evidence summaries, workspace context, diagnostics, result state, and live method/payload fields.

## Logical Project Model

Status: implemented.
Files:
- `src/ccad_core/model.hpp`
- `src/ccad_core/model.cpp`
- `tests/test_serialize.cpp`

What it does:
- Represents project metadata.
- Represents components, pins, nets, net members, and constraints.
- Represents separate optional schematic and board documents through `Project::schematics` and `Project::boards`.
- Represents optional physical board data: outline, layers, pads, vias, tracks, graphics, texts, zones, keepouts, placement regions, and route requests.
- Provides primary-document helpers so callers can safely work with board-only projects, schematic-only projects, or linked projects without unsafe index assumptions.
- Provides the data model used by CLI, ERC, review model, and future GUI/RPC clients.

Test:
```bash
cmake --build build --target ccad_tests
ctest --test-dir build -R serialize --output-on-failure
```

Expected result:
- `serialize` test passes.

## Deterministic JSON Serialization

Status: implemented.
Files:
- `src/ccad_core/serialize.hpp`
- `src/ccad_core/serialize.cpp`
- `tests/test_serialize.cpp`

What it does:
- Converts `Project` objects to stable JSON.
- Loads JSON back into `Project`.
- Escapes JSON strings correctly for control characters, quotes, and slashes.
- Rejects malformed JSON such as trailing garbage and trailing commas.

Test:
```bash
ctest --test-dir build -R serialize --output-on-failure
```

Expected result:
- Round-trip JSON test passes.
- Escape tests pass.
- Malformed JSON rejection tests pass.

## ERC Diagnostics

Status: implemented.
Files:
- `src/ccad_core/erc.hpp`
- `src/ccad_core/erc.cpp`
- `tests/test_erc.cpp`

What it does:
- Runs logical electrical-rule checks.
- Reports typed diagnostics with severity, code, message, and object ID.
- Detects unknown components, unknown pins, duplicate component IDs, duplicate pins, duplicate net members, and empty projects.

Test:
```bash
cmake --build build --target ccad_erc_tests
ctest --test-dir build -R erc --output-on-failure
```

Expected result:
- `erc` test passes.

## Native CLI

Status: implemented.
Files:
- `src/ccad_cli/main.cpp`
- `src/ccad_cli/app.hpp`
- `src/ccad_cli/app.cpp`
- `src/ccad_cli/common.hpp`
- `src/ccad_cli/common.cpp`
- `src/ccad_cli/project_commands.hpp`
- `src/ccad_cli/project_commands.cpp`
- `src/ccad_cli/pcb_commands.hpp`
- `src/ccad_cli/pcb_commands.cpp`
- `src/ccad_cli/sch_commands.hpp`
- `src/ccad_cli/sch_commands.cpp`
- `src/ccad_cli/lib_commands.hpp`
- `src/ccad_cli/lib_commands.cpp`
- `src/ccad_cli/agent_commands.hpp`
- `src/ccad_cli/agent_commands.cpp`
- `src/ccad_cli/agent_kicad_evidence.hpp`
- `src/ccad_cli/agent_kicad_evidence.cpp`
- `src/ccad_cli/agent_observability_config.hpp`
- `src/ccad_cli/agent_observability_config.cpp`
- `src/ccad_cli/agent_policy.hpp`
- `src/ccad_cli/agent_policy.cpp`
- `src/ccad_cli/agent_provider_config.hpp`
- `src/ccad_cli/agent_provider_config.cpp`
- `src/ccad_cli/agent_session.hpp`
- `src/ccad_cli/agent_session.cpp`
- `tests/test_cli.cpp`
- `tests/test_agent_serve.cpp`

What it does:
- Creates empty CCad project files.
- Emits deterministic command discovery JSON with `ccad help --format json`.
- Creates project files with board outline when given `--width-mm` and `--height-mm`.
- Adds PCB pads, vias, and track segments to existing board projects.
- Adds rectangular keepouts to existing board projects.
- Runs physical DRC diagnostics with `ccad drc`.
- Reports fixed default copper clearance errors with `COPPER_CLEARANCE`.
- Imports basic KiCad `.kicad_mod` footprint files, including oval drills, with `ccad lib import-footprint`.
- Inspects, looks up, and searches local CCad library catalog records with `ccad lib catalog-info`, `ccad lib catalog-find`, and `ccad lib catalog-search`.
- Exposes provider-free Agent harness metadata and headless workspace-state JSON with `ccad agent methods`, `ccad agent harness-context`, `ccad agent tool-guide`, `ccad agent state`, `ccad agent tasks`, `ccad agent evidence`, and `ccad agent approvals`.
- Creates, reads, checkpoints, and replays provider-free local Agent session files with `ccad agent session-schema`, `ccad agent session-new`, `ccad agent session-state`, `ccad agent checkpoint-add`, and `ccad agent replay`.
- Classifies Agent command risk before execution with `ccad agent policy-schema`, `ccad agent policy-check -- <ccad command args...>`, and `ccad agent dry-run -- <ccad command args...>`.
- Exposes no-secret provider setup metadata with `ccad agent provider-config-schema`, `ccad agent provider-config-template`, and `ccad agent provider-status`.
- Exposes disabled-by-default trace export metadata with `ccad agent trace-export-schema`, `ccad agent trace-export-template`, `ccad agent trace-redaction-policy`, and `ccad agent trace-export-dry-run`.
- Exposes structured KiCad CLI evidence metadata, command planning, readiness checks, and guarded local execution with `ccad agent kicad-evidence-schema`, `ccad agent kicad-evidence-plan`, `ccad agent kicad-evidence-dry-run`, and `ccad agent kicad-evidence-run`.
- Exposes the first KiCad PCB API parity schema with `ccad agent pcb-api-schema` and JSON-RPC `agent.pcb_api_schema`, mapping KiCad PCB API handlers to CCad's current headless commands and declaring first-slice parity limits.
- Resolves KiCad wildcard pad layer sets such as `*.Cu` and `*.Mask` in board-context PCB query/export surfaces, exposing both `resolved_layers` and canonical `kicad_layer_numbers` for agents and routers.
- Places imported CCad footprint pads onto a board with `ccad pcb place-footprint`.
- Places converted CCad/KiCad schematic symbols into `Project::components` with `ccad sch place-symbol`.
- Allows board-only PCB projects to run physical DRC, export KiCad PCB data, export PnP CSV, diff revisions, and feed agent context without requiring a linked schematic document.
- Creates a primary schematic document automatically when schematic mutation commands such as `sch place-symbol`, `sch add-wire`, `sch add-label`, or `sch add-power` run on a project that does not yet contain one.
- Validates CCad project files.
- Inspects projects and emits review JSON.
- Diffs two project files and emits machine-readable diff JSON.
- Prints ERC diagnostics as JSON.
- Returns `0` when validation has no errors.
- Returns `1` when ERC errors exist.
- Returns `2` for CLI usage, file, or parse failures.
- Keeps process entry, top-level dispatch, project commands, PCB commands, library commands, and shared helpers in separate native C++ modules.

Build:
```bash
cmake --build build --target ccad
```

Use:
```bash
build/ccad init --name demo --out demo.ccad.json
build/ccad help --format json
build/ccad validate demo.ccad.json
```

On Windows PowerShell:
```powershell
.\build\ccad.exe init --name demo --out demo.ccad.json
.\build\ccad.exe help --format json
.\build\ccad.exe init --name board --width-mm 42 --height-mm 28 --out board.ccad.json
.\build\ccad.exe validate demo.ccad.json
.\build\ccad.exe inspect demo.ccad.json
.\build\ccad.exe diff before.ccad.json after.ccad.json
.\build\ccad.exe drc board.ccad.json
.\build\ccad.exe lib import-footprint --in R_0805_2012Metric.kicad_mod --out R_0805_2012Metric.ccad-footprint.json
.\build\ccad.exe lib catalog-info --catalog kicad.ccad-library.json
.\build\ccad.exe lib catalog-find --catalog kicad.ccad-library.json --id footprint:Resistor_SMD:R_0603_1608Metric
.\build\ccad.exe lib catalog-search --catalog kicad.ccad-library.json --query 0603
.\build\ccad.exe lib catalog-search --catalog kicad.ccad-library.json --query 0603 --kind footprint
.\build\ccad.exe agent state
.\build\ccad.exe agent tasks
.\build\ccad.exe agent evidence
.\build\ccad.exe agent approvals
.\build\ccad.exe agent session-schema
.\build\ccad.exe agent session-new --out session.ccad-agent-session.json --session-id run-001 --title AgentRun --project board.ccad.json --created-at 2026-06-05T00:00:00Z
.\build\ccad.exe agent session-state --session session.ccad-agent-session.json
.\build\ccad.exe agent checkpoint-add --session session.ccad-agent-session.json --checkpoint-id cp-001 --kind verification --summary DrcPassed --artifact artifacts\reports\drc.json --created-at 2026-06-05T00:01:00Z
.\build\ccad.exe agent replay --session session.ccad-agent-session.json
.\build\ccad.exe agent policy-schema
.\build\ccad.exe agent policy-check -- pcb add-via --file board.ccad.json
.\build\ccad.exe agent dry-run -- pcb add-via --file board.ccad.json
.\build\ccad.exe agent provider-config-schema
.\build\ccad.exe agent provider-config-template
.\build\ccad.exe agent provider-status
.\build\ccad.exe agent trace-export-schema
.\build\ccad.exe agent trace-export-template
.\build\ccad.exe agent trace-redaction-policy
.\build\ccad.exe agent trace-export-dry-run
.\build\ccad.exe agent kicad-evidence-schema
.\build\ccad.exe agent kicad-evidence-plan --kind pcb-drc --input board.kicad_pcb --output artifacts\kicad\drc.json --format json --units mm --severity all --exit-code-violations
.\build\ccad.exe agent kicad-evidence-dry-run --kind pcb-export-gerbers --input board.kicad_pcb --output artifacts\fab\gerbers --layers F.Cu,B.Cu
.\build\ccad.exe agent kicad-evidence-run --kind sch-erc --input root.kicad_sch --output artifacts\kicad\erc.json --format json
.\build\ccad.exe agent pcb-api-schema
.\build\ccad.exe pcb place-footprint --file board.ccad.json --footprint R_0805_2012Metric.ccad-footprint.json --component R1 --at-x-mm 16 --at-y-mm 14 --layer F.Cu --rotation-deg 90
.\build\ccad.exe sch place-symbol --file schematic.ccad.json --symbol library-cache\symbols\1N4007.json --component D1 --at-x-mm 20 --at-y-mm 15 --rotation-deg 0
.\build\ccad.exe pcb list-enabled-layers --file board.ccad.json
.\build\ccad.exe pcb list-visible-layers --file board.ccad.json
.\build\ccad.exe pcb get-layer-name --file board.ccad.json --id F.Cu
.\build\ccad.exe pcb get-board-stackup --file board.ccad.json
.\build\ccad.exe pcb get-rules --file board.ccad.json
.\build\ccad.exe pcb get-outline --file board.ccad.json
.\build\ccad.exe pcb list-by-net --file board.ccad.json --net GND
.\build\ccad.exe pcb list-connected --file board.ccad.json --id U1.1
.\build\ccad.exe pcb add-pad --file board.ccad.json --id P1 --component U1 --pin 1 --net N1 --layer F.Cu --x-mm 5 --y-mm 6 --width-mm 1.5 --height-mm 1.0
.\build\ccad.exe pcb add-via --file board.ccad.json --id V1 --net N1 --x-mm 8 --y-mm 9 --diameter-mm 0.8 --drill-mm 0.4
.\build\ccad.exe pcb add-track --file board.ccad.json --id T1 --net N1 --layer F.Cu --start-x-mm 5 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9 --width-mm 0.25
.\build\ccad.exe pcb add-keepout --file board.ccad.json --id K1 --kind placement --x-mm 20 --y-mm 10 --width-mm 4 --height-mm 3
```

PCB authoring command behavior:
- `pcb add-pad` writes one rectangular pad into `board.pads`.
- `pcb add-via` writes one via into `board.vias`.
- `pcb add-track` writes one straight track segment into `board.tracks`.
- `pcb add-keepout` writes one rectangular keepout into `board.keepouts`.
- `pcb list-enabled-layers`, `pcb list-visible-layers`, `pcb get-layer-name`, and `pcb get-board-stackup` expose read-only KiCad-style layer/stackup query data for agents.
- `pcb get-rules` and `pcb get-outline` expose the current board-level design-rule and bounding-box slices.
- `pcb list-by-net` and `pcb list-connected` expose same-net pad, via, track, and zone queries with explicit first-slice connectivity scope.
- Board-context pad rows in `pcb list-objects`, `pcb list-by-net`, `pcb list-connected`, and `pcb export-route-job` include resolved KiCad layer-set metadata so agents do not need to re-expand wildcard layer selectors from raw storage.
- Commands mutate the file passed through `--file`.
- Commands reject missing boards, duplicate primitive IDs, invalid numeric dimensions, unknown layers, out-of-board positions/areas, and via drill larger than via diameter.

Help command behavior:
- `help --format json` emits a deterministic `commands` array.
- Each command entry includes `name`, `summary`, and `usage`.
- This is the preferred command discovery surface for agents.

Agent command behavior:
- `agent state` emits a deterministic headless workspace snapshot with session labels, empty first-slice task/evidence/approval state, and `durable_store:"not_configured"`.
- `agent tasks`, `agent evidence`, and `agent approvals` expose the corresponding state subsections as separate low-token JSON documents for batch jobs and external orchestrators.
- `agent serve` exposes matching JSON-RPC methods `agent.state`, `agent.workspace_state`, `agent.tasks`, `agent.evidence`, and `agent.approvals`.
- `agent session-schema` documents the provider-free local session file fields and forbidden secret fields.
- `agent session-new` creates one `.ccad-agent-session.json` file with `session_id`, matching `thread_id`, local resource URI, empty checkpoint list, and no provider or trace configuration.
- `agent session-state` reads the local session file and emits canonical deterministic JSON.
- `agent checkpoint-add` appends one ordered checkpoint record with checkpoint ID, sequence, kind, summary, optional artifact path, timestamp, and checkpoint resource URI.
- `agent replay` emits a replay manifest with the session resource URI, latest checkpoint ID, checkpoint count, replayable flag, and ordered checkpoint records.
- `agent serve` exposes read-only session routes `agent.session_schema`, `agent.session_state`, and `agent.replay_manifest`.
- `agent policy-schema` emits the local policy result fields, approval reasons, and decision names that future runners and GUI surfaces must understand.
- `agent policy-check -- <ccad command args...>` classifies a command by argument tokens without opening design files or executing the command.
- `agent dry-run -- <ccad command args...>` returns the same classification with `dry_run:true`, `would_execute:false`, and `decision:"dry_run_only"`.
- `agent serve` exposes matching `agent.policy_schema` and `agent.policy_check` routes and uses the same classifier for `execute`/`tools/call` permission gates.
- `agent provider-config-schema`, `agent provider-config-template`, and `agent provider-status` document official API, OpenAI-compatible, Anthropic, Google Gemini, and local model server configuration through env-var names only.
- `agent trace-export-schema`, `agent trace-export-template`, `agent trace-redaction-policy`, and `agent trace-export-dry-run` document OpenTelemetry/Langfuse trace export planning, keep export disabled by default, redact OTLP header values, and perform no network probes.
- `agent kicad-evidence-schema`, `agent kicad-evidence-plan`, `agent kicad-evidence-dry-run`, and `agent kicad-evidence-run` document and plan KiCad CLI DRC/ERC/fabrication evidence commands as structured arrays rather than free-form shell strings.
- `agent kicad-evidence-run` does not execute unless the caller passes `--execute` or JSON-RPC `execute:true`; read-only `agent serve` rejects JSON-RPC execution with `-32604` and `external_process_file_write`.
- `agent pcb-api-schema` documents the current KiCad PCB API handler mapping for headless CCad automation. It declares complete, first-slice, and not-yet-supported surfaces so agents do not confuse `list-connected` same-net queries with KiCad's full connectivity graph.
- `agent serve` exposes matching `agent.provider_config_schema`, `agent.provider_config_template`, `agent.provider_status`, `agent.trace_export_schema`, `agent.trace_export_template`, `agent.trace_redaction_policy`, `agent.trace_export_dry_run`, `agent.kicad_evidence_schema`, `agent.kicad_evidence_plan`, `agent.kicad_evidence_dry_run`, and `agent.kicad_evidence_run` routes.
- Write-capable or unknown commands are reported with `approval_required:true`, a risk level, and an approval reason such as `project_mutation`, `file_write`, `agent_session_write`, or `unknown_command`.
- The method catalog and `agent tool-guide --method <name>` include the new state methods so agents can discover them without scraping docs.
- These commands do not read GUI-only in-memory state, call providers, store secrets, export telemetry, or run model loops. They create only local session/checkpoint metadata files for future GUI, policy, provider, and observability binding.

Current limitation:
- These commands are explicit primitive authoring verbs, not automatic placement or routing.
- They do not yet enforce schematic-layout parity or physical DRC beyond command argument guards.

DRC command behavior:
- `drc <path>` runs physical board checks.
- Primary DRC reports `UNROUTED_NET` errors when same-net pad/via physical endpoints remain in disconnected exact-coordinate components; GUI diagnostics and markers consume this shared result.
- Reports duplicate primitive IDs, unknown layers, unknown non-empty net references, geometry outside board outline, invalid dimensions, rectangular keepout violations, unconnected pads/vias/tracks, unconnected track endpoints, via drill larger than diameter, and zero-length track segments.
- Reports `COPPER_CLEARANCE` errors when different-net pad-pad, track-track, pad-track, via-via, via-pad, or via-track copper is closer than the current fixed default `0.20 mm` clearance.
- Allows same-net copper to touch.
- Pad centers, via centers, track endpoints, and track segments crossing rectangular keepouts are errors: `PAD_IN_KEEPOUT`, `VIA_IN_KEEPOUT`, `TRACK_ENDPOINT_IN_KEEPOUT`, and `TRACK_CROSSES_KEEPOUT`.
- Empty pad `net_id` is reported as warning code `UNCONNECTED_PAD`.
- Empty via and track `net_id` values are warnings: `UNCONNECTED_VIA` and `UNCONNECTED_TRACK`.
- Track endpoints that do not exactly touch a same-net pad center, via center, or another track endpoint are reported as warning code `UNCONNECTED_TRACK_ENDPOINT`.
- Unknown physical net references are errors: `UNKNOWN_PAD_NET`, `UNKNOWN_VIA_NET`, and `UNKNOWN_TRACK_NET`.
- Exits `0` for no DRC errors, `1` for DRC errors, and `2` for usage/file/parse failure.

KiCad footprint import behavior:
- `lib import-footprint --in <file.kicad_mod> --out <file.json>` reads one KiCad footprint file.
- Imports footprint name and basic pad number/type/shape/position/size/simple drill/layers.
- Writes deterministic CCad footprint JSON.
- Rejects malformed s-expressions and non-`footprint` roots.
- Treats KiCad input as data only; it does not execute scripts or resolve external model paths.

Catalog CLI behavior:
- `lib catalog-info --catalog <path>` loads a local CCad native library catalog and emits summary JSON.
- `lib catalog-find --catalog <path> --id <id>` emits the matching item metadata as JSON, including component-knowledge fields.
- `lib catalog-search --catalog <path> --query <text>` emits matching item metadata as JSON, including component-knowledge fields.
- `lib catalog-search --catalog <path> --query <text> --kind <kind>` restricts matches to one item kind such as `footprint`, `symbol`, or `model`.
- `lib catalog-validate --catalog <path>` validates required metadata and duplicate item IDs before a catalog is trusted.
- `lib catalog-validate --catalog <path> --root <dir>` also verifies local native artifact existence and SHA-256 checksums.
- Missing catalog items return exit code `1` and emit `{ "found": false, ... }`.
- Parse/file/usage failures return exit code `2`.
- These commands read local catalog files only and do not fetch network sources.

Footprint placement behavior:
- `pcb place-footprint` loads CCad footprint JSON and creates board pads.
- Generated pad IDs are `<component>.<pad-number>`.
- Generated pad `net_id` values inherit existing logical net membership when a project net contains the placed component ID and matching footprint pad number.
- Optional `--rotation-deg` rotates footprint-local pad centers and pad orientation.
- Command rejects missing board, unknown layer, duplicate generated pad IDs, empty footprints, and out-of-board pad positions.

Schematic symbol placement behavior:
- `sch place-symbol` loads one converted CCad/KiCad symbol JSON file and adds a logical component.
- Generated component placement stores position and rotation in nanometer-backed project coordinates.
- The component stores a `symbol` snapshot containing pins and supported graphics, so schematic rendering does not depend on re-opening the library file after save/reload.
- Local converted-symbol inheritance is resolved before the snapshot is stored, so variants such as diode aliases inherit parent pins.
- Command rejects duplicate component IDs, missing symbol files, empty symbols, malformed JSON, and invalid numeric placement values.

Test:
```bash
cmake --build build --target ccad_cli_tests
ctest --test-dir build -R cli --output-on-failure
```

Expected result:
- CLI creates a project file.
- CLI validates a clean file with exit code `0`.
- CLI validates an invalid file with nonzero exit and JSON diagnostics.
- CLI PCB commands append primitives and reject invalid mutations.
- CLI DRC command emits JSON physical diagnostics.
- CLI footprint import command emits deterministic footprint JSON.
- CLI footprint placement command creates visible board pads from imported footprint data.
- CLI schematic symbol placement command stores a component symbol snapshot with preserved pin lead metadata.

## Project Review Model

Status: implemented.
Files:
- `src/ccad_core/review.hpp`
- `src/ccad_core/review.cpp`
- `tests/test_review.cpp`

What it does:
- Builds a human-review summary from a `Project`.
- Reports project ID/name.
- Reports component, net, and constraint counts.
- Carries ERC diagnostics.
- Carries physical DRC diagnostics.
- Produces status text such as `Clean: 1 component, 1 net, 1 constraint`, `Warnings: 1`, or `Errors: 1, warnings: 0`.

Test:
```bash
cmake --build build --target ccad_review_tests
ctest --test-dir build -R review --output-on-failure
```

Expected result:
- `review` test passes.

## Project Diff Model

Status: implemented.
Files:
- `src/ccad_core/diff.hpp`
- `src/ccad_core/diff.cpp`
- `tests/test_diff.cpp`

What it does:
- Compares two project objects.
- Reports added, removed, and changed components, nets, and constraints.
- Provides counts and entries for CLI, GUI, and future transaction review.

Test:
```bash
cmake --build build-qt --target ccad_diff_tests
ctest --test-dir build-qt -R diff --output-on-failure
```

Expected result:
- `diff` test passes.

## Physical Units And Board Primitives

Status: implemented.
Files:
- `src/ccad_core/geometry.hpp`
- `src/ccad_core/geometry.cpp`
- `src/ccad_core/model.hpp`
- `src/ccad_core/serialize.cpp`
- `tests/test_geometry.cpp`
- `tests/test_serialize.cpp`

What it does:
- Stores physical lengths as integer nanometers.
- Converts millimeters and mils deterministically.
- Represents points, sizes, rectangles, board outline, layers, and rectangular keepouts.
- Represents drawable PCB primitives: pads, vias, track segments, dimensions, groups, barcodes, reference images, and tables.
- Preserves primitive IDs, net IDs, layer IDs, component/pin ownership, and geometry through JSON round-trip.
- Serializes board data in project JSON.

Example board JSON shape:
```json
{
  "id": "proj-primitive-demo",
  "name": "primitive-demo",
  "board": {
    "outline": {
      "origin": { "x_nm": 0, "y_nm": 0 },
      "size": { "width_nm": 42000000, "height_nm": 28000000 }
    },
    "layers": [
      { "id": "F.Cu", "name": "Front copper", "kind": "signal" },
      { "id": "B.Cu", "name": "Back copper", "kind": "signal" }
    ],
    "pads": [
      {
        "id": "pad-u1-1",
        "component_id": "U1",
        "pin_name": "1",
        "net_id": "GND",
        "layer_id": "F.Cu",
        "position": { "x_nm": 5000000, "y_nm": 6000000 },
        "size": { "width_nm": 1500000, "height_nm": 1000000 }
      }
    ],
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
    ],
    "vias": [
      {
        "id": "via-gnd-1",
        "net_id": "GND",
        "position": { "x_nm": 8000000, "y_nm": 9000000 },
        "diameter_nm": 800000,
        "drill_nm": 400000
      }
    ],
    "tracks": [
      {
        "id": "trk-gnd-1",
        "net_id": "GND",
        "layer_id": "F.Cu",
        "start": { "x_nm": 5000000, "y_nm": 6000000 },
        "end": { "x_nm": 8000000, "y_nm": 9000000 },
        "width_nm": 250000
      }
    ]
  }
}
```

Test:
```bash
cmake --build build-qt --target ccad_geometry_tests ccad_tests
ctest --test-dir build-qt -R "geometry|serialize" --output-on-failure
```

Expected result:
- `geometry` and `serialize` tests pass.

## Board Canvas Scene Model

Status: implemented.
Files:
- `src/ccad_core/canvas.hpp`
- `src/ccad_core/canvas.cpp`
- `tests/test_canvas.cpp`

What it does:
- Converts project board data into a deterministic canvas scene model.
- Reports whether a board exists.
- Converts board width/height from nanometers to millimeter view units.
- Converts pads, vias, and track segments into millimeter view units.
- Converts rectangular keepouts into millimeter view units.
- Keeps GUI rendering inputs testable outside Qt.

Test:
```bash
cmake --build build-qt --target ccad_canvas_tests
ctest --test-dir build-qt -R canvas --output-on-failure
```

Expected result:
- `canvas` test passes.

## Transaction Journal Model

Status: implemented.
Files:
- `src/ccad_core/transaction.hpp`
- `src/ccad_core/transaction.cpp`
- `tests/test_transaction.cpp`

What it does:
- Creates a transaction record from before/after project states.
- Records ID, command, summary, project IDs, and diff.
- Emits deterministic transaction JSON.

Test:
```bash
cmake --build build-qt --target ccad_transaction_tests
ctest --test-dir build-qt -R transaction --output-on-failure
```

Expected result:
- `transaction` test passes.

## Native Library Catalog Metadata

Status: implemented foundation.
Files:
- `src/ccad_core/library_catalog.hpp`
- `src/ccad_core/library_catalog.cpp`
- `tests/test_library_catalog.cpp`

What it does:
- Represents a local/offline CCad library catalog.
- Records upstream source metadata: name, kind, URL, commit, mirror, and fetch timestamp.
- Records item metadata: ID, kind, name, source path, native path, checksum, license, provenance, component-knowledge fields, and import warnings.
- Preserves `usage_summary`, `layout_notes`, `source_confidence`, and `review_status` for future local component search and human/AI curation.
- Serializes catalog data to deterministic JSON.
- Loads catalog JSON strictly and rejects missing source/items data.
- Finds catalog items by stable ID.
- Searches catalog identity, paths, usage summaries, layout notes, source confidence, and review status.
- Validates `review_status` when present. Current accepted values are `generated`, `needs_review`, `reviewed`, and `rejected`.

Design intent:
- KiCad/GitHub/Gitee libraries are source data.
- CCad runtime should query local native catalogs instead of repeatedly fetching remote libraries or reparsing raw KiCad files.
- Huge caches should live under ignored local paths such as `library-cache/` or `catalog-cache/`, or in a future dedicated catalog package.
- Rich component knowledge should be preprocessed into the local catalog so agents can pick parts using local metadata before escalating to web research or human review.

Test:
```bash
cmake --build build-qt --target ccad_library_catalog_tests
ctest --test-dir build-qt -R library_catalog --output-on-failure
```

Expected result:
- `library_catalog` test passes.

## Optional Qt Review GUI

Status: implemented as optional target.
Files:
- `src/ccad_gui/main.cpp`
- `CMakeLists.txt`

What it does:
- Builds `ccad_gui` when Qt 6 Widgets is available.
- Provides a native desktop window for human review.
- Opens `.ccad.json` project files.
- Shows project summary.
- Shows board dimensions and layer count when board data exists.

## GUI Features
- Renders board outline, tracks, and pads natively via `QGraphicsScene` and a decoupled `CanvasScene` model.
- Schematic tab renders logical `Components` and `Wires` from the same JSON backend.
- Displays KiCad-styled Top, Left, and Right toolbars.
- The top toolbar uses KiCad-style icons and executes real Save, Board Setup, Undo, Redo, and Run DRC behavior.
- The Tools menu can export a JSON DRC report.
- The Add Symbol and Add Footprint chooser uses filtered library/name rows with detail metadata and visual symbol/footprint previews.
- The Add Symbol and Add Footprint chooser indexes cache filenames first and parses only the selected row for details and preview, keeping large `library-cache` folders responsive.
- Footprint previews share the GUI board canvas layer palette so KiCad layer IDs such as `F.Cu`, `B.Cu`, `F.SilkS`, `F.Fab`, `F.CrtYd`, `Edge.Cuts`, and user layers render with distinct intent colors.
- Symbol placement resolves converted KiCad symbol inheritance from sibling `library-cache` JSON files before checking pins.
- Footprint placement accepts raw KiCad `.kicad_mod` files from the chooser as well as converted CCad footprint JSON.
- Provides a summary dock with component and routing counts.
- Uses a CAD editor shell with a central PCB canvas tab, docked project/layer/diagnostics panels, explicit Fit, wheel zoom, middle-button pan, and cursor/zoom status.
- GUI project summary and diagnostics rendering are split into dedicated Qt widgets to keep the shell maintainable.
- GUI canvas primitives are selectable and expose stable type/ID feedback in the status bar and Layers / Objects dock.
- GUI selection has a dedicated read-only inspector panel that shows selected object type and ID.
- GUI right dock includes a read-only layer and object browser derived from the canvas scene.
- GUI selected primitives use shape-level highlights instead of loose bounding boxes.
- GUI selected primitive highlights derive from each object's display color so future theme palettes can stay coherent.
- GUI diagnostic rows can select a matching PCB canvas object when the row carries a stable object ID.
- GUI draws read-only diagnostic markers over PCB canvas objects named by diagnostics.
- GUI toolbar exposes Fit, Zoom Out, Zoom In, and 100% canvas review controls.
- GUI status readout distinguishes board coordinates from off-board canvas coordinates.
- GUI screenshot mode captures the app through `ccad_gui --screenshot <project.ccad.json> <out.png>` after the current 7-second single-preview event-processing wait.
- GUI cursor, selection, status, and inspector callbacks tolerate empty projects and board-only projects instead of assuming `boards[0]` exists.
- Shows ERC diagnostics table.
- Supports reload.
- **Measurement Tool**: Click the Measure button on the Right Toolbar, click a start point on the canvas, and drag to see a real-time overlay line and text displaying Euclidean distance (mm), `dx`, and `dy`.

Build with Qt:
```bash
cmake -S . -B build-qt -DCCAD_BUILD_GUI=ON -DCMAKE_PREFIX_PATH=C:\Qt\6.11.1\mingw_64
cmake --build build-qt --target ccad_gui
```

Run:
```bash
build-qt/ccad_gui
```

On Windows:
```powershell
$env:PATH = "C:\Qt\6.11.1\mingw_64\bin;$env:PATH"
.\build-qt\ccad_gui.exe
```

Capture a sprint demo screenshot without depending on the foreground window:
```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --screenshot artifacts\demos\sprint42-gui-canvas-mvp-review.ccad.json artifacts\screenshots\sprint42-gui-canvas-mvp-review.png"
```

Expected result:
- The command waits about 7 seconds, writes the PNG, exits cleanly, and leaves no `ccad_gui.exe` process running.

Open a project directly:
```powershell
$env:PATH = "C:\Qt\6.11.1\mingw_64\bin;$env:PATH"
.\build-qt\ccad.exe init --name gui-demo --out .\build-qt\gui-demo.ccad.json
.\build-qt\ccad_gui.exe .\build-qt\gui-demo.ccad.json
```

Board canvas demo:
```powershell
$env:PATH = "C:\Qt\6.11.1\mingw_64\bin;$env:PATH"
.\build-qt\ccad.exe init --name canvas-demo --width-mm 42 --height-mm 28 --out .\build-qt\canvas-demo.ccad.json
.\build-qt\ccad_gui.exe .\build-qt\canvas-demo.ccad.json
```

Or run the demo script:
```powershell
.\scripts\run_gui_demo.ps1
```

If Qt is not installed:
- CMake prints `Qt6 Widgets not found; skipping ccad_gui target`.
- Core, CLI, and tests still build.

Manual test:
1. Build with Qt installed.
2. Run `ccad_gui`.
3. Choose `File > Open`.
4. Open a `.ccad.json` file created by `ccad init`.
5. Confirm summary and diagnostics display.
6. Edit the file externally or regenerate it.
7. Click Reload.

Expected result:
- GUI opens project file.
- Summary counts match the file.
- Clean files show clean status.
- Invalid files show diagnostics.
- Board files show the outline on the dark canvas.
- Board files with primitives show red tracks, pink pads, yellow vias, and orange dashed keepout regions.
- Empty projects, board-only projects, no-argument startup, and the current sprint demo project keep `ccad_gui.exe` alive for at least 7 seconds after launch.

Current limitation:
- The GUI is now an early CAD editor shell, but DRC overlay markers, layer visibility toggles, net highlight, richer object properties, editing, schematic rendering, and transaction timeline are still future work.
- Primitive authoring currently happens through the project JSON/kernel path; command verbs for placement/routing are planned next.

## Research And Architecture Docs

Status: implemented.
Files:
- `docs/research/2026-05-14-llm-native-pcb-tool-report.md`
- `docs/research/2026-05-14-kicad-feature-map.md`
- `docs/superpowers/specs/2026-05-14-kernel-mvp-design.md`
- `docs/superpowers/plans/2026-05-14-kernel-mvp.md`
- `docs/superpowers/specs/2026-05-14-qt-review-gui-design.md`
- `docs/superpowers/plans/2026-05-14-qt-review-gui.md`
- `docs/technical-handover.md`
- `AGENTS.md`

What it does:
- Documents the LLM-native PCB thesis.
- Documents prior art and reuse strategy.
- Documents the current kernel MVP.
- Documents the Qt review GUI design.
- Documents agent handover and sprint process.

Review:
Open the files above in the editor.

## Sprint 127 Route Review Addendum

The project review model, `ccad inspect`, and native GUI now expose board route progress derived from route requests and track provenance. The GUI project summary shows route request count, open routes, partial routes, completed routes, and routed segment count. The object browser shows route-request intent rows, shows `source_route_request_id` provenance on track rows, and lets route rows select routed tracks generated from that request.

## Sprint 128 KiCad Layer Foundation Addendum

CCad now has a core KiCad standard PCB layer registry in `src/ccad_core/layers.hpp/.cpp`. The registry preserves KiCad canonical layer IDs for copper, paired technical layers, board geometry layers, and user layers through `User.9`, while mapping them into CCad layer kinds such as `copper`, `paste`, `mask`, `silkscreen`, `courtyard`, `fabrication`, `board_edge`, `margin`, and `user`.

The CLI now exposes `ccad pcb add-standard-layers --file <path>`. It appends missing standard KiCad layers idempotently, preserves existing custom layers, and leaves copper-only authoring guards unchanged for pads, tracks, route requests, and placed footprints.

Test with:

```bash
cmake --build build-qt --target ccad_layer_tests ccad_cli_tests
ctest --test-dir build-qt -R layers --output-on-failure
ctest --test-dir build-qt -R cli --output-on-failure
```

## Sprint 129 Layer Review Summary Addendum

`ProjectReview`, `ccad inspect`, and the native GUI project summary now expose layer category and visibility summaries. Agents can read `board.layer_summary.copper`, `board.layer_summary.non_copper`, `board.layer_summary.visible`, and `board.layer_summary.hidden` from inspect JSON. Humans see matching "Layer Breakdown" and "Layer Visibility" cards in the project summary dock. This file provides a user-facing inventory of all capabilities fully implemented in the project up to **Sprint 225**. It serves as a verification checklist for agents, documenting exactly *what* works and *how* to prove it.

This is review metadata only. It does not change routing, placement, stackup material modeling, or copper authoring guards.

## Sprint 130 Interactive Layer Visibility Addendum

Interactive layer visibility controls are added to the right-dock layer browser in the native review GUI. Checkable list items permit toggling layer visibility on the fly, immediately updating the board model and triggering canvas re-rendering.

## Sprint 131 Coordinate Inspection Addendum

The Selection Inspector Panel in the GUI displays coordinate and physical property readouts for selected board primitives (pads, vias, tracks, keepouts, placement regions), rendering all lengths in both millimeters (mm) and mils (mil).

## Sprint 132 Physical DRC and Clearance Constraints Editing Addendum

The Selection Inspector Panel supports interactive input fields to edit design rules (copper clearance, track width, via annular ring) and physical object properties (pad size/rotation, via diameter/drill, track width, keepout/region area), updating the model, saving changes to the project JSON, and triggering canvas and DRC diagnostics re-rendering.

## Sprint 133 KiCad Board Export Addendum

Implemented KiCad-compliant S-expression PCB export (`.kicad_pcb` format) from the CCad `Board` model. Users can run the `ccad pcb export-kicad --file <path> --output <path>` command to export their CCad projects into fabricatable KiCad PCB design files.

Test with:
```bash
cmake --build build-qt --target ccad_kicad_pcb_export_tests ccad_cli_tests
ctest --test-dir build-qt -R kicad_pcb_export --output-on-failure
ctest --test-dir build-qt -R cli --output-on-failure
```

## Sprint 134 KiCad Footprint Export Addendum

Implemented KiCad-compliant S-expression footprint export (`.kicad_mod` format) from the CCad `Footprint` model. Users can run the `ccad lib export-footprint --in <path.json> --out <path.kicad_mod>` command to export native footprints back into KiCad footprint files.

Test with:
```bash
cmake --build build-qt --target ccad_kicad_footprint_export_tests ccad_cli_tests
ctest --test-dir build-qt -R kicad_footprint_export --output-on-failure
ctest --test-dir build-qt -R cli --output-on-failure
```

## Sprint 217 Marketplace & Slash Command UI Addendum

Implemented a dynamic `/` command autocomplete popup directly over the `chat_input_`, wiring keyboard events (Up/Down/Enter/Escape) to slash-command templates like `/route`. Transitioned `AgentMarketplaceDialog` to natively load JSON components from the live HTTP catalog via `QNetworkAccessManager`. Completed the integration of the right-side Agent panel buttons (Settings, STT, Context, File Attachment), binding them to dispatch JSON-RPC commands like `agent.provider_config_schema` directly to the `orchestrator.py` standard input.

Test with:
```bash
ctest --test-dir build-qt -R gui_agent_panel --output-on-failure
```

## Sprint 135 Specctra DSN Export

Implemented Specctra `.dsn` export from the CCad `Board` model to enable integration with external routing engines (e.g. Freerouting). Users can run `ccad pcb export-dsn --file <path> --output <path.dsn>` to export their board state.

Test with:
```bash
cmake --build build-qt --target ccad_dsn_export_tests ccad_cli_tests
ctest --test-dir build-qt -R dsn_export --output-on-failure
ctest --test-dir build-qt -R cli --output-on-failure
```

## Sprint 141 KiCad Component Ingestion & Primitive Rendering

CCad now natively renders Symbol and Footprint primitive graphics (Lines, Arcs, Circles, Polygons, Text) within the Qt Canvas. `kicad_footprint_import` and `kicad_symbol_import` map KiCad S-expression graphics into CCad's `Graphic` primitives structure. The GUI Review Window can natively preview ingested KiCad component geometries. The Python script `scripts/ingest_kicad_libraries.py` orchestrates bulk library conversion.

## Sprint 142 BOM Export

Implemented Bill of Materials (BOM) export to CSV format from the CCad `Project` model. Users can run `ccad project export-bom --file <path.json> --output <path.csv>` to extract the list of components, designators, and parts for manufacturing and assembly.

Test with:
```bash
cmake --build build-qt --target ccad_bom_export_tests ccad_cli_tests
ctest --test-dir build-qt -R bom_export --output-on-failure
ctest --test-dir build-qt -R cli --output-on-failure
```

## Sprint 143 Pick and Place (PnP) Export

Implemented Pick and Place (PnP) data export to CSV format for assembly. Users can run `ccad pcb export-pnp --file <path.json> --output <path.csv>` to extract component placement centroids and side (Top/Bottom) inferred from pad geometries.

Test with:
```bash
cmake --build build-qt --target ccad_pnp_export_tests ccad_cli_tests
ctest --test-dir build-qt -R pnp_export --output-on-failure
ctest --test-dir build-qt -R cli --output-on-failure
```

## Sprint 144 Excellon Drill Export

Implemented Excellon NC Drill file export for manufacturing. The `ccad pcb export-drill` command emits coordinates of all vias and through-hole pads (which now correctly preserve the `drill` property after placement) into standard Excellon format.

Test with:
```bash
cmake --build build-qt --target ccad_drill_export_tests ccad_cli_tests
ctest --test-dir build-qt -R drill_export --output-on-failure
ctest --test-dir build-qt -R cli --output-on-failure
```

## Sprint 145 Interactive Placement and Library Browser

Implemented interactive footprint placement and a unified library browser in the Qt native GUI. Users can click "Add Footprint" or "Add Symbol" to invoke a flat, filtered Library Browser dialog which searches `library-cache/footprints` or `library-cache/symbols`. After selecting a footprint, the user enters an interactive placement mode where a ghost of the footprint tracks the mouse cursor on the canvas. A left-click commits the placement by invoking the core kernel API. Users can also press 'M' over an existing component's pad to pick up and move the component interactively, with left-click to drop and 'Esc' to cancel.

## Sprint 148 KiCad GUI Parity and Pad Rendering

The native Qt GUI now uses KiCad-style icon toolbars for the left and right PCB tool strips, resolving icon SVGs from the local KiCad source checkout through `CCAD_KICAD_SRC` or a sibling `kicad_src` directory. The board canvas renders pad shape metadata more faithfully: circular and oval pads draw as curved geometry, through-hole pads show drill openings and annular rings, and interactive footprint-placement previews use the same shape-aware drawing instead of rectangular placeholders. Placement and move interactions convert canvas scene coordinates back to board millimeters before invoking the core placement APIs.

Test with:
```bash
cmake --build build-qt --target ccad_gui_canvas_selection_style_tests ccad_gui
ctest --test-dir build-qt -R gui_canvas_selection_style --output-on-failure
powershell -ExecutionPolicy Bypass -File .\scripts\run_sprint_demo.ps1 -Name sprint148-gui-kicad-parity
```

## Sprint 149 Pad Shape and Layer Fidelity

KiCad footprint import now preserves `roundrect_rratio` and `chamfer_ratio` pad metadata for library footprints, placed board pads, CCad project JSON, project diffs, and KiCad PCB export. The native Qt canvas and interactive placement/move ghost previews render ratio-controlled roundrect pads, trapezoid pads, chamfered rectangles, circular through-hole pads, and drill openings through the same shape-aware path logic, reducing the old rectangle-heavy rendering gap.

Test with:
```bash
cmake --build build-qt
ctest --test-dir build-qt -R "^(serialize|kicad_footprint_import|kicad_footprint_export|kicad_pcb_export|canvas|gui_canvas_selection_style)$" --output-on-failure
powershell -ExecutionPolicy Bypass -File .\scripts\run_sprint_demo.ps1 -Name sprint149-pad-shape-layer-fidelity
```

## Sprint 150 Standard Layer Registry Fidelity

CCad now aligns its standard KiCad PCB layer registry with current KiCad source ordering: `Edge.Cuts` is canonical layer 44, `Margin` is 45, `B.CrtYd` and `F.CrtYd` are 46 and 47, fabrication layers are 48 and 49, and `User.1` through `User.9` occupy 50 through 58. KiCad PCB export now emits that same layer table instead of the older misplaced Edge/Margin ordering.

The kernel exposes `standardKiCadPcbLayerNumber(id)` so other code paths do not duplicate layer-number knowledge. Agent-facing `ccad pcb list-objects` and `ccad pcb get-object` layer JSON include `kicad_layer_number` for canonical layers, while custom layers remain unnumbered.

Test with:
```bash
cmake --build build-qt --target ccad_layer_tests ccad_kicad_pcb_export_tests ccad_cli_tests
ctest --test-dir build-qt -R "^(layers|kicad_pcb_export|cli)$" --output-on-failure
```

## Sprint 151 KiCad Pad Authoring CLI

The machine-callable PCB authoring surface now exposes KiCad-style pad metadata directly. `ccad pcb add-pad` accepts `--type`, `--shape`, `--drill-mm`, `--roundrect-rratio`, and `--chamfer-ratio` in addition to multi-layer `--layers`. `ccad pcb set-pad` can update pad type, shape, roundrect ratio, and chamfer ratio along with component, pin, net, layers, and rotation.

The ratio options are validated to KiCad-compatible `0.0` through `0.5` ranges before the project file is written. This lets agents author roundrect SMD pads, chamfered pads, and through-hole circular pads without manually editing JSON or importing a temporary footprint.

Test with:
```bash
cmake --build build-qt --target ccad_cli_tests
ctest --test-dir build-qt -R cli --output-on-failure
```

## Sprint 152 Rich Pad Query Contract

`ccad pcb get-object --id <pad>` now returns KiCad-style pad metadata in the agent-facing object contract: `pad_type`, `shape`, `layers`, optional `drill_nm`, optional `roundrect_rratio`, optional `chamfer_ratio`, position, rotation, and size. `ccad pcb list-objects --type pad` emits the same metadata in compact rows, including the optional drill and ratio fields when present.

This closes the query side of Sprint 151. Agents can now author a through-hole, roundrect, or chamfered pad, then inspect the same metadata without scraping the full project JSON.

Test with:
```bash
cmake --build build-qt --target ccad_cli_tests
ctest --test-dir build-qt -R cli --output-on-failure
```

## Sprint 153 Route-Job Pad Metadata

`ccad pcb export-route-job` now includes KiCad-style pad metadata in `route_job.physical_objects.pads`. Each pad entry keeps the legacy first `layer_id` field and adds full `layers`, `pad_type`, `shape`, `rotation_degrees`, optional `drill_nm`, optional `roundrect_rratio`, and optional `chamfer_ratio`.

This gives external routers and AI planning tools the same pad geometry intent that the CLI can author and query, instead of collapsing every pad into a rectangle-like obstacle.

Test with:
```bash
cmake --build build-qt --target ccad_cli_tests
ctest --test-dir build-qt -R cli --output-on-failure
```

## Sprint 154 GUI Actions And Library Cache Placement

The native Qt GUI now replaces the top-toolbar placeholder buttons with working KiCad-style actions. Save writes the current project JSON, Board Setup edits board-level DRC rule values, Undo and Redo restore GUI-originated project snapshots, Run DRC refreshes diagnostics and markers, and the Tools menu can export a JSON DRC report.

The library chooser is closer to KiCad's symbol and footprint chooser shape. It filters local library-cache entries, displays library/name rows, shows selected-item details, and explains the placement path. Symbol placement now resolves local converted-symbol inheritance, so variants such as `1N4007` can inherit pins from `1N4001` before the GUI calls `placeComponent()`. Footprint placement now accepts raw `.kicad_mod` files selected from the chooser, not only converted CCad JSON.

Test with:
```bash
cmake --build build-qt --target ccad_kicad_symbol_import_tests ccad_gui_footprint_placement_tests ccad_gui_symbol_placement_tests ccad_gui_review_window_cursor_status_tests
ctest --test-dir build-qt -R "^(kicad_symbol_import|gui_footprint_placement|gui_symbol_placement|gui_review_window_cursor_status)$" --output-on-failure
```

## Sprint 155 KiCad Placement Chooser And Ghost Placement

The native Qt GUI now routes placement through the active editor tab instead of exposing raw KiCad file previews as normal editor actions. In the PCB tab, the Add tool opens a local `library-cache/footprints` chooser and enters footprint ghost placement. In the Schematic tab, the Add tool opens a local `library-cache/symbols` chooser and enters symbol ghost placement. The selected item follows the mouse, left click commits through the core placement APIs, and Escape cancels the active placement before commit.

The chooser now uses a KiCad-inspired table layout with item, description, and library columns, plus a detail panel showing path and cache metadata. It lazy-loads enough metadata to show pad counts for footprints and pin counts for symbols without asking the user to browse a file path manually.

The PCB canvas now uses different default colors for front and back copper, matching KiCad's intent that `F.Cu` and `B.Cu` read as separate layers. Selected tracks now draw a selection overlay wider than the rendered copper stroke, so selection no longer appears as only a thin centerline.

Known limitation: the immediate schematic placement ghost renders imported symbol primitives, but the durable project model still stores placed symbols as logical components. A future sprint needs schematic primitive persistence or symbol-library references so saved schematic views can rehydrate the full selected symbol after reload.

Test with:
```bash
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
powershell -ExecutionPolicy Bypass -File .\scripts\run_sprint_demo.ps1 -Name sprint155-kicad-placement-chooser-final
```

## Sprint 156 Lazy Library Chooser Loading

The native Qt library chooser now opens from a lightweight catalogue scan instead of parsing every cached footprint or symbol file during dialog construction. This matches KiCad's scale model more closely: the chooser lists library/name/file rows first, selection parses exactly one row to show details and a visual preview, and accepting the row leaves final geometry loading on the existing placement path.

This keeps Add Footprint and Add Symbol usable when `library-cache` contains large KiCad-derived libraries. The chooser still supports converted CCad JSON, raw `.kicad_mod` footprints, and raw `.kicad_sym` symbol libraries.

Footprint preview colors now come from the same KiCad-inspired layer palette as the actual PCB canvas. This keeps copper, silkscreen, fabrication, courtyard, edge, and user-layer graphics visually distinct in both contexts.

Test with:
```bash
cmake --build build-qt --target ccad_gui_footprint_placement_tests ccad_gui_symbol_placement_tests
ctest --test-dir build-qt -R "^(gui_footprint_placement|gui_symbol_placement)$" --output-on-failure
powershell -ExecutionPolicy Bypass -File .\scripts\run_gui_interaction_demo.ps1 -Name sprint156-live-footprint-preview -Mode Footprint
powershell -ExecutionPolicy Bypass -File .\scripts\run_gui_interaction_demo.ps1 -Name sprint156-live-symbol-preview -Mode Symbol
```

## Sprint 160 Placement Crash And CI Hardening

The GUI placement commit path now closes and validates the project file before reloading it. This fixes the Windows race where left-click footprint placement could reload an empty or partially flushed `.ccad.json` file, show a blocking parse-error modal, and leave placement tests hung. The same interaction path cancels placement ghosts before re-rendered scene items are destroyed, preventing stale ghost pointers after commit.

The GUI has an app-owned placement-click smoke mode for regression testing:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --test-place-footprint-click <project.ccad.json> <footprint.json-or-kicad_mod> <x-mm> <y-mm> <out.json>"
```

Expected result: the output JSON reports `performed:true`, `reason:"placed"`, and the updated board pad count for a valid board and footprint.

KiCad SVG icon lookup is more robust. The GUI checks `CCAD_KICAD_SRC`, the known local `F:\kicad_src` checkout, current-working-directory candidates, and executable-directory candidates before falling back. The GUI target now links Qt SVG explicitly, and CI installs `qt6-svg-dev` for the Linux GUI lane so KiCad-style SVG icons are a real dependency instead of accidental runtime behavior.

The KiCad symbol importer no longer includes a local untracked `symbol_json_reader.cpp.tmp` file. The reader is tracked as normal source, and symbol fill `type` parsing avoids variable shadowing under warnings-as-errors. DSN export now formats 64-bit nanometer coordinates directly so large board origins do not narrow through 32-bit integers.

Test with:

```cmd
cmd /c cmake --build build-qt --target ccad_gui_ui_map_tests ccad_gui ccad_kicad_symbol_import_tests ccad_dsn_export_tests --config Debug
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && ctest --test-dir build-qt -R gui_ui_map --output-on-failure"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && ctest --test-dir build-qt -R kicad_symbol_import --output-on-failure"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && ctest --test-dir build-qt -R dsn_export --output-on-failure"
```

## Sprint 161 UI Map Mouse Target Harness

The native GUI UI map now exposes menu and panel targets, including `menu:file`, `panel:project`, `panel:properties`, `panel:layers_objects`, and `panel:diagnostics`. Agents can resolve these through the same `--ui-target-id` contract as toolbar actions, tabs, canvases, and canvas objects.

The app-owned target-sequence harness proves that semantic targets can drive real cursor positioning and screenshot evidence. It moves the cursor to Select, Measure, Save, File, the properties/DRC panel, and the Agent tab, uses a 5-second initial load wait, waits about 800 ms before each per-target screenshot, draws a magenta marker at the resolved target point, resizes the window, and repeats the same sequence.

Run it with:

```cmd
cmd /c powershell -ExecutionPolicy Bypass -File .\scripts\run_ui_map_mouse_target_demo.ps1 -Name sprint161-ui-map-mouse-targets -ProjectPath artifacts\demos\sprint160-placement-crash-ci-final.ccad.json
```

Expected result: the script plays the docs beep, waits two seconds, captures marked screenshots under `artifacts\screenshots`, captures stdout/stderr logs, and writes a target-sequence JSON report.

## Sprint 162 Pad Layer Rendering Fidelity

The native Qt board canvas now treats pad copper, solder mask, and solder paste as separate visible layer classes instead of collapsing all pad-related layers into the copper color. `F.Mask`, `B.Mask`, `F.Paste`, and `B.Paste` have explicit KiCad-style colors, visible mask apertures render as `pad-mask` overlay objects, visible paste apertures render as `pad-paste` overlay objects, and hidden copper no longer renders as copper only because a matching technical layer is visible.

Footprint chooser previews and footprint placement ghosts use the same layer palette, so a selected footprint's copper, mask, paste, shape, drill, and annular-ring intent is visible before placement and remains consistent with the final board canvas.

Test with:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && cmake --build build-qt --target ccad_gui_canvas_selection_style_tests ccad_gui_footprint_placement_tests ccad_gui --config Debug && ctest --test-dir build-qt -R ""^(gui_canvas_selection_style|gui_footprint_placement)$"" --output-on-failure"
```

## Sprint 163 Live UI Map Server

The native GUI can keep a local UI-map query surface alive while the window remains open:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --serve-ui-map <project.ccad.json> <server-name> <ready-file>"
```

The server uses Qt local sockets and newline-delimited JSON. Initial request methods are `ui.map`, `ui.target`, and `ui.epoch`. This lets agents query semantic coordinates and UI state repeatedly without restarting the GUI process or relying on screenshots for every target lookup.

Test with:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && cmake --build build-qt --target ccad_gui_ui_map_tests ccad_gui --config Debug && ctest --test-dir build-qt -R gui_ui_map --output-on-failure"
```

## Sprint 164 Toolbar Action Contracts

The native GUI no longer leaves unfinished right-toolbar editor actions as silent stubs. At Sprint 164, Route Track, Add Via, Add Zone, Add Keepout, Draw Graphic, Place Text, and Delete stayed visible and enabled, but clicking them updated the status bar with a planned-tool state. Agent callers using `ccad_gui --ui-trigger-safe` got `performed:false`, `reason:"future_tool_not_implemented"`, and the user-facing label, while file-writing and dialog-opening actions continued to use the unsafe-action refusal contract. Later sprints promote Add Via, Route Track, Add Keepout, Delete, Draw Graphic, Place Text, and Add Zone to real kernel-backed tools.

Test with:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && cmake --build build-qt --target ccad_gui_ui_map_tests ccad_gui --config Debug && ctest --test-dir build-qt -R gui_ui_map --output-on-failure"
```

## Sprint 165 Left Toolbar Action Contracts

The native GUI now applies the same no-silent-stubs contract to left-toolbar display and panel controls. Toggle Grid, Polar Coordinates, Toggle Units, Crosshair Cursor, Show Ratsnest, Net Highlight, Display Modes, Show Layers, and Show Properties report visible planned-tool status on click and return `future_tool_not_implemented` through the safe UI trigger path until their full KiCad-like behavior is implemented.

Test with:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && cmake --build build-qt --target ccad_gui_ui_map_tests ccad_gui --config Debug && ctest --test-dir build-qt -R gui_ui_map --output-on-failure"
```

## Sprint 166 Left Toolbar Real Toggles

Show Layers and Show Properties now perform real panel toggles through the left toolbar. `action:layers_manager` hides or shows the Layers / Objects panel, `action:part_properties` hides or shows the Properties / DRC panel, and both safe-trigger responses return `performed:true` with `reason:"panel_toggled"`. Other left-toolbar display controls remain planned-tool contracts until their renderer/state behavior is implemented.

Test with:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && cmake --build build-qt --target ccad_gui_ui_map_tests ccad_gui --config Debug && ctest --test-dir build-qt -R gui_ui_map --output-on-failure"
```

## Sprint 167 Native Agent Panel Shell

The native Qt GUI now has a persistent bottom `Agent` panel. This is the first visible in-app shell for LLM-native co-working. It shows the active project and UI-map epoch, lets a human or automation refresh the current semantic UI map into read-only JSON output, and can trigger allowlisted safe UI actions by semantic ID. It does not call external providers or store API keys yet.

The panel is included in the semantic UI map as `panel:agent`, and `--ui-target-id` can resolve it like the other docked panels. This makes the panel itself targetable by the existing UI-map harness while future work connects LangGraph-style orchestration, OpenTelemetry/Langfuse tracing, provider settings, and tool-call streams.

Test with:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && cmake --build build-qt --target ccad_gui_agent_panel_tests ccad_gui_ui_map_tests ccad_gui --config Debug && ctest --test-dir build-qt -R ""^(gui_agent_panel|gui_ui_map)$"" --output-on-failure"
```

## Sprint 168 Left Toolbar Display Controls

The native Qt GUI now gives the remaining left-toolbar display controls real behavior instead of planned-tool responses. `action:grid` toggles the canvas grid overlay, `action:polar_coord` adds board-origin-relative radius and angle to the cursor status, `action:unit_inch` switches cursor coordinates to inches, `action:cursor_shape` toggles a full-window crosshair overlay, `action:show_ratsnest` shows or hides lightweight same-net guide lines, `action:net_highlight` selects same-net canvas objects, and `action:contrast_mode` switches to a high-contrast canvas render theme.

These controls are agent-callable through `ccad_gui --ui-trigger-safe` and return `performed:true`, `reason:"display_state_toggled"`, and the current state. UI-map action nodes now include a `checked` field so automation can inspect active display modes. The app-owned target-sequence harness now visits and triggers each display control in both the initial and resized passes.

The selection inspector also cleans up stale form editor widgets immediately during rapid multi-selection changes. This prevents net highlight and display-mode target runs from stacking old pad and track property editors in the right properties panel.

Test with:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && cmake --build build-qt --target ccad_gui_inspector_tests ccad_gui_board_canvas_view_tests ccad_gui_ui_map_tests ccad_gui_review_window_cursor_status_tests ccad_gui --config Debug && ctest --test-dir build-qt -R ""(gui_inspector|gui_board_canvas_view|gui_ui_map|gui_review_window_cursor_status)"" --output-on-failure"
```

Visual proof:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && powershell -ExecutionPolicy Bypass -File .\scripts\run_sprint_demo.ps1 -Name sprint168-left-toolbar-display-controls-verified"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && powershell -ExecutionPolicy Bypass -File .\scripts\run_ui_map_mouse_target_demo.ps1 -Name sprint168-left-toolbar-display-controls-targets-fixed -ProjectPath artifacts\demos\sprint168-left-toolbar-display-controls-final.ccad.json"
```

## Sprint 169 Visual Harness Timing Policy

The GUI visual-validation harness now matches the user timing policy directly. Single-preview screenshots use the 7-second settle default in `run_sprint_demo.ps1` and the app-owned screenshot modes. Multi-target GUI validation uses the existing 5-second initial load and 800 ms per-target waits in `run_ui_map_mouse_target_demo.ps1`, while `run_gui_interaction_demo.ps1` now exposes the same `InitialLoadMilliseconds = 5000`, `PerActionMilliseconds = 800`, and `WindowReadySeconds = 7` defaults for live mouse/keyboard tests.

The new `visual_harness_policy` CTest target reads the scripts and `.agents/workflows/visual-validation.md` as data. It fails if the obsolete 20-second wait pattern returns or if the required 7-second, 5000 ms, and 800 ms defaults drift.

The live mouse/keyboard wrapper now targets the foreground chooser dialog before selecting the first row. This fixes the prior main-window-relative click path where the dialog could open but no catalogue row was selected and no preview rendered.

Focused verification:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && cmake --build build-qt --target ccad_visual_harness_policy_tests --config Debug && ctest --test-dir build-qt -R visual_harness_policy --output-on-failure"
```

## Sprint 170 PCB Edit Tool Entry

The native Qt GUI now promotes the first right-toolbar PCB editor primitives from planned-tool status to real modal tools. Add Via places a default 0.8 mm by 0.4 mm via, Route Track captures two board points and writes one straight `TrackSegment` using the board minimum track width, Add Keepout captures two board points and writes a rectangular routing keepout, and Delete removes selected pads, vias, tracks, keepouts, and placement regions by stable model ID.

The same behavior is agent-callable. `ccad_gui --ui-trigger-safe` returns `editor_tool_selected` for `action:add_tracks`, `action:add_via`, and `action:add_keepout_area`, while `action:delete_cursor` returns `deleted` or a structured refusal such as `nothing_selected`. App-owned click hooks exercise the real Qt viewport event path instead of mutating the project behind the GUI.

Test with:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && cmake --build build-qt --target ccad_gui_ui_map_tests ccad_gui --config Debug && ctest --test-dir build-qt -R gui_ui_map --output-on-failure"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --test-place-via-click <project.ccad.json> 15 11 <via-result.json>"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --test-route-track-click <project.ccad.json> 8 9 15 11 <track-result.json>"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --test-place-keepout-click <project.ccad.json> 20 10 25 14 <keepout-result.json>"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --test-delete-board-object <project.ccad.json> V1 <delete-result.json>"
```

Known limitations: the first right-toolbar PCB edit batch is complete for Add Via, Route Track, Add Zone, Add Keepout, Draw Graphic, Place Text, and Delete, but richer KiCad editing remains open for zone refill settings, zone cutouts, polygon editing, text property dialogs, graphic shape variants, and precise resize/edit workflows.

## Sprint 171 PCB Active Layer Context

The native Qt GUI now has an active PCB layer selector in the top toolbar. It lists board copper layers, defaults to `F.Cu` when available, rejects missing or non-copper layer IDs, updates the status bar layer readout, and exposes a stable UI-map target as `control:active_pcb_layer`.

The same state is agent-callable. `ReviewWindow::activePcbLayerJson()` reports the selected copper layer, `ReviewWindow::setActivePcbLayerForAutomation()` changes it, `ccad_gui --ui-active-layer` and `ccad_gui --ui-set-active-layer` expose one-shot app hooks, and the live UI-map socket now supports `ui.active_layer` plus `ui.set_active_layer`.

Footprint placement and Route Track now use the selected active copper layer instead of silently defaulting to the first copper layer. Focused GUI tests prove `B.Cu` footprint placement writes back-layer pad metadata and `B.Cu` routing writes a back-layer `TrackSegment`.

Test with:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && cmake --build build-qt --target ccad_gui_ui_map_tests ccad_gui --config Debug && ctest --test-dir build-qt -R gui_ui_map --output-on-failure"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --ui-active-layer <project.ccad.json> <active-layer.json>"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --ui-set-active-layer <project.ccad.json> B.Cu <set-layer.json>"
```

## Sprint 172 PCB Active Net Context

The native Qt GUI now has an active PCB net selector in the top toolbar. It derives available nets from top-level `project.nets` first and then from non-empty board copper net IDs, defaults to the first available net, rejects unknown net IDs, updates the status bar net readout, and exposes a stable UI-map target as `control:active_pcb_net`.

The same state is agent-callable. `ReviewWindow::activePcbNetJson()` reports the selected net, `ReviewWindow::setActivePcbNetForAutomation()` changes it, `ccad_gui --ui-active-net` and `ccad_gui --ui-set-active-net` expose one-shot app hooks, and the live UI-map socket now supports `ui.active_net` plus `ui.set_active_net`.

Add Via and Route Track now use the selected active net instead of writing empty `net_id` values. Focused GUI tests prove `N2` via placement writes a via with `net_id == "N2"` and `N2` routing writes a `TrackSegment` with the same active net. Footprint placement still maps pad nets from component-pin connectivity rather than the active net selector.

Test with:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && cmake --build build-qt --target ccad_gui_ui_map_tests ccad_gui --config Debug && ctest --test-dir build-qt -R gui_ui_map --output-on-failure"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --ui-active-net <project.ccad.json> <active-net.json>"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --ui-set-active-net <project.ccad.json> DC_NEG <set-net.json>"
```

## Sprint 173 PCB Graphics and Text Tools

The board model now has durable KiCad-compatible first-slice graphical primitives: `BoardGraphic` with `kind=line`, a layer, endpoints, and stroke width, plus `BoardText` with layer, payload text, origin, rotation, and text size. These objects round-trip through deterministic CCad JSON, export to KiCad PCB S-expressions as `gr_line` and `gr_text`, appear in canvas scenes and the Qt board canvas, are listed by the object browser, and are inspected by the selection inspector.

The CLI can author and query the new objects through `ccad pcb add-graphic-line`, `ccad pcb add-text`, `ccad pcb get-object`, and `ccad pcb list-objects --type graphic|text`. DRC validates stable IDs, duplicate IDs, layer existence, positive graphic width, non-zero line length, non-empty text, positive text size, and board bounds. Project diffs now include board graphic and board text additions, removals, and changes so transaction summaries do not hide visual board edits.

The native GUI right-toolbar Draw Graphic and Place Text actions now enter real editor modes instead of planned-tool stubs. The app-owned automation hooks place board lines and text through the same Qt viewport event path used by the human GUI, expose the placed objects in the UI map, and allow Delete to remove them by stable object ID. Graphic placement avoids hidden `Dwgs.User` defaults by choosing a visible drawing layer when needed, so automation does not create invisible board objects on standard KiCad-style layer sets.

Focused verification:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && cmake --build build-qt --target ccad_tests ccad_canvas_tests ccad_drc_tests ccad_cli_tests ccad_kicad_pcb_export_tests ccad_diff_tests ccad_gui_ui_map_tests ccad_gui_object_browser_tests ccad_gui_inspector_tests ccad_visual_harness_policy_tests ccad_gui --config Debug"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && ctest --test-dir build-qt -R gui_ui_map --output-on-failure"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && ctest --test-dir build-qt -R diff --output-on-failure"
```

Visual proof:

```cmd
cmd /c "powershell -ExecutionPolicy Bypass -File scripts\run_sprint_demo.ps1 -Name sprint173-pcb-graphics-text-tools-final -GuiWaitSeconds 7"
```

Verified screenshot artifact: `artifacts\screenshots\sprint173-pcb-graphics-text-tools-final-20260603-021600.png`.

Sprint-end gate:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && cmake --build build-qt --config Debug"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && ctest --test-dir build-qt --output-on-failure"
```

Result: full build passed, and CTest passed 36 of 36 tests.

## Sprint 180 Agent Project Evidence Tools

The shared Agent-panel/live-socket dispatcher now exposes project evidence methods alongside GUI interaction methods. `ui.screenshot` grabs the visible `ReviewWindow` through Qt, supports dry-run mode, reports the target path, pixel size, device pixel ratio, and writes a PNG only when `dry_run` is false. The default path stays under ignored `artifacts/screenshots/` when no path is supplied.

Project evidence uses the loaded in-memory GUI project, not a CLI shell-out. `project.context` returns the loaded path, project ID/name, schema version, active PCB layer/net, UI epoch, interaction mode, and object counts. `project.object_counts` gives a compact count-only view. `project.review` serializes the existing `ccad_core::buildReview()` result. `project.erc`, `project.drc`, and `project.diagnostics` expose typed diagnostic evidence with severity, code, message, object ID, and error/warning counts so agents can decide whether to retry, repair, or stop.

Example live requests:

```json
{"method":"ui.screenshot","dry_run":true}
{"method":"project.context"}
{"method":"project.object_counts"}
{"method":"project.review"}
{"method":"project.erc"}
{"method":"project.drc"}
{"method":"project.diagnostics"}
```

Focused verification:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --target ccad_gui_ui_map_tests --config Debug&& ctest --test-dir build-qt -R gui_ui_map --output-on-failure"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --target ccad_gui_agent_panel_tests ccad_review_tests --config Debug&& ctest --test-dir build-qt -R \"gui_agent_panel|review\" --output-on-failure"
```

Visual proof:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& powershell -NoProfile -ExecutionPolicy Bypass -File scripts\run_sprint_demo.ps1 -Name sprint180-agent-project-evidence-tools-final -GuiWaitSeconds 7"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& powershell -NoProfile -ExecutionPolicy Bypass -File scripts\run_ui_map_mouse_target_demo.ps1 -Name sprint180-agent-project-evidence-tools-targets-final -ProjectPath artifacts\demos\sprint180-agent-project-evidence-tools-final.ccad.json"
```

Verified artifacts: `artifacts\screenshots\sprint180-agent-project-evidence-tools-final-20260603-123654.png` and `artifacts\screenshots\sprint180-agent-project-evidence-tools-targets-final-target-sequence.json`.

Sprint-end gate:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --config Debug&& ctest --test-dir build-qt --output-on-failure"
```

Result: full build passed, and CTest passed 36 of 36 tests.

## Sprint 181 UI Map Dirty Deltas And Canvas Index

`ReviewWindow` now tracks first-slice dirty UI-map state behind `markUiMapChanged()`. Exact changes such as tab switches, Agent-panel visibility, active layer changes, active net changes, and panel toggles mark semantic IDs and broad roles, while full canvas renders still fall back to a safe compact snapshot. `ui.map_delta` reports `dirty_node_count`, `changed_roles`, `dirty_ids`, and `full_snapshot` so an agent can tell whether it received a bounded dirty batch or a conservative fallback.

The Agent panel and live socket now expose `ui.wait_for_delta`. It pumps Qt events for a bounded timeout and returns the same delta contract, letting an agent wait for a coordinate or visibility update without immediately dumping the full map again.

`ui.nearest_canvas_object` now reports `index_kind:"uniform_grid"`, `indexed_object_count`, and `scanned_candidate_count`. The lookup builds a small scene-space grid over rendered selectable `QGraphicsItem` bounds and searches nearby cells first, keeping the result tied to the actual GUI render tree while preparing the path toward a larger KiCad-style spatial index.

Focused verification:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --target ccad_gui_ui_map_tests --config Debug&& ctest --test-dir build-qt -R gui_ui_map --output-on-failure"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --target ccad_gui_agent_panel_tests --config Debug&& ctest --test-dir build-qt -R gui_agent_panel --output-on-failure"
```

Visual proof used `scripts\run_sprint_demo.ps1` and `scripts\run_ui_map_mouse_target_demo.ps1`. Verified artifacts are `artifacts\screenshots\sprint181-ui-map-dirty-index-final-20260603-131018.png` and `artifacts\screenshots\sprint181-ui-map-dirty-index-targets-final-target-sequence.json`. The sprint-end full build and CTest gate passed 36 of 36 tests.

## Sprint 182 UI Map Index Cache And Delta Watch

`ReviewWindow` now keeps a bounded dirty-event history alongside the current UI-map epoch. `ui.map_delta` aggregates retained dirty events newer than the requested epoch and reports `dirty_event_count`; if the caller is older than the retained history, or if a full canvas render dirtied the map, it falls back to a compact full snapshot instead of pretending the dirty set is exact.

The shared Agent-panel/live-socket dispatcher now exposes indexed semantic map lookups. `ui.index_stats` reports cache state, node count, ID-index count, role-index count, and role-specific counts such as actions, controls, and canvas objects. `ui.get_node` resolves a single semantic node through the ID index, while `ui.nodes_by_role` returns compact role-index nodes with limit and truncation metadata.

`ui.watch_delta` returns a bounded array of dirty events newer than `since_epoch`, with per-event `from_epoch`, `ui_epoch`, changed roles, dirty IDs, and compact nodes. It pumps Qt events only for the requested bounded timeout and keeps using the same JSON Lines local-socket transport rather than adding a separate streaming server in this sprint.

Focused verification:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --target ccad_gui_ui_map_tests --config Debug&& ctest --test-dir build-qt -R gui_ui_map --output-on-failure"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --target ccad_gui_agent_panel_tests --config Debug&& ctest --test-dir build-qt -R gui_agent_panel --output-on-failure"
```

Visual proof used `scripts\run_sprint_demo.ps1` and `scripts\run_ui_map_mouse_target_demo.ps1`. Verified artifacts are `artifacts\screenshots\sprint182-ui-map-index-cache-final-20260603-133922.png` and `artifacts\screenshots\sprint182-ui-map-index-cache-targets-final-target-sequence.json`. The sprint-end full build and CTest gate passed 36 of 36 tests.

## Sprint 183 Agent Protocol Catalog

The shared Agent-panel/live-socket dispatcher now exposes `agent.methods`, `agent.method_schema`, and `agent.quickstart`. The catalog is a stable JSON method registry for the current native GUI agent protocol. Each method entry carries a category, title, description, read-only and mutation flags, project requirement flag, dry-run support flag, JSON-schema-shaped input description, output summary, and example payload.

This follows the KiCad local-source pattern of central named actions through `TOOL_ACTION` and `ACTION_MANAGER::GetActionList()`, adapted to CCad's LLM-native JSON surface. It also follows MCP-style tool discovery by making method names and input schemas queryable instead of burying them in prose docs.

`agent.method_schema` returns one catalog entry by `method_name`, `name`, or direct-panel `method`, and reports `found:false` for unknown names without treating the lookup as a dispatcher failure. `agent.quickstart` returns the recommended live UI-map loop: discover methods, query project context, confirm `ui.index_stats`, target with `ui.get_node` or `ui.nodes_by_role`, act with dry-run where possible, observe with `ui.watch_delta`, and use `ui.screenshot` only for visual proof.

Sprint 963 extends `agent.methods` into `ccad_unified_agent_registry`. Native GUI methods are explicitly marked callable and name their C++ dispatch binding. CLI command descriptors come from the same `commandHelp()` table used by `ccad help --format json`; their policy flags come from `classifyAgentCommandPolicy()`. Each CLI descriptor includes command usage, a non-shell `argv` schema, validation owner, context needs, result shape, and a truthful unavailable reason. These entries are discoverable through `agent.methods` and `agent.method_schema`, but Python excludes `callable:false` entries when constructing provider tools. This remains intentional until the guarded CLI executor is connected to the GUI broker. Python memory/runtime controls and a full kernel transaction inventory remain open. Verification: Qt/MinGW Release build passed; CTest 92/92; `scripts/test_agent_native_tool_catalog_protocol.py` passed without provider access; the live GUI catalog contained 170 entries (59 callable native methods, 111 non-callable CLI methods), with all 10 UI-map screenshots visually inspected and empty GUI stdout/stderr in `artifacts/screenshots/sprint963-registry-20260923/`.

Focused verification:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --config Debug --target ccad_gui_ui_map_tests&& ctest --test-dir build-qt -R gui_ui_map --output-on-failure"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --config Debug --target ccad_gui_agent_panel_tests&& ctest --test-dir build-qt -R gui_agent_panel --output-on-failure"
```

Visual proof used `scripts\run_sprint_demo.ps1` and `scripts\run_ui_map_mouse_target_demo.ps1`. Verified artifacts are `artifacts\screenshots\sprint183-agent-protocol-catalog-final-20260603-140218.png` and `artifacts\screenshots\sprint183-agent-protocol-catalog-targets-final-target-sequence.json`. The sprint-end full build and CTest gate passed 36 of 36 tests.

## Sprint 184 Library Placement Stability

Placed schematic symbols now survive save/reload as visible schematic geometry. `ccad::Component` carries an optional `ccad::Symbol` snapshot, `placeComponent()` stores the selected symbol, and deterministic project JSON reads and writes the snapshot under each component. `buildSchematicScene()` transforms reloaded symbol lines, rectangles, circles, polygons, text, and pin leads by component position and rotation, while `board_canvas_renderer` uses separate symbol body and symbol pin colors so reloaded schematic geometry is not mistaken for PCB copper.

The CLI now exposes `ccad sch place-symbol --file <project> --symbol <symbol.json> --component <id> --at-x-mm <x> --at-y-mm <y> [--rotation-deg <deg>]`. It is the agent-native schematic placement path for one selected converted symbol JSON file. It resolves local converted-symbol inheritance before writing the component snapshot and rejects duplicate component IDs or empty symbols.

The GUI chooser now returns a concrete `LibrarySelection` instead of only a path. The legacy path result remains for compatibility, but accepted rows also carry library name, item name, file name, source kind, and source path. Symbol placement uses the selected item name when accepting a KiCad `.kicad_sym` source, so the next multi-symbol row expansion has an identity slot instead of reusing "first symbol with pins" forever.

Schematic-only projects now open on the Schematic tab, and `ccad_gui --screenshot-project <project> <png>` captures a clean loaded-project screenshot without entering the measurement overlay. The focused schematic visual proof is `artifacts\screenshots\sprint184-symbol-schematic-visual-bounds-fixed.png`, where four placed diode symbols render with body geometry and visible pin leads after reload.

Focused verification:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --config Debug --target ccad_placement_tests&& ctest --test-dir build-qt -R placement --output-on-failure"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --config Debug --target ccad_cli_tests&& ctest --test-dir build-qt -R cli --output-on-failure"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --config Debug --target ccad_gui_symbol_placement_tests ccad_gui_footprint_placement_tests&& ctest --test-dir build-qt -R gui_symbol_placement --output-on-failure&& ctest --test-dir build-qt -R gui_footprint_placement --output-on-failure"
```

Visual proof used `scripts\run_sprint_demo.ps1`, `scripts\run_ui_map_mouse_target_demo.ps1`, and `ccad_gui --screenshot-project`. Verified artifacts are `artifacts\screenshots\sprint184-library-placement-stability-shipping-20260603-152917.png`, `artifacts\screenshots\sprint184-library-placement-stability-targets-shipping-target-sequence.json`, and `artifacts\screenshots\sprint184-symbol-schematic-visual-shipping.png`. The sprint-end full Qt build passed, and CTest passed 36 of 36 tests.

## Sprint 185 KiCad Symbol Catalog Scale

The KiCad symbol importer now has `listKiCadSymbolLibraryItems()`, a lightweight top-level listing API for raw `.kicad_sym` files. It returns one `KiCadSymbolLibraryItem` per top-level KiCad `symbol` definition and preserves `extends` metadata while ignoring nested unit/body symbols such as `Parent_1_1`.

The native GUI symbol chooser uses that listing API when a raw `.kicad_sym` file appears in the cache. It expands the file into one chooser row per top-level symbol, uses the `.kicad_sym` basename as the library column when the file is directly under `symbols`, includes `extends` text in the row and filter haystack, carries `extends` in `LibrarySelection`, and passes the selected item name into lazy preview loading. This keeps preview and final placement tied to the selected symbol item instead of falling back to the first symbol with pins.

Focused verification:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --config Debug --target ccad_kicad_symbol_import_tests ccad_gui_symbol_placement_tests&& ctest --test-dir build-qt -R kicad_symbol_import --output-on-failure&& ctest --test-dir build-qt -R gui_symbol_placement --output-on-failure"
```

Visual proof used `scripts\run_sprint_demo.ps1`, `scripts\run_ui_map_mouse_target_demo.ps1`, and `ccad_gui --screenshot-chooser-symbol` against `artifacts\demos\sprint185-symbol-cache`. Verified artifacts are `artifacts\screenshots\sprint185-kicad-symbol-catalog-scale-shipping-20260603-160243.png`, `artifacts\screenshots\sprint185-kicad-symbol-chooser-shipping.png`, and `artifacts\screenshots\sprint185-kicad-symbol-catalog-scale-targets-shipping-target-sequence.json`. The sprint-end full Qt build passed, and CTest passed 36 of 36 tests.

## Sprint 174 PCB Zone Tool First Slice

The board model now has durable KiCad-compatible first-slice copper-zone primitives through `BoardZone`. A zone records a stable ID, optional name, optional net, ordered copper layer IDs, outline points, priority, clearance, minimum thickness, fill enabled state, and pad connection mode. Zones round-trip through deterministic project JSON, appear in the GUI-independent canvas scene, render in the Qt board canvas as translucent layer-colored copper polygons, export to KiCad PCB S-expressions as `zone`, `polygon`, and deterministic preview `filled_polygon` records, and participate in project diffs.

The CLI can author and query zones through `ccad pcb add-zone`, `ccad pcb get-object`, `ccad pcb list-objects --type zone`, `ccad pcb export-route-job`, and `ccad pcb remove-object`. DRC validates duplicate or empty IDs, cross-type duplicate IDs, layer existence, copper-only zone layers, duplicate layer entries, assigned net existence, minimum outline size, board containment, positive clearance, positive minimum thickness, and supported pad connection modes.

The native GUI right-toolbar Add Zone action now enters a real editor mode instead of returning `future_tool_not_implemented`. Human clicks and app-owned automation both commit rectangular zones through the Qt viewport event path, save deterministic project JSON, refresh the core-derived canvas, expose `canvas_object:<zone-id>` UI-map entries, show zones in the object browser, and show zone properties in the selection inspector. The app-owned hook is:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && build-qt\ccad_gui.exe --test-place-zone-click artifacts\demos\sprint174-pcb-zone-tool-final.ccad.json 6 20 38 28 artifacts\demos\sprint174-zone-result.json"
```

Visual proof:

```cmd
cmd /c "powershell -NoProfile -ExecutionPolicy Bypass -File scripts\run_sprint_demo.ps1 -Name sprint174-pcb-zone-tool-final -GuiWaitSeconds 7"
```

Verified screenshot artifact: `artifacts\screenshots\sprint174-pcb-zone-tool-final-20260603-031321.png`.

Sprint-end gate:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && cmake --build build-qt --config Debug"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && ctest --test-dir build-qt --output-on-failure"
```

Result: full build passed, and CTest passed 36 of 36 tests.

## Sprint 175 Agent Panel Live UI Map

The native Agent panel now has a live-query row with a protocol method input, JSON payload input, and `Live Query` action. The panel is still local and provider-free, but it can now ask the running GUI for compact UI-map data through the same method names as the local socket. This gives the future model harness a usable in-GUI tool surface before BYOK providers, LangGraph orchestration, or OTEL tracing are added.

`ReviewWindow::runAgentUiQueryJson()` is the shared dispatcher. It supports `ui.map`, `ui.map_delta`, `ui.find`, `ui.target`, `ui.target_board_point`, `ui.trigger_safe`, `ui.active_layer`, `ui.set_active_layer`, `ui.active_net`, `ui.set_active_net`, and `ui.epoch`. `UiMapServer` now parses incoming JSON Lines requests with Qt JSON APIs and delegates to that dispatcher, so socket clients and the Agent panel do not drift into separate protocols.

The UI map now exposes named Agent-panel controls such as `control:agent_live_method`, `control:agent_live_payload`, and `action:agent_live_query`. These controls can be found and targeted by the existing UI-map lookup path. The new `ui.find` method returns compact role/query matches, while `ui.map_delta` lets an agent poll by `ui_epoch` and avoid dumping the whole map when nothing changed.

Focused verification:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --target ccad_gui_agent_panel_tests ccad_gui_ui_map_tests -j 4"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& ctest --test-dir build-qt -R gui_agent_panel --output-on-failure"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& ctest --test-dir build-qt -R gui_ui_map --output-on-failure"
```

Visual proof:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& powershell -NoProfile -ExecutionPolicy Bypass -File scripts\run_sprint_demo.ps1 -Name sprint175-agent-panel-live-map-final -GuiWaitSeconds 7"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& powershell -NoProfile -ExecutionPolicy Bypass -File scripts\run_ui_map_mouse_target_demo.ps1 -Name sprint175-agent-panel-live-map-targets-final -ProjectPath artifacts\demos\sprint175-agent-panel-live-map-final.ccad.json"
```

Verified artifacts: `artifacts\screenshots\sprint175-agent-panel-live-map-final-20260603-102936.png` and `artifacts\screenshots\sprint175-agent-panel-live-map-targets-final-target-sequence.json`. The target report found `control:agent_live_method`, `control:agent_live_payload`, and `action:agent_live_query` in both initial and resized passes.

Sprint-end gate:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --config Debug"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& ctest --test-dir build-qt --output-on-failure"
```

Result: full build passed, and CTest passed 36 of 36 tests.

## Sprint 176 UI Map Compact Deltas

The live UI-map protocol now has lower-token map queries for agents. `ui.map_compact` filters by role and returns compact nodes with stable ids, labels, visibility/enabled state, target centers, and CAD metadata. `ui.role_summary` reports role counts so an agent can understand map shape cheaply. `ui.map_delta` keeps current-epoch responses empty and now returns compact nodes for stale epochs instead of embedding the full nested map. `ui.hit_test` resolves a logical screen coordinate against exported UI rectangles, and `ui.nearest_canvas_object` scans rendered selectable canvas items to return the nearest CAD object candidate for a board point.

Focused verification:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --target ccad_gui_ui_map_tests -j 4&& ctest --test-dir build-qt -R gui_ui_map --output-on-failure"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --target ccad_gui_agent_panel_tests -j 4&& ctest --test-dir build-qt -R gui_agent_panel --output-on-failure"
```

Visual proof:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& powershell -NoProfile -ExecutionPolicy Bypass -File scripts\run_sprint_demo.ps1 -Name sprint176-ui-map-compact-deltas-final -GuiWaitSeconds 7"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& powershell -NoProfile -ExecutionPolicy Bypass -File scripts\run_ui_map_mouse_target_demo.ps1 -Name sprint176-ui-map-compact-deltas-targets-final -ProjectPath artifacts\demos\sprint176-ui-map-compact-deltas-final.ccad.json"
```

Verified artifacts: `artifacts\screenshots\sprint176-ui-map-compact-deltas-final-20260603-110523.png` and `artifacts\screenshots\sprint176-ui-map-compact-deltas-targets-final-target-sequence.json`. The target report found `action:cursor`, `action:measurement`, `action:save`, `menu:file`, `panel:properties`, display actions, `tab:agent`, `control:agent_live_method`, `control:agent_live_payload`, and `action:agent_live_query` in both initial and resized passes.

Sprint-end gate:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --config Debug"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& ctest --test-dir build-qt --output-on-failure"
```

Result: full build passed, and CTest passed 36 of 36 tests.

## Sprint 177 Agent UI Interaction Tools

The live UI-map protocol now has a first direct interaction slice for agents. `ui.click` can dry-run any semantic ID, trigger existing safe actions and tab changes, click named Agent-panel buttons, focus controls, and select `canvas_object:*` entries. `ui.double_click` currently reuses the same target-aware safety boundary for dry-run workflows. `ui.type_text` writes only to whitelisted Agent-panel inputs such as `control:agent_live_method` and `control:agent_live_payload`, so agents can fill the in-GUI protocol row without scraping pixels. `ui.key` implements Escape cancellation for active GUI tool modes. `ui.select_canvas_object` and `ui.get_selection` select and report PCB canvas objects by stable IDs with object type, net, layer, and route provenance where available. `ui.wait_for_epoch` lets live agents wait briefly for `ui_epoch` changes without dumping the whole map.

Focused verification:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --target ccad_gui_ui_map_tests -j 4&& ctest --test-dir build-qt -R gui_ui_map --output-on-failure"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& ctest --test-dir build-qt -R gui_agent_panel --output-on-failure"
```

Visual proof:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& powershell -NoProfile -ExecutionPolicy Bypass -File scripts\run_sprint_demo.ps1 -Name sprint177-agent-ui-interaction-tools-final -GuiWaitSeconds 7"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& powershell -NoProfile -ExecutionPolicy Bypass -File scripts\run_ui_map_mouse_target_demo.ps1 -Name sprint177-agent-ui-interaction-tools-targets-final -ProjectPath artifacts\demos\sprint177-agent-ui-interaction-tools-final.ccad.json"
```

Verified artifacts: `artifacts\screenshots\sprint177-agent-ui-interaction-tools-final-20260603-112907.png` and `artifacts\screenshots\sprint177-agent-ui-interaction-tools-targets-final-target-sequence.json`. The target report found every configured initial and resized target, including `tab:agent`, `control:agent_live_method`, `control:agent_live_payload`, and `action:agent_live_query`, with an empty stderr log.

Sprint-end gate:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --config Debug"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& ctest --test-dir build-qt --output-on-failure"
```

Result: full build passed, and CTest passed 36 of 36 tests.

## Sprint 178 Map-Driven Viewport Input

The live UI-map protocol now has board-point input methods for agents. `ui.canvas_click` accepts `x_mm`, `y_mm`, optional `canvas`, optional `dry_run`, and optional `text`; it resolves the board point, maps it through the PCB `QGraphicsView` viewport, and sends a real press/release pair to the viewport. This means an agent can enter Add Via, Route Track, Add Zone, Add Keepout, Draw Graphic, or Place Text through the existing semantic action contract, then place the resulting object through the same `ReviewWindow::eventFilter()` path used by human clicks.

`ui.canvas_drag` accepts `start_x_mm`, `start_y_mm`, `end_x_mm`, `end_y_mm`, optional `canvas`, and optional `dry_run`. It sends a first point, an intermediate move so the current ghost preview updates, and a second point for CCad's current two-point tools. Responses include board, scene, viewport, and target metadata where relevant, the interaction mode before and after the gesture, and board object counts for pads, vias, tracks, zones, keepouts, graphics, and text.

Focused verification:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --config Debug --target ccad_gui_ui_map_tests"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& ctest --test-dir build-qt -R gui_ui_map --output-on-failure"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& ctest --test-dir build-qt -R gui_agent_panel --output-on-failure"
```

Visual proof:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& powershell -NoProfile -ExecutionPolicy Bypass -File scripts\run_sprint_demo.ps1 -Name sprint178-map-driven-viewport-input-final -GuiWaitSeconds 7"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& powershell -NoProfile -ExecutionPolicy Bypass -File scripts\run_ui_map_mouse_target_demo.ps1 -Name sprint178-map-driven-viewport-input-targets-final -ProjectPath artifacts\demos\sprint178-map-driven-viewport-input-final.ccad.json"
```

Verified artifacts: `artifacts\screenshots\sprint178-map-driven-viewport-input-final-20260603-115354.png` and `artifacts\screenshots\sprint178-map-driven-viewport-input-targets-final-target-sequence.json`.

Sprint-end gate:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --config Debug"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& ctest --test-dir build-qt --output-on-failure"
```

Result: full build passed, and CTest passed 36 of 36 tests.

## Sprint 179 Agent PCB Workflow Tools

The live UI-map protocol now has higher-level PCB workflow methods for agents. `ui.current_tool` reports the current interaction mode, active layer, active net, and anchor state. `ui.cancel_tool` sends Escape through the same keyboard path as human cancellation and returns the tool to `default`.

The mutating workflow methods are convenience wrappers over existing GUI behavior, not direct model writes. `ui.place_via` activates `action:add_via` and sends one board-point viewport click. `ui.route_track` activates `action:add_tracks` and sends two board-point viewport clicks. `ui.add_zone`, `ui.add_keepout`, and `ui.draw_graphic` activate their right-toolbar actions and send a two-point viewport gesture. `ui.place_text` activates `action:text`, sets the requested text for placement, and sends one viewport click. `ui.delete_object` selects a canvas object by stable ID, then invokes `action:delete_cursor`. Responses include nested activation/gesture evidence and top-level object counts such as `via_count`, `track_count`, `zone_count`, `keepout_count`, `graphic_count`, and `text_count`.

Focused verification:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --config Debug --target ccad_gui_ui_map_tests"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& ctest --test-dir build-qt -R gui_ui_map --output-on-failure"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& ctest --test-dir build-qt -R gui_agent_panel --output-on-failure"
```

Visual proof:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& powershell -NoProfile -ExecutionPolicy Bypass -File scripts\run_sprint_demo.ps1 -Name sprint179-agent-pcb-workflow-tools-final -GuiWaitSeconds 7"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& powershell -NoProfile -ExecutionPolicy Bypass -File scripts\run_ui_map_mouse_target_demo.ps1 -Name sprint179-agent-pcb-workflow-tools-targets-final -ProjectPath artifacts\demos\sprint179-agent-pcb-workflow-tools-final.ccad.json"
```

Verified artifacts: `artifacts\screenshots\sprint179-agent-pcb-workflow-tools-final-20260603-121454.png` and `artifacts\screenshots\sprint179-agent-pcb-workflow-tools-targets-final-target-sequence.json`.

Sprint-end gate:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --config Debug"
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& ctest --test-dir build-qt --output-on-failure"
```

Result: full build passed, and CTest passed 36 of 36 tests.

## Sprint 186 Agent Harness Foundation

The native Agent panel is now a right-side Qt dock instead of a bottom Diagnostics tab. The UI map preserves `panel:agent` and the semantic `tab:agent` alias, and both report `dock_area:"right"` where relevant. `triggerSafeUiActionJson("tab:agent")` now shows and raises the dock while keeping the existing `tab_selected` success reason for scripts. `ReviewWindow::~ReviewWindow()` disconnects the Agent dock visibility signal before child teardown so dock destruction cannot call back into stale UI-map state.

`ReviewWindow::runAgentUiQueryJson()` now exposes read-only harness metadata methods: `agent.harness_context`, `agent.run_profile`, `agent.safety_policy`, `agent.provider_policy`, `agent.observability_config`, `agent.evidence_manifest_schema`, and `agent.tool_guide`. These methods do not run models or export telemetry yet. They define the local state, retry, approval, BYOK/local-model, OpenTelemetry/Langfuse, redaction, and evidence-manifest contracts future agents should use.

`src/ccad_cli/agent_commands.cpp` now exposes matching headless metadata through direct commands such as `ccad agent methods`, `ccad agent harness-context`, and `ccad agent tool-guide --method <name>`. The same metadata is also available over `ccad agent serve` as JSON-RPC methods including `agent.methods`, `agent.harness_context`, and `agent.tool_guide`. This lets external orchestrators discover CCad's agent contract without launching Qt.

Focused verification commands:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --config Debug --target ccad_gui_ui_map_tests&& ctest --test-dir build-qt -R gui_ui_map --output-on-failure"
cmd /c "cmake --build build-qt --config Debug --target ccad_agent_serve_tests&& ctest --test-dir build-qt -R agent_serve --output-on-failure"
```

Both focused checks passed. The official visual harness produced `artifacts\screenshots\sprint186-agent-harness-foundation-rightsplit-20260604-150605.png`, the UI-map target harness produced `artifacts\screenshots\sprint186-agent-harness-foundation-rightsplit-targets-target-sequence.json` with all configured initial and resized targets found and empty stderr, and the full build plus CTest gate passed 36 of 36 tests.

## Sprint 187 Agent Panel Workspace

The right-side Agent dock now shows a compact workspace strip instead of only raw protocol controls. `AgentPanel::setWorkspaceContext()` displays the active editor view, active PCB layer, active PCB net, current interaction mode, and cached diagnostic counts. `ReviewWindow::updateAgentPanelContext()` feeds those values from the real `ReviewWindow` state and uses counts captured during `renderReview()` so UI-map epoch updates do not rerun ERC or DRC.

The panel now has one-click local presets for `agent.harness_context`, `project.diagnostics`, and `agent.tool_guide` plus a `Clear` action. A new result-state label summarizes map refreshes, safe action outcomes, live-query success or error state, and clear/reset state, so humans and agents do not have to inspect raw JSON for every small action.

The UI map exposes the new controls as stable semantic action IDs: `action:agent_preset_harness_context`, `action:agent_preset_project_diagnostics`, `action:agent_preset_tool_guide`, and `action:agent_clear_output`. The generic `ui.click` path can trigger those buttons through the same direct Qt button-click mechanism used by existing Agent live-query controls.

Focused `gui_agent_panel`, `gui_ui_map`, and broader `gui_` test runs passed. The official visual harness produced `artifacts\screenshots\sprint187-agent-panel-workspace-corrected-20260604-195903.png`, and the target harness produced `artifacts\screenshots\sprint187-agent-panel-workspace-targets-target-sequence.json` with all configured initial and resized targets found and empty stderr. The sprint-end full Qt build plus CTest gate passed 36 of 36 tests.

## Sprint 188 Agent Task Workspace

The right-side Agent dock now carries the first local task workspace state. `AgentPanel` owns a targetable `control:agent_goal` input, `action:agent_stage_goal`, visible task-state text, `action:agent_pin_evidence`, `action:agent_clear_evidence`, and an evidence-count label. Pinned evidence is local in-memory state bounded to eight entries, so this remains a safe GUI workspace slice rather than provider execution, BYOK storage, or remote tracing.

`AgentPanel::workspaceStateJson()` serializes the staged goal, task state, evidence count, evidence entries, current project, UI-map epoch, workspace context, diagnostic summary, status, result state, and live method/payload. `ReviewWindow::agentWorkspaceStateJson()` exposes that snapshot through the shared Agent-panel/live-socket dispatcher as `agent.workspace_state`, and `agent.methods` advertises the method so agents can discover it without scraping docs.

The semantic UI path can now type into `control:agent_goal`, click the Stage Goal, Pin Evidence, and Clear Evidence controls, then query `agent.workspace_state` to prove the state transition. Focused `gui_agent_panel` and `gui_ui_map` tests passed. The official visual harness produced `artifacts\screenshots\sprint188-agent-task-workspace-corrected-20260604-203103.png`, and the target harness produced `artifacts\screenshots\sprint188-agent-task-workspace-targets-target-sequence.json` with the new Agent controls found before and after resize and empty stderr. The sprint-end full Qt build plus CTest gate passed 36 of 36 tests.

## Sprint 189 Agent Approval Workspace

The right-side Agent dock now carries the first local approval lane. `AgentPanel` owns `control:agent_approval_request`, `action:agent_request_approval`, visible approval status text, `action:agent_approve_next`, `action:agent_decline_next`, `action:agent_cancel_approval`, and `action:agent_clear_approvals`. This is one pending local approval at a time and is intentionally secret-free, provider-free, and project-mutation-free.

`AgentPanel::workspaceStateJson()` now includes `approval_pending_count`, `approval_request`, `approval_input`, `approval_status`, and `approval_last_decision`. The same `agent.workspace_state` method used by the Agent panel and live socket exposes pending, accepted, declined, canceled, and cleared approval states, so a future durable runner can pause around a human decision without scraping pixels.

The semantic UI path can type into `control:agent_approval_request`, request approval, accept, decline, cancel, clear, and then query `agent.workspace_state` to prove the decision. Focused `gui_agent_panel` and `gui_ui_map` tests passed. The official visual harness produced `artifacts\screenshots\sprint189-agent-approval-workspace-final-20260604-210030.png`, and the target harness produced `artifacts\screenshots\sprint189-agent-approval-workspace-targets-target-sequence.json` with every approval target found before and after resize and empty stderr. The sprint-end full Qt build plus CTest gate passed 36 of 36 tests.

## Sprint 190 Agent Harness Backlog Map

Sprint 190 does not add runtime behavior. It records the current agent-harness feature boundary after reviewing `docs/req_agentHarness.md` and current external references. The implemented state remains Sprints 186 through 189: right-side Agent dock, read-only harness metadata, compact workspace context, task goal, pinned evidence, and one local approval lane.

The planned state is now explicit in `docs/devops/backlog.md`: a full-height vertical Agent workspace with session controls, command input, task checklist, reasoning/tool stream, evidence cards, approval cards, trace links, CLI parity, durable session/checkpoint state, policy gates, BYOK/BYOT configuration, OpenTelemetry/Langfuse export, KiCad/Altium automation, ngspice simulation, and specialized EDA subagents. Future feature entries should move items from that backlog into this file only after implementation and verification.

## Sprint 191 Agent Panel Vertical Workspace

The native GUI Agent dock is now a full-height right-side workspace column beside Layers / Objects instead of a stacked lower pane. `ReviewWindow` keeps `panel:agent` and `tab:agent`, but splits the right dock area horizontally so the Agent surface has the vertical rhythm expected from code-editor agent panes.

`AgentPanel` now presents a compact dark workspace shell with session title/model chip, workspace context, goal/task area, approval card, pinned evidence section, command stream, and a fixed command bar. The new semantic IDs are `control:agent_command_input`, `action:agent_submit_command`, `action:agent_footer_request_context`, and `action:agent_footer_trigger_drc`. The footer buttons call the existing harness-context and diagnostics presets, so they are functional controls rather than decorative placeholders.

`AgentPanel::workspaceStateJson()` now serializes `panel_layout:"vertical_agent_workspace"`, session labels, command input, and staged command text alongside the earlier task, evidence, approval, project, UI-map, diagnostics, live method, and live payload fields. The UI-map harness validates the Agent pane geometry, command input, footer actions, and approval buttons before and after resize.

Focused `gui_agent_panel` and `gui_ui_map` tests passed. Visual proof is `artifacts\screenshots\sprint191-agent-panel-vertical-workspace-v6-20260605-031347.png`. Target proof is `artifacts\screenshots\sprint191-agent-panel-vertical-workspace-v6-targets-target-sequence.json`, with no `found:false` entries and empty stderr. The approval target screenshot `artifacts\screenshots\sprint191-agent-panel-vertical-workspace-v6-targets-initial-action_agent_approve_next.png` shows the marker on the visible Accept button.

## Sprint 192 Agent CLI Workspace State

The native CLI now exposes the first headless parity slice for the Agent workspace. `ccad agent state` returns a deterministic provider-free workspace snapshot with `panel_layout:"headless_cli_workspace"`, empty first-slice task/evidence/approval fields, session labels, and `durable_store:"not_configured"` so callers know that GUI-only in-memory state is not being silently invented.

`ccad agent tasks`, `ccad agent evidence`, and `ccad agent approvals` return smaller state documents for batch jobs, CI, and future durable runners. `ccad agent serve` exposes the same surfaces as `agent.state`, `agent.workspace_state`, `agent.tasks`, `agent.evidence`, and `agent.approvals`, while `agent.methods` and `agent.tool_guide` advertise those methods for discovery.

This sprint intentionally does not add provider calls, secrets, LangGraph persistence, OpenTelemetry export, or project mutation. Sprint 193 should build the durable local session schema that lets the GUI and CLI share real run state.

## Sprint 193 Agent Session Checkpoints

The native CLI now has a provider-free local Agent session file format. `ccad agent session-schema` reports the schema fields and forbidden secret fields. `ccad agent session-new` writes a `.ccad-agent-session.json` file with `session_id`, matching `thread_id`, title, optional project path, created/updated timestamps, local session resource URI, and an empty checkpoint list.

`ccad agent checkpoint-add` appends ordered checkpoint records with checkpoint ID, sequence, kind, summary, optional artifact path, timestamp, and checkpoint resource URI. `ccad agent session-state` canonicalizes the current file back to deterministic JSON, and `ccad agent replay` emits a replay manifest with the latest checkpoint ID, checkpoint count, replayable flag, and ordered checkpoints.

`ccad agent serve` exposes read-only routes `agent.session_schema`, `agent.session_state`, and `agent.replay_manifest`. This is not a provider runner, LangGraph runtime, or telemetry exporter yet; it is the local checkpoint substrate that later GUI, policy, BYOK, and observability sprints can share.

## Sprint 194 Agent Policy Gates

The native CLI now has the first deterministic command policy gate for Agent execution. `src/ccad_cli/agent_policy.hpp/.cpp` classifies commands from argv tokens only, so it does not open projects, parse library files, call providers, or execute commands just to determine risk. Read-only inspection commands require `allow-read`; file-writing, project-mutating, session-writing, and unknown command families require `allow-write` and return `approval_required:true`.

`ccad agent policy-schema` reports the policy fields, decisions, and approval reasons. `ccad agent policy-check -- <ccad command args...>` returns a structured policy object for a planned command, including `read_only`, `mutates_project`, `mutates_files`, `requires_allow_read`, `requires_allow_write`, `approval_required`, `approval_reason`, `risk_level`, and `decision`. `ccad agent dry-run -- <ccad command args...>` returns the same classification with `dry_run:true`, `would_execute:false`, and `decision:"dry_run_only"`.

`ccad agent serve` exposes `agent.policy_schema` and `agent.policy_check`, advertises both through `agent.methods`, `agent.quickstart`, `agent.harness_context`, and `agent.tool-guide`, and now uses the shared classifier for `execute` and MCP `tools/call` permission checks. A write command sent to `agent serve --allow-read` returns JSON-RPC error `-32604` with an approval-oriented message such as `Approval required: project_mutation` instead of the old generic permission denial.

Focused verification passed for `agent_serve` after a red test proved the missing policy routes. Direct CLI spot checks returned schema, write-policy, and dry-run policy JSON. The sprint-end full Qt build plus CTest gate passed 36 of 36 tests.

## Sprint 195 Agent Evidence Manifests

The native Agent pane now renders pinned outputs as structured evidence cards instead of only flat text summaries. Cards are local, bounded, and targetable with stable IDs such as `card:agent_evidence_1`. The supported first-slice card kinds are `screenshot`, `drc_report`, `erc_report`, `diagnostics_report`, `review_report`, and generic `tool_result`.

`AgentPanel::workspaceStateJson()` now keeps backward-compatible `evidence` summaries and adds `evidence_cards` with ID, kind, title, summary, method, artifact path, created timestamp, trace ID, span ID, source, diagnostic counts, error/warning counts, DRC/ERC counts, and image dimensions when available. `agent.evidence_manifest_schema` documents these fields in both the GUI live protocol and the headless CLI/JSON-RPC metadata surface.

The visual Agent layout now places Pinned Evidence before Approval Pending so evidence stays visible in the first viewport, matching the full-height side-pane direction. The app-owned target-sequence harness now clicks Trigger DRC and Pin through the existing semantic `ui.click` dispatcher before taking pin-target screenshots, so visual proof exercises the same Agent controls that a model or script would use.

Focused verification:

```cmd
cmd /c "cd /d F:\CCad&& set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --target ccad_agent_serve_tests ccad_gui_agent_panel_tests ccad_gui_ui_map_tests --config Debug&& ctest --test-dir build-qt -R \"agent_serve|gui_agent_panel|gui_ui_map\" --output-on-failure"
```

Visual proof used `scripts\run_sprint_demo.ps1` and `scripts\run_ui_map_mouse_target_demo.ps1`. Verified artifacts are `artifacts\screenshots\sprint195-agent-evidence-manifests-final-v2-20260605-042859.png`, `artifacts\screenshots\sprint195-agent-evidence-manifests-targets-final-target-sequence.json`, `artifacts\screenshots\sprint195-agent-evidence-manifests-targets-final-initial-action_agent_pin_evidence.png`, and `artifacts\screenshots\sprint195-agent-evidence-manifests-targets-final-resized-action_agent_pin_evidence.png`. The sprint-end full build and CTest gate passed 36 of 36 tests.

## Sprint 196 Agent Workspace Command Center

The native Agent pane now uses a denser command-center layout instead of a loose stack of cards. The top session strip has visible icon actions for request context, diagnostics, and clear output, followed by local model, mode, and policy chips. The pane keeps the board editor primary, but presents Agent state as a full-height right-side workspace with Command, Evidence, and Approvals selectors, a first-viewport Activity stream, workspace context, existing task/evidence/approval controls, and the fixed bottom command input.

The redesign remains provider-free. The model and mode chips are local metadata only, and the header buttons call existing local harness-context, diagnostics, and clear-output paths. `AgentPanel::workspaceStateJson()` now includes `visual_style:"command_center_dark"`, `workspace_layout_version:2`, `active_agent_tab:"command"`, `visible_sections`, `permission_label`, `activity_event_count`, and structured `activity_events` with ID, kind, title, detail, method, and timestamp.

The UI-map and target resolver now expose passive Agent regions as semantic coordinates alongside existing controls. New stable IDs include `panel:agent_session_strip`, `panel:agent_mode_strip`, `panel:agent_activity_stream`, `tab:agent_command`, `tab:agent_evidence`, `tab:agent_approvals`, `label:agent_permission_chip`, `card:agent_activity_1`, `action:agent_header_request_context`, `action:agent_header_trigger_drc`, and `action:agent_header_clear_output`. Passive targets use scroll-clipped visible rectangles when possible so target markers land on the visible part of the Agent pane.

## Sprint 197 Agent BYOK Configuration

The native CLI now exposes no-secret BYOK/BYOT provider configuration metadata without launching Qt or calling any remote provider. `ccad agent provider-config-schema` documents accepted provider families, environment-variable names, secret-storage policy, disallowed consumer-web-session paths, and Gemini key-restriction guidance. `ccad agent provider-config-template` emits a disabled-by-default template that stores only env-var names and contains no secret values. `ccad agent provider-status` reports presence-only environment status, redacts every value, performs no network probe, and keeps provider execution disabled.

The same surfaces are available through JSON-RPC routes `agent.provider_config_schema`, `agent.provider_config_template`, and `agent.provider_status`, and the method catalog plus tool-guide discovery route them to `headless_cli_provider_config`. The supported first-slice provider families are OpenAI official API, OpenAI-compatible API, Anthropic Claude API, Google Gemini API, and local model server. Sprint 205 adds GUI provider-readiness controls for this metadata. This is not a model runner yet; provider execution, LangGraph orchestration, and observability export remain future work.

## Sprint 198 Agent Observability Configuration

The native CLI now exposes disabled-by-default trace export configuration metadata without launching Qt, calling Langfuse, opening an OTLP connection, exporting prompts, or printing secret header values. `ccad agent trace-export-schema` documents the planned OpenTelemetry GenAI mapping, backend families, span plan, and OTLP environment variable names. `ccad agent trace-export-template` emits a no-secret template that keeps export, prompt content, tool payloads, screenshots, and design files disabled. `ccad agent trace-redaction-policy` records the default redaction stance, and `ccad agent trace-export-dry-run` reports env-var presence and readiness while performing no network probe.

The same surfaces are available through JSON-RPC routes `agent.trace_export_schema`, `agent.trace_export_template`, `agent.trace_redaction_policy`, and `agent.trace_export_dry_run`, and the method catalog plus tool-guide discovery route them to `headless_cli_observability_config`. `agent.observability_config` now points callers to those more specific trace methods. This is not a telemetry exporter yet; GUI trace controls, LangGraph run binding, trace IDs on live runs, and real OTLP/Langfuse export remain future work.

Focused verification passed for `agent_serve` after a red test proved the missing trace export method catalog entry. Direct CLI spot checks returned schema, template, redaction policy, dry-run, tool-guide, and help metadata. The sprint-end full Qt build plus CTest gate passed 36 of 36 tests.

Focused verification:

```cmd
cmd /c "cd /d F:\CCad&& set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --target ccad_gui_agent_panel_tests ccad_gui_ui_map_tests ccad_gui --config Debug&& ctest --test-dir build-qt -R \"gui_agent_panel|gui_ui_map\" --output-on-failure"
```

Visual proof used `scripts\run_sprint_demo.ps1` and `scripts\run_ui_map_mouse_target_demo.ps1`. Verified artifacts are `artifacts\screenshots\sprint196-agent-workspace-command-center-v2-20260605-094606.png`, `artifacts\screenshots\sprint196-agent-workspace-command-center-v2-targets-target-sequence.json`, `artifacts\screenshots\sprint196-agent-workspace-command-center-v2-targets-initial-action_agent_header_trigger_drc.png`, and `artifacts\screenshots\sprint196-agent-workspace-command-center-v2-targets-initial-panel_agent_activity_stream.png`. The sprint-end full build and CTest gate passed 36 of 36 tests.

## Sprint 199 Agent Panel UI Polish

The native Agent pane now has the next dense vertical command-surface slice inspired by the provided side-agent UI references. The top of the pane keeps the compact session header and model/mode/local-policy chips, then adds a separate trace/session strip, local run controls, and a first-viewport active-plan section so the pane reads as an agent workspace rather than a raw stack of forms.

The run controls are local state only. `action:agent_pause_run`, `action:agent_resume_run`, and `action:agent_stop_run` update the visible run chip, status/result labels, Activity stream, and `agent.workspace_state` without starting a provider runner, exporting telemetry, or mutating the project. `AgentPanel::workspaceStateJson()` now reports `visual_style:"agent_command_center_dense"`, `workspace_layout_version:3`, `run_state`, `run_label`, `trace_label`, `session_label`, `plan_item_count`, and bounded `plan_items`.

The semantic UI map exposes the new regions and controls through stable object names: `panel:agent_trace_strip`, `label:agent_trace_chip`, `label:agent_session_chip`, `panel:agent_run_controls`, `label:agent_run_state_chip`, `action:agent_pause_run`, `action:agent_resume_run`, `action:agent_stop_run`, `panel:agent_active_plan`, and `panel:agent_plan_row_1` through `panel:agent_plan_row_3`.

Focused verification:

```cmd
cmd /c "cd /d F:\CCad&& set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --target ccad_gui_agent_panel_tests ccad_gui_ui_map_tests ccad_gui --config Debug&& ctest --test-dir build-qt -R gui_ --output-on-failure"
```

Visual proof used `scripts\run_sprint_demo.ps1` and `scripts\run_ui_map_mouse_target_demo.ps1`. Verified artifacts are `artifacts\screenshots\sprint199-agent-panel-ui-polish-final-20260605-110500.png`, `artifacts\screenshots\sprint199-agent-panel-ui-polish-targets-target-sequence.json`, and `artifacts\screenshots\sprint199-agent-panel-ui-polish-targets-initial-panel_agent_active_plan.png`. The focused GUI gate passed 14 of 14 tests, and the sprint-end full Qt build plus CTest gate passed 36 of 36 tests.

## Sprint 200 KiCad CLI Evidence Integration

The native CLI now exposes the first structured KiCad evidence surface for agents. `ccad agent kicad-evidence-schema` documents the supported KiCad CLI evidence kinds, `ccad agent kicad-evidence-plan` emits command arrays and artifact manifests, `ccad agent kicad-evidence-dry-run` resolves executable and input readiness without running KiCad, and `ccad agent kicad-evidence-run` is guarded so it only executes with an explicit `--execute` flag.

The supported first slice covers `pcb-drc`, `sch-erc`, `pcb-export-gerbers`, `pcb-export-drill`, `pcb-export-pos`, `pcb-export-ipc2581`, and `pcb-export-odb`. The planner maps CCad options onto KiCad CLI families such as `pcb drc`, `sch erc`, and `pcb export ...`, including report format, units, severity filters, violation exit-code behavior, schematic parity, zone refill/save options, export layers, position side, and manufacturing artifact paths.

The same surfaces are available through JSON-RPC routes `agent.kicad_evidence_schema`, `agent.kicad_evidence_plan`, `agent.kicad_evidence_dry_run`, and `agent.kicad_evidence_run`. The run route denies `execute:true` from read-only `agent serve` with JSON-RPC error `-32604` and `external_process_file_write`; this keeps automated agents from producing external KiCad artifacts unless the server was explicitly started with write permission.

Focused verification passed for `agent_serve` after red tests proved the missing KiCad evidence catalog entries and the missing JSON-RPC execution guard. Direct CLI spot checks returned schema, plan, dry-run, guarded run, policy-check, help, and tool-guide metadata. Visual proof used `scripts\run_sprint_demo.ps1` and `scripts\run_ui_map_mouse_target_demo.ps1`. Verified artifacts are `artifacts\screenshots\sprint200-kicad-cli-evidence-final-20260605-114425.png` and `artifacts\screenshots\sprint200-kicad-cli-evidence-final-targets-target-sequence.json`. The sprint-end full Qt build plus CTest gate passed 36 of 36 tests.

## Sprint 201 Agent Panel Visual Refinement

The native Agent pane now has a fourth visual contract slice aimed at the provided vertical agent UI references. The sprint keeps the existing local-only behavior but makes the pane more structured by adding targetable subregions for a compact status rail, command composer, plan deck, evidence lane, and approval lane.

`AgentPanel::workspaceStateJson()` now reports `visual_style:"agent_reference_panel_v4"` and `workspace_layout_version:4`. The existing workspace state fields remain intact: run state, trace/session labels, plan items, activity events, evidence cards, approvals, command input, and local staged command. This is not a provider runner, not telemetry export, and not a project-mutating workflow.

The stable new semantic IDs are `panel:agent_status_rail`, `panel:agent_command_composer`, `panel:agent_plan_deck`, `panel:agent_evidence_lane`, and `panel:agent_approval_lane`. Existing IDs such as `panel:agent_session_strip`, `panel:agent_run_controls`, `panel:agent_active_plan`, `panel:agent_evidence_tray`, `panel:agent_approval_card`, `action:agent_pause_run`, `action:agent_resume_run`, `action:agent_stop_run`, and the footer command controls remain available.

Focused GUI verification passed 14 of 14 tests after a red test proved the missing style/version and new targetable regions. Visual proof used the official harness and produced `artifacts\screenshots\sprint201-agent-panel-visual-refinement-v2-20260605-115850.png`. Target validation produced `artifacts\screenshots\sprint201-agent-panel-visual-refinement-v2-targets-target-sequence.json` with no `false` entries and empty stderr. The sprint-end Qt build plus CTest gate passed 36 of 36 tests.

## Sprint 202 Agent Session GUI Binding

The native Agent pane now binds to CCad's local `.ccad-agent-session.json` file contract. The feature is intentionally metadata-only: it reads session ID, thread ID, checkpoint count, latest checkpoint ID, and replayable state, and it can append a GUI checkpoint record without starting a provider run, exporting telemetry, storing secrets, or mutating the active project.

The visible controls are targetable through `panel:agent_session_binding`, `control:agent_session_path`, `action:agent_load_session`, `action:agent_checkpoint_session`, and `label:agent_session_status`. A human can enter or browse to a local session file, load it, and use the compact checkpoint action to write the next `gui-checkpoint-N` metadata record. Automation can target the same controls through the UI map and target harness.

`AgentPanel::workspaceStateJson()` now reports `durable_session_bound`, `session_file_path`, `session_path_input`, `durable_session_id`, `thread_id`, `checkpoint_count`, `latest_checkpoint_id`, `replayable`, and `session_status` while preserving `visual_style:"agent_reference_panel_v4"` and `workspace_layout_version:4`.

Focused GUI verification passed 14 of 14 tests after a red test proved the missing session-binding contract. Visual proof used the official harness and produced `artifacts\screenshots\sprint202-agent-session-gui-binding-v2-20260605-122454.png`. Target validation produced `artifacts\screenshots\sprint202-agent-session-gui-binding-v2-targets-target-sequence.json` with no `false` entries and empty stderr. The sprint-end Qt build plus CTest gate passed 36 of 36 tests.

## Sprint 203 Agent Policy GUI Binding

The native Agent pane now previews CCad command policy before any future runner execution. The policy classifier was moved into `ccad_core` so the CLI, GUI, JSON-RPC agent service, and future durable runner can share the same read/write/dry-run contract. The old `src/ccad_cli/agent_policy.hpp/.cpp` path remains as a compatibility surface for existing CLI code.

The visible policy strip is targetable through `panel:agent_policy_surface`, `label:agent_policy_decision`, `label:agent_policy_risk`, `control:agent_policy_dry_run`, and `action:agent_policy_preview`. `agent.workspace_state` now reports `policy_decision`, `policy_risk_level`, `policy_approval_required`, `policy_approval_reason`, `policy_dry_run`, `policy_would_execute`, `policy_read_only`, `policy_mutates_project`, `policy_mutates_files`, `policy_command`, and `policy_args`.

Submitting `help --format json` reports low-risk `allow_read`. Submitting `pcb add-via --file board.ccad.json` reports `approval_required`, `project_mutation`, and high risk, and fills the local approval lane. Enabling the dry-run checkbox and previewing the same write command reports `dry_run_only` and `policy_would_execute:false`. The GUI still does not execute commands, call providers, export telemetry, store secrets, or mutate project files from this policy preview.

The UI map now treats `QCheckBox` widgets with `control:` object names as first-class controls. They appear in `ui.map`, `validateUiMapTargetsJson`, `ui.target`, and `ui.click`, with checked state reported for semantic clicks. This specifically fixes `control:agent_policy_dry_run` being missing from target validation.

Focused verification:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH%&& cmake --build build-qt --target ccad_gui ccad_gui_ui_map_tests ccad_gui_agent_panel_tests ccad_agent_serve_tests --config Debug&& ctest --test-dir build-qt -R gui_ --output-on-failure&& ctest --test-dir build-qt -R agent_serve --output-on-failure"
```

Visual proof used `scripts\run_sprint_demo.ps1` and `scripts\run_ui_map_mouse_target_demo.ps1`. Verified artifacts are `artifacts\screenshots\sprint203-agent-policy-gui-binding-20260605-124953.png`, `artifacts\screenshots\sprint203-agent-policy-gui-binding-targets-v2-target-sequence.json`, `artifacts\screenshots\sprint203-agent-policy-gui-binding-targets-v2-initial-control_agent_policy_dry_run.png`, and `artifacts\screenshots\sprint203-agent-policy-gui-binding-targets-v2-resized-control_agent_policy_dry_run.png`. The sprint-end full Qt build plus CTest gate passed 36 of 36 tests.

## Sprint 204 Agent Trace Links GUI Binding

The native Agent pane now exposes local trace-link metadata in the visible right-side workspace. This is deliberately a metadata-only binding: the GUI creates local deterministic trace IDs and span IDs, links them to the current durable session and thread when a session is bound, and reports export state as disabled. It does not start model/provider execution, emit OpenTelemetry spans, call Langfuse, open trace URLs, capture prompt or tool payload content, store secrets, or mutate the active project.

The visible trace surface is targetable through `panel:agent_trace_links`, `label:agent_trace_id`, `label:agent_span_id`, `label:agent_trace_status`, `label:agent_trace_export_status`, and `action:agent_new_trace_context`. `AgentPanel::workspaceStateJson()` now reports `trace_id`, `span_id`, `trace_status`, `trace_export_status`, `trace_backend:"local_metadata_only"`, `trace_export_enabled:false`, `trace_link:""`, `trace_link_available:false`, `trace_session_id`, `trace_thread_id`, and `trace_content_policy:"metadata_only_no_prompt_tool_or_design_payloads"`.

Creating a local trace context appends `agent_trace_context_ready` output and an Activity event, then updates the trace labels and workspace-state fields. Semantic `ui.click` can activate `action:agent_new_trace_context`, so the same trace action is available to human clicks, target-map automation, and the Agent panel/live-socket dispatcher.

The UI target resolver now scrolls targetable widgets inside ancestor scroll areas before returning coordinates. This keeps Agent-pane targets from resolving to negative offscreen positions after the pane is scrolled or after a resize. Canvas-object target validation also samples real hit points inside object bounds instead of trusting only the bounding-box center, which makes validation stable for thin lines, tracks, and overlapped CAD primitives.

Focused GUI verification passed 14 of 14 tests. Visual proof used the official harness and produced `artifacts\screenshots\sprint204-agent-trace-links-gui-binding-final2-20260605-140444.png`. Target validation produced `artifacts\screenshots\sprint204-agent-trace-links-gui-binding-targets-final-target-sequence.json` and an empty `artifacts\screenshots\sprint204-agent-trace-links-gui-binding-targets-final.stderr.log`; the inspected trace-action screenshot is `artifacts\screenshots\sprint204-agent-trace-links-gui-binding-targets-final-initial-action_agent_new_trace_context.png`. The sprint-end full Qt build plus CTest gate passed 36 of 36 tests.

## Sprint 205 Agent Provider Controls GUI Binding

The native Agent pane now exposes no-secret provider readiness in the visible right-side workspace. This is deliberately a metadata-only binding: the GUI lets a human or agent choose a provider family, enter a model hint, refresh environment-presence status, and see that execution remains disabled. It does not call providers, probe networks, automate consumer browser accounts, read or display secret values, write provider settings into project files, export telemetry, or mutate the active project.

The visible provider surface is targetable through `panel:agent_provider_controls`, `control:agent_provider_family`, `control:agent_provider_model`, `action:agent_provider_refresh_status`, `label:agent_provider_status`, `label:agent_provider_env`, and `label:agent_provider_execution_status`. `AgentPanel::workspaceStateJson()` now reports provider family, provider label, access path, model hint, model environment variable, primary key environment variable, all accepted environment-variable names, presence-only readiness, configured state, status method names, execution-disabled state, no-secret visibility state, no-network-probe state, browser-automation-disabled state, project-file-secret-storage-disabled state, and the conservative secret/readiness policies.

The UI-map bridge now treats `QComboBox` widgets with `control:` object names as first-class controls. They appear in map export, target validation, target lookup, and semantic focus. `ui.type_text` can set `control:agent_provider_model`, and `ui.click` can activate `action:agent_provider_refresh_status`, so agents can update the visible provider metadata without scraping pixels or using raw coordinates.

Focused GUI verification passed 2 of 2 tests for `gui_agent_panel` and `gui_ui_map`. Visual proof used the official beep-and-screenshot harness and produced `artifacts\screenshots\sprint205-agent-provider-controls-gui-binding-final-20260605-150230.png`. Target validation produced `artifacts\screenshots\sprint205-agent-provider-controls-gui-binding-targets-target-sequence.json` with no `found:false` entries and empty stderr; inspected provider screenshots include `artifacts\screenshots\sprint205-agent-provider-controls-gui-binding-targets-initial-panel_agent_provider_controls.png`, `artifacts\screenshots\sprint205-agent-provider-controls-gui-binding-targets-initial-control_agent_provider_family.png`, `artifacts\screenshots\sprint205-agent-provider-controls-gui-binding-targets-initial-control_agent_provider_model.png`, and `artifacts\screenshots\sprint205-agent-provider-controls-gui-binding-targets-resized-panel_agent_provider_controls.png`. The sprint-end full Qt build plus CTest gate passed 36 of 36 tests.

## Sprint 206 Agent Run Queue Foundation

The native Agent pane now exposes a first-slice local run queue in the visible right-side workspace. This is deliberately metadata-only: the GUI shows queue status, queue counts, current step, and local cancel/clear controls, but it does not call providers, start LangGraph execution, emit OpenTelemetry spans, export to Langfuse, start a background worker thread, launch external processes, or mutate the active project.

The visible queue surface is targetable through `panel:agent_run_queue`, `label:agent_run_queue_status`, `label:agent_run_queue_counts`, `label:agent_run_queue_current_step`, `action:agent_cancel_run_queue`, and `action:agent_clear_run_queue`. The queue section is placed near the top of the Agent pane so the official screenshot can show it without relying only on scroll-target proof.

`AgentPanel::workspaceStateJson()` now reports queue metadata: `run_queue_available`, `run_queue_id`, `run_queue_thread_id`, `run_queue_session_id`, `run_queue_status`, `run_queue_depth`, `run_queue_completed_count`, `run_queue_failed_count`, `run_steps_total`, `run_step_current`, `run_step_current_index`, `run_queue_cancelable`, `run_queue_provider_execution_enabled:false`, `run_queue_worker_thread_enabled:false`, `run_queue_trace_export_enabled:false`, `run_queue_external_process_enabled:false`, `run_queue_project_mutation_enabled:false`, `run_queue_persistence`, `run_queue_policy`, and ordered `run_queue_steps`.

Semantic `ui.click` can activate `action:agent_cancel_run_queue` and `action:agent_clear_run_queue`. Cancel writes an `agent_run_queue_canceled` event, clears cancelability, and updates the run chip to stopped. Clear writes an `agent_run_queue_cleared` event and resets the queue depth and step count to zero. Both actions are local GUI state changes only.

Focused GUI verification passed 2 of 2 tests for `gui_agent_panel` and `gui_ui_map`. Visual proof used the official beep-and-screenshot harness and produced `artifacts\screenshots\sprint206-agent-run-queue-foundation-final2-20260605-153920.png`. Target validation produced `artifacts\screenshots\sprint206-agent-run-queue-foundation-targets-target-sequence.json` with 156 entries, zero missing targets, empty stderr, and 12 queue-related initial/resized screenshots, including `artifacts\screenshots\sprint206-agent-run-queue-foundation-targets-initial-panel_agent_run_queue.png` and `artifacts\screenshots\sprint206-agent-run-queue-foundation-targets-resized-panel_agent_run_queue.png`. The sprint-end full Qt build plus CTest gate passed 36 of 36 tests.

## Sprint 207 Agent Panel Reference Polish

The native Agent pane now exposes the fifth reference-inspired visual contract through `visual_style:"agent_reference_panel_v5"`, `workspace_layout_version:5`, `reference_layout_density:"compact_sidebar"`, and `reference_inspiration:"provided_agent_sidebar_samples"`. This is a visual and targetability sprint only: it does not add provider execution, worker threads, OpenTelemetry export, Langfuse calls, external process execution, or project mutation.

The pane now has a targetable compact header action bar, evidence thumbnail strip, approval preview, and one-row footer quick-action strip. New semantic IDs are `panel:agent_header_action_bar`, `panel:agent_evidence_thumbnail_strip`, `card:agent_evidence_thumbnail_datasheet`, `card:agent_evidence_thumbnail_drc`, `card:agent_evidence_thumbnail_schematic`, `card:agent_evidence_thumbnail_revision`, `panel:agent_approval_preview`, `panel:agent_approval_preview_artifact`, `label:agent_approval_preview_summary`, `label:agent_approval_preview_delta`, `panel:agent_footer_quick_actions`, `action:agent_quick_request_context`, and `action:agent_quick_trigger_drc`.

The layout keeps every existing Agent control ID alive. Existing header and footer context/DRC controls still call the same local preset methods, while the new quick actions call the same safe local handlers. Advanced provider, trace-link, session-binding, policy, and run-control panels remain targetable but move lower in the scroll so the first viewport gives more space to evidence and approval.

Focused GUI verification passed 2 of 2 tests for `gui_agent_panel` and `gui_ui_map` after red tests proved the missing v5 style and UI-map regions. Visual proof used the official beep-and-screenshot harness and produced `artifacts\screenshots\sprint207-agent-panel-reference-polish-v4-20260605-163527.png`. Target validation produced `artifacts\screenshots\sprint207-agent-panel-reference-polish-targets-target-sequence.json` with 182 entries, zero missing targets, 26 v5-region captures, and empty stderr. Inspected target screenshots include `artifacts\screenshots\sprint207-agent-panel-reference-polish-targets-initial-panel_agent_approval_preview.png`, `artifacts\screenshots\sprint207-agent-panel-reference-polish-targets-initial-panel_agent_evidence_thumbnail_strip.png`, and `artifacts\screenshots\sprint207-agent-panel-reference-polish-targets-initial-panel_agent_footer_quick_actions.png`. The sprint-end full Qt build plus CTest gate passed 36 of 36 tests.
## Sprint 174 UI Visual Parity and Context Menus Addendum

The native Qt review GUI now supports context menus via Right-Click, interactive object deletion via the `Delete` or `Backspace` keys, and drag-and-drop dragging of elements by selecting them first. The GUI features rounded scrollbars with a dark theme, less intrusive diagnostic markers for minor warnings like unconnected pads (small 1.5mm dots instead of 5mm hollow circles), and hides the `SelectionInspectorPanel` properties pane by default. Properties open via right-click "Properties" or by double-clicking objects.

## Sprint 218 Schematic Parity Addendum

Full UI parity has been extended from the PCB Canvas to the Schematic Canvas. The Schematic canvas now supports:
- Interactive component dragging via mouse drag
- Component deletion via Delete/Backspace hotkeys
- Context menus via right-click, including component deletion
- A functioning `ReviewWindow::deleteSelectedBoardObject` that handles both `BoardCanvasView` and `SchematicCanvasView` selected objects gracefully.

## Sprint 219 Python Orchestrator De-stubbing Addendum

The agent's Python orchestrator (`src/ccad_agent/orchestrator.py`) has been fully de-stubbed and integrated with the LangGraph state machine. Mock tool implementations (`_mock_call_kicad`, `_mock_policy_check`, etc.) were removed, and proper `ToolNode` instances wrapping the real tool implementations are now executed during the `call_tools` graph node. Provider and Execution contexts have been cleaned up.

## Sprint 220 KiCad PCB API Parity Query Addendum

The first KiCad PCB API parity audit slice starts from `F:\kicad_src\pcbnew\api\api_handler_pcb.h` and `api_handler_pcb.cpp`. CCad now exposes read-only headless analogues for the safe subset of KiCad handler behavior through `pcb list-enabled-layers`, `pcb list-visible-layers`, `pcb get-layer-name`, `pcb get-board-stackup`, `pcb get-rules`, `pcb get-outline`, `pcb list-by-net`, and `pcb list-connected`.

The Agent surface now has `agent pcb-api-schema` and JSON-RPC `agent.pcb_api_schema`. The schema maps KiCad handlers to CCad commands, declares first-slice behavior, and marks not-yet-complete areas such as full KiCad connectivity graph parity, board origin, graphics defaults, custom rules, pad shape polygons, selection, net classes, and active-layer mutation.

The same-net commands intentionally report `connectivity_scope:"net_equivalent_first_slice"`. They inspect current pad, via, track, and zone net IDs and do not yet infer copper contact, zone-fill islands, net ties, or KiCad connectivity solver state.

## Sprint 224 KiCad Board Bounding Box Addendum

The KiCad PCB source walk now includes `F:\kicad_src\pcbnew\board_bounding_box.cpp` and `F:\kicad_src\pcbnew\board_bounding_box.h`. CCad maps KiCad's transient `BOARD_BOUNDING_BOX` wrapper to derived scene and CLI metadata rather than a persisted project primitive. `CanvasScene` now exposes `board_bounding_boxes` with `class_name:"BOARD_BOUNDING_BOX"`, `layer_id:"LAYER_BOARD_BOUNDING_BOX"`, `skip_struct:true`, and millimeter rectangle coordinates derived from `Board::outline`.

The agent-facing `ccad pcb get-outline --file <project>` output still reports `outline_kind:"ccad_board_outline"` and `kicad_handler:"GetBoundingBox"`, and now also includes `kicad_class:"BOARD_BOUNDING_BOX"`, `kicad_view_layer:"LAYER_BOARD_BOUNDING_BOX"`, `kicad_skip_struct:true`, and a `bounding_box` payload. This lets automation reason about KiCad's view/runtime wrapper without treating it as a second durable board outline.

## Sprint 224 KiCad Board Commit Addendum

The KiCad PCB source walk now includes `F:\kicad_src\pcbnew\board_commit.cpp` and `F:\kicad_src\pcbnew\board_commit.h`. KiCad's `BOARD_COMMIT` stages board-item mutations, pushes them through undo, view, connectivity, ratsnest, zone, teardrop, component-class, board-outline, solder-mask, and dirty-state update paths, and can revert staged mutations.

CCad's analogue is deterministic transaction impact metadata. `ccad::Transaction` now carries `CommitImpact`, which reports whether the transaction changed board data or schematic data, dirtied view rendering, DRC, ERC, connectivity, ratsnest, board-outline refresh, or solder-mask rendering, and lists dirty object IDs and object types derived from the transaction diff. `dumpTransactionJson()` emits this `impact` object, so CLI audit JSONL records and future LLM harnesses can decide which checks, views, and evidence to refresh after a mutation.

The Agent orchestrator also restores the default provider safety contract: generated `agent.plan_with_provider` tasks enter the plan as `blocked` with `provider_execution_disabled` until a future approved provider runner explicitly enables provider execution. This keeps the current Agent layer honest while the KiCad parity work continues.

## Sprint 224 KiCad Board Connected Item Addendum

The KiCad PCB source walk now includes `F:\kicad_src\pcbnew\board_connected_item.cpp` and `F:\kicad_src\pcbnew\board_connected_item.h`. KiCad uses `BOARD_CONNECTED_ITEM` as the common net-owned base for connected board items such as pads, vias, tracks, and zones, with shared behavior for net naming, netclass lookup, clearance, local ratsnest visibility, and teardrop settings.

CCad now exposes the first headless analogue across the current connectable object surfaces. `pcb list-objects`, `pcb list-by-net`, `pcb list-connected`, direct object lookup, and `pcb export-route-job` mark pads, vias, tracks, and zones with `connected_item:true` and `kicad_connected_class:"BOARD_CONNECTED_ITEM"`. The same rows include `net_name`, `net_name_message`, `short_net_name`, `display_net_name`, `net_class_name`, `net_class_scope`, a compatibility `netclass_scope` alias, `local_ratsnest_visible`, and `teardrops_supported`.

This is intentionally not a fake full KiCad connectivity solver. The same-net query still uses CCad's first-slice `connectivity_scope:"net_equivalent_first_slice"`, and the netclass fields report `Default` with `default_netclass_until_model_exists` until the real netclass model is implemented.

## Sprint 221 KiCad PCB API Utility Layer-Set Addendum

The next KiCad PCB API parity slice reads `F:\kicad_src\pcbnew\api\api_pcb_utils.h` and `api_pcb_utils.cpp`. CCad now has a first analogue for KiCad's ordered layer-set utility behavior. `ccad::expandKiCadLayerSet` expands KiCad wildcard selectors such as `*.Cu`, `*.Mask`, `*.Paste`, `*.SilkS`, `*.Fab`, `*.CrtYd`, and `*.Adhes` against the current board layer list, de-duplicates layer IDs in board order, and preserves unmatched selectors so DRC or validation can still report them.

`ccad::standardKiCadPcbLayerNumbersForSet` converts resolved CCad layer IDs into canonical KiCad PCB layer numbers while omitting unknown custom entries. Footprint placement uses the expansion helper after front/back placement flips, so placed through-hole pads imported from KiCad footprints no longer keep raw `*.Cu` or `*.Mask` selectors when the board has matching layers.

Board-context PCB query and export surfaces now expose resolved pad layer metadata. `pcb list-objects --type pad`, `pcb list-by-net`, `pcb list-connected`, and `pcb export-route-job` include `resolved_layers` and `kicad_layer_numbers` for pads while direct `pcb get-object` still reports the raw stored `layers` array for compatibility and debugging.

## Sprint 222 KiCad PCB API Items, Matrix, Autoplace, and Spread Addendum

The next KiCad PCB editor parity slice records the remaining first-pass `F:\kicad_src\pcbnew\api` context files and starts the top-level PCB item walk. `agent pcb-api-schema` now records the complete registered handler ledger from `api_handler_pcb.cpp`, the enum groups from `api_pcb_enums.cpp`, the `BOARD_CONTEXT` ownership and save surface from `board_context.h/.cpp`, and the `HEADLESS_BOARD_CONTEXT` board/project/tool-manager lifecycle from `headless_board_context.h/.cpp`. The schema marks CCad's current split file-command context as a compatibility gap so future agents do not confuse per-command project reloads with KiCad's live board session.

`src/ccad_core/pad_number_provider.hpp/.cpp` is the first analogue for KiCad `array_pad_number_provider`. It can return the current pad number, advance with an incrementing numeric suffix, and generate bounded vectors of pad numbers without mutating board state. Agents can use it when they need deterministic array-like pad-number previews before creating package geometry.

`src/ccad_core/autorouter_matrix.hpp/.cpp` is the first analogue for KiCad `autorouter/ar_matrix`. It owns a board-grid occupancy matrix with top and bottom side masks, grid-aligned rectangle tracing, bitwise cell operations, distance-map generation, keepout-cost rectangles, and rectangle cost queries. This is not a complete autorouter, but it gives the placement and future route-planning layers a fast deterministic cost field instead of ad hoc geometry scans only.

`src/ccad_core/autoplacer.hpp/.cpp` now uses the matrix cost field while preserving explicit board-outline, keepout, and pad-overlap guards. The CLI exposes that behavior through `ccad pcb autoplace-footprint --file <project> --footprint <footprint.json> --component <id> --layer <copper-layer> [--grid-mm <n>] [--rotation-deg <n>]`, which places an imported footprint at the best low-cost candidate and reports the selected origin, score, and reason as JSON.

`src/ccad_core/spread_footprints.hpp/.cpp` is the first analogue for KiCad `autorouter/spread_footprints`. It groups board pads by component ID, naturally sorts references such as `R1`, `R2`, and `R10`, and moves the selected component groups into a non-overlapping placement lane while preserving each group's internal pad offsets. The CLI exposes this through `ccad pcb spread-footprints --file <project> [--components <a,b,...>] --target-x-mm <n> --target-y-mm <n> [--component-gap-mm <n>] [--group-gap-mm <n>]`.

Current limitations remain explicit. The autoplacer is a deterministic first slice and does not yet implement KiCad's full placement scoring, rotation search, ratsnest-driven global optimization, locked-footprint policy, or interactive confirmation loop. The spread command lays selected component groups into one lane and does not yet implement KiCad's full sheet/grid spreading behavior. The matrix is ready for future agent-guided routing and classic-router integration, but there is still no complete KiCad-equivalent autorouter.

## Sprint 223 KiCad Board Document Model Addendum

The next KiCad PCB editor parity slice reads `F:\kicad_src\pcbnew\board.cpp` and `F:\kicad_src\pcbnew\board.h`. CCad now treats a board as a first-class project document instead of assuming every PCB operation has a linked schematic at `schematics[0]`.

The model layer has `primaryBoard`, `primarySchematic`, and `ensurePrimarySchematic` helpers. Core, CLI, export, review, diff, placement, and agent-context code use those helpers for the touched paths, so missing schematics no longer crash board-owned behavior.

Physical DRC now works on board-only projects. When no schematic document exists, DRC still checks outline, design rules, layers, pads, vias, tracks, graphics, texts, zones, route requests, placement regions, keepouts, physical object IDs, and copper clearance. It deliberately skips schematic-link diagnostics such as unknown logical component, unknown logical pin, unknown logical net, and pad-net member mismatch. When a schematic document exists, those logical cross-checks remain active.

ERC now distinguishes an absent schematic document from an empty schematic document. An absent schematic is skipped because there is no schematic to electrically validate. An explicitly empty schematic still reports the existing `EMPTY_PROJECT` warning.

Review, BOM export, PnP export, KiCad PCB export, project diff, and agent-orchestrator context now tolerate board-only projects. KiCad PCB export also preserves board-local net names collected from pads, vias, tracks, and zones, so a PCB-only board does not silently collapse all routed copper to net 0.

Placement behavior is split by document ownership. Footprint placement remains a board operation and does not create a schematic. Symbol placement and schematic mutation commands create the primary schematic document when needed.

## Sprint 226 KiCad Board Design Settings Validation Addendum

The KiCad PCB editor source walk now includes `F:\kicad_src\pcbnew\board_design_settings.cpp`. CCad maps the first useful headless part of KiCad's `BOARD_DESIGN_SETTINGS::ValidateDesignRules()` behavior into `src/ccad_core/board_design_settings.hpp/.cpp`.

`validateDesignRules(const DesignRules&)` now checks KiCad-shaped field ranges for minimum clearance, connection, track width, via annular width, via diameter, through-hole drill, microvia diameter, microvia drill, hole-to-hole spacing, hole clearance, silk clearance, groove width, copper-edge clearance, solder mask expansion, solder mask minimum width, solder-mask-to-copper clearance, solder paste margin, solder paste margin ratio, and board thickness. DRC calls this helper instead of maintaining a separate partial range checker.

The compatibility decision is deliberate: KiCad allows zero minima for several rule fields, so CCad no longer reports `INVALID_COPPER_CLEARANCE`, `INVALID_MIN_TRACK_WIDTH`, or `INVALID_MIN_VIA_ANNULAR_RING` merely because those values are zero. Negative ranges are also preserved where KiCad allows them, such as small negative solder mask expansion and negative paste-margin-style values. Stricter manufacturer-specific policies should be modeled later as explicit constraints rather than hidden hard-coded DRC assumptions.

Focused coverage lives in `tests/test_board_design_settings.cpp` and the updated `tests/test_drc.cpp`. The focused green checks are `cmd /c ctest --test-dir build-qt -R board_design_settings --output-on-failure` and `cmd /c ctest --test-dir build-qt -R drc --output-on-failure`.

## Sprint 226 KiCad BOARD_ITEM Metadata Addendum

The KiCad PCB editor source walk now includes `F:\kicad_src\pcbnew\board_item.cpp` and `F:\kicad_src\include\board_item.h`. CCad maps the first useful headless part of KiCad's `BOARD_ITEM` base behavior into `src/ccad_core/board_item.hpp/.cpp` without changing the project JSON schema.

`BoardItemMetadata` derives KiCad-shaped metadata for current board objects. The emitted fields include `kicad_base_class`, `kicad_groupable`, `primary_layer_id`, `layer_ids`, `layer_mask_description`, `side_specific`, `is_on_copper_layer`, `has_hole`, `has_drilled_hole`, `locked`, `knockout`, `view_layer_ids`, and `parity_scope`. Pads resolve KiCad wildcard layer selectors through the active board layer list, vias use board copper layers, tracks and single-layer graphics/texts report their concrete layer, and zones report their stored layer set.

The metadata is available to agents through `pcb get-object`, `pcb list-objects`, `pcb list-by-net`, and `pcb list-connected` for pads, vias, tracks, graphics, texts, and zones. This gives downstream tools a KiCad-compatible board-object envelope before CCad has a full KiCad inheritance tree, property editor, object-group system, or per-item lock mutation commands.

Focused coverage lives in `tests/test_cli.cpp`. The red check failed with `test failure: pcb get-object exposes KiCad board item base class`, and the focused green checks are `cmd /c build-qt\ccad_cli_tests.exe` and `cmd /c ctest --test-dir build-qt -R cli --output-on-failure`.

## Sprint 226 KiCad BOARD_LOADER State Addendum

The KiCad PCB editor source walk now includes `F:\kicad_src\pcbnew\board_loader.cpp`, `F:\kicad_src\pcbnew\board_loader.h`, and `F:\kicad_src\qa\tests\pcbnew\test_board_loader.cpp`. CCad maps the first useful headless part of KiCad's `BOARD_LOADER` behavior into `src/ccad_core/board_loader.hpp/.cpp` as a derived state summary.

`summarizeLoadedBoard(const Project&, const BoardLoadOptions&)` reports a KiCad-shaped load envelope without pretending that CCad already has KiCad's PCB IO plugin manager or persistent DRC engine. The state reports `kicad_class:"BOARD_LOADER"`, source format, initialization mode, board attachment, design-rule readiness, DRC readiness, connectivity readiness, netlist readiness, user-unit readiness, board and schematic counts, layer visibility counts, physical object counts, board net counts, and explicit `pending_kicad_loader_steps`.

Agents can query the same state through `ccad pcb load-state --file <project.ccad.json>`. Passing `--initialize false` mirrors KiCad's raw load path before project attachment and post-load initialization by reporting `board_attached:false`, `drc_ready:false`, and `connectivity_ready:false` while still parsing the source file as data.

Focused coverage lives in `tests/test_board_loader.cpp` and `tests/test_cli.cpp`. The red check failed because `ccad_core/board_loader.hpp` did not exist, and the focused green checks are `cmd /c ctest --test-dir build-qt -R board_loader --output-on-failure` and `cmd /c ctest --test-dir build-qt -R cli --output-on-failure`.

## Sprint 226 KiCad BOARD_STACKUP Default Stackup Addendum

The KiCad PCB editor source walk now includes `F:\kicad_src\pcbnew\board_stackup_manager\board_stackup.cpp`, `board_stackup.h`, `dielectric_material.cpp`, `dielectric_material.h`, `board_stackup_reporter.cpp`, and `board_stackup_reporter.h`. CCad maps the first useful headless part of KiCad's `BOARD_STACKUP` behavior into `src/ccad_core/board_stackup.hpp/.cpp`.

`buildDefaultBoardStackup(const Board&)` derives a KiCad-style physical stack from current board layers and board thickness rules. It orders top silkscreen, top paste, top mask, copper layers, dielectric layers, bottom mask, bottom paste, and bottom silkscreen; assigns KiCad's default 0.035 mm copper thickness, 0.01 mm solder-mask thickness, FR4 dielectric material, epsilon-r 4.5, loss tangent 0.02, and 1 GHz specification frequency; distributes dielectric thickness from the remaining board thickness; and computes copper-to-copper layer distances with KiCad's half-internal-copper rule.

`ccad pcb get-board-stackup --file <project.ccad.json>` now emits `kicad_class:"BOARD_STACKUP"`, `kicad_parity_scope:"default_stackup_first_slice"`, stackup item rows, computed stackup thickness, copper layer distance rows, finish metadata, and the older enabled-layer rows for compatibility. The Agent PCB API schema now maps `GetBoardStackup` to the same first-slice physical stackup scope rather than the old `enabled_layer_order` label.

This remains a derived first slice. CCad does not yet persist editable stackup items, material libraries, dielectric sublayers, locked impedance-control thicknesses, copper finish, edge connector constraints, edge plating, or stackup-backed impedance/3D/Gerber-job behavior in project JSON.

Focused coverage lives in `tests/test_board_stackup.cpp` and `tests/test_cli.cpp`. The core red check failed because `ccad_core/board_stackup.hpp` did not exist, the CLI red checks failed on the missing KiCad stackup class and stale agent schema scope, and the focused green checks are `cmd /c ctest --test-dir build-qt -R board_stackup --output-on-failure` and `cmd /c ctest --test-dir build-qt -R cli --output-on-failure`.

## Sprint 226 KiCad Board Statistics Drill Table And Report Addendum

The KiCad PCB editor source walk now includes `F:\kicad_src\pcbnew\board_statistics.cpp`, `F:\kicad_src\pcbnew\board_statistics.h`, `F:\kicad_src\pcbnew\board_statistics_report.cpp`, `F:\kicad_src\pcbnew\board_statistics_report.h`, and `F:\kicad_src\qa\tests\pcbnew\test_board_statistics.cpp`. CCad maps the first useful headless part of KiCad's `CollectDrillLineItems()` behavior into `src/ccad_core/board_statistics.hpp/.cpp`.

`collectDrillLineItems(const Board&)` scans current board pads and vias, derives KiCad-style drill rows, and aggregates identical rows by shape, x/y drill size, plated state, source kind, start layer, and stop layer. Pads use their stored drill diameter and KiCad layer selectors resolved against the board layer list; vias report plated circular holes across the board copper span. `buildBoardStatisticsReport(const Board&, std::string, std::string)` builds the first KiCad `BOARD_STATISTICS_REPORT` analogue by combining outline dimensions, rectangular board area, current object counts, minimum track width, minimum drill diameter, board thickness, and drill rows.

Agents can query the same state through `ccad pcb drill-statistics --file <project.ccad.json>`. The command emits `kicad_reference:"board_statistics"`, `parity_scope:"drill_line_items_first_slice"`, a summary with unique row and total drill counts, and a `drill_holes` array with count, shape, x/y size, plated state, source, and nullable copper start/stop layers.

Agents can query the report through `ccad pcb board-statistics --file <project.ccad.json>`. The command emits `kicad_reference:"board_statistics_report"`, `parity_scope:"summary_report_first_slice"`, `board_outline`, `board_width_nm`, `board_height_nm`, `board_area_square_mm`, `counts`, nullable minimum width/drill fields, `board_thickness_nm`, and `drill_holes`.

This remains a truthful first slice. CCad does not yet compute KiCad's exact polygonal board area, copper areas, courtyard area, footprint density, minimum clearance across geometry, localized text report formatting, or the report options that subtract holes from board/copper areas.

Focused coverage lives in `tests/test_board_statistics.cpp` and `tests/test_cli.cpp`. The core red check failed because `ccad_core/board_statistics.hpp` did not exist, the CLI red checks failed with missing drill-statistics and board-statistics help entries, and the focused green checks are `cmd /c ctest --test-dir build-qt -R board_statistics --output-on-failure` and `cmd /c ctest --test-dir build-qt -R cli --output-on-failure`.

## Sprint 226 KiCad BOARD_ITEM_CONTAINER Remove/Delete Addendum

The KiCad PCB editor source walk now includes `F:\kicad_src\pcbnew\board_item_container.h`. CCad maps the first useful headless part of KiCad's `BOARD_ITEM_CONTAINER` behavior into `src/ccad_core/board_item_container.hpp/.cpp`.

`summarizeBoardItemContainer(const Board&)` reports the KiCad class name, board container kind, supported add modes, supported remove modes, current board-item count, current constraint-item count, and the fact that KiCad `Delete()` delegates through `Remove()`. `findBoardContainerItem()`, `hasBoardContainerItemId()`, `requireUniqueBoardContainerItemId()`, and `removeBoardContainerItem()` provide a shared board-level lookup and removal path over pads, vias, tracks, graphics, texts, zones, keepouts, and placement regions.

Agents can see the same compatibility contract through `ccad pcb remove-object --file <project.ccad.json> --id <object-id> [--mode normal|bulk]`. The command mutates the project as before, but it now emits a compact JSON result with `kicad_container_class:"BOARD_ITEM_CONTAINER"`, `kicad_method:"Delete"`, `remove_mode`, `removed`, `id`, `kind`, `index`, and `kicad_delete_semantics`. This lets agent tooling distinguish ordinary object deletion from bulk container removal while preserving CCad's current typed-vector storage model.

Focused coverage lives in `tests/test_board_item_container.cpp` and `tests/test_cli.cpp`. The core red check failed because `ccad_core/board_item_container.hpp` did not exist, the CLI red check failed on the missing `--mode` option, and the focused green checks are `cmd /c "ctest --test-dir build-qt -R cli --output-on-failure && ctest --test-dir build-qt -R board_item_container --output-on-failure"`.

## Sprint 226 KiCad BOARD_TEXT_VAR_ADAPTER Addendum

The KiCad PCB editor source walk now includes `F:\kicad_src\pcbnew\board_text_var_adapter.cpp`, `F:\kicad_src\pcbnew\board_text_var_adapter.h`, `F:\kicad_src\pcbnew\api\api_handler_pcb.cpp`, and `F:\kicad_src\common\api\api_handler_common.cpp`. CCad maps the first useful headless part of KiCad's board text-variable behavior into `src/ccad_core/board_text_var_adapter.hpp/.cpp`.

`Project::text_variables` stores the first KiCad-style project text variable map in project JSON. `expandTextVariables()` expands `${NAME}` tokens from that map, keeps unresolved tokens visible, and reports whether each reference resolved. `expandBoardTexts()` applies the same logic to every current `BoardText`.

Agents and scripts can mutate and inspect the map with `ccad project set-text-variable --file <project.ccad.json> --key <name> --value <text>` and `ccad project list-text-variables --file <project.ccad.json>`. They can resolve explicit text or all board texts with `ccad pcb expand-text-variables --file <project.ccad.json> [--text <value>]`. The agent PCB API schema now maps KiCad `ExpandTextVariables` to `pcb expand-text-variables` with `project_text_variable_expansion_first_slice`.

This remains a headless first slice. CCad does not yet implement KiCad's live `TEXT_VAR_TRACKER`, listener invalidation on board edits, footprint cross-reference sources such as `${U1:FIELD}`, title-block variables, barcode text dependencies, or GUI repaint invalidation.

Focused coverage lives in `tests/test_board_text_var_adapter.cpp` and `tests/test_cli.cpp`. The core red check failed because `ccad_core/board_text_var_adapter.hpp` did not exist, the CLI red check failed with `test failure: help json describes board text variable expansion`, and the focused green checks are `cmd /c ctest --test-dir build-qt -R board_text_var_adapter --output-on-failure` and `cmd /c ctest --test-dir build-qt -R cli --output-on-failure`.

## Sprint 226 KiCad Legacy Board BOM Export Addendum

The KiCad PCB editor source walk now includes `F:\kicad_src\pcbnew\build_BOM_from_board.cpp`. CCad maps KiCad's legacy PCB-editor BOM export into `src/ccad_core/bom_export.hpp/.cpp` and exposes it through `ccad pcb export-board-bom --file <project.ccad.json> --output <board-bom.csv>`.

Placed footprint metadata now exists on `Board::footprints`. Core placement records the placed reference, value, footprint name, layer, position, rotation, and BOM exclusion flag beside the concrete pads it already places. The KiCad footprint importer also preserves root `(attr exclude_from_bom)` as footprint metadata so excluded parts can be skipped by board-side BOM export.

`exportBoardToBomCsv(const Project&)` follows the legacy KiCad board-BOM shape: it rejects boards with no placed footprints, skips excluded footprints, groups rows by value plus footprint name, naturally sorts references such as `C2` before `C10`, sorts groups by first designator, and emits the KiCad header columns `Id`, `Designator`, `Footprint`, `Quantity`, `Designation`, and `Supplier and ref`.

This is intentionally not the full schematic-driven BOM generator. KiCad's own PCB editor documentation treats the board-side BOM as a simple fabrication output with no configurable options and recommends the schematic editor's BOM generator for richer output. CCad still needs future schematic BOM, supplier/procurement, manufacturer part, assembly, and configurable report work.

Focused coverage lives in `tests/test_bom_export.cpp` and `tests/test_cli.cpp`. The core red check failed because `BoardFootprint`, `Board::footprints`, and `exportBoardToBomCsv()` did not exist, the CLI red check failed with `test failure: help json describes KiCad-style board BOM export`, and the focused green checks are `cmd /c ctest --test-dir build-qt -R bom_export --output-on-failure` and `cmd /c ctest --test-dir build-qt -R cli --output-on-failure`.

## Sprint 226 KiCad CLEANUP_ITEM Catalog Addendum

The KiCad PCB editor source walk now includes `F:\kicad_src\pcbnew\cleanup_item.cpp` and `F:\kicad_src\pcbnew\cleanup_item.h`. CCad maps the cleanup action row catalog into `src/ccad_core/cleanup_item.hpp/.cpp` and exposes it through `ccad pcb cleanup-actions`.

The core catalog records the KiCad action IDs, titles, and domains for tracks/vias cleanup and graphics cleanup. The action set covers shorting tracks, shorting vias, redundant vias, duplicate tracks, co-linear track merging, dangling tracks, dangling vias, zero-length tracks, tracks inside pads, zero-size graphics, duplicate graphics, line-to-rectangle conversion, and overlapping-shape-to-pad merging.

`CleanupActionProvider` mirrors the first useful behavior of KiCad's `VECTOR_CLEANUP_ITEMS_PROVIDER`: indexed row access is bounds checked, non-deep deletion leaves the backing row in place, and deep deletion erases the row from the backing vector. This gives agents a truthful planning surface for future cleanup automation without claiming the full cleaner algorithms are implemented.

`pcb cleanup-actions` emits a JSON envelope with `kicad_class:"CLEANUP_ITEM"`, `provider_class:"VECTOR_CLEANUP_ITEMS_PROVIDER"`, `provider_semantics:"vector_indexed_rows"`, `parity_scope:"cleanup_action_catalog_first_slice"`, and one row per cleanup action. The current implementation is discovery-only; actual cleanup execution, dialogs, preview lists, redundant-track detection, dangling-via detection, duplicate-graphic detection, and merge algorithms remain future work.

Focused coverage lives in `tests/test_cleanup_item.cpp` and `tests/test_cli.cpp`. The red check failed because `src/ccad_core/cleanup_item.cpp` did not exist, and the focused green checks are `cmd /c ctest --test-dir build-qt -R cleanup_item --output-on-failure` and `cmd /c ctest --test-dir build-qt -R cli --output-on-failure`.

## Sprint 226 GUI Empty Project Survival Addendum

The native Qt GUI now handles empty and board-only project loads without crashing during viewport/status updates. The regression was in `ReviewWindow::updateCursorStatus()` and `ReviewWindow::updateSelectionStatus()`, where the old code assumed `project_cache_.boards[0]` existed while `BoardCanvasView::zoomToFit()` was still allowed to emit viewport callbacks for a project with no board.

The fixed behavior treats the active board as optional at the UI callback boundary. Cursor status uses the board-aware formatter only when a board is present, selection status falls back to a generic canvas item when no board exists, and the DRC/rules inspector is only rendered when a real board is loaded.

Focused coverage lives in `tests/test_gui_ui_map.cpp`. The test now loads an empty project and a board-only project through `ReviewWindow`, verifies the UI-map contract after each load, shows the window, and keeps the Qt event loop alive for 7 seconds. The red check failed with `SEGFAULT`; the green check is `cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && ctest --test-dir build-qt -R gui_ui_map --output-on-failure"`.

Visual and executable survival proof used the official harness and direct launches. The harness command was `cmd /c powershell -NoProfile -ExecutionPolicy Bypass -File scripts\run_sprint_demo.ps1 -BuildDir build-qt -Name sprint226-gui-crash-smoke -GuiWaitSeconds 7`, producing `artifacts\screenshots\sprint226-gui-crash-smoke-20260621-180502.png`. Direct 7-second launch checks covered `build-qt\ccad_gui.exe` with no arguments, `artifacts\demos\gui-min.ccad.json`, `artifacts\demos\gui-board.ccad.json`, and `artifacts\demos\sprint226-gui-crash-smoke.ccad.json`.

## Sprint 226 KiCad GENERAL_COLLECTOR Locked-Item Addendum

The KiCad PCB editor source walk now includes `F:\kicad_src\pcbnew\collectors.cpp` and `F:\kicad_src\pcbnew\collectors.h`. CCad maps the first useful headless part of KiCad's `GENERAL_COLLECTOR::IgnoreLockedItems()` behavior into the existing board collector and PCB collection CLI.

Pads, vias, tracks, board graphics, board texts, zones, and placed board footprints now carry an optional `locked` flag in project JSON. The serializer accepts the field on input and only emits it when true, so older projects and ordinary unlocked objects stay compact. `BoardItemMetadata` and board-collector candidates derive the same lock state, and `pcb collect-items` includes a `locked` boolean on each returned row.

Agents and scripts can now call `ccad pcb collect-items --file <project.ccad.json> --ignore-locked true` to exclude locked candidates from the returned collection. Without that flag, locked items are still visible and explicitly marked, which matches KiCad's pattern of treating lock suppression as a collector option rather than hiding object state globally.

This is not the full KiCad collector stack yet. Preferred-layer secondary ordering, footprint-child filters, text and footprint side filters, pad/via type filters, zone-fill suppression, ignored-track lists, interactive hit-testing, and selection-policy integration remain future slices.

Focused coverage lives in `tests/test_board_collector.cpp` and `tests/test_cli.cpp`. The focused green checks are `cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && ctest --test-dir build-qt -R board_collector --output-on-failure"` and `cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && ctest --test-dir build-qt -R cli --output-on-failure"`.

## Sprint 226 Safe CLI Project Writes Addendum

CLI project writes now serialize the `Project` to memory before opening the destination path. This prevents a serialization-time failure from first truncating the existing project file, which was observed during the Sprint 226 model-layout change while debugging a zero-byte project output.

The behavior lives in `src/ccad_cli/common.cpp::writeProjectFile()`. It still writes the final JSON through the same deterministic serializer and returns the same success or failure result to callers, but the destination file is not opened until `dumpProjectJson(project)` has completed.

This is a safety hardening step, not a complete crash-proof write transaction. Atomic temporary-file replacement and fsync-style durability remain backlog for a later persistence sprint.

## Sprint 226 KiCad ConvertShapeListToPolygon Addendum

The KiCad PCB editor source walk now includes `F:\kicad_src\pcbnew\convert_shape_list_to_polygon.cpp` and `F:\kicad_src\pcbnew\convert_shape_list_to_polygon.h`. CCad maps the first useful headless part of KiCad's `ConvertOutlineToPolygon` behavior into `src/ccad_core/board_outline_polygon.hpp/.cpp`.

`buildBoardOutlinePolygonReport(const Board&, const BoardOutlinePolygonOptions&)` scans current board graphics for `Edge.Cuts` line segments, chains exact nanometer endpoints, reports the ordered outline points, identifies whether the chain is closed and valid, derives a bounding box for a closed contour, and preserves source graphic IDs. If `infer_outline_if_necessary` is true, an open or missing Edge.Cuts outline can fall back to the durable rectangular `Board::outline` while marking `used_inferred_outline:true`.

Agents and scripts can inspect the same state with `ccad pcb outline-polygon --file <project.ccad.json> [--infer true|false]`. The JSON response includes `kicad_source:"convert_shape_list_to_polygon"`, `kicad_function:"ConvertOutlineToPolygon"`, `parity_scope:"edge_cuts_segment_chain_first_slice"`, segment counts, closed and valid flags, fallback state, point rows, bounding box fields, diagnostics, and pending KiCad feature names.

This is deliberately not the full KiCad polygon engine. CCad does not yet support arc-preserving conversion, circle/rectangle/poly/ellipse conversion, hole contour nesting, disjoint-outline policy, endpoint epsilon matching, self-intersection diagnostics, or footprint Edge.Cuts hole detection.

Focused coverage lives in `tests/test_board_outline_polygon.cpp` and `tests/test_cli.cpp`. The core red check failed because `ccad_core/board_outline_polygon.hpp` did not exist, and the focused green checks are `cmd /c ctest --test-dir build-qt -R board_outline_polygon --output-on-failure` and `cmd /c ctest --test-dir build-qt -R cli --output-on-failure`.

## Sprint 226 KiCad Cross-Probing Packet Addendum

The KiCad PCB editor source walk now includes `F:\kicad_src\pcbnew\cross-probing.cpp`, and the official KiCad schematic documentation was checked for cross-probing behavior. CCad maps the first useful headless part of KiCad's PCB/schematic cross-probing packet flow into `src/ccad_core/cross_probing.hpp/.cpp`.

`resolveCrossProbePacket(const Project&, std::string_view)` accepts KiCad-style `$NET`, `$NETS`, `$PART`, `$PAD`, `$SELECT`, and `$CLEAR` packets. It resolves net packets to schematic net rows and same-net board pads, vias, tracks, and zones; resolves part and pad packets to footprint, component, and pad targets; preserves selection focus metadata for `$SELECT`; and records `clear_highlight` for `$CLEAR`.

Agents and scripts can inspect the same state with `ccad pcb cross-probe --file <project.ccad.json> --packet <packet>`. The JSON response includes `kicad_source:"pcbnew/cross-probing.cpp"`, `kicad_class:"PCB_EDIT_FRAME"`, `kicad_function:"ExecuteRemoteCommand"`, packet kind, requested nets, target rows, diagnostics, and `pending_kicad_features` for the live GUI and IPC behavior that still needs to be ported.

This is deliberately not full KiCad cross-probing yet. CCad does not yet open a live Kiway/socket channel, flash or zoom GUI targets, synchronize live PCB/schematic selection state, honor full sheet-path prefixes, update connected net highlighting, or dispatch KiCad's DRC/config/custom-rule remote commands.

Focused coverage lives in `tests/test_cross_probing.cpp` and `tests/test_cli.cpp`. The red check failed because `ccad_cross_probing_tests` did not exist, and the focused green checks are `cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && ctest --test-dir build-qt -R cross_probing --output-on-failure"` and `cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && ctest --test-dir build-qt -R cli --output-on-failure"`.

## Sprint 242 KiCad Net Info Addendum

The KiCad PCB editor source walk now includes `F:\kicad_src\pcbnew\netinfo_item.cpp` and `F:\kicad_src\pcbnew\netinfo_list.cpp`. CCad maps the first useful headless part of KiCad's net statistic reporting into `src/ccad_core/net_info.hpp/.cpp`.

`getNetInfo(const Project&, const std::string& net_id)` evaluates the board and returns a `NetInfoReport` containing total pad count, via count, total Euclidean track length, and a bounding box enveloping all board primitives connected to the net.

Agents and scripts can inspect the same state with `ccad pcb get-net-info --file <project.ccad.json> --net <net_id>`. The JSON response includes the net ID, geometric counts, `track_length_nm`, bounding box fields, diagnostics, and `pending_kicad_features` for unmapped capabilities (e.g. pad-to-die internal IC delays, dynamic netname disambiguation).

Focused coverage lives in `tests/test_net_info.cpp` and `tests/test_cli.cpp`. The focused green checks are `cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && ctest --test-dir build-qt -R net_info --output-on-failure"` and `cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && ctest --test-dir build-qt -R cli --output-on-failure"`.

## Sprint 243 KiCad Pad Machining Addendum

The KiCad PCB editor source walk now includes `F:\kicad_src\pcbnew\pad.cpp`. CCad maps the headless post-machining knockout calculations (`GetPostMachiningKnockout` and `IsBackdrilledOrPostMachined`) into `src/ccad_core/pad_utils.hpp/.cpp`.

`getPostMachiningKnockout(const Board&, const Pad&, const std::string& layer_id)` computes the removed copper diameter from countersink and counterbore on specific layers, estimating layer depth from the ordinal layer fraction against `board_thickness`.

`isBackdrilledOrPostMachined(const Board&, const Pad&, const std::string& layer_id)` checks if a layer falls within the secondary/tertiary drill start and end layer boundaries, or if it is affected by post-machining knockout.

Agents and scripts can inspect the same state with `ccad pcb get-pad-machining --file <project.ccad.json> --pad <pad_id> --layer <layer_id>`. The JSON response includes the post machining knockout size, boolean flag for backdrilled/post machined, and `pending_kicad_features` for unmapped capabilities (e.g. pad-to-die internal delay math, free pad checking).

Focused coverage lives in `tests/test_pad_utils.cpp` and `tests/test_cli.cpp`. The focused green checks are `cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && ctest --test-dir build-qt -R pad_utils --output-on-failure"` and `cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && ctest --test-dir build-qt -R cli --output-on-failure"`.

## Sprint 82 Dimensions and Groups

The CCad model now supports `BoardDimension` and `BoardGroup` natively in the `Board` object model. These correspond to KiCad's dimension primitives and group primitives.

Agents and scripts can author these objects through the CLI commands:
- `ccad pcb add-dimension --file <project.ccad.json> --id <id> --layer <layer> --kind <kind> --text <text> --start-x-mm <n> --start-y-mm <n> --end-x-mm <n> --end-y-mm <n>`
- `ccad pcb add-group --file <project.ccad.json> --id <id> --name <name> --members <id1,id2,...>`

The JSON board representation includes these elements, and they are queryable using `ccad pcb list-objects` and `ccad pcb get-object`. Dimensions are also visualized via the Qt GUI canvas using a light path connecting the start and end coordinates with the centered value text.

## Sprint 226 KiCad Additional PCB Objects

The CCad model now supports BoardBarcode, BoardReferenceImage, BoardTable, BoardTarget, and BoardTextbox natively in the Board object model, corresponding to KiCad's equivalent primitives.

These objects are fully supported in CCad's JSON IO and are exposed via the CLI commands pcb add-barcode, pcb add-reference-image, pcb add-table, and pcb add-target. They are also queryable via pcb list-objects and pcb get-object.

In the GUI, these objects are mapped to Qt QGraphicsItem derivatives inside the CanvasScene, rendering basic placeholders, reference bounding boxes, table borders, and standard alignment target crosshairs, matching their KiCad behaviors.

## Sprint 228 KiCad PCB Editor Interaction Slice

CCad's human GUI and agent APIs now support a robust set of KiCad-like interaction mechanics:
- **Select**: Single-clicking an object highlights it and enables `Move` operations.
- **Measure**: A dedicated measure tool overlays cyan `dx`, `dy`, and `dist` metrics during viewport drags.
- **Route/Add Via/Add Zone/Draw Graphic/Place Text**: Native `InteractionMode` handlers display a live placement ghost tracking the cursor. Agents use `pcb add-track`, `pcb add-via`, `pcb add-zone`, etc.
- **Move**: Pressing `<M>` or dragging selection starts a move interaction, snapping the ghost to the cursor.
- **Delete**: The `<Delete>` key removes selected items.
- **Properties**: Double-clicking an object or using the context menu's 'Properties...' action opens the `SelectionInspectorPanel` which allows coordinate/size edits (achieving resize parity without complex GUI corner handles).
- **Context Menu**: Right-clicking exposes quick actions like Properties, Delete, and Zoom to Fit.
- **Tooltip**: Hovering over objects displays semantic information via Qt's native tooltip system.
- **Escape-Cancel**: Pressing `<Esc>` terminates the active placement or interaction mode.

## Sprint 229 PCB Appearance panel
CCad's object browser was overhauled to mirror the KiCad appearance panel:
- **Tabs**: Separates layers, objects, and nets into distinct QListWidget tabs.
- **Color Swatches**: Shows accurate layer and net colors using custom generated QIcons to match the CanvasRenderTheme.
- **Active Layer Tracking**: Selecting a layer in the Appearance dock automatically pushes it to the active board layer state and synchronizes with the top toolbar.
- **Toggle Visibility**: Unchecking a layer properly hides its associated geometries from the board canvas.

## Sprint 230 PCB Barcode generation
- **C++ QR Code Generation**: CCad natively generates QR Codes utilizing Project Nayuki's lightweight C++ `qrcodegen` library to mirror KiCad's `pcb_barcode.cpp`.
- **Canvas Rendering**: Barcodes compute their matrix data internally into standard coordinate systems, and are fully rendered natively using `QGraphicsPathItem` for optimal performance in the canvas scene.

## Sprint 232 Schematic Symbol Snapshot Persistence

Placed schematic symbols now preserve the imported symbol body in the project model. A `SchSymbol` can carry an embedded `Symbol` snapshot, and the placement path stores the selected symbol snapshot when a symbol is placed. The project reader accepts both the newer `symbols` array and the legacy `components` array, accepts both `lib_id` and `part`, and accepts both pin `type` and legacy `kind` fields.

The serializer writes embedded symbol snapshots with properties, pins, rectangles, lines, arcs, circles, polylines, and texts. The schematic canvas builder expands those snapshots into transformed canvas primitives at the placed symbol position and rotation, so saved and reloaded schematic symbols render with body, labels, and pin-lead geometry instead of becoming generic placeholders.

Focused verification covered placement, CLI compatibility, and the tabbed object browser. The sprint-end gate passed `cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && ctest --test-dir build-qt --output-on-failure"` with 56 of 56 tests. Visual proof used the official GUI harness at `artifacts\screenshots\sprint232-symbol-snapshot-proof-internal-20260629-181543.png` and a targeted schematic screenshot at `artifacts\screenshots\sprint232-schematic-symbol-snapshot.png`, with empty targeted stderr.

## Sprint 244 Footprint Losslessness Harness

Status: implemented.
Files:
- `src/ccad_core/footprint_losslessness.hpp`
- `src/ccad_core/footprint_losslessness.cpp`
- `tests/test_footprint_losslessness.cpp`
- `src/ccad_cli/lib_commands.cpp`

What it does:
- Provides a verification harness to compare two footprint structures (original vs candidate).
- Detects discrepancies in properties (footprint name, BOM exclusion), pad numbers, pad shapes, dimensions, locations, rotations, and layer sets.
- Adds the CLI subcommand `lib verify-footprint-losslessness --in-kicad <file.kicad_mod> --in-ccad <file.json>` which outputs a JSON report with details of any mismatched footprint features.
- Emits exit code `0` on successful losslessness match, and exit code `1` with diagnostic information on mismatches.
- Fully verified via unit testing and visual validation checks.


## Windows Unicode Path Support

Sprint 246 adds robust Unicode/UTF-8 path support on Windows for the command-line interface (CLI) and all internal filesystem calls, enabling symbol imports and design files containing non-ASCII characters (e.g. `π120U30.kicad_sym`):
- **Unicode CLI Interception**: Custom wide-character command-line parser intercepting native OS arguments using `GetCommandLineW` and `CommandLineToArgvW` on Windows, ensuring argument lists remain lossless before translation into UTF-8 formats.
- **FS Streams UTF-8 Translation**: Updated internal file operations (`std::ifstream` and `std::ofstream`) using `ccad::u8ToPath` helper methods to correctly translate UTF-8 string parameters into Windows wide-character paths (`std::wstring`).
- **Catalog Integration**: Integrated UTF-8 path helpers with `ccad_core` library catalog validation logic to seamlessly verify native items containing Unicode filenames.
- **Verification**: Verified end-to-end command compatibility by importing S-expression library files containing Greek letters and symbols.


## Canvas Spatial Indexes

Sprint 247 introduces a fast uniform-grid spatial index for QGraphicsItems in the board canvas viewport:
- **Fast Nearest Queries**: Added a `CanvasSpatialIndex` to bucket graphics items by grid cell coordinates, optimizing queries from O(N) linear scans to localized queries.
- **Auto Rebuilding**: Integrated spatial indexing into the UI map epoch lifecycle, ensuring index accuracy whenever the canvas scene layout updates.
- **Test Coverage**: Added dedicated unit tests verifying the grid bounds indexing and nearest lookup correctness.


## Deterministic GUI Action Tools

Sprint 248 expands the native review window's automated GUI control interface to simulate key interactive capabilities:
- **Scroll Controls**: Allows automated vertical and horizontal scroll adjustment on targeted widgets (`ui.scroll`).
- **Key Sequence Sending**: Supports sending complex modifier key combinations and sequence inputs based on `QKeySequence` (`ui.key`).
- **List and Canvas Double clicks**: Implemented double-click event simulation for canvas objects and list items (`ui.double_click`).
- **Dialog and Menu Dispatch**: Resolves active child modal dialogs and active dropdown menus to automate option selection and dismiss button triggers.
- **Properties Editing**: Supports updating properties of selected items (`ui.edit_properties`).


 
 # #   S p r i n t   2 2 7   K i C a d   P C B   I t e m   G e o m e t r y 
 T h e   K i C a d   P C B   e d i t o r   s o u r c e   w a l k   n o w   i n c l u d e s   g e o m e t r y   c a l c u l a t i o n s .   C C a d   m a p s   t h e   h e a d l e s s   g e o m e t r y   m a t h   i n t o   \ s r c / c c a d _ c o r e / i t e m _ g e o m e t r y . h p p / . c p p \ . 
 
 \ i t e m B o u n d i n g B o x \ ,   \ i t e m H i t T e s t \ ,   \ i t e m L e n g t h \ ,   a n d   \ p a d A n n u l a r R i n g \   c o m p u t e   e x a c t   d i m e n s i o n s   a n d   b o u n d i n g   b o x e s   f o r   B o a r d   i t e m s   s u c h   a s   p a d s ,   v i a s ,   t r a c k s ,   a r c s ,   a n d   z o n e s . 
 
 A g e n t s   a n d   s c r i p t s   c a n   i n s p e c t   t h e   g e o m e t r y   m a t h   t h r o u g h   \ c c a d   p c b   i t e m - g e o m e t r y   - - f i l e   < p r o j e c t >   - - i d   < i d >   [ - - h i t - t e s t - x - m m   < x >   - - h i t - t e s t - y - m m   < y >   - - h i t - t e s t - a c c u r a c y - n m   < a c c u r a c y > ] \ .  
 
## Refdes Tracker and Pin Type Support

Status: implemented.
Files:
- `src/ccad_core/refdes_tracker.hpp`
- `src/ccad_core/refdes_tracker.cpp`
- `src/ccad_core/pin_type.hpp`
- `src/ccad_core/pin_type.cpp`

What it does:
- Provides `ccad::RefdesTracker` for efficient reference designator tracking and O(1) existence checks.
- Provides gap-filling auto-allocation for next available reference designators (e.g. creating R3 when R1 and R2 exist).
- Provides serialization and canonical string conversion for KiCad parity pin types (ElectricalPinType, GraphicPinShape, PinOrientation).

Use:
- Core integration: Call `tracker.allocate("R")` to get "R1".
## DRC drilled-hole and board-edge checks

Implemented in Sprint 358. Through-hole pads report missing or below-minimum drills, pads report copper closer than the configured board-edge clearance, and different-net vias report colocated or insufficiently separated drill geometry. Focused tests and the full Qt/CTest gate cover the behavior; the official visual proof confirms the surrounding board and agent UI remain usable.
## FastMath3D trigonometry

`FastMath3D::fastSin` and `fastCos` now return correct float sine/cosine values across representative angles. Regression coverage is registered as CTest `3d_fastmath`.
## Math3D homogeneous transforms

The core `Math3D::transform` API now applies a 4x4 matrix to 3D points and performs the homogeneous divide when required. Regression coverage is included in CTest `3d_fastmath`.
## Ratnest minimum-spanning tree

The core connectivity helper now computes shortest flying-lead connectivity with deterministic Prim MST output. CTest `nearest_neighbor_connectivity` covers the behavior.
## Board-backed ratnest graph

The core ratnest graph now scans board pads, vias, and track endpoints into deterministic per-net node sets. CTest `dynamic_ratnest_graph` covers collection and ordering.
## Core-driven GUI ratsnest overlay

The PCB review canvas now renders deterministic MST-style ratsnest edges generated by the core connectivity algorithm instead of scene-order endpoint chains. The overlay remains controlled by the existing Show Ratsnest action.
## Event-driven ratnest updates

Board modification updates now recompute and publish active per-net MST ratnest edges. The review canvas renders the same core-driven result.
## Complete current UI-map target coverage

Menu, Agent dock, chat composer, and send action are now addressable through the UI-map automation interface. The official 15-target sequence passes all targets in both initial and resized runs.
## Maximum track and via constraints

Projects can now configure optional maximum track width and via diameter rules through `pcb set-rules`; DRC reports violations while preserving backward compatibility with zero-disabled defaults.
CLI help exposes both maximum-rule options, and serialization/CLI regression tests verify exact persistence.

## Track-length measurement

Core measurement sums Euclidean lengths of track segments per net in millimetres. Meander generation is not yet implemented.
Track arcs are included using their three-point circular length.
Matching vias optionally contribute board thickness when height-for-length calculation is enabled.
## Silkscreen clearance

DRC checks front/back silkscreen text bounding boxes against copper pads using configured silk clearance.
Valid copper vias are also checked.
Copper track segments are checked with width-aware clearance.
Board-edge proximity is checked from text bounding-box corners.
Copper zone polygon overlap/proximity is checked.
## Schematic/board footprint parity

DRC reports missing, extra, and duplicate footprint references when schematic and board data are available. Pad component IDs support imported boards without footprint metadata.
Matching schematic/footprint BOM inclusion is also checked.
Explicit board footprints are checked for matching schematic pin pads.
Schematic-only (`on_board=false`) symbols are excluded from physical footprint/pad requirements.

## Solder-mask bridge DRC

DRC reports `SOLDERMASK_BRIDGE` when expanded valid copper-pad mask regions on different non-empty nets violate configured minimum mask web width. Defaults remain disabled when `solder_mask_min_width` is zero.

## Minimum board-text height

Projects can configure `min_text_height` through `pcb set-rules --min-text-height-mm`; DRC reports `TEXT_HEIGHT_BELOW_MINIMUM` for undersized board text. Text stroke thickness remains pending an explicit text stroke-width model.

## Board-text mirroring

Board text stores mirror state and persists it through JSON. DRC enforces unmirrored front-layer text and mirrored back-layer text; `pcb add-text` accepts optional `--mirrored true|false`.

## Track-angle DRC

Projects can configure minimum and maximum connected-track angles through `pcb set-rules`. DRC reports `TRACK_ANGLE` for same-net, same-layer straight segments sharing an endpoint outside that range.

## Track-segment length DRC

Projects can configure minimum and maximum straight track-segment lengths through `pcb set-rules`. DRC reports `TRACK_SEGMENT_LENGTH` for segments outside the configured range; track arcs remain pending.
Track arcs now use circular-sweep length for the same DRC rule, with collinear chord fallback.
Board text stores optional stroke width; CLI accepts stroke-width setting, and DRC enforces configured minimum text thickness.
### Router obstacle checks

Interactive and agent route gestures reject same-layer different-net track crossings and different-net via proximity. Checks preserve same-net and cross-layer routing, use physical clearance, and leave blocked multi-segment requests uncommitted. Via checks are conservative until layer-span metadata exists.
### Zone-aware routing

Interactive and agent route gestures now avoid filled different-net copper-zone polygons on the selected layer. Boundary crossings, interior endpoints, and configured clearance are rejected while same-net zones and other layers remain available.
### Arc-aware routing

Interactive and agent route gestures avoid different-net track arcs on the active copper layer using conservative chord and vertex-clearance checks. Same-net arcs and arcs on other layers remain routable.
### Sampled curved-copper routing safety

Different-net track arcs are now checked across a 16-point curve envelope, not only at their endpoints or two coarse chords. Routes crossing or approaching the sampled curved copper are rejected on the active layer.
### Circular arc route clearance

Different-net circular track arcs are now checked using their reconstructed three-point circular sweep rather than a distorted quadratic approximation. Routes crossing sampled arc geometry or its configured clearance are rejected on the active layer.
### Reconstructed circular-arc obstacles

Router obstacle checks reconstruct stored three-point circular arcs and follow the midpoint-containing sweep. Different-net routes crossing that sampled circular path or its clearance are rejected on the active layer.
### Agent provider notice presentation

Provider fallback/status messages now use a compact warning card, visually separating environment configuration notices from normal assistant responses and tool cards while preserving local CCad tool availability text.
### Agent route rejection feedback

Agent-facing route responses now report `blocked_obstacle` when copper clearance prevents placement, allowing callers to distinguish a real design-rule rejection from a zero-length gesture.
### Typed route obstacle feedback

Agent-facing route responses identify the blocking copper class, such as `blocked_obstacle_via` or `blocked_obstacle_zone`, instead of returning only a generic obstacle failure.
### Width-aware track obstacle clearance

Route gestures now account for both candidate route width and existing different-net track width when checking proximity, reducing false-safe near-parallel routing.
### Width-clearance regression contract

Dedicated automated coverage now proves wide existing copper blocks near-parallel different-net routes and leaves the board unchanged.
### Zone-hole persistence and routing

Board-zone holes are stored and restored in native JSON. A route whose endpoints remain within the same zone hole is allowed, while routes crossing the hole boundary remain subject to the parent zone obstacle check.

### CI portability and clearance units

Track-arc nanometre coordinates use explicit long-double casts for strict MSVC builds, `Project` forward declarations match its struct definition, and track obstacle distances are converted to millimetres before comparison with design-rule clearance.

### Typed courtyard overlap DRC

Footprints can persist front- and back-courtyard polygon geometry in native JSON, and the courtyard DRC provider reports overlapping same-side courtyard polygons. Missing-definition and pad-hole semantics remain queued for a later slice.
## Sprint 434 Job Manager Completion Semantics

`JobManager::waitAll()` now provides a true completion barrier for background work. It waits until both the pending queue and active worker count reach zero, avoiding premature API responses and CPU-heavy polling. `ccad_job_manager_tests` covers a task that remains running after dequeue and verifies that `waitAll()` waits for its completion.

JobManager workers also isolate task failures: exceptions are reported as task failures, worker threads remain available, and later queued tasks still execute. This keeps agent-triggered background services alive when one operation fails.

`AgentRunner::load_queue()` now restores saved pending goals/tasks after process restart. It preserves goal context and task arguments and rejects missing `pending_goals`, incomplete task identities, unterminated JSON, and trailing root data. Save/load round-trip and malformed-root behavior are covered by `ccad_job_manager_tests`.

AgentPanel session checkpoints now include local run-queue state. Loading a bound session restores queue status, current step, counts, depth, and cancelability through existing mapped session controls; provider and external execution remain disabled by policy.

The GUI regression suite now exercises this persistence path with a real temporary session file, proving checkpoint write and second-panel reload rather than only inspecting widget construction.

`convertImageToPolygons()` now emits real schematic polygon geometry from RGBA image data instead of a bounding-box placeholder. Transparent pixels are skipped, adjacent same-color pixels on each row are merged, RGB color is retained as hex, and pixel scale is converted to CCad lengths. `ccad_gfx_import_utils_tests` covers this behavior.

`convertSVGToLibShapes()` now imports common SVG `line`, `rect`, `polygon`, and `polyline` elements into typed schematic graphics, retaining fill values and applying scale/offset transforms. Unknown or unsupported elements are ignored without crashing; the focused graphics-import test covers representative primitives.

`DiffPairTuning::calculateCurrentSkew()` now reports measured positive/negative routed-length difference in millimetres from board track geometry. It safely returns zero when either requested net is absent; `ccad_diff_pair_tuning_tests` covers measurement and missing-net behavior.

Differential-pair tuning now has a truthful mutation contract: `applyTuning()` returns failure until the board model exposes the topology and clearance data required to generate real coupled meanders, preventing silent no-op success.

Schematic labels distinguish local, global, hierarchical, and directive types. JSON reads and writes `label_type` while retaining the legacy `global` field, and the schematic collector recognizes KiCad directive labels.

Board reference images with valid base64 image data render as scaled, opacity-preserving pixmaps in the PCB canvas; malformed data renders a tagged diagnostic placeholder.

Schematic sheet pins may persist KiCad side values (`left`, `right`, `top`, `bottom`) and render with that explicit direction; older projects continue using position-based inference. CLI integration tests use isolated temporary workspaces for concurrent safety.

PNS board obstacles now respect blind and buried via layer spans, preventing a via on another copper layer from falsely blocking routing while retaining conservative handling for legacy incomplete via metadata.

PNS board obstacles now preserve track segment geometry, so active-layer different-net tracks can block a candidate route when their spans cross or violate configured clearance.

Agent panel navigation and composer controls expose stable semantic IDs and tooltips for reliable human and agent interaction.

PNS obstacle adaptation now includes active-layer different-net track arcs with width-aware sampled envelope collision checks.

CLI cross-probe integration coverage now passes packet values containing `$` safely on POSIX and Windows shells.

Cross-probe packet parsing tolerates a preserved leading dollar escape, keeping `$NET` classification stable across command transports.

Filled zone obstacle checks now respect zone holes: routes inside holes remain available, while zone solids and hole boundaries remain protected.

PNS obstacle adaptation includes filled active-layer different-net zones, detecting route boundary crossings and routes inside zone solids.
## PNS spatial index

PNS items can now expose integer position and radius, and the native `PnsIndex` performs radius-aware point queries with duplicate suppression and invalid-radius rejection. `PnsNode` synchronizes ownership and index membership. This establishes query infrastructure for later topology-aware shove and differential-pair routing.
Zone settings validate thermal spoke width, thermal gap, and minimum island area as finite, non-negative values. Invalid values throw `std::invalid_argument`; zone refill geometry is not yet implemented.

`calculateZoneFill()` now validates enabled zone contours, preserves outer and hole contours, and reports net contour area for agent/kernel consumers. It does not claim obstacle clearance, thermal spoke generation, or island pruning yet.
`ccad pcb refill-zones --file <path> [--zone-id <id>]` exposes this deterministic result as JSON, including diagnostics for malformed contours.
Add `--apply true` to persist valid contours in `BoardZone.filled_contours`; omit it for read-only inspection.
Agent capability discovery maps `RefillZones` to this CLI surface and labels unsupported full-filler semantics explicitly.
KiCad PCB export consumes committed fill contours when present, with backward-compatible outline fallback.
PNS routing obstacle adaptation consumes committed fill contours when present, avoiding stale-outline blocking after refill application.
Rectangular zone refill applies configured clearance as an inset before persistence/export; non-rectangular clearance is diagnosed as unsupported.
Rectangular zone holes expand under clearance knockout, preventing over-reporting of filled copper.
Zone fill reports negative clearance as invalid instead of silently clamping it.
Zone fill rejects holes outside their containing outer contour.
Committed filled contours have JSON save/load regression coverage.
Rectangular thermal-spoke preparation now returns up to four axial spokes from the pad edge plus gap to the zone boundary, with focused geometry coverage. Integration with refill results and KiCad-compatible thermal clearance/mode behavior remains future work.
Thermal-spoke preparation rejects pad centers inside zone holes, avoiding false relief geometry. KiCad reference checked; pad/antipad fit and connection mode semantics remain deferred.
Thermal-spoke preparation also blocks axial spokes crossing supported rectangular zone holes. General polygon clipping and full KiCad connection-mode behavior remain future work.
Regression coverage confirms a hole placed on a spoke ray removes only blocked spoke geometry while preserving other directions.
Thermal-spoke hole blocking now handles arbitrary valid polygon edges, with rectangular and triangular crossing regressions.
Board-aware zone refill reports thermal spokes for matching-net pads through `pcb refill-zones`; explicit gap and width are currently 0.5 mm. Spokes are not yet persisted or exported as filled copper geometry.
Board-aware thermal reporting now ignores pads on other copper layers and suppresses `direct`, `solid`, and `none` zone connection modes.
Per-pad copper-layer thermal gap/spoke-width overrides and per-pad no-connection/solid/direct suppression are supported in board-aware refill reporting.
PTH-only thermal connection mode (`pth_thermal`) is supported in zone validation and refill reporting; SMD/NPTH pads are excluded.
Applied zone refill now persists thermal spoke start/end/width records through JSON reload; export/render integration of those records remains future work.
KiCad PCB export preserves PTH-only zone connection semantics with `connect_pads thru_hole_only`; resolved spoke record emission into KiCad filled polygons remains deferred.
Qt canvas renders persisted thermal spoke records on visible zone layers, and official demo fixture visibly exercises the path with an applied B.Cu PTH pad.
Qt thermal-spoke rendering preserves persisted spoke width visually.
KiCad PCB export now preserves persisted horizontal/vertical thermal spokes as rectangular filled copper polygons; diagonal spoke records remain unsupported and are omitted.
Agent chat bubbles use content-sized, non-scrolling message browsers so notices and responses render without nested scrollbar artifacts or excess internal padding.

Headless board context owns one loaded project snapshot, reports loader readiness, rejects save-before-load, and tracks explicit unsaved edits for future shared CLI, GUI, and agent use.
The headless board context also supports file-backed load/save with round-trip coverage.
`project validate` is the first CLI consumer of the shared file-backed headless context.
`pcb add-via` is now a mutation consumer; its existing geometry validation and audit trail remain intact.
`pcb set-layer-visibility` is now a shared-context mutation consumer with its existing validation and audit behavior intact.
`pcb set-via` is now a shared-context mutation consumer with its existing validation and audit behavior intact.
`pcb add-track` is now a shared-context mutation consumer with its existing routing validation and audit behavior intact.
`pcb add-track-arc` is now a shared-context mutation consumer with its existing arc validation and audit behavior intact.
`pcb refill-zones` is now a shared-context mutation consumer for persisted contours and thermal spokes.
`pcb add-zone` is now a shared-context mutation consumer with its existing geometry and audit behavior intact.
`pcb add-text` is now a shared-context mutation consumer with its existing geometry and audit behavior intact.
`pcb add-graphic-line` is now a shared-context mutation consumer with its existing geometry and audit behavior intact.
`pcb add-graphic-arc` is now a shared-context mutation consumer with its existing arc geometry and audit behavior intact.
`pcb add-target` is now a shared-context mutation consumer with its existing shape, layer, and audit behavior intact.
`pcb add-pad` is now a shared-context mutation consumer with its existing pad validation, geometry, and audit behavior intact.
`pcb set-pad` is now a shared-context mutation consumer with its existing pad validation, geometry, and audit behavior intact.
`pcb add-barcode` is now a shared-context mutation consumer with its existing barcode validation, geometry, and audit behavior intact.
`pcb add-dimension` is now a shared-context mutation consumer with its existing dimension validation, geometry, and audit behavior intact.
`pcb add-group` is now a shared-context mutation consumer with its existing member validation and audit behavior intact.
`pcb add-route-request` is now a shared-context mutation consumer with its existing endpoint, layer, width, and audit behavior intact.
`pcb set-route-request` is now a shared-context mutation consumer with its existing endpoint, layer, width, update, and audit behavior intact.
`pcb remove-route-request` is now a shared-context mutation consumer with its existing request lookup, removal, and audit behavior intact.
`pcb apply-route-segment` is now a shared-context mutation consumer with its existing routing geometry, request completion, and audit behavior intact.
`pcb apply-route-polyline` is now a shared-context mutation consumer with its existing polyline geometry, request completion, and audit behavior intact.
`pcb set-track` is now a shared-context mutation consumer with its existing track geometry, metadata, and audit behavior intact.
`pcb add-keepout` is now a shared-context mutation consumer with its existing keepout geometry, validation, and audit behavior intact.
`pcb update-teardrops` is now a shared-context mutation consumer with its existing teardrop settings, generation, and audit behavior intact.
`pcb add-placement-region` is now a shared-context mutation consumer with its existing placement-region geometry, validation, and audit behavior intact.
`pcb set-region-kind` is now a shared-context mutation consumer with its existing keepout/placement-region update and audit behavior intact.
`pcb remove-object` is now a shared-context mutation consumer with its existing multi-type removal, modes, result, and audit behavior intact.
`pcb move-object` is now a shared-context mutation consumer with its existing multi-type movement, geometry validation, and audit behavior intact.
`pcb resize-object` is now a shared-context mutation consumer with its existing multi-type resizing, geometry validation, and audit behavior intact.
`pcb place-footprint` now uses the shared project context while preserving its footprint placement, validation, and audit behavior.
`pcb autoplace-footprint` now uses the shared project context while preserving its automatic placement, validation, and audit behavior.
`pcb spread-footprints` now uses the shared project context while preserving multi-component movement, gap handling, result reporting, and audit behavior.
`pcb add-reference-image` now uses the shared project context while preserving reference-image authoring and audit behavior.
`pcb add-table` now uses the shared project context while preserving table authoring and audit behavior.
`clean-graphics`, layer mutations, `set-rules`, `set-outline`, `add-zone`, and `import-ses` now use the shared project context while preserving authoring, validation, and audit behavior.
Schematic mutation commands and `project set-text-variable` now use the shared project context while preserving serialization and audit behavior; CLI coverage exercises the primary schematic primitives.
Agent orchestrator read tools now report live project context; four PCB tool adapters dispatch through the CLI and return captured exit code, stdout, and stderr.
Agents can call registered PCB tools through `agent.tool_call`; requests expose required fields, enforce mutation approval, and return stable JSON-RPC IDs plus real command results.

Agent PCB tool adapters validate required fields before dispatch and return the missing field as a structured error when a request is incomplete.

Agent `pcb.drc` runs the native DRC command and exposes its exit code and captured diagnostics.

Agent JSON-RPC string fields preserve Windows paths and decode common escaped characters.

Agent `pcb.add-zone` authors rectangular copper zones through the native CLI adapter.

Agent `pcb.add-keepout` authors rectangular keepouts through the native CLI adapter.

Agents can author placement regions and request native zone refill through `pcb.add-placement-region` and `pcb.refill-zones`.

Agents can place schematic symbols and author wires/labels through `sch.place-symbol`, `sch.add-wire`, and `sch.add-label`.

`scripts/cli_demo_drc.ps1` provides a user-facing rectifier-board story: construct board, show intentional DRC failure, repair with a valid layer transition, and prove a clean final DRC plus GUI screenshot.

The demo prints plain-language stage narration so viewers can follow engineering intent and outcome without knowing CCad command names.

Agent Settings includes a masked, process-memory-only API-key field and reports `configured_memory_only`; secrets are not persisted or emitted. Network provider execution is not yet enabled.

When the Python agent child is running, entering a provider key sends it through private IPC as `agent.set_provider_secret`; the child stores it only in process environment and emits only redacted provider readiness. This is credential plumbing, not proof of successful provider connectivity; network execution still requires a real provider runtime test.


CCAD_PROVIDER=mock enables deterministic offline chat through the real Python orchestration graph for harness tests; responses are labeled mock and network-free. It is test infrastructure, not a substitute for provider connectivity.


Agent orchestration uses LangGraph; Langfuse callbacks remain opt-in, and LangSmith LangChainTracer is opt-in through LANGCHAIN_TRACING_V2 plus LANGCHAIN_API_KEY. Graph-level callback config covers node/tool runs; no telemetry exporter activates by default.


Real-provider ui.place_via tool path can await matching C++ broker result by call_id and feed result into LangGraph state; mock and dry-run paths stay offline/nonblocking. Timeout, cancellation, and approval-before-execution remain backlog.


LangGraph runs attach content-safe metadata/tags to opt-in Langfuse/LangSmith callbacks; no prompt/context/tool payload or secret is placed in run metadata. Full callback redaction integration test remains backlog.


Agent protocol uses one queue-backed stdin reader; broker waits are bounded by CCAD_BROKER_TIMEOUT_SECONDS and return structured timeout/closed errors instead of hanging indefinitely.


Core ToolBroker enforces equire_approval before any non-readonly executor callback; default mutation request returns pproval_required. Explicit auto-execute mode remains available for controlled tests.


Core approvals support one-shot pproved_tool_name scope: exact matching mutation may execute while all other mutations remain blocked. GUI approval-token wiring remains pending.
AgentPanel approval wiring is now connected to the broker: approval-required calls show the exact tool request, Accept executes only that scoped call and returns its correlated result, while Decline returns `approval_denied` immediately. Real-provider network execution and an external-provider integration test remain pending.
Canceling the same approval now returns correlated `approval_canceled` immediately and clears the retained call, so the broker does not wait for its timeout.
Broker waits preserve unrelated inbound JSON-RPC messages in a deferred queue and return only the matching `tool_result`; main dispatcher processes deferred messages first.
Real `ui.place_via` broker calls use unique per-invocation correlation IDs, so late results from prior attempts cannot satisfy a newer wait.
Real broker mode now returns authoritative results for track and zone mutations, not optimistic dispatch text; offline/mock paths remain deterministic and nonblocking.
Footprint and schematic mutation tools now share unique call IDs and return authoritative broker results in real-provider mode; offline/mock behavior remains deterministic.
Broker protocol now has a pending-call registry: matching results wake exact tool waits, while unknown or unrelated messages remain available to the main dispatcher.
Agent graph invocations now carry explicit thread correlation through LangGraph configurable state; no prompt, tool payload, credential, or durable checkpoint data is added to telemetry.
Agent dependency requirements now include a compatible SQLite checkpoint package; installation is reproducible, but runtime checkpointer wiring remains explicitly pending.
Set `CCAD_AGENT_CHECKPOINT_DB` to an explicit local SQLite path to persist LangGraph thread state across orchestrator restarts. Unset path keeps persistence disabled; no credentials are stored by this feature.
Loading a saved Agent session now propagates its thread ID to the Python orchestrator, aligning GUI session identity with optional SQLite LangGraph checkpoints without sending secrets.
Resume control now queries SQLite-backed LangGraph thread state and reports whether recovery is available; it does not silently replay stale mutations.
GUI Resume now surfaces checkpoint availability and recovered thread/checkpoint metadata instead of silently discarding the Python response.
Checkpointed agent threads now expose protocol-side LangGraph resume execution through `agent.resume_thread` and correlated `tool_result`; GUI reports resumed state.
With `CCAD_AGENT_CHECKPOINT_DB` enabled, mutating agent tools pause as durable LangGraph interrupts and resume from correlated client results; unset DB preserves legacy local wait behavior.
Restart-safe checkpoint behavior is covered by `scripts/test_agent_checkpoint_restart.py`, proving interrupt persistence and same-thread resume across two processes.
Agent run and result status labels have stable GUI object names for semantic automation and visual validation.
Approval lane regression test covers pending, decline, and cancel state transitions with visible user feedback.
Provider adapter initialization failures now surface a redacted error classification in Agent status while preserving secret non-disclosure.
Agent Settings now updates provider readiness after successful adapter initialization, while explicitly distinguishing readiness from a network probe.
MCP support is a guarded stdio subset centered on `ccad_execute`; GUI-map methods use separate local JSON Lines transport.
External MCP hosts can read harness capability/safety context and provider-free workspace state through `ccad_harness_context` and `ccad_workspace_state`.
External MCP hosts can query a running GUI through `scripts/ccad_mcp_gui_bridge.py`; bridge exposes live read-only UI/project context and rejects GUI mutations.
GUI MCP tool metadata declares read-only, non-destructive, closed-world behavior for safer host presentation.
MCP clients can stage a native approval card with `ccad_gui_request_approval`; acceptance remains human-controlled in Agent panel.
Live MCP bridge verification now checks real GUI pipe and reports native approval staging failure instead of claiming success; route harness cleans up its own GUI process.
Sprint 607 adds a visible Agent approval card with GUI-map-addressable request and human decision controls. MCP may stage the card but cannot accept it.
Sprint 608 verifies the live human decision lane: the native Decline action updates the approval status exposed by the GUI map.
Sprint 610 verifies the end-to-end LangGraph chat path with the mock provider; users can enable OpenAI, Anthropic, or Gemini adapters by configuring the corresponding provider and key.
Sprint 611 ensures clearing the API-key field and saving removes the in-memory provider credential.
Sprint 612 adds a Test Provider action for immediate selected-adapter initialization, with redacted readiness feedback and no prompt execution.
Sprint 613 reports approximate context usage from the actual context payload sent with chat instead of a hardcoded number.
Sprint 614 keeps approval-required tool calls pending until a human Approve, Decline, or Cancel decision returns the authoritative result to the orchestrator.
Sprint 615 verifies restart-safe checkpoint continuation for both accepted tool results and explicit approval denial.
Sprint 618 enables agents to type into the native chat composer through `ui.type_text` and its semantic control ID.
Sprint 616 adds a GitHub Actions Python agent gate for mock chat and both checkpoint decision outcomes.
Sprint 619 adds a live harness check that types into the mapped Agent chat editor and verifies the response before exercising board controls.
Sprint 620 adds offline mock-provider Send verification through the real mapped chat button, enabling repeatable GUI roundtrip tests without API keys.
Sprint 621 exposes `label:agent_result` after backend chat responses; live harness verifies response delivery through a fresh GUI-map snapshot.
Sprint 622 hides approval UI until a change request exists, restores readable dark-card controls, and reserves ignored local secret directories for developer-run provider testing.
Sprint 623 makes MCP approval staging reveal the contextual card before Request, and verifies pending-visible/declined-hidden state through the live GUI map.
Sprint 624 gives slash suggestions executable `/workflow`, `/hooks`, `/set`, and `/schedule` forms, semantic popup mapping, and dark hover/selection styling.
Sprint 625 adds visible `Summarize`, `Run DRC`, and `Route` quick replies that insert editable commands without automatic side effects.
### Sprint 627: one-shot approval tokens

Agent-requested project mutations are now scoped to a single approval token as well as a tool name. Replaying an accepted token returns `approval_token_consumed`; accepting a different tool still requires approval. The native panel generates and discards tokens within the approval-card lifecycle. This prevents a stale approval from authorizing a later mutation.
### Sprint 629: BYOK provider selection handoff

The Agent panel's `openai_compatible` and `local_model_server` provider IDs are now understood by the Python orchestrator. Their configured model and base URL are passed to the OpenAI-protocol adapter, and session-only keys are mirrored into the SDK's expected environment variable without being emitted or persisted. A real authenticated request still requires the user's own provider endpoint and credentials.
### Sprint 630: terminal BYOK compatibility

Terminal configuration now works for OpenAI-compatible and local model providers: provider-specific keys are mirrored before adapter construction, provider-specific model variables override stale config, and readiness remains truthful without probing the network. The contract check uses a non-routable endpoint and a test-only key; it performs no API request.
### Sprint 631: local provider tool-call contract

CI now runs a localhost-only OpenAI-compatible provider test. It checks that CCad sends the selected model and registered routing tools to the adapter, receives the supervisor/router responses, and exposes the returned tool call through the agent protocol without external credentials or network access.
### Sprint 632: provider-to-GUI approval status

The offline mock provider and live GUI harness prove approval and via creation. A separate localhost provider-boundary test proves adapter tool schemas. A deterministic combined provider-to-GUI fixture remains open because the current embedded-process test can race before provider startup.
### Sprint 633: visible tool-result acknowledgment

After an approved mutation, the native Agent panel displays `Tool result accepted` or `Tool result rejected`, includes the correlated call ID in activity history, and exposes the state through the semantic status label. This distinguishes approval from successful broker handoff.
### Sprint 634: truthful run telemetry

Chat runs now expose running/completed state, trace ID, span ID, provider identity, and explicit unavailable cost/token fields through the Agent panel. This is local protocol metadata; it does not claim provider billing or token accounting until an adapter supplies authoritative usage.
# Schematic review layout

When a schematic contains multiple symbols with no persisted coordinates, the review canvas separates them into a deterministic display grid. This is display-only; saving or editing the project does not silently rewrite symbol positions.

## Contextual approval card

The Agent panel keeps approval UI hidden while idle, during read-only commands, and during dry-run previews. It appears when a real project-mutating command is classified and remains available until accepted, declined, canceled, or cleared.

## Versioned agent context

Agent requests receive a `ccad_agent_context` version-one envelope. The complete project snapshot is under `project`, while `constraints` explicitly states mutation approval and secret exclusion rules. This gives providers stable context semantics without exposing secrets.

## Intake risk gate

Before planning, IntakeLayer normalizes and scans the goal. Destructive or external-risk requests are recorded as a failed `intake_risk_scan_blocked` task and never reach provider or tool decomposition.

The scan also rejects common instruction-override phrases and credential markers, including `ignore previous instructions`, `api_key`, `password`, and `secret`. It is a conservative boundary check, not a substitute for provider-side prompt-injection defense.

Verification includes accepted safe inspection goals and rejected unsafe examples before decomposition.

## Context revision signaling

Each agent chat request emits an opaque context revision and whether it changed since the prior request. Only digest, size, and presence are reported; project context text is never emitted in this state event.

## Provider retry boundary

Supervisor, router, and librarian provider calls retry at most twice on raised provider errors. Tool execution is outside this retry wrapper, preventing repeated project mutations. Retry events expose only attempt count, exception type, and redaction flags.

If all retries fail, the chat protocol emits failed telemetry and a provider error message, then continues serving later requests. Prompts, credentials, and endpoint details remain excluded.

## Provider setup feedback

Agent Settings shows the exact provider/model targeted by Test Provider and changes the session-only key placeholder when provider selection changes. Test Provider applies the visible provider/model to the live agent process before initializing its in-memory credential. The Agent header reports the model returned by provider state, or Auto when no model is configured. Backend stderr startup/runtime diagnostics appear as bounded redacted chat warnings.

When an optional provider adapter dependency is missing, chat reports the module name and repository requirements install command. Local CCad tools remain usable; credentials and endpoint values are not shown.

Test Provider also reports its transient result directly in Agent Settings. The status is initially `not run`, becomes `ready (network not probed)` for an initialized adapter, or shows a bounded dependency/adapter error. This is session feedback only; Save remains the only way to persist provider settings.

Agent Settings is modeless and remains discoverable through the live UI map. External harnesses can select its category QListWidget rows semantically, then operate provider controls without relying on approximate row coordinates. The Windows robot validates the provider-test action and captures the visible status card.

Provider testing cannot remain indefinitely busy: the status shows `running...` immediately and changes to `no response` after five seconds if no provider-state or backend result arrives. Successful or failed provider-state messages retain precedence.

The live Windows UI robot rejects both `not run` and `running...` as final states, proving the timeout reaches a terminal user-visible result.

Top-level Settings controls are now addressable through `ui.target`/`ui.click` discovery, including provider/model inputs, checkboxes, combos, and category lists. Agent clients can operate the visible dialog without coordinate guessing.

Top-level control clicks now focus line and text editors, open combo boxes, and toggle checkboxes through the same UI-map API. The provider flow uses this surface for truthful button/list interaction; broader control-action coverage remains backlog.

`ui.type_text` also addresses visible Settings dialog editors: `control:modelInput`, `control:apiKeyInput`, `control:projectNameInput`, `control:projectPathInput`, `control:followUpInput`, and `control:customInstructionsText`. Hidden duplicate widgets from inactive tabs are skipped. API keys remain transient and are never returned in responses.

The physical UI robot proves `control:apiKeyInput` entry with a dummy value, then runs the provider test and waits for a terminal result. Real Gemini network testing still requires a user-provided environment key.

Provider requirements pin Gemini adapter `2.0.10` and bound LangSmith below `0.3`, matching the current LangGraph/Core 0.2/0.3 runtime. This avoids a resolver selecting Core 1.x. Gemini adapter migration to Google's newer SDK remains open.

The native Choose Footprint/Choose Symbol dialogs resolve local library roots from `CCAD_LIBRARY_CACHE`, current working directory, executable directory, and parent directories. Set `CCAD_LIBRARY_CACHE` for an explicit catalog location. The repository's local ignored `library-cache` contains converted footprints, symbols, and `catalog.json`; it is not committed as product source.

General Agent Settings now drives the live PCB canvas: metric grid choices update
spacing and Hidden toggles visibility through ReviewWindow, while provider/model and
other settings remain addressable through the UI-map harness.

Chooser status text shows how many local items loaded and the resolved cache path. A search with no matches gives an explicit clear-search message; a missing cache gives the exact environment-variable remedy.

## CI speed and syntax gate

The CI workflow compiles agent and test Python sources before protocol tests and invokes CMake with parallel workers on all build jobs. This catches syntax regressions early and reduces idle build time without weakening CTest.
### Agent Settings model and general preferences

Agent Settings exposes provider-aware editable model choices, including current OpenAI, Anthropic, and Gemini families plus custom entries for compatible/local endpoints. General Settings exposes theme, canvas grid, autosave, and session restoration preferences. Saved configuration is reloaded into the dialog across project, memory, personalisation, and general fields. Provider/API secrets remain session-only and are never persisted.

The physical Settings harness now selects Cerebras and its `gpt-oss-120b` model by
semantic row, selects a metric grid, enters a session-only dummy key, and reaches the
provider-test terminal state. Save no longer crashes the live modeless dialog because
the dialog receives its owning AgentPanel and unregisters callbacks on close. Reopen
verification of the selected grid is deliberately not claimed yet; the Save action's
compact UI-map coverage after tab switching and the resulting persistence path remain
backlog items.

Malformed editable model values are guarded: known preset-plus-suffix values are
replaced by the active provider's valid preset on load or provider switch, while
custom model IDs remain supported. The UI bridge can operate Save buttons in top-level
modeless Settings dialogs. Full visual harness proof is current; physical Save/reopen
persistence proof remains pending the intermittent Windows screenshot capture issue.

Backend persistence is covered independently by `scripts/test_agent_config_persistence.py`,
which verifies provider, model, and metric grid values across an orchestrator restart.

Windows Settings key reveal uses the current account name with `LogonUserW` and
clears temporary password buffers. The physical UI robot selects Cerebras using
the current provider catalog order; live GUI verification remains user-paused.

UI-map `ui.click` accepts exact combo `value` or visible `text` selectors in
addition to row selection. Provider/model harnesses should use these stable
selectors so catalog ordering may change safely.

The discoverable `ui.click` and `ui.double_click` schemas document `row`,
`value`, and `text` selection fields, keeping external agent harnesses aligned
with the runtime protocol.

Settings Test Provider now returns a terminal redacted provider state when a
key is absent or adapter initialization fails, so the UI reports the cause
without waiting for its fallback timeout or exposing credential data.
Bundled-venv subprocess coverage exercises this no-key terminal state without
making a provider request.

Settings saves provider, model, metric grid, security, memory, and personalisation
preferences through one config message; no second grid-only write can overwrite
the saved state during dialog dismissal.
The no-GUI persistence contract also checks security, memory, chat-mode,
context-usage, and personality values after restart.

Curated provider model selectors are read-only by default; OpenAI-compatible,
OpenRouter, and local-model providers explicitly enable custom model IDs.

The Anthropic curated selector now starts with the same `claude-opus-5` preset
used by the provider runtime default. Stale `claude-fable-5-1` selection is no
longer offered.

Model-catalog callbacks are lifetime-safe: responses arriving during Settings
dialog teardown are ignored when either provider or model control is absent.
Settings teardown also unregisters the model-catalog callback, preventing a
late response from calling a lambda that captured the destroyed dialog.

Agent chat uses one selectable, continuous QTextBrowser stream for user,
assistant, and tool text. The former per-message bubble fallback is removed, so
normal and degraded rendering cannot diverge into separate message boxes.
Unused user/tool bubble CSS is also removed; checklist styling remains separate
from the conversation stream by design.
ReviewWindow's duplicate legacy selectors are removed as well, preventing global
stylesheet drift from reintroducing the old boxed-message visual language.
Transcript deduplication is warning-specific; repeated user and assistant text
remains selectable and visible.
Activity events now appear inline in that same transcript, while their structured
records remain available to agents through workspace state. No per-event chat
checklist widgets are created.
The stream is directly hosted in the Agent layout; an obsolete nested scroll
area/container was removed, preventing double scrollbars and wrapper gaps.
The Python orchestrator also emits separate `backend_state` readiness, so
harnesses can distinguish a live local runtime from provider adapter readiness.
Native Agent workspace state now carries that distinction as `backend_ready`
and `backend_provider_initialized`, without exposing telemetry chips or secrets.
`test_backend_state_contract.py` proves the startup JSON event through a mock
subprocess without network access.
Python intake guardrails now emit redacted `intake_state` and block known
prompt-injection or inline-secret requests before provider/tool execution.
`test_agent_intake_guard_contract.py` proves safe acceptance, category-only
blocking, no tool call, and no secret echo without network access.
Native CLI MCP stdio consumes `notifications/initialized` without emitting an
error response, preserving standard notification framing for following
requests.

Provider readiness messages distinguish a missing API key from an adapter
failure, so Agent Settings gives an actionable configuration message without
claiming that a configured provider is broken.

The Cerebras startup path also recognizes LangChain's missing-key
initialization error, reports it as `missing_api_key`, and keeps the provider
dropdown aligned with the official curated model snapshot after rebuilding the
Qt GUI.

Settings restore also rejects stale non-custom model IDs and selects the first
current provider model, preventing obsolete catalog values from reappearing.

API key entry is colocated with provider testing and clearing in API & Providers;
Configuration no longer hides credential input on an unrelated tab.
The page explains that the active value is held in memory while persistent
storage uses the operating-system credential vault.

The read-only `agent.mcp_status` method exposes normalized MCP entries and
explicitly reports `runtime: "not_started"` and `process_execution: false`.
Harnesses can therefore distinguish saved configuration from a connected MCP
runtime. The Settings page also states this boundary directly to users.
The MCP section queries the same status method and displays the current runtime,
configured-server count, and process-execution policy.

External harnesses can query read-only `agent.mcp_plan` to inspect normalized
server commands and the explicit approval boundary. It never launches a
process; supervised MCP execution remains a separate runtime feature.

Provider validation is selection-scoped. `agent.test_provider` restores the
active adapter first and then emits one `provider_test_result` event for the
provider selected in Settings. The dialog therefore cannot mistake the restored
provider's ambient status for the validation result. This action initializes an
adapter only and reports `network_access: "not_probed"`; it never sends a prompt
or spends provider quota.

`Refresh models` is explicit for every supported hosted provider and does not
run on application startup, while opening Settings, or when sending an agent
prompt. OpenAI, Anthropic, Google Gemini, and OpenRouter use their documented
credentialed model-list APIs. Cerebras uses its documented public
`/public/v1/models` catalog, so discovery works without reading the saved
inference key or spending inference quota. Endpoint-backed local and generic
OpenAI-compatible servers remain manual because CCad cannot discover their
authentication or model-list contract safely.

Anthropic and Gemini refreshes now follow each provider's pagination contract,
with a defensive finite page cap and repeated-cursor stop. A user therefore
receives the complete model catalog allowed by their key rather than a silent
first-page subset. This work remains refresh-only; it never performs inference.

Provider test failures now retain their structured backend category in the
Settings page and translate it into actionable text, distinguishing missing or
rejected keys, quota limits, missing models, timeouts, and missing dependencies.

Live chat failures now preserve a narrower safe cause: authentication,
permission denial, missing model, payment/billing required, rate limiting,
timeout, dependency, or generic availability. HTTP 402 is always classified as
payment/billing required rather than rate limiting or consumed quota. If a
provider SDK exposes an HTTP status, only that status is shown; response
bodies, prompt data, endpoints, and keys remain hidden. These terminal
failures do not receive automatic retries.

Sprint 972 preserves exhausted quota/credits separately from transient rate
limits, carries bounded `Retry-After` through the provider error path, and shows
actionable quota/billing guidance in Settings. The local adapter validation is
explicitly no-network and reports `ready (network not probed)`; a separate
live-connection action is visibly labeled as quota-using. The mapped UI proof
clicked only local validation and confirmed its result without a provider call.

The Agent panel's internal provider-status selector is populated with every
supported provider and switches before a session secret causes an activity
entry. A Cerebras key therefore reports Cerebras local status rather than the
previous hidden OpenAI default. `gui_agent_panel` covers this selection path
without a provider request.

### MCP server settings

Agent Settings now exposes the persisted `mcp_servers` configuration as an
editable table rather than displaying fabricated placeholder servers. Users
can add or remove entries and edit name, command, whitespace-separated
arguments, port, and enabled state. Empty rows are ignored on save, arguments
are serialized as a JSON array, and values are restored from `agent.get_config`.
The Python config boundary normalizes entries, drops incomplete rows, clamps
invalid ports to zero, and never launches a server while settings are saved.

`scripts/test_provider_cerebras_secret.py` verifies Cerebras API-key injection
through the private child-process session path, with configured state exposed
but the key value redacted and no provider request sent.

The Qt Agent Settings regression selects Cerebras and Qwen, then switches to
Gemini, proving model IDs remain exact and stale provider text is not carried
into the next dropdown selection.

The official UI-map target sequence now opens the modeless Agent Settings dialog
and verifies provider, model, masked API-key, and MCP table controls after the
dialog appears. This extends visual validation beyond the dock itself without
making a network request or changing credentials.

Pad JSON loading accepts the canonical `drill_nm` field emitted by CCad's own
serializer and restores it into the padstack drill geometry. This preserves
through-hole pad drill data across a save/load cycle.

Provider model refresh now matches the Settings label: Cerebras uses its public
`/public/v1/models` endpoint only after an explicit Refresh models action,
while startup continues using the first-party snapshot. The public catalog does
not need the saved inference key. HTTP failures, malformed catalogs, and
timeouts return redacted provider catalog errors without exposing credentials
or response bodies.

The Cerebras settings hint now states this exact split: an offline startup
snapshot keeps opening Settings quota-safe, while the explicit Refresh models
button fetches the current authenticated catalog. It no longer describes the
provider catalog as permanently offline.

Provider-details text now reserves enough wrapped height in the compact native
Settings dialog, so its key and refresh guidance remains readable instead of
being vertically clipped.

Catalog parsers reject malformed provider payloads as `invalid_catalog_shape`
instead of propagating an internal parsing exception. Controlled local-response
tests verify the normalized model IDs and authentication headers for every
first-party Refresh models integration.

The API & Providers page now has an explicit `Set key` action. It stores the
selected provider key in Windows Credential Manager, sends that key only over
the private child-process IPC, and activates the selected provider and model in
the running agent. It does not write a key to project files, agent JSON, or
logs.

`Set key` reports a dedicated terminal activation result after the child process
has attempted adapter initialization. The Settings status no longer claims a
provider is active optimistically; it distinguishes a ready adapter from a
missing dependency or other local setup failure without exposing the key.

The Python orchestration module no longer initializes a provider merely because
a test or external harness imports it. Provider initialization and the backend
ready event occur only when the agent process is launched.

Agent provider credentials can also be managed without exposing them to command
history: `ccad agent credential status <provider>`, `set <provider>`, and
`remove <provider>`. On Windows these commands use the same Credential Manager
entry as Agent Settings (`CCad/provider/<provider>`). `set` reads the value from
an invisible interactive prompt, `status` reports only whether an entry exists,
and `remove` deletes that entry. No key is accepted on a command line, emitted
in JSON, written to project configuration, or recorded in logs.

The Settings live-provider check is deliberately a separate quota-bearing
operation. It makes one minimal request, does not retry or run tools, restores
the active adapter afterwards, and displays either a redacted success preview
or a safe terminal failure category. The physical GUI harness waits for that
terminal callback rather than aborting the request while it is still sending.

### Runtime Langfuse configuration

Agent Settings now has an Observability page with enablement, masked Langfuse
keys, base URL, environment, service name, runtime status, test export, and
credential removal. Keys use Windows Credential Manager; status never treats
stored credentials as a successful export. The UI-map screenshot method captures
the active settings or review dialog when it is open.

Sprint 951 makes the running adapter observable rather than configuration-only.
The Python runtime rebuilds Langfuse when its non-secret configuration or vault
credentials change, attaches the current LangChain callback to graph calls, and
creates typed observations for context assembly, routing, library work, native
tool dispatch, model generation, and each agent turn. The agent CLI accepts
`langfuse_public` and `langfuse_secret` as vault-only credential targets. UI-map
output redacts every password-style or secret-named line edit, including masked
Langfuse controls, so inspection tools cannot recover stored values. A real
remote trace retrieval remains an opt-in verification step.

### Typed Agent project context and staged PCB previews

`project.context` and `project.state` expose a bounded typed board/schematic
snapshot with binary payloads removed. Vias, board text, schematic wires, and
labels now save through typed project data. Route, zone, keepout, and graphic
line proposals use copied-project staging and native before/after rendering;
unsupported operations return an explicit unavailable reason.

### Sprint 965 - GUI method discovery and focus feedback

The GUI's `agent.methods` and `agent.method_schema` responses now include typed
Python JSON-RPC control descriptors while keeping them non-model-callable.
Qt's native dotted focus primitive is suppressed; themed focus and selected
states remain visible, and PCB selection stays attached to the selected
geometry. Verification: Qt Release build and CTest 93/93; 30 repeated
AgentRunner tests; Pyright reported zero diagnostics for the changed Python
catalog; cmake-language-server initialize handshake passed; scoped live
GUI-map/mouse/keyboard screenshots and empty stdout/stderr were inspected in
`artifacts/screenshots/sprint965-focus-registry/`. Preferences-menu actions
remain unmapped and are not claimed complete.

### Sprint 966 - provider failures and development Langfuse delivery

The Python runtime walks wrapped provider exceptions to retain safe quota,
rate-limit, payment, authentication, model, timeout, connection, and dependency
categories. Chat output uses category-level troubleshooting guidance and HTTP
status when available; it does not expose raw SDK exception text or claim that
no tool ran. Development turns flush Langfuse export after both success and
provider failure, then report the trace ID, exported span count, and result to
stderr and the Settings status event; production keeps background batching.
Focused no-network provider, Langfuse-runtime, trace-privacy, and UI source
contracts pass. Qt Release build and full CTest passed 93/93. A live GUI-map
turn returned `trace test`; its exact trace ID was fetched from Langfuse and
verified with 16 spans. Development Settings showed the fresh result, and all
screenshots plus empty stdout/stderr were inspected in
`artifacts/screenshots/sprint966-live-turn/`. Telemetry-only Pyright reports
zero diagnostics; six known orchestrator diagnostics remain. References: Langfuse's current
[LangChain integration](https://langfuse.com/integrations/frameworks/langchain),
[SDK flushing](https://langfuse.com/docs/observability/sdk/instrumentation), and
[export-stage masking](https://langfuse.com/docs/observability/features/masking).

### Sprint 967 - full request and memory accounting

Before each provider generation, CCad now measures the assembled system
instructions, project context, retrieved memories, conversation messages, and
live bound tool schemas. Reports contain sizes and safe provenance only, never
the corresponding content. They label token counts as estimates, flag
multimodal payloads the text estimator cannot count, and show the selected
model's context limit as unavailable unless authoritative metadata is present.
When estimated input crosses `CCAD_AGENT_LARGE_CONTEXT_TOKENS` (default 4096),
the chat explains the breakdown, enabled/loaded/retrieved memory counts,
retrieval rank/keyword overlap, namespace scopes, truncation state, and the
current explicit-memory-capture limitation. Setting `CCAD_TRACE_DEBUG=1` also
writes the structured content-free report to stderr and each generation span
receives the same safe component counts. Conversation history is identified as
separate provider messages, not falsely attributed to the context envelope.
Six Python context contracts pass and Pyright reports zero diagnostics for the
changed Python modules. The Qt/MinGW Release build and full CTest pass 94/94.
Feature-specific rendered chat proof is still pending; the GUI checklist is
deliberately not marked complete because triggering the report through the
normal live path would submit a real provider request.

## Durable Agent memory activation and persistence

Personalisation memory toggles persist before runtime activation. LTM and episodic activation validates or initializes their durable store; storage failure unloads runtime content and reports an unknown persistent count rather than pretending the tier is empty. Disabling preserves durable entries and conversation checkpoints. Reset is separately confirmed. Validation: Qt MinGW Release build, CTest 104/104, changed-module Pyright (0 diagnostics), offline orchestrator persistence contracts, and the mapped GUI create/cancel-reset/reopen-settings flow; all 42 run screenshots and stdout/stderr were inspected. Durable memory semantic compaction is still not implemented.
# Agent runtime features

## Sprint 979 protocol and provider-catalog boundaries

Provider model-catalog refreshes use dedicated bounded HTTP clients in `src/ccad_agent/model_catalog.py`; the orchestrator retains its established JSON-RPC facade and safe failure classification. The full human-message turn is handled outside the JSON-RPC dispatcher loop, keeping protocol continuation separate from turn execution and satisfying Pyright's configured complexity analysis without suppressions. CTest contract `agent_protocol_handler_boundaries` verifies the source boundary; catalog parser contracts use controlled responses without contacting providers. Qt MinGW Release, full CTest (112/112), changed-module Pyright, and a provider-free seven-action GUI-map conversation-persistence/`/clear` scenario passed. Four meaningful screenshots and captured stdout/stderr were inspected.

## Sprint 978 Langfuse turn hierarchy and memory deduplication

The orchestrator opens one `agent.turn` trace before it assembles project context. The current thread ID is propagated as the Langfuse session, and context assembly, memory retrieval, context packaging, and LangGraph invocation become children of that same trace. Terminal model turns close the root before flush/readback; local or early-exit turns close and flush at the next protocol boundary. Disabled tracing remains a truthful no-export state. The offline ancestry contract uses the installed Langfuse SDK and OpenTelemetry in-memory exporter to prove root/child trace IDs and parent relationships without sending a provider request or telemetry over the network.

Automatic memory selection now compares candidates with bounded recent messages, retrieved TurnRecords, and the actual persisted thread recap. The recap digest is included in cache identity, and targeted memory refresh reuses those comparison sources. Regression coverage: `scripts/test_agent_turn_trace_hierarchy.py` and `scripts/test_context_broker.py`. This does not claim a real remote trace was received; the opt-in account readback and other Langfuse delivery gates remain open.

## Agent local context preview

Type `/context` in the Agent chat to inspect how the current bounded request would be assembled, or `/context <draft>` to include a hypothetical user message and rank memory for that intent. The command uses the real project snapshot, enabled memory namespaces, conversation messages, system instructions, and bound model tool schemas, then renders safe counts, token estimates, ranking, omissions, the selected provider/model, and model-limit availability in chat. Provider/model identity follows the same persisted settings and environment override resolution as provider startup; a context limit remains unavailable unless authoritative selected-model metadata supplies one. The preview is local-only: no provider request, tool execution, history mutation, or draft persistence occurs. Automatic requests also receive the existing count-only explanation when estimated input crosses `CCAD_AGENT_LARGE_CONTEXT_TOKENS` (default 4096). Token counts are estimates rather than provider tokenizer measurements.
## Sprint 991 semantic settings and verification evidence

Personalisation > Memory offers opt-in loopback Ollama semantic retrieval with editable endpoint/model fields and live readiness. Exact lexical retrieval remains available when Ollama or its requested installed model is unavailable. Settings use a readable dark scroll surface with enough initial height for runtime state.

`scripts/verify_sprint.ps1` runs the pinned preflight, Qt Release build, full CTest, and the named app-owned UI-map scenario in `config/gui_interaction_plans/sprint991-semantic-memory.json`. It stores a hash-bound evidence manifest together with that run's logs, mapped-action report, and distinct screenshots. The hook and CI checker verify artifact integrity, not visual correctness; every screenshot and log must still be inspected. These checks use local processes and do not contact a provider.

## Sprint 977 deterministic context and memory retrieval

Before model reasoning, the Python orchestrator extracts bounded design/task signals and builds a thread/revision-scoped `TurnContext`. Enabled memory tiers are scope-filtered and ranked deterministically; selected records are limited by a configurable estimated-token budget (`CCAD_AGENT_MEMORY_TOKENS`, default 1000) and eight-entry ceiling. The provider envelope includes those authorized records, a bounded title-only Memory Summary, and an allowlisted availability/count manifest. `ccad_search_memory` is a real read-only local LangChain tool that lets the model request additional relevant records during the same turn, preserves prior valid matches, re-applies the budget, and reports an incremented context version. Sprint 988 adds the active project's durable LTM namespace and safe manifest count; project entity indexing, semantic memory retrieval, provider-tokenizer accounting, and a unified Langfuse root trace remain separate work. Diagnostics expose counts and digests, not prompt or memory text. Offline contracts: `scripts/test_context_broker.py`, `scripts/test_context_budget.py`, `scripts/test_agent_context_contract.py`, `scripts/test_agent_native_tool_catalog_protocol.py`, and `scripts/test_agent_memory_search_tool.py`.
