# CCad Agent production TODO

Check a box only after implementation and its required evidence exist.

## Update protocol and active slice

Update this file in the same commit as each implementation slice.

### Sprint 949 checklist

- [x] Native catalog transport before provider activation.
- [x] JSON schema to provider-safe `StructuredTool` conversion.
- [x] ToolNode and provider binding rebuild from native catalog.
- [x] Native method ID and authoritative broker-result preservation.
- [x] Catalog IPC contract, focused CTest, and scoped UI-map validation.
- [x] Qt Release build and full CTest gate (92/92).
- [x] Bounded real-provider catalog tool call and broker result.

## Provider, context, and memory

- [x] Restore persisted provider, model, and OS-vault credential before first chat turn.
- [x] Verify every provider/model integration against official documentation and classify failures safely.
- [x] Map explicit provider catalog refresh failures to safe authentication, permission, payment, rate-limit, timeout, connection, and invalid-response categories.
- [x] Resolve the remaining Pyright complexity diagnostic in the provider/orchestration module without suppressing analysis.
- [x] Build bounded context from project, PCB, schematic, selection, coordinates, layers, nets, rules, libraries, tool state, conversation, and memories.
- [x] Report safe metadata for the exact context package sent on each turn.
- [x] Implement STM, project-long-term, and episodic memory: retrieval, scope, ranking, update, deletion, reset, expiry, compaction, and secret redaction.
- [x] Publish the exhaustive memory/context lifecycle report.

## Typed CCad tool surface

- [ ] Generate one typed registry from real CLI, core transactions, UI-map, DRC/ERC, library/catalog, schematic, PCB, routing, export, inspection, memory, and evidence surfaces.
  - [x] Publish native GUI methods and real CLI command descriptors in one discovery response; mark only broker-backed GUI methods callable.
  - [x] Derive CLI command effect metadata through the shared CCad command policy classifier.
  - [x] Verify discovery contains the exact CLI help inventory and Python binds only callable GUI broker methods.
  - [ ] Register Python memory/runtime controls and all kernel transaction capabilities in the unified registry.
- [x] Give each supported command schemas, examples, validation, side-effect class, context needs, and result shape.
- [ ] Add guarded real CLI execution with structured stdout, stderr, artifacts, and safe failure mapping.
- [ ] Add deterministic unit-aware calculator and coordinate-transform tool.
- [ ] Add constrained project-scoped Python computation with explicit artifacts, no inherited secrets or shell interpolation, bounded execution, and approval for persistence.
- [x] Add screenshot/evidence capture with viewport, layer, and selection metadata.
- [ ] Add UI-map inspection and mapped-action tools; never fixed-coordinate scripts for normal operation.
- [ ] Add exact PCB/schematic state inspection and typed placement/edit transactions with units, snap, net, geometry, rules, and validation.
- [x] Let models compose real tools dynamically.

## Safety and approvals

- [ ] Keep read-only, calculation, screenshot, and dry-run calls immediate.
- [ ] Gate every persistent, destructive, external, CLI, Python-write, export, or process action before execution.
- [ ] Bind approval to immutable typed action plan and project/context revision; expire stale approvals.
- [ ] Support approve, reject, revise, cancel, undo, and post-action verification through transaction/audit path.
- [ ] Block command injection, unrestricted filesystem access, secret exposure, and unbounded subprocess/network execution.
- [x] Remove fabricated runner success when no executor exists; report `task_executor_unavailable` with zero tool/project action.

## Real orchestration validation

- [ ] Prove a real model reports exact tools and exact context received.
- [ ] Prove read-only calls do not open approval UI.
- [ ] Prove approved disposable-board PCB mutation: inspect, calculate, propose, visual diff, approve, transact, verify, DRC, screenshot, undo.
- [ ] Prove equivalent schematic, catalog, DRC/ERC, export, guarded CLI, calculator, Python, screenshot, and UI-map flows.
- [ ] Prove failure, cancellation, schema-invalid, stale-approval, provider/process error, and resume paths cannot mutate incorrectly.

## Langfuse observability

- [ ] Add masked Langfuse public key, secret key, base URL, enable, and export-status controls.
- [x] Store tracing credentials separately in Windows Credential Manager; never in args, project/config files, logs, prompts, screenshots, or git.
- [x] Initialize Langfuse before first traced graph call and attach official callback to every invocation.
- [ ] Trace context, memory, routing, generations, plans, approvals, tool execution, transactions, verification, failures, latency, tokens, model/provider.
- [ ] Use conversation/thread sessions, safe tags, and deliberate redaction.
- [ ] Run opt-in real trace and inspect hierarchy, cost/token metrics, tool spans, approval path, and redaction.

## Autorouter

- [ ] Audit KiCad autorouter architecture, dependencies, data model, and licensing.
- [ ] Define native CCad routing interface for geometry, layers, nets, constraints, keepouts, clearance, vias, widths, and DRC.
- [ ] Port or reimplement only legally compatible components behind the kernel.
- [ ] Add deterministic previews, scoring, visual diff, approval application, rollback, and DRC verification.
- [ ] Add reproducible route-quality coverage for clearances, continuity, keepouts, layers/vias, and supported differential pairs.

## Delivery gates

- [x] Install and verify global C++/Qt, Python, and CMake language servers; expose the real Qt/MinGW compile database to clangd.
- [x] Bound agent-orchestrator completion tests so stalled callbacks fail explicitly instead of hanging the CTest gate.
- [ ] Write contract tests before behavior changes.
- [x] Build Qt and run targeted/full CTest.
- [x] Run scoped UI-map/mouse-keyboard validation; inspect screenshots and logs.
- [x] Run bounded real provider tests.
- [ ] Run redacted repository and staged-diff secret scans before every commit.
- [ ] Update architecture, feature, CLI, methodology, provider, memory, tracing, autorouter, backlog, and progress docs in the same commit.
- [ ] Commit only verified source/tests/docs; never keys, vault data, local config, logs, screenshots, generated boards, or unrelated user files.

## UI truthfulness and parity

- [ ] Replace text-only proposal review with typed staged PCB/schematic before/after renders and real change lists.
- [ ] Make proposal review a scrollable chat popout with viewport controls, object focus, and DRC/ERC delta.
- [ ] Add editable annotations, reviewer comments, structured revision scope, revise/reject/cancel, and one approval boundary.
- [ ] Render no preview for unsupported actions; report the exact unavailable/staging reason without fabricated geometry.
- [x] Fix live Agent Settings opening and modeless dialog discovery through the UI map.
- [x] Make quick DRC execute authoritative DRC or remove the chip; do not merely insert `/drc`.
- [x] Map `/drc` to authoritative `project.drc`, return its diagnostics, and never leave a chat turn at “Running DRC checks…”.
- [ ] Preserve 429/quota/rate-limit categories from provider SDK exceptions; never relabel them `provider_unavailable`.
- [x] Remove the stale Gemini adapter notice that says no provider was contacted after a configured provider has initialized or completed a real call.
- [ ] Bind one immutable proposal/call ID to one approval and one execution; reject duplicate/replayed approval results.
- [ ] Never narrate a failed UI gesture or transaction as a completed board change; surface the authoritative failure reason.
- [ ] Make DRC marker visibility explicit and map each canvas marker to its diagnostic instead of silently drawing a center marker on every errored object.
- [x] Commit supported agent zone, keepout, and graphic mutations through typed board data, persist/redraw them, and clear stale interaction anchors.
- [x] Reject agent graphic endpoints outside the board outline instead of committing a DRC-error marker.
- [ ] Make collapsed Agent dock restorable through a mapped action and release its unused dock space.
- [x] Expose Layers/Objects child tabs as stable UI-map targets.
- [ ] Repair Layers / Objects dock topology: prevent Agent dock from crushing the Appearance panel; preserve user dock geometry and minimum usable widths.
- [ ] Add adaptive right-dock behavior: side-by-side on wide windows, tabified Layers/Agent on constrained widths.
- [ ] Remove native dotted focus rectangles application-wide while preserving solid keyboard-focus indication.
- [ ] Implement real collapsible conversation-history sidebar with pinned/recent sessions.
- [ ] Implement New Chat as a real session/thread operation and bind it to LangGraph thread identity.
- [ ] Bind current chat title to durable session metadata.
- [ ] Replace dead `action:agent_menu` with actual history/back behavior.
- [ ] Replace all temporary/stand-in Agent icons with semantic, theme-aware CCad icons.
- [ ] Remove `Summarize`, `Run DRC`, and `Route` quick chips unless they remain intentional product actions.
- [ ] Replace text-path attachment insertion with structured attachment transport.
- [ ] Hide voice control until real STT capture/transcription exists.
- [ ] Replace fake context-refresh chat message with real context state inspection.
- [ ] Implement circular context-usage indicator using actual/estimated token usage and selected-model context limit.
- [ ] Make `show_context_usage` control visibility only; do not confuse it with context refresh or settings.
- [ ] Add context-breakdown popover for conversation/project/memory/tool-schema contribution.

## Marketplace

- [ ] Remove "Live" naming until catalogue contents are actually live/dynamic.
- [ ] Replace hardcoded pseudo-plugin catalogue with one typed marketplace registry.
- [ ] Implement real search filtering.
- [ ] Implement working category navigation for workflows, prompts, hooks, tools, and plugins.
- [ ] Add typed Marketplace cards with Install/Remove/Enable/Open-details actions.
- [x] Remove false "activated and hooked into context" claims when installation has not occurred.
- [ ] Implement one real marketplace install/uninstall backend path; do not mutate config independently of tool execution.
- [ ] Clear Marketplace callbacks safely on dialog destruction.
- [ ] Persist installed/enabled state through the canonical `plugins` / `workflows` schema.

## Slash command registry

- [ ] Replace GUI hardcoded slash list plus separate `/commands` and `/help` strings with one authoritative command registry.
- [ ] Provide syntax and plain-language description for every slash command in autocomplete.
- [ ] Add `/memory` to slash autocomplete.
- [ ] Make `/help` an alias/view over the same registry as `/commands`.
- [ ] Validate `/workflow use:` against installed workflows.
- [ ] Either implement `chaining_phase` semantics or remove `/workflow chaining phase:`.
- [ ] Add hook list/remove/validation/persistence instead of free-form string append only.
- [ ] Make `/set provider:model` use the same canonical provider/model state as Settings.
- [ ] Preserve `/cc`/`/compact` as bounded context compaction but implement semantic compaction before claiming summarization.
- [ ] Replace fake `/schedule` string queue with a real persistent scheduler before exposing the command.
- [ ] Make `/marketplace` open Marketplace and `/marketplace install <id>` execute real install.
- [x] Fix `/drc` to invoke authoritative read-only `project.drc`.
- [ ] Scope `/route` wording to actual routing capability; do not claim complete autorouting until autorouter exists.
- [ ] Keep `/place` behind real typed placement tools and normal approval flow.
- [ ] Keep `/design` but remove fabricated fallback pins on generation failure.
- [ ] Make `/clear` clear the current UI/thread context consistently, without deleting persistent memories.
- [ ] Implement a real `/settings` native action.
- [x] Implement `/revise` through the pending proposal/thread path or stop generating it from the UI.

## Langfuse observability

- [ ] Use Langfuse as the only user-facing observability backend for the current product slice; keep generic OTel internal.
- [x] Add Settings -> Observability -> Langfuse.
- [x] Add enable state, public key, secret key, base URL, environment, test/status controls.
- [x] Store Langfuse secret key in the OS credential vault, never config/project/logs/prompts.
- [x] Update LangChain integration to the current `langfuse.langchain.CallbackHandler` API.
- [x] Pin a tested Langfuse SDK major/minor range instead of unconstrained `langfuse>=2.30.0`.
- [x] Replace import-time-only tracer/callback initialization with reconfigurable `LangfuseRuntime`.
- [ ] Wire `agent.langfuse_set_config`, `agent.langfuse_set_secret`, `agent.langfuse_status`, and `agent.langfuse_test` through the live Settings test/status controls.
- [ ] Use durable Agent thread ID as Langfuse session ID.
- [ ] Trace agent run, prompt assembly, model calls, routing, tool calls, approvals, transactions, verification, DRC/ERC, retries, cancellation, and failures.
- [ ] Record provider/model/token/cost/latency when available.
- [ ] Implement Langfuse `mask_otel_spans` redaction before export.
- [ ] Default prompt contents, raw tool arguments, screenshots, and project contents to OFF.
- [x] Flush Langfuse on explicit test, application shutdown, and bounded process termination.
- [ ] Prove one opt-in real Langfuse trace contains the expected hierarchy and no secrets.

## Audit intake — 2026-09-22

- [ ] Re-run CI on the current branch head and record the exact workflow SHA/results.
- [ ] Eliminate Linux `-Werror` unused-function regressions in Agent Settings.
- [ ] Keep provider retry/error classification defined before every call site and covered by the Python CI contract.
- [ ] Remove compatibility `action.drc`, `action.route`, and `action.place` success payloads unless they delegate to authoritative operations.
- [ ] Collapse duplicate C++/Python orchestration ownership: Python plans; C++ brokers, policies, transactions, verification, and undo.
- [ ] Remove empty C++ ContextBuilder loaders or implement their documented behavior.
- [ ] Remove static C++ run/session identities and provider-disabled pseudo-tasks from production paths.
- [ ] Make UI status/model/mode/permission chips authoritative or remove them.
- [ ] Make context token counts model-specific, tokenizer-backed where possible, and visibly estimated otherwise.
- [ ] Add structured attachment content to human-message IPC, context policy, and provider invocation.
- [ ] Add capture, transcription, edit, and send flow before exposing voice input.
- [ ] Migrate plugin/workflow configuration to one canonical schema with backward-compatible reads.
- [ ] Restrict Marketplace catalog entries to verified installable/runtime-loadable capabilities.
- [ ] Give Marketplace item records IDs, type, source, version, installed/enabled state, permissions, and capabilities.
- [ ] Implement one canonical Marketplace loader, install/remove/enable path, and runtime-load verification.
- [ ] Replace all command-list copies with a single registry consumed by autocomplete, `/commands`, and `/help`.
- [ ] Disable or remove commands without an executing backend: schedule, chaining phase, Marketplace install, and unsupported route/place modes.
- [ ] Make `/settings` invoke the native Settings action through the broker.
- [ ] Define one `/clear` operation across visible transcript, thread/checkpoint runtime, and preserved memory tiers.
- [ ] Implement per-tier STM, LTM, and episodic capture/retrieval namespaces, ranking, expiry, compaction, reset, and redaction.
- [ ] Ensure memory toggle-off unloads only process state; separate reset/delete remains confirmed and durable.
- [ ] Bind every proposal to base revision, immutable typed change set, preview artifact, and single-use approval token.
- [ ] Stage every supported mutation in a project copy before preview; reject unsupported staging truthfully.
- [ ] Render typed PCB and schematic before/after overlays, object lists, coordinate annotations, and DRC/ERC deltas.
- [ ] Focus/zoom actual editor objects from change-list and review annotations.
- [ ] Run post-commit verification, DRC/ERC, screenshot evidence, and undo/targeted revert through the same transaction audit path.
- [ ] Implement stale proposal, cancellation, rejection, duplicate/replay, and failed-gesture protection contracts.
- [ ] Persist and restore dock geometry without overriding user layout after startup.
- [ ] Add semantic, theme-consistent icons for all Agent controls; remove Unicode and stand-in icon mixes.
- [ ] Persist durable chat history metadata, pinned/recent grouping, current title, and thread/session resume.
- [ ] Use Langfuse as the only user-facing tracing backend while retaining OTel solely as transport/instrumentation.
- [ ] Store and remove Langfuse public/secret credentials through the OS vault with no config, prompt, event, screenshot, or git leakage.
- [ ] Expose canonical `agent.langfuse_*` config/status/test methods or document stable aliases across GUI, CLI, and Python IPC.
- [ ] Add spans for context, memory, supervisor/router/librarian, model generation, plan, tool, approval, transaction, verification, DRC/ERC, retry, cancellation, and failure.
- [ ] Emit safe provider/model, token, cost, latency, workflow, thread/session, revision, and outcome metadata.
- [ ] Apply centralized redaction before every Langfuse/OTel export and keep prompt, tool arguments, screenshots, and design contents opt-in.
- [ ] Flush tracing on explicit test, clean shutdown, and bounded child-process termination.
- [ ] Fetch and inspect an opt-in trace from Langfuse, including hierarchy, observations, tokens/costs, tool spans, approval path, and redaction.
- [ ] Group future changes into cohesive user-visible slices: implementation, tests, docs, TODO, and verification evidence in one commit.

## Atomic implementation breakdown — SPA parity and native Qt adaptation

This section decomposes the existing production TODO into bounded implementation slices for GPT-5.6 Luna.

The interactive SPA is a behavioral and visual reference only. Production implementation remains native Qt using the existing CCad design system, C++ project kernel, Python/LangGraph orchestrator, UI-map, transaction system, and Agent IPC.

Do not mark a parent feature complete because its widget exists. A feature is complete only when the GUI, runtime state, backend path, persistence where required, tests, and visual/runtime evidence all agree.

### Reference UI contract

- [ ] Add the final Agent SPA to the repository as a product/reference artifact without adding a web runtime dependency.
- [ ] Document that the SPA is not production frontend code and must not be embedded into CCad.
- [ ] Map every SPA surface to its intended native Qt window, dock, dialog, widget, or editor overlay.
- [ ] Map every interactive SPA control to a stable semantic UI-map ID before implementing it in Qt.
- [ ] Map every interactive control to its authoritative backend owner: Qt, Python/LangGraph, C++ core, transaction system, provider adapter, memory runtime, Marketplace registry, or Langfuse runtime.
- [ ] Treat the supplied handwritten designs and final SPA as the target product UI where they conflict with older Agent dashboard/status-chip designs.
- [ ] Remove obsolete screenshots/mockups from active product documentation or clearly label them historical.
- [ ] Keep a single UI parity document listing each reference screen, corresponding Qt surface, backend contract, implementation state, and validation evidence.

## Native Agent shell

### Agent dock structure

- [ ] Reduce the visible Agent dock to the product surfaces actually required by the reference design.
- [ ] Remove permanent provider/run/session/trace/permission status-dashboard strips from normal chat UI.
- [ ] Keep internal telemetry/run/session/provider state in models/runtime state instead of hidden QLabel/QWidget containers.
- [ ] Keep developer diagnostics accessible through an explicit developer/debug surface rather than normal user UI.
- [ ] Remove legacy run-queue presentation when no actual runtime event stream backs it.
- [ ] Remove static/fabricated plan steps such as generic “Read context / Collect evidence / Apply changes”.
- [ ] Render runtime activity only from actual orchestration/tool events.
- [ ] Ensure no widget is retained solely because an old UI test expects it; update tests to match the product contract instead.
- [ ] Give the Agent dock a stable minimum width that does not crush the Layers/Objects panel.
- [ ] Make Agent dock hide/show/collapse preserve the active conversation and runtime thread.
- [ ] Restore the Agent dock through a stable `action:show_agent` UI-map action after it is hidden.
- [ ] Persist only presentation state such as dock visibility, width, and sidebar collapse state through native Qt window-state persistence.

### Agent header

- [ ] Implement semantic history/back icon rather than hamburger/menu stand-in.
- [ ] Implement current conversation title bound to durable session metadata.
- [ ] Implement Templates action as a real template/prompt picker.
- [ ] Make template selection insert content into the composer without automatically sending it.
- [ ] Keep Settings opening modeless and discoverable through the UI map.
- [ ] Implement a real close/hide Agent action that does not destroy thread state.
- [ ] Remove obsolete header controls not present in the approved design.
- [ ] Use the same icon system and visual language as the rest of CCad rather than mixed Unicode, Material-like inline SVG, and placeholder glyphs.

## Conversation history and New Chat

### History sidebar

- [ ] Implement a native collapsible conversation-history sidebar inside the Agent surface.
- [ ] Expanded sidebar must show New Chat, search, pinned sessions, and recent sessions.
- [ ] Collapsed sidebar must reduce to a narrow icon rail instead of consuming the full width.
- [ ] Persist expanded/collapsed state and last usable width as UI state.
- [ ] Do not persist conversation content inside arbitrary Qt widget settings.
- [ ] Implement search over actual durable Agent session metadata.
- [ ] Implement session row selection using actual session/thread/checkpoint identifiers.
- [ ] Implement pinned/unpinned session state.
- [ ] Implement rename session.
- [ ] Implement delete session with confirmation and clear storage semantics.
- [ ] Implement “Show all” or equivalent full history browser if history exceeds the compact sidebar.
- [ ] Show actual last-used timestamps rather than hardcoded example dates.
- [ ] Show actual project association where available.

### New Chat

- [ ] Add stable `action:agent_new_chat`.
- [ ] Generate a new durable Agent session ID.
- [ ] Generate/bind a new LangGraph thread ID.
- [ ] Send the new thread ID through the existing thread-binding IPC path.
- [ ] Create clean per-thread conversation state.
- [ ] Clear only the visible/new-thread conversation state.
- [ ] Preserve provider/model settings.
- [ ] Preserve project state.
- [ ] Preserve enabled durable memory tiers according to their scopes.
- [ ] Preserve previous session records and checkpoints.
- [ ] Add the new session to history immediately.
- [ ] Update the chat title to the new session title.
- [ ] Generate/update a useful conversation title after the first user turn without overwriting an explicit user rename.
- [ ] Prove switching between two sessions restores the correct thread/checkpoint state without mixing messages or tool results.

## Layers / Objects / Agent dock topology

- [ ] Keep `Layers / Objects` usable when the Agent panel is open.
- [ ] Give Layers/Objects a nonzero minimum usable width.
- [ ] Keep Agent dock minimum width consistent with the approved reference.
- [ ] Use side-by-side right docks only when the main window has enough horizontal space.
- [ ] Automatically tabify Layers/Objects and Agent at constrained widths instead of squeezing either panel into a sliver.
- [ ] Choose a tested responsive threshold based on Qt logical pixels rather than one unverified hardcoded physical-screen assumption.
- [ ] Preserve the user’s manually rearranged dock topology after startup.
- [ ] Do not repeatedly call `resizeDocks()` in ways that overwrite user-adjusted dock sizes.
- [ ] Persist/restore dock topology with the native Qt main-window state mechanism.
- [ ] Expose `Layers`, `Objects`, and `Nets` child tabs as stable UI-map targets.
- [ ] Verify opening Agent does not hide, clip, or collapse Layers unexpectedly.
- [ ] Verify resizing the window across the responsive threshold does not orphan a dock or destroy its state.

## Focus, selection, and native theme consistency

- [ ] Remove the unwanted native dotted focus rectangle from interactive UI controls.
- [ ] Suppress the native `PE_FrameFocusRect` centrally rather than patching individual widgets inconsistently.
- [ ] Preserve keyboard accessibility with an explicit solid focus border/highlight.
- [ ] Apply the same focus language to buttons, list items, text inputs, combo boxes, side navigation, Marketplace cards, and review controls.
- [ ] Verify mouse interaction does not leave dotted rectangles on clicked widgets.
- [ ] Verify keyboard Tab navigation still visibly indicates focus.
- [ ] Use CCad/KiCad-derived theme colours instead of introducing an unrelated SaaS visual language.
- [ ] Use existing application font/fallbacks; do not require bundled web fonts.
- [ ] Use restrained borders, separators, corners, and status colours consistent with the main editor.
- [ ] Avoid unnecessary pills/status chips.
- [ ] Avoid decorative cards when a normal desktop form/list/control is sufficient.

## Agent icon system

- [ ] Define one native semantic icon registry for Agent UI.
- [ ] Add proper history icon.
- [ ] Add proper new-chat icon.
- [ ] Add proper collapse/expand icons.
- [ ] Add proper templates icon.
- [ ] Add proper Marketplace icon.
- [ ] Add proper settings icon.
- [ ] Add proper attachment icon.
- [ ] Add proper context-usage icon/ring treatment.
- [ ] Add proper microphone icon.
- [ ] Add proper Send/Stop icon.
- [ ] Add proper Approve icon.
- [ ] Add proper Revise icon.
- [ ] Add proper Reject icon.
- [ ] Add proper Undo/Revert icon.
- [ ] Add proper Locate/Focus icon for visual review.
- [ ] Add proper annotation/comment/highlight/rectangle/measure icons.
- [ ] Remove icon comments containing “stand-in”.
- [ ] Remove Unicode glyphs used as permanent production icons where a native icon should exist.
- [ ] Use theme-aware icon colours/states for enabled, disabled, hover, pressed, and selected states.
- [ ] Verify every Agent icon at normal and high-DPI scaling.

## Chat timeline and activity rendering

- [ ] Replace monolithic chat HTML/string rendering with typed timeline items where practical.
- [ ] Define typed user-message item.
- [ ] Define typed assistant-message item.
- [ ] Define typed activity-group item.
- [ ] Define typed proposal/change-review item.
- [ ] Define typed verification-result item.
- [ ] Define typed error/notice item.
- [ ] Define typed evidence item.
- [ ] Render raw tool names/arguments only in an explicit expandable developer/details view.
- [ ] Render user-facing activity summaries from actual tool/orchestration events.
- [ ] Never synthesize “Ran DRC”, “Updated board”, “Applied route”, or similar activity unless authoritative backend evidence exists.
- [ ] Keep streaming assistant text distinct from tool activity.
- [ ] Correlate tool call, tool result, proposal, approval, transaction, and verification by stable IDs.
- [ ] Ensure failed tool calls remain visually failures and cannot be converted into completion language.
- [ ] Group related tool operations into a compact activity group instead of flooding chat with one card per low-level call.
- [ ] Make evidence cards show actual DRC/ERC counts, screenshots, artifacts, or result summaries where present.

## Composer

### Message input

- [ ] Keep one multiline composer field.
- [ ] Support Enter/Shift+Enter behavior consistently with the approved interaction model.
- [ ] Add generation Stop action only while a turn is actively running.
- [ ] Cancel a running model/tool flow through the real runtime cancellation path.
- [ ] Preserve unsent composer text when opening Settings, Marketplace, or visual review.
- [ ] Preserve unsent composer text when history is collapsed/expanded.
- [ ] Disable Send only when there is genuinely nothing valid to submit.

### Attachments

- [ ] Replace `[Attached: <path>]` prompt insertion with structured attachment objects.
- [ ] Define attachment ID.
- [ ] Define display name.
- [ ] Define canonical path/resource handle.
- [ ] Define MIME/type.
- [ ] Define attachment kind.
- [ ] Define byte size.
- [ ] Define allowed context policy.
- [ ] Add attachment chips/rows above the composer.
- [ ] Add attachment remove action before sending.
- [ ] Support CCad/KiCad PCB files.
- [ ] Support CCad/KiCad schematic files.
- [ ] Support text.
- [ ] Support Markdown.
- [ ] Support JSON.
- [ ] Support supported image formats.
- [ ] Extend `human_message` IPC with structured attachments.
- [ ] Ensure binary content is not dumped into logs.
- [ ] Ensure attachment paths are not exported to Langfuse unless explicitly safe/redacted.
- [ ] Add provider/context handling for text attachments.
- [ ] Add project-context handling for PCB/schematic attachments.
- [ ] Add image attachment handling only for providers/runtime paths that genuinely support it.
- [ ] Reject unsupported file types truthfully.

### Voice / STT

- [ ] Keep voice control hidden or disabled until a real STT backend exists.
- [ ] Remove `[STT Recording...]` text insertion.
- [ ] Define recording state.
- [ ] Define cancel-recording action.
- [ ] Define transcription state.
- [ ] Insert final transcript into composer rather than auto-send.
- [ ] Allow user to edit transcript before sending.
- [ ] Surface transcription errors without fabricating text.
- [ ] Add STT provider/model configuration only after a real implementation exists.

## Context usage indicator

- [ ] Replace settings/gear stand-in with a real circular context usage indicator.
- [ ] `show_context_usage` must control visibility only.
- [ ] Clicking the context indicator must never append a fake Agent chat message.
- [ ] Derive selected model context limit from authoritative provider/model metadata when available.
- [ ] Do not hardcode `128k` globally.
- [ ] Use provider tokenizer/token accounting when available.
- [ ] Mark estimated token counts with `~`.
- [ ] Define context usage percentage from current assembled context tokens / selected model context limit.
- [ ] Add context breakdown popover.
- [ ] Report conversation token contribution.
- [ ] Report project/PCB/schematic contribution.
- [ ] Report active selection contribution.
- [ ] Report memory contribution by enabled tier.
- [ ] Report attachments contribution.
- [ ] Report tool-schema contribution.
- [ ] Report system/custom-instruction contribution where safe.
- [ ] Show compaction state when `/cc` has compacted older context.
- [ ] Keep context breakdown metadata safe and secret-free.
- [ ] Update indicator after provider/model changes.
- [ ] Update indicator after attachments change.
- [ ] Update indicator after memory tier enable/disable.
- [ ] Update indicator after compaction.
- [ ] Update indicator after significant tool/context refresh.

## Settings information architecture

- [ ] Keep Settings as one modeless native Qt window.
- [ ] Keep one left navigation list.
- [ ] Keep one stacked content area.
- [ ] Keep one canonical Save Preferences path.
- [ ] Keep Cancel from mutating persisted settings.
- [ ] Ensure Settings initializes from actual `agent.get_config`/runtime state rather than stale widget defaults.
- [ ] Ensure asynchronous Settings callbacks are disconnected safely on dialog destruction.
- [ ] Keep categories in the agreed order:
  - [ ] General.
  - [ ] Configuration.
  - [ ] Personalisation.
  - [ ] MCP.
  - [ ] Providers & Models.
  - [ ] Observability.
  - [ ] Plugins.
  - [ ] Workflows/Hooks.
- [ ] Remove duplicated controls when one preference appears in more than one page without a deliberate reason.
- [ ] Make every Settings control correspond to one canonical config/runtime field.
- [ ] Read back persisted config after save and verify the round trip before reflecting success.

## Settings — General

- [ ] Keep theme control bound to actual theme state.
- [ ] Keep grid spacing control bound to actual editor grid state.
- [ ] Keep autosave preference truthful.
- [ ] Keep restore-session preference truthful.
- [ ] Add chat review behavior: Inline vs Detached.
- [ ] Add context usage visibility toggle.
- [ ] Add include-current-project-context toggle if context assembly supports it.
- [ ] Add include-current-selection toggle if context assembly supports it.
- [ ] Add Marketplace button visibility toggle only if product still requires user-configurable visibility.
- [ ] Add voice button visibility only after STT capability exists.
- [ ] Add confirmation-before-persistent-change setting bound to approval policy.
- [ ] Do not add settings that merely alter labels while backend behavior remains unchanged.

## Settings — resolved Configuration

- [ ] Rename any misleading `config.toml` preview to `Resolved configuration` while JSON remains the persisted format.
- [ ] Generate the preview from the effective canonical configuration rather than reconstructing it independently in Qt.
- [ ] Exclude provider secrets.
- [ ] Exclude Langfuse secrets.
- [ ] Exclude vault material.
- [ ] Exclude ephemeral approval tokens.
- [ ] Show provider/model.
- [ ] Show sandbox/approval policy.
- [ ] Show project configuration.
- [ ] Show memory tier enablement.
- [ ] Show personalisation.
- [ ] Show MCP server configuration without secrets.
- [ ] Show installed/enabled plugins.
- [ ] Show installed/enabled workflows.
- [ ] Show hook configuration.
- [ ] Show observability non-secret configuration.
- [ ] Add Copy action.
- [ ] Keep preview read-only unless an explicitly validated advanced raw-config editor is later implemented.

## Settings — Personalisation

### Personality and custom instructions

- [ ] Bind Agent Personality to actual system-prompt construction.
- [ ] Bind Custom Instructions to actual system-prompt construction.
- [ ] Verify both are present in the exact context/prompt metadata without exposing them to unsafe traces by default.
- [ ] Add Edit personality only if personality definitions are genuinely editable.
- [ ] Remove decorative personality controls with no runtime effect.

### Memory tier controls

- [ ] Move STM, LTM, and episodic enablement to the canonical Personalisation memory section.
- [ ] Render each memory tier as an independent checkbox/toggle.
- [ ] Define STM as current goal/task working memory.
- [ ] Define LTM as current conversation/thread durable memory.
- [ ] Define episodic memory as cross-conversation/project experiences.
- [ ] Enabling a tier must create/open its backing store/namespace if required.
- [ ] Enabling a tier must load relevant entries into runtime state.
- [ ] Enabling a tier must permit capture/update for that tier.
- [ ] Enabling a tier must permit retrieval from that tier.
- [ ] Disabling a tier must stop new capture for that tier.
- [ ] Disabling a tier must stop retrieval/context injection for that tier.
- [ ] Disabling a tier must unload its process/runtime cache immediately.
- [ ] Disabling a tier must not silently delete durable records.
- [ ] Add `Manage memories`.
- [ ] Add explicit `Reset/Delete memories` separately from enable/disable.
- [ ] Require confirmation before destructive reset/delete.
- [ ] Report enabled state, loaded runtime count, and persistent count truthfully where practical.
- [ ] Ensure disabling LTM does not delete ordinary chat transcript/checkpoint state.
- [ ] Ensure disabling episodic memory does not erase project/conversation state.
- [ ] Implement per-tier ranking/retrieval and not one shared flat store presented as three different systems.
- [ ] Implement secret redaction before memory persistence.
- [ ] Implement expiry policy.
- [ ] Implement compaction policy.
- [ ] Implement duplicate/near-duplicate handling.
- [ ] Implement per-scope deletion.
- [ ] Implement complete reset.
- [ ] Add IPC/runtime state event for memory tier enable/disable instead of waiting only for application restart.

## Providers & Models

- [ ] Keep one canonical provider ID shared by Settings, composer selector, `/set`, config, and runtime.
- [ ] Keep one canonical model ID shared by Settings, composer selector, `/set`, config, and runtime.
- [ ] Remove duplicate GUI-only provider/model state.
- [ ] Populate provider list only with adapters that are genuinely supported/configurable.
- [ ] Keep OpenAI adapter truthful.
- [ ] Keep Anthropic adapter truthful.
- [ ] Keep Gemini adapter truthful.
- [ ] Keep OpenRouter adapter truthful.
- [ ] Keep Cerebras adapter truthful.
- [ ] Keep Ollama/local adapter truthful.
- [ ] Keep OpenAI-compatible adapter truthful.
- [ ] Do not add Hugging Face, Kimi, DeepSeek, or another provider merely as a dropdown string without a working adapter/compatible endpoint contract.
- [ ] Implement provider-specific model catalogue retrieval where the provider offers it.
- [ ] Cache successful model catalogues.
- [ ] Keep custom model ID support for compatible/local endpoints where discovery is unavailable.
- [ ] Do not hardcode model metadata as authoritative.
- [ ] Show context window only from provider/catalogue data or clearly labelled curated metadata.
- [ ] Show pricing only from authoritative/current provider metadata.
- [ ] Show tool-calling capability where known.
- [ ] Show text/input/output modality where known.
- [ ] Do not invent parameter counts.
- [ ] Keep `Refresh models` explicit rather than silently performing network access.
- [ ] Persist selected provider/model after successful save.
- [ ] Restore selected provider/model before the first Agent turn.
- [ ] Use OS-vault/session credentials for providers.
- [ ] Never persist provider secrets in `agent_config.json`.
- [ ] Test provider without mutating persisted provider/model settings unless user saves them.
- [ ] Report authentication, permission, model-not-found, credit/quota, rate-limit, connection, timeout, and provider-unavailable categories distinctly.
- [ ] Preserve provider SDK error category/status when safe.
- [ ] Never include response bodies containing secrets in GUI error messages or traces.

## Langfuse and internal OTel instrumentation

### User-facing Langfuse page

- [ ] Keep Langfuse as the only user-facing observability product for this slice.
- [ ] Keep generic OpenTelemetry instrumentation internal unless an advanced developer mode is explicitly added later.
- [ ] Add/retain `Enable Langfuse tracing`.
- [ ] Add/retain public key field.
- [ ] Add/retain secret key field.
- [ ] Add/retain base URL.
- [ ] Add/retain environment.
- [ ] Add/retain service name.
- [ ] Add real test connection/export action.
- [ ] Show separate states for configured, enabled, exporter initialized, and last test.
- [ ] Never display `Connected` merely because credentials were saved.
- [ ] Store public/secret credential material according to the agreed vault policy.
- [ ] Never write secret key into normal config.
- [ ] Never export Langfuse credentials to Langfuse itself.

### Langfuse runtime

- [ ] Keep observability runtime reconfigurable after Settings changes.
- [ ] Flush/shutdown previous Langfuse client before replacing configuration.
- [ ] Use current supported Langfuse SDK APIs.
- [ ] Use official LangChain/LangGraph callback integration.
- [ ] Attach callback to every intended graph/model invocation.
- [ ] Use durable Agent thread ID as Langfuse session ID.
- [ ] Record safe provider/model metadata.
- [ ] Record token usage when returned by provider.
- [ ] Record cost only from reliable pricing/usage information.
- [ ] Record latency.
- [ ] Trace context assembly.
- [ ] Trace memory retrieval.
- [ ] Trace supervisor/router/librarian nodes.
- [ ] Trace model generations.
- [ ] Trace tool calls.
- [ ] Trace approval wait/decision.
- [ ] Trace staged transaction generation.
- [ ] Trace transaction commit.
- [ ] Trace DRC/ERC verification.
- [ ] Trace retry.
- [ ] Trace cancellation.
- [ ] Trace failure.
- [ ] Flush on explicit test.
- [ ] Flush on normal Agent-process shutdown.
- [ ] Flush on bounded application shutdown.
- [ ] Do not swallow exporter shutdown errors silently in validation/debug mode.

### OTel internal instrumentation

- [ ] Keep OpenTelemetry as the instrumentation/transport layer underneath Langfuse where required.
- [ ] Keep generic OTel endpoint/header configuration out of normal Settings while Langfuse is the selected product.
- [ ] Do not show a second competing “OTel tracing” plugin in Marketplace if tracing is already an integrated Langfuse capability.
- [ ] Centralize trace attribute redaction before export.
- [ ] Redact API keys.
- [ ] Redact authorization headers.
- [ ] Redact passwords.
- [ ] Redact provider secrets.
- [ ] Redact Langfuse secret key.
- [ ] Redact sensitive filesystem paths according to policy.
- [ ] Keep prompt contents opt-in.
- [ ] Keep raw tool arguments opt-in.
- [ ] Keep screenshots opt-in.
- [ ] Keep raw board/schematic contents opt-in.
- [ ] Add one real opt-in end-to-end Langfuse trace validation and inspect it manually.

## MCP settings

- [ ] Replace stub MCP Settings rows with typed MCP server records.
- [ ] Define server ID.
- [ ] Define display name.
- [ ] Define enabled/configured state.
- [ ] Define transport.
- [ ] Define command/path for stdio.
- [ ] Define argument list.
- [ ] Define working directory where needed.
- [ ] Define host for network transports.
- [ ] Define port where applicable.
- [ ] Add Add Server.
- [ ] Add Edit Server.
- [ ] Add Remove Server.
- [ ] Add Enable/Disable Server.
- [ ] Persist canonical `mcp_servers`.
- [ ] Normalize persisted MCP schema on load.
- [ ] Distinguish `Configured` from `Connected/Ready`.
- [ ] Do not show a green enabled/connected status unless actual initialization succeeded.
- [ ] Reuse the existing CCad MCP bridge rather than creating another native-control protocol.
- [ ] Apply normal tool/approval policy to MCP-provided actions.
- [ ] Never allow an MCP server to bypass CCad mutation approval.

## Marketplace native redesign

### Catalogue model

- [ ] Remove “Live” from Marketplace title until catalogue contents are genuinely dynamically discovered.
- [ ] Define one typed Marketplace item schema.
- [ ] Add item ID.
- [ ] Add item type.
- [ ] Add name.
- [ ] Add description.
- [ ] Add version.
- [ ] Add source/provider.
- [ ] Add installed state.
- [ ] Add enabled state.
- [ ] Add capabilities.
- [ ] Add required permissions.
- [ ] Add compatibility requirements.
- [ ] Add actual runtime availability.
- [ ] Separate workflow items.
- [ ] Separate prompt items.
- [ ] Separate hook items.
- [ ] Separate tool items.
- [ ] Separate plugin items.

### Catalogue source

- [ ] Remove hardcoded pseudo-plugin records presented as real Marketplace availability.
- [ ] Implement one canonical catalogue provider.
- [ ] Do not claim AutoRouter, KiCad Sync, Freerouting Hook, OTel plugin, or other item is installable unless a real installation/runtime load path exists.
- [ ] Do not represent integrated core functionality as an installable plugin unless it genuinely is one.
- [ ] Make Settings Plugins/Workflows and Marketplace read from the same registry.
- [ ] Remove duplicate `installed_plugins` / `plugins` schema.
- [ ] Remove duplicate `active_workflows` / `workflows` schema.
- [ ] Add backward-compatible migration reads for old keys.
- [ ] Persist only canonical keys after migration.

### Marketplace UI

- [ ] Implement search.
- [ ] Implement All category.
- [ ] Implement Workflows category.
- [ ] Implement Prompts category.
- [ ] Implement Hooks category.
- [ ] Implement Tools category.
- [ ] Implement Plugins category.
- [ ] Show actual source/version/install state.
- [ ] Implement details view.
- [ ] Implement real Install only where supported.
- [ ] Implement real Remove only where supported.
- [ ] Implement real Enable/Disable only where runtime loading exists.
- [ ] Show `Unavailable` instead of a working-looking Install button for unsupported items.
- [ ] Clear asynchronous Marketplace callback on dialog destruction.
- [ ] Prevent callbacks from targeting a destroyed dialog.
- [ ] Verify installed/enabled state survives restart.
- [ ] Verify removing an item removes its actual runtime capability.
- [ ] Never report “activated and hooked into context” unless runtime evidence verifies it.

## Workflows and hooks

### Workflows

- [ ] Define one canonical installed-workflow registry.
- [ ] Define one canonical active-workflow state.
- [ ] Validate `/workflow use:` against installed workflows.
- [ ] Remove arbitrary free-form activation of nonexistent workflow names.
- [ ] Define workflow phases explicitly where supported.
- [ ] Implement chaining phase semantics before exposing the command.
- [ ] Remove chaining-phase controls/commands if no runtime behavior consumes them.
- [ ] Make workflow settings modify the same state consumed by LangGraph.
- [ ] Make Marketplace workflow installation update the same canonical workflow registry.
- [ ] Add workflow enable/disable state.
- [ ] Add workflow details.
- [ ] Add workflow compatibility/required-tools metadata.

### Hooks

- [ ] Keep actual supported hook lifecycle points explicit.
- [ ] Support post-prompt.
- [ ] Support pre-tool-call.
- [ ] Support post-tool-call.
- [ ] Support pre-exit/end.
- [ ] Support pre-node/post-node only when their contract is intentionally exposed.
- [ ] Replace free-form `active_hooks.append(string)` behavior with typed hook records.
- [ ] Add hook ID.
- [ ] Add hook name.
- [ ] Add trigger.
- [ ] Add enabled state.
- [ ] Add prompt/action body.
- [ ] Add tool/node filter.
- [ ] Add workflow filter where useful.
- [ ] Add persistence.
- [ ] Add list.
- [ ] Add edit.
- [ ] Add remove.
- [ ] Add enable/disable.
- [ ] Validate hook targets.
- [ ] Execute configured hook at its actual runtime lifecycle point.
- [ ] Prevent hooks from bypassing tool policy/approval.
- [ ] Trace hook execution safely in Langfuse.

## Canonical slash command registry

- [ ] Create one authoritative slash-command registry in the orchestration/runtime layer.
- [ ] Expose registry to Qt through IPC.
- [ ] Make composer autocomplete consume this registry.
- [ ] Make `/commands` render this registry.
- [ ] Make `/help` render/filter this same registry.
- [ ] Define command name.
- [ ] Define syntax.
- [ ] Define plain-language description.
- [ ] Define category.
- [ ] Define availability.
- [ ] Define required capability.
- [ ] Define whether the command is read-only, runtime-only, or can lead to mutation.
- [ ] Hide/disable unavailable commands rather than pretending they work.

### Slash command definitions

- [ ] `/commands` — show the canonical available command catalogue with syntax, description, and availability.
- [ ] `/help [command]` — display help from the same canonical registry; do not maintain a second hardcoded help list.
- [ ] `/set <provider>:<model>` — change the active provider and model using the same canonical state as Settings and the composer model selector.
- [ ] `/cc` — compact older conversation/context while retaining recent turns, pinned constraints, useful summaries, tool state, and revision identity.
- [ ] `/compact` — alias for `/cc`.
- [ ] `/memory list [scope:<scope>]` — list stored memory records in the requested enabled/persistent scope.
- [ ] `/memory add ...` — add a memory through the canonical typed memory manager with scope, redaction, ranking metadata, and persistence.
- [ ] `/memory update <id> ...` — update one persistent memory record through the canonical memory manager.
- [ ] `/memory delete <id>` — delete one explicit memory record.
- [ ] `/memory clear <scope|all>` — destructively clear the explicit scope only after confirmation where appropriate; this is different from toggling a memory tier off.
- [ ] `/workflow use: <name>` — activate an installed validated workflow.
- [ ] `/workflow chaining phase: <phase>` — configure the supported chaining phase only after the runtime consumes the value; otherwise remove this command.
- [ ] `/workflow chaining state: <true|false>` — enable/disable supported workflow chaining using the canonical workflow runtime.
- [ ] `/hooks` — list actual configured hooks and their enable state.
- [ ] `/hooks add ...` — add a typed hook only if command-based hook editing is intentionally supported.
- [ ] `/hooks remove <id>` — remove a real configured hook.
- [ ] `/schedule ...` — create/manage persisted scheduled Agent jobs only after a real scheduler exists; otherwise hide the command.
- [ ] `/marketplace` — open the native Marketplace window.
- [ ] `/marketplace install <id>` — install a real installable Marketplace item through the same canonical installation backend as the GUI.
- [ ] `/drc` — run authoritative read-only `project.drc`, return actual diagnostics, and require no mutation approval.
- [ ] `/route` — enter/request routing through currently supported typed routing tools; wording must not imply full autorouting until the autorouter exists.
- [ ] `/place` — enter/request placement through typed placement tools and normal mutation approval.
- [ ] `/design` — open/use the real component-design path; generation failure must remain a failure and must never fabricate VCC/GND/IN/OUT pins.
- [ ] `/explain` — explain the current design/selection from actual assembled context without mutating the project.
- [ ] `/clear` — clear current thread/chat runtime consistently while preserving persistent memories unless explicitly deleted.
- [ ] `/settings` — invoke the real native Settings action.
- [ ] `/revise` — revise the current pending proposal with structured feedback in the same Agent thread and approval lifecycle.
- [ ] Add command-specific tests for every command advertised as available.
- [ ] Remove commands from autocomplete until their executing backend passes its contract test.

## Scheduled Agent jobs

- [ ] Do not expose `/schedule` as complete while it only appends strings to an in-memory list.
- [ ] Define typed scheduled-job schema.
- [ ] Add job ID.
- [ ] Add enabled state.
- [ ] Add prompt.
- [ ] Add schedule/recurrence.
- [ ] Add next-run time.
- [ ] Add project/session scope.
- [ ] Add creation/update timestamps.
- [ ] Persist jobs.
- [ ] Reload jobs after application restart.
- [ ] Add disable.
- [ ] Add delete.
- [ ] Add run-now where safe.
- [ ] Ensure scheduled jobs use the normal Agent context/orchestration path.
- [ ] Ensure scheduled persistent mutations still require normal approval unless a future explicitly designed unattended policy exists.
- [ ] Ensure scheduled jobs cannot inherit unrestricted secrets/subprocess permissions.

## Proposal and approval data model

- [ ] Define one `PendingAgentChange`/equivalent typed proposal model.
- [ ] Include immutable proposal/change-set ID.
- [ ] Include Agent thread ID.
- [ ] Include correlated tool call ID(s).
- [ ] Include tool names.
- [ ] Include exact validated typed arguments.
- [ ] Include base project revision.
- [ ] Include staged project revision.
- [ ] Include `ProjectDiff`.
- [ ] Include preview transaction.
- [ ] Include affected object IDs.
- [ ] Include affected nets.
- [ ] Include affected layers.
- [ ] Include verification evidence.
- [ ] Include annotations/revision feedback.
- [ ] Include proposal creation timestamp.
- [ ] Include approval token/state.
- [ ] Keep proposal immutable after it is shown; revision must create a new proposal/version.
- [ ] Bind one approval decision to one exact proposal/call ID.
- [ ] Make approval token single-use.
- [ ] Reject replayed approval.
- [ ] Reject approval for a different tool/action.
- [ ] Expire proposal when base project revision changes.
- [ ] Regenerate preview after stale-state detection.
- [ ] Never apply a proposal that was not the one visually reviewed.

## Staged mutation engine

- [ ] Stage mutating Agent operations against a project copy.
- [ ] Never modify `project_cache_` merely to create a preview.
- [ ] Reuse native CCad CAD operation code for staged and live execution.
- [ ] Run validation against staged state.
- [ ] Compute `diffProjects(before, staged)`.
- [ ] Build transaction from the real diff.
- [ ] Generate typed change list from the real diff.
- [ ] Return explicit `preview_unavailable` for unsupported mutation types.
- [ ] Return exact staging failure reason.
- [ ] Do not manufacture before/after geometry.
- [ ] Add staged preview support incrementally for each supported mutation.
- [ ] Start with route-track preview if it is the only genuinely supported preview.
- [ ] Add via preview.
- [ ] Add footprint placement/move preview.
- [ ] Add schematic symbol placement/move preview.
- [ ] Add wire preview.
- [x] Add zone preview.
- [x] Add keepout preview.
- [ ] Add supported property/edit preview.
- [ ] Do not mark proposal-review parity complete until PCB and schematic pathways both have real staged state.

## Visual PCB/schematic diff review

### Review window

- [ ] Replace text-only “BEFORE / AFTER” proposal browser with native EDA visual review.
- [ ] Support PCB review tab.
- [ ] Support Schematic review tab.
- [ ] Support structured Change List tab.
- [ ] Use actual staged `Project` state for proposed rendering.
- [ ] Use actual live/base project for old rendering.
- [ ] Never render arbitrary fake board thumbnails as authoritative review.

### Editor focus and zoom

- [ ] Clicking `View details` switches to the correct PCB/Schematic editor.
- [ ] Locate all affected object IDs.
- [ ] Compute affected CAD-space bounding box.
- [ ] Center editor on affected region.
- [ ] Zoom to a useful review level.
- [ ] Highlight selected diff object.
- [ ] Clicking a change-list item focuses the corresponding real editor object.
- [ ] Clicking a locate/crosshair action focuses the corresponding object.
- [ ] Preserve board/schematic coordinates rather than screenshot pixel coordinates.

### Old/new overlay

- [ ] Render old tracks/wires/objects using muted versions of their actual theme/layer colours.
- [ ] Render proposed objects using normal active theme/layer colours.
- [ ] Avoid hardcoded generic green/red when it conflicts with actual layer colours.
- [ ] Show deleted geometry as muted/ghosted old geometry.
- [ ] Show added geometry as proposed geometry.
- [ ] Show moved objects at old and new positions.
- [ ] Show route replacement as old route ghost plus proposed route.
- [ ] Show via additions/removals.
- [ ] Show width changes.
- [ ] Show net/layer changes.
- [ ] Add old-overlay visibility toggle.
- [ ] Keep review overlays separate from permanent project rendering.

### Change list/details

- [ ] Show object ID.
- [ ] Show change type.
- [ ] Show before value/geometry.
- [ ] Show after value/geometry.
- [ ] Show net.
- [ ] Show layer.
- [ ] Show width/via dimensions where relevant.
- [ ] Show reason/Agent intent where available.
- [ ] Show DRC/ERC before/preview delta.
- [ ] Never invent a “reason” when the runtime did not provide one.

## Review annotations

- [ ] Add select tool.
- [ ] Add comment annotation.
- [ ] Add arrow annotation.
- [ ] Add highlight/paint annotation.
- [ ] Add rectangular region annotation.
- [ ] Add measure tool.
- [ ] Add clear annotations.
- [ ] Store annotations in CAD coordinate space.
- [ ] Attach annotations to object IDs where possible.
- [ ] Keep annotation data separate from project geometry.
- [ ] Serialize annotations into structured revision feedback.
- [ ] Allow annotation selection/edit/delete.
- [ ] Preserve annotations while the same proposal is open.
- [ ] Clear annotations when proposal is discarded unless user intentionally saves them into revision feedback.

## Revise proposal flow

- [ ] Implement dedicated native Revise dialog/window.
- [ ] Show current proposal summary.
- [ ] Show base/proposed preview.
- [ ] Ask “What should the agent change before resubmitting?”
- [ ] Add Avoid this area option.
- [ ] Add Keep original route here option.
- [ ] Add Use fewer vias option.
- [ ] Add Do not change track width option.
- [ ] Add Preserve component placement option.
- [ ] Add Revise only selected objects option.
- [ ] Add free-text feedback.
- [ ] Add Open annotation view.
- [ ] Add Entire proposal scope.
- [ ] Add Selected changes only scope.
- [ ] Add Reject proposal.
- [ ] Add Cancel.
- [ ] Add Send for revision.
- [ ] Serialize structured selected object IDs.
- [ ] Serialize selected regions/coordinates.
- [ ] Serialize annotations.
- [ ] Serialize explicit constraints.
- [ ] Serialize free-text feedback.
- [ ] Resolve/release current paused pending call safely.
- [ ] Feed revision request back into the same LangGraph thread.
- [ ] Generate a fresh proposal ID/version after revision.
- [ ] Do not mutate the project during revision.

## Approve / Reject / Cancel / Undo / Revert

### Approve

- [ ] Verify live project revision still matches proposal base revision.
- [ ] Reject stale proposal before commit.
- [ ] Push native undo snapshot immediately before approved commit.
- [ ] Apply exact approved transaction only.
- [ ] Prevent execution of additional unreviewed operations under the same approval token.
- [ ] Verify authoritative mutation result.
- [ ] Run required DRC/ERC/post-action checks.
- [ ] Add result to transaction/audit trail.
- [ ] Render completion only after authoritative success.

### Reject

- [ ] Reject through the existing pending approval continuation.
- [ ] Record proposal rejection.
- [ ] Do not mutate live project.
- [ ] Return control to the same Agent thread cleanly.

### Cancel

- [ ] Cancel review/pending operation through explicit cancellation state.
- [ ] Do not convert Cancel into Reject unless contract deliberately defines that behavior.
- [ ] Do not leave Python/LangGraph indefinitely blocked after cancellation.

### Undo/Revert

- [ ] Make one approved logical Agent action one coherent undoable operation.
- [ ] Use native project undo snapshot/transaction mechanics.
- [ ] Show `Undo changes` when no unrelated later edits would be destroyed.
- [ ] Show `Revert this change…` when later unrelated edits exist.
- [ ] Implement targeted revert from transaction/diff where required.
- [ ] Never ask the LLM to “reconstruct” the previous project as the primary undo mechanism.
- [ ] Verify undo/revert restores project state.
- [ ] Record undo/revert in audit/transaction timeline.

## Native typed tool surface decomposition

- [ ] Enumerate every currently registered Agent/native method.
- [ ] Remove duplicate aliases unless compatibility requires them.
- [ ] Mark compatibility aliases deprecated.
- [ ] Ensure aliases delegate to authoritative methods rather than returning fabricated `{ok:true}`.
- [ ] Remove fake `action.route` success interception.
- [ ] Remove fake `action.place` success interception.
- [ ] Remove fake compatibility DRC payload if it does not delegate to real `project.drc`.
- [ ] Define one source for method ID.
- [ ] Define method description.
- [ ] Define JSON schema.
- [ ] Define side-effect class.
- [ ] Define approval requirement.
- [ ] Define context requirement.
- [ ] Define dry-run support.
- [ ] Define result schema.
- [ ] Define examples.
- [ ] Define expected failure reasons.
- [ ] Define evidence requirements.
- [ ] Generate provider `StructuredTool` bindings from this registry.
- [ ] Generate tool help/documentation from this registry where possible.
- [ ] Keep native authoritative result unchanged through Python and back to GUI.

## C++ / Python orchestration ownership cleanup

- [ ] Document Python/LangGraph as the only model/planning owner.
- [ ] Document C++ as native tool broker/policy/project/transaction/verification owner.
- [ ] Remove production dependence on C++ `EDAAgent::decompose()` provider pseudo-planning.
- [ ] Remove blocked `agent.plan_with_provider` pseudo-task from user-visible runtime path.
- [ ] Remove static `run_001`, `user`, `workspace`, `main` session identities from production behavior.
- [ ] Remove/retire empty C++ `ContextBuilder::load_stable_prompts()`.
- [ ] Remove/retire empty C++ `ContextBuilder::load_project_memory()`.
- [ ] Remove/retire empty C++ `ContextBuilder::load_repo_map()`.
- [ ] If any loader is retained, implement its real documented behavior and test it.
- [ ] Keep C++ `ToolBroker` for native policy/execution where useful.
- [ ] Keep C++ transaction/preview/undo functions authoritative.
- [ ] Do not create a second model router in C++.
- [ ] Remove architecture/documentation claims that C++ has capabilities it no longer owns.

## Truthful component generation

- [ ] Remove generic fallback `VCC/GND/IN/OUT` pins after model/generation failure.
- [ ] Return explicit generation failure.
- [ ] Preserve original provider/tool error category safely.
- [ ] Require valid generated symbol/footprint schema before preview.
- [ ] Validate pins/pads.
- [ ] Validate numbering.
- [ ] Validate geometry.
- [ ] Validate units.
- [ ] Require review/approval before project persistence.
- [ ] Add failure regression test proving no fabricated component is created.

## DRC/ERC interaction parity

- [ ] Keep `/drc` read-only and immediate.
- [ ] Ensure quick DRC uses the same authoritative DRC method.
- [ ] Remove duplicate fake DRC implementations.
- [ ] Correlate every visual DRC marker with a real diagnostic ID.
- [ ] Remove generic center-of-object error markers with no diagnostic mapping.
- [ ] Add marker visibility control.
- [ ] Selecting diagnostic focuses actual object/location.
- [ ] Selecting canvas marker opens/selects the corresponding diagnostic.
- [ ] Include DRC before/preview/after counts in change review when relevant.
- [ ] Run post-commit DRC for PCB changes where required.
- [ ] Run ERC for schematic changes where required.
- [ ] Never report successful verification when the validator failed to run.

## Security and execution boundaries

- [ ] Keep read-only native tool calls immediate.
- [ ] Keep calculator immediate.
- [ ] Keep dry-run immediate.
- [ ] Define screenshot policy explicitly because screenshot capture can expose sensitive design data even if it does not mutate the project.
- [ ] Gate persistent mutation.
- [ ] Gate destructive mutation.
- [ ] Gate external process execution.
- [ ] Gate CLI execution according to typed allowlist/policy.
- [ ] Gate Python persistence.
- [ ] Gate export/file writes.
- [ ] Validate all file paths against project/sandbox policy.
- [ ] Prevent shell interpolation.
- [ ] Do not inherit unnecessary secrets into subprocesses.
- [ ] Bound subprocess duration.
- [ ] Bound output size.
- [ ] Bound network access.
- [ ] Reject commands outside the supported CCad executable surface.
- [ ] Redact secrets from stderr/stdout before GUI/log/trace propagation.
- [ ] Add regression tests for prompt injection attempting to bypass tool policy.
- [ ] Add regression tests for command injection.
- [ ] Add regression tests for path traversal.
- [ ] Add regression tests for secret exfiltration through tool arguments.
- [ ] Add regression tests for stale/replayed approval.

## CI / CT / CD reliability

- [ ] Fix all Linux `-Werror` failures before claiming a green full gate.
- [ ] Remove or use dead helper functions such as stale credential helpers rather than leaving unused-function CI failures.
- [ ] Ensure provider error-classification helpers are defined/imported before every call site.
- [ ] Add the provider retry regression that previously exposed `NameError: classify_provider_error`.
- [ ] Run Python contract suite on the branch head after every orchestration/provider slice.
- [ ] Run native build after every Qt/C++ slice.
- [ ] Run GUI tests after every Agent UI slice.
- [ ] Run full CTest before marking a slice complete.
- [ ] Run official GUI-map/mouse-keyboard validation for every visible Agent UI change.
- [ ] Inspect every produced screenshot instead of only generating it.
- [ ] Inspect stdout.
- [ ] Inspect stderr.
- [ ] Treat a build failure as failed verification even if a later CLI-only test passes.
- [ ] Do not quote an older `91/91` run as proof for a newer branch head.
- [ ] Record exact tested SHA with every full-gate claim.
- [ ] Record exact workflow/run ID for CI evidence.
- [ ] Re-run CI after fixing a CI-specific Linux warning/error.
- [ ] Do not mark a checkbox complete from a local Windows run when its required CI contract is cross-platform.
- [ ] Add secret scan before final commit/push.
- [ ] Add staged-diff secret scan.
- [ ] Ensure generated screenshots/logs/local config/keys are not staged.

## Commit discipline for Luna

- [ ] Do not make one commit per checkbox.
- [ ] Do not make documentation-only follow-up commits for a feature whose code was just committed.
- [ ] Do not make “fix typo”, “fix lint”, “fix unused helper”, “fix test expected string”, and “update progress” microcommits when they are consequences of the same implementation slice.
- [ ] Before committing, run the slice’s tests so trivial follow-up fixes remain inside the same commit.
- [ ] Bundle source + tests + UI-map + docs + TODO update + evidence references into one logical commit.
- [ ] Use one commit for Agent shell/history/dock parity.
- [ ] Use one commit for composer/context/attachments parity.
- [ ] Use one commit for Settings/Personalisation/memory parity.
- [ ] Use one commit for Providers/Langfuse parity.
- [ ] Use one commit for Marketplace/workflow/hook registry.
- [ ] Use one commit for canonical slash-command registry.
- [ ] Use one commit per coherent staged-review capability slice rather than per button.
- [ ] Use one commit for CI repair when the repair is independent of a feature slice.
- [ ] Do not update progress/TODO claiming completion before tests have run.
- [ ] Include exact verification commands/results in the commit body.
- [ ] Keep commit messages in the repository’s required `Why / Changed / Behavior / Verification / Demo` structure.
- [ ] Do not commit generated boards, keys, local config, logs, transient screenshots, vault material, or unrelated files.
- [ ] Squash/rework obvious same-slice microcommits before merging where practical.

## Suggested implementation slices for Luna

### Slice A — Shell and dock parity

- [ ] Repair Layers/Objects vs Agent dock topology.
- [ ] Add adaptive tabification.
- [ ] Remove dotted native focus rectangle.
- [ ] Replace stand-in icons needed by the shell.
- [ ] Implement history sidebar visual structure.
- [ ] Implement New Chat/thread creation.
- [ ] Implement history collapse/expand.
- [ ] Implement session title binding.
- [ ] Remove obsolete top-level status/dashboard clutter.
- [ ] Add UI-map targets.
- [ ] Add native GUI tests.
- [ ] Run screenshot validation.
- [ ] Commit as one slice.

### Slice B — Composer and context

- [ ] Replace fake attachment text with structured attachment objects.
- [ ] Implement attachment visual rows.
- [ ] Implement attachment IPC.
- [ ] Replace gear/context stand-in with circular context indicator.
- [ ] Make context visibility setting control visibility.
- [ ] Add context breakdown.
- [ ] Remove fake context-refresh message.
- [ ] Hide voice until STT exists.
- [ ] Canonicalize composer model selector with Settings.
- [ ] Add tests and screenshots.
- [ ] Commit as one slice.

### Slice C — Settings and memory

- [ ] Restructure Settings to match approved information architecture.
- [ ] Remove duplicate memory controls.
- [ ] Add real three-tier memory toggles to Personalisation.
- [ ] Implement enable/load/unload semantics.
- [ ] Separate disable from destructive delete/reset.
- [ ] Canonicalize plugins/workflows config keys.
- [ ] Fix resolved configuration page.
- [ ] Add config round-trip tests.
- [ ] Add memory runtime tests.
- [ ] Add Settings screenshots.
- [ ] Commit as one slice.

### Slice D — Providers and Langfuse

- [ ] Canonicalize provider/model state.
- [ ] Verify provider catalog/adapter behavior.
- [ ] Finish Langfuse Settings.
- [ ] Finish reconfigurable Langfuse runtime.
- [ ] Bind thread/session ID.
- [ ] Complete trace hierarchy.
- [ ] Complete centralized redaction.
- [ ] Run real opt-in test trace.
- [ ] Add provider/Langfuse tests.
- [ ] Add screenshots.
- [ ] Commit as one slice.

### Slice E — Marketplace, workflows, hooks

- [ ] Replace pseudo Marketplace catalogue.
- [ ] Define typed registry.
- [ ] Implement search/categories/details.
- [ ] Implement truthful installation availability.
- [ ] Canonicalize plugin/workflow config.
- [ ] Implement typed workflows.
- [ ] Implement typed hooks.
- [ ] Remove false installation claims.
- [ ] Add tests and screenshots.
- [ ] Commit as one slice.

### Slice F — Slash commands

- [ ] Create canonical command registry.
- [ ] Generate autocomplete from registry.
- [ ] Generate `/commands` from registry.
- [ ] Generate `/help` from registry.
- [ ] Add descriptions for every command.
- [ ] Hide/remove commands without backend.
- [ ] Fix `/settings`.
- [ ] Fix `/clear`.
- [ ] Validate `/workflow`.
- [ ] Integrate `/marketplace`.
- [ ] Integrate `/set`.
- [ ] Keep `/drc` authoritative.
- [ ] Test every advertised command.
- [ ] Commit as one slice.

### Slice G — Proposal staging and review foundation

- [ ] Define immutable pending change model.
- [ ] Bind proposal ID/call ID/base revision.
- [ ] Stage supported mutation in copied project.
- [ ] Compute real diff.
- [ ] Build preview transaction.
- [ ] Render real change list.
- [ ] Implement stale-proposal detection.
- [ ] Implement single-use approval.
- [ ] Add tests.
- [ ] Commit as one slice.

### Slice H — PCB visual review

- [ ] Replace text-only PCB review.
- [ ] Focus/zoom actual editor.
- [ ] Add old/new theme-derived overlays.
- [ ] Add track/via change list.
- [ ] Add annotations.
- [ ] Add Approve/Revise/Reject.
- [ ] Add post-commit DRC.
- [ ] Add Undo/Revert.
- [ ] Add full disposable-board orchestration proof.
- [ ] Commit as one slice.

### Slice I — Schematic visual review

- [ ] Add staged schematic preview.
- [ ] Add symbol/wire/net change list.
- [ ] Add old/new schematic overlay.
- [ ] Add schematic annotations.
- [ ] Add ERC delta.
- [ ] Reuse same approval/revision/undo contracts.
- [ ] Add equivalent schematic orchestration proof.
- [ ] Commit as one slice.

### Slice J — Remaining execution surfaces

- [ ] Complete guarded CLI execution.
- [ ] Complete calculator/coordinate-transform tool.
- [ ] Complete constrained Python computation.
- [ ] Complete export path.
- [ ] Complete exact UI-map action surface.
- [ ] Complete memory tool surface.
- [ ] Prove every surface through real orchestration.
- [ ] Commit in coherent capability groups rather than one commit per tool.

## Final product parity gate

Do not claim Agent UI parity until all of the following are simultaneously true.

- [ ] Layers/Objects and Agent docks remain usable at target window sizes.
- [ ] No unwanted dotted focus rectangles remain.
- [ ] Conversation history is real and collapsible.
- [ ] New Chat creates a real independent thread.
- [ ] Conversation switching restores the correct thread.
- [ ] Header icons are semantic and theme-consistent.
- [ ] No obsolete status-chip/dashboard slop remains in normal chat.
- [ ] Attachments are structured.
- [ ] Context usage is real/estimated honestly and model-specific.
- [ ] Voice is hidden unless STT is real.
- [ ] Settings controls all have real runtime/config effects.
- [ ] STM/LTM/episodic toggles have distinct real runtime semantics.
- [ ] Provider/model state is canonical across all UI/commands/runtime.
- [ ] Langfuse configuration is real and secret-safe.
- [ ] Marketplace shows only truthful capabilities/install state.
- [ ] Slash autocomplete has canonical descriptions and no fake commands.
- [ ] Read-only tools do not request mutation approval.
- [ ] Persistent mutations always use the staged proposal/approval boundary.
- [ ] PCB visual diff operates on real staged geometry.
- [ ] Schematic visual diff operates on real staged geometry.
- [ ] Review can focus/zoom actual changed objects.
- [ ] Review annotations are stored in CAD coordinates.
- [ ] Revise feeds structured feedback into the same thread.
- [ ] Reject/cancel cannot mutate.
- [ ] Approval cannot be replayed or applied after stale revision.
- [ ] Commit result is authoritative.
- [ ] Post-action verification is authoritative.
- [ ] Undo/Revert uses native project transaction state.
- [ ] One complete real-provider PCB workflow has been demonstrated end-to-end.
- [ ] One complete real-provider schematic workflow has been demonstrated end-to-end.
- [ ] One real Langfuse trace has been inspected for hierarchy and redaction.
- [ ] Current branch head passes the full build/test gate.
- [ ] Current branch head passes the official GUI validation gate.
- [ ] Current branch head passes secret scanning.
- [ ] TODO/progress/docs describe the actual current implementation without overstating capability.

## Langfuse trace topology — one complete trace per Agent prompt

The required observability topology is:

`CCad conversation/thread = Langfuse session`

`one user prompt / Agent turn = one Langfuse trace`

`every operation caused by that turn = nested Langfuse observations inside that trace`

Do not create independent top-level traces for model calls, tools, routing nodes, context assembly, memory retrieval, approvals, transactions, or verification when they belong to the same Agent turn.

### Canonical trace identity

- [ ] Assign one stable `turn_id` when `human_message` is accepted.
- [ ] Create exactly one Langfuse trace identity for that turn.
- [ ] Keep that trace identity unchanged from prompt receipt through final Agent completion.
- [ ] Bind the trace to the durable CCad Agent `thread_id` through Langfuse `session_id`.
- [ ] Use the same `session_id` for every turn belonging to the same CCad conversation.
- [ ] Create a new trace for the next independently submitted user prompt while keeping the same session ID.
- [ ] Do not use a new Langfuse session for every prompt.
- [ ] Do not use one Langfuse trace for the entire lifetime of a long chat.
- [ ] Do not create one top-level trace per tool call.
- [ ] Do not create one top-level trace per model generation.
- [ ] Do not let LangChain/LangGraph callbacks create orphan traces outside the active CCad turn trace.
- [ ] Record the Langfuse trace ID in the Agent turn/runtime state.
- [ ] Record the Langfuse root observation ID where needed for resume/reparenting.
- [ ] Keep trace IDs out of project files unless deliberately required for audit metadata.
- [ ] Expose a developer-only `Open trace in Langfuse` action for the current/completed turn where a trace URL is available.

### Root Agent-turn observation

- [ ] Start one root observation when a user prompt enters the Agent runtime.
- [ ] Name the root consistently, for example `agent.turn`.
- [ ] Use an appropriate Langfuse observation type such as `agent` or `span`.
- [ ] Make the root observation active before context construction begins.
- [ ] Run the LangGraph invocation while this root observation is the active OTel/Langfuse context.
- [ ] Pass the official Langfuse LangChain callback while the same root trace context is active.
- [ ] Ensure callback-created model/chain/tool observations inherit the active turn trace instead of becoming independent traces.
- [ ] End the logical turn trace only when the turn reaches an actual terminal state:
  - [ ] completed.
  - [ ] failed.
  - [ ] cancelled.
  - [ ] explicitly abandoned.
- [ ] Do not end the turn trace merely because the Agent requested a tool.
- [ ] Do not end the turn trace merely because human approval is required.
- [ ] Distinguish total end-to-end duration from active-compute duration so human approval waiting does not hide model/tool performance.

### Conversation/session grouping

- [ ] Propagate the durable CCad `thread_id` as Langfuse `session_id`.
- [ ] Propagate `session_id` at the start of the trace so all child observations inherit it.
- [ ] Verify all turns from one CCad chat appear grouped together in one Langfuse session.
- [ ] Verify switching to another CCad chat produces a different Langfuse session ID.
- [ ] Verify resuming an old CCad thread reuses its original Langfuse session ID.
- [ ] Never derive `session_id` from temporary QWidget addresses, process IDs, run counters, or random telemetry fallback IDs.
- [ ] Keep one explicit mapping:
  `CCad durable thread ID -> Langfuse session ID`.
- [ ] Add a regression test proving two prompts in one chat create two traces inside one session.
- [ ] Add a regression test proving prompts in different chats do not enter the same session.

### Required observation hierarchy

For a non-trivial Agent turn, the Langfuse trace should resemble this logical tree:

`agent.turn`
- `input.receive`
- `context.assemble`
  - `project.snapshot`
  - `pcb.context`
  - `schematic.context`
  - `selection.context`
  - `rules.context`
  - `libraries.context`
  - `attachments.process`
  - `memory.retrieve`
    - `memory.stm`
    - `memory.ltm`
    - `memory.episodic`
  - `tool_catalog.bind`
  - `context.compact` when applicable
- `agent.orchestration`
  - `supervisor`
  - `router`
  - `librarian` when used
  - `workflow.select`
  - other real graph nodes
- `model.generate`
- `tool.call`
  - `tool.validate`
  - `policy.evaluate`
  - `approval.evaluate`
  - `broker.dispatch`
  - `tool.execute`
  - `tool.result`
- additional `model.generate` observations as the Agent reasons after tool results
- `proposal.stage` when mutation is proposed
  - `project.clone`
  - `mutation.stage`
  - `project.diff`
  - `transaction.build`
  - `preview.validate`
  - `drc.preview` / `erc.preview`
  - `preview.render`
- `approval`
  - `approval.wait`
  - `approval.decision`
- `transaction.commit` when approved
- `verification`
  - `project.verify`
  - `drc.after`
  - `erc.after`
  - `screenshot.capture` when enabled
- `final.generate`
- `turn.complete`

Only create observations for operations that actually execute.

### Observation typing

- [ ] Record LLM/provider calls as Langfuse `generation` observations.
- [ ] Record Agent orchestration as `agent`/`chain`/`span` observations according to current supported Langfuse types.
- [ ] Record actual native tool invocations as `tool` observations.
- [ ] Record memory retrieval as `retriever` or appropriately named span observations.
- [ ] Record policy/guardrail decisions using a suitable `guardrail` or span observation.
- [ ] Record context construction, diff generation, transactions, and verification as spans.
- [ ] Do not represent every operation as a generic generation.
- [ ] Do not represent tool calls as model generations.
- [ ] Preserve correct parent-child relationships so the Langfuse tree itself explains why each operation occurred.

### Context assembly detail

- [ ] Add `context.assemble` under the current turn trace.
- [ ] Record safe metadata for project context contribution.
- [ ] Record safe metadata for PCB contribution.
- [ ] Record safe metadata for schematic contribution.
- [ ] Record safe metadata for active selection contribution.
- [ ] Record safe metadata for rules/libraries contribution.
- [ ] Record safe metadata for attachment contribution.
- [ ] Record safe metadata for enabled memory contribution.
- [ ] Record safe metadata for tool-schema contribution.
- [ ] Record total token estimate/actual count when available.
- [ ] Record model context limit when authoritative metadata exists.
- [ ] Record whether token count was exact or estimated.
- [ ] Record compaction occurrence and before/after token counts.
- [ ] Do not export raw project/design contents by default.
- [ ] Do not export raw prompt text by default.
- [ ] Use hashes, counts, IDs, sizes, revision IDs, and redacted summaries when raw content export is disabled.

### Memory detail

- [ ] Create one parent `memory.retrieve` observation when memory retrieval runs.
- [ ] Record STM retrieval separately when enabled.
- [ ] Record LTM retrieval separately when enabled.
- [ ] Record episodic retrieval separately when enabled.
- [ ] Record candidate count.
- [ ] Record selected count.
- [ ] Record ranking strategy/version.
- [ ] Record token contribution.
- [ ] Record scope.
- [ ] Record cache/store hit state where useful.
- [ ] Never export secret-bearing raw memory values unless explicitly enabled and redacted.
- [ ] Record memory writes/update/delete operations as nested observations when they actually occur.
- [ ] Ensure disabled memory tiers produce no retrieval/write observation except an optional safe `disabled` metadata state.

### Model-generation detail

- [ ] Every real provider request must appear as a nested Langfuse generation inside the current turn trace.
- [ ] Record provider.
- [ ] Record exact model ID.
- [ ] Record model parameters that are safe to expose.
- [ ] Record input token count.
- [ ] Record output token count.
- [ ] Record cached token usage where supplied.
- [ ] Record total token count.
- [ ] Record provider-returned usage metadata.
- [ ] Record cost when calculated from authoritative pricing/usage information.
- [ ] Record request latency.
- [ ] Record time-to-first-token where measurable.
- [ ] Record retry number.
- [ ] Record finish reason.
- [ ] Record safe provider error category.
- [ ] Never expose provider API keys, authorization headers, raw secret-bearing request metadata, or unredacted exception bodies.

### Tool-call detail

- [ ] Each actual tool call must appear under the Agent step/model reasoning that caused it.
- [ ] Record native method ID.
- [ ] Record `call_id`.
- [ ] Record side-effect classification.
- [ ] Record dry-run state.
- [ ] Record approval requirement.
- [ ] Record schema-validation result.
- [ ] Record policy decision.
- [ ] Record broker dispatch.
- [ ] Record execution duration.
- [ ] Record authoritative success/failure.
- [ ] Record safe result summary.
- [ ] Record artifact IDs where relevant.
- [ ] Preserve exact authoritative failure category.
- [ ] Keep raw tool arguments disabled by default.
- [ ] Redact tool argument fields before export when detailed tracing is enabled.
- [ ] Do not create a successful tool span when the native broker reports failure.

### Proposal/approval detail

- [ ] Keep the proposal lifecycle inside the same prompt trace that produced the proposal.
- [ ] Record proposal/change-set ID.
- [ ] Record base project revision.
- [ ] Record staged project revision.
- [ ] Record affected-object count.
- [ ] Record affected-net/layer counts.
- [ ] Record `ProjectDiff` summary.
- [ ] Record preview validation result.
- [ ] Record DRC/ERC before/preview delta.
- [ ] Record whether the proposal was shown to the user.
- [ ] Add `approval.wait`.
- [ ] Add `approval.decision`.
- [ ] Record decision as approve/reject/revise/cancel.
- [ ] Record human wait duration separately from active compute time.
- [ ] Do not record raw reviewer comments by default.
- [ ] Record safe revision metadata such as selected-object count and constraint categories.
- [ ] If the user chooses Revise inside the proposal UI, keep it within the originating turn trace where technically practical because it is continuation of the same pending Agent request.
- [ ] If the user submits a completely new independent composer prompt, create a new trace.

### Approval pause/resume trace continuity

- [ ] Persist `trace_id` with the pending Agent turn before entering human approval wait.
- [ ] Persist the relevant parent/root observation identity needed to restore hierarchy.
- [ ] On approval/revision/cancellation resume, restore the same trace context.
- [ ] Use Langfuse/OTel trace-context APIs rather than generating a new unrelated trace after resume.
- [ ] Ensure resumed tool/transaction/verification observations retain the original turn trace ID.
- [ ] Handle LangGraph checkpoint resume without losing Langfuse trace correlation.
- [ ] Handle asynchronous Qt -> Python approval continuation without losing trace correlation.
- [ ] Handle process boundaries/restarts using explicit trace context when continuation of the same logical turn is supported.
- [ ] Generate a new trace only when the logical Agent turn has actually ended and a new turn begins.
- [ ] Add a regression test that pauses on approval, resumes, and verifies pre-approval and post-approval observations share the same trace ID.
- [ ] Add a regression test for Revise preserving the intended trace/session relationship.
- [ ] Add a regression test for cancellation closing the originating turn trace cleanly.

### Transaction and verification detail

- [ ] Record transaction build as a child observation.
- [ ] Record transaction ID.
- [ ] Record base revision.
- [ ] Record committed revision.
- [ ] Record operation count.
- [ ] Record native commit success/failure.
- [ ] Record stale-revision rejection.
- [ ] Record rollback where it occurs.
- [ ] Record DRC result after commit.
- [ ] Record ERC result after commit.
- [ ] Record screenshot/evidence capture metadata when enabled.
- [ ] Record undo/revert when it occurs as part of the same still-active interaction.
- [ ] If Undo/Revert is requested later as a separate user interaction, create a new trace within the same conversation session and link safe transaction/proposal IDs through metadata.

### Final response detail

- [ ] Record the final provider generation separately from intermediate reasoning/model calls.
- [ ] Record final turn state.
- [ ] Record safe output metadata.
- [ ] Record whether project state changed.
- [ ] Record final project revision.
- [ ] Record final DRC/ERC summary when relevant.
- [ ] Record proposal state if the turn ended pending/rejected/cancelled.
- [ ] Close the root turn observation after the final terminal state has been recorded.
- [ ] Flush according to the existing bounded Langfuse runtime policy.

### Required trace metadata

- [ ] Propagate `session_id = durable CCad thread ID`.
- [ ] Propagate safe trace name such as `ccad.agent.turn`.
- [ ] Add `turn_id`.
- [ ] Add provider ID.
- [ ] Add model ID.
- [ ] Add workflow ID where active.
- [ ] Add project revision/hash rather than raw project contents.
- [ ] Add project mode/type such as PCB/schematic/mixed where useful.
- [ ] Add proposal ID when present.
- [ ] Add transaction ID when present.
- [ ] Add result state.
- [ ] Add application/version/build SHA where useful.
- [ ] Add safe tags such as `ccad-agent`, provider, workflow, PCB/schematic.
- [ ] Do not use high-cardinality raw filenames, filesystem paths, prompt text, or secret values as tags.

### Trace input/output privacy

- [ ] Keep raw prompt contents OFF by default.
- [ ] Keep raw assistant output contents OFF by default if the current privacy policy requires it.
- [ ] When prompt-content tracing is disabled, record prompt hash/length/token count instead.
- [ ] Keep raw tool arguments OFF by default.
- [ ] Keep raw tool result payloads OFF by default.
- [ ] Keep screenshots OFF by default.
- [ ] Keep PCB/schematic raw data OFF by default.
- [ ] Keep memory contents OFF by default.
- [ ] Expose separate explicit opt-in controls for content-bearing trace data if retained as a product requirement.
- [ ] Apply central redaction before Langfuse/OTel receives any attribute or payload.
- [ ] Test representative nested structures for recursive secret redaction.

### No orphan-trace rule

- [ ] Add a development assertion/diagnostic for Agent operations emitted without an active `turn_id`.
- [ ] Add a development assertion/diagnostic for tool/model observations emitted without the expected trace context.
- [ ] Detect a model call that unexpectedly creates a second top-level trace during one Agent turn.
- [ ] Detect a tool call that unexpectedly creates a second top-level trace during one Agent turn.
- [ ] Detect approval resume that loses the originating trace ID.
- [ ] Log a safe diagnostic locally when trace parenting is lost.
- [ ] Do not silently accept fragmented traces as successful observability.

### LangGraph callback integration

- [ ] Start the CCad root turn observation before invoking the graph.
- [ ] Enter the Langfuse attribute-propagation scope before invoking the graph.
- [ ] Pass the official Langfuse callback to the graph invocation inside that scope.
- [ ] Keep manually instrumented CCad spans inside the same active OTel context.
- [ ] Verify LangGraph node observations nest beneath `agent.orchestration`.
- [ ] Verify generations created by the LangChain/LangGraph integration remain inside the turn trace.
- [ ] Verify native tool spans correlate back to the tool call requested by the corresponding generation.
- [ ] Avoid instrumenting the same model/tool call twice through both automatic and manual instrumentation.
- [ ] Define which layer owns each observation to prevent duplicate spans.

### Recommended ownership of trace observations

- [ ] CCad turn wrapper owns `agent.turn`.
- [ ] Context builder owns `context.assemble`.
- [ ] Memory manager owns memory observations.
- [ ] LangGraph integration owns graph/chain/model-generation observations where it already instruments them correctly.
- [ ] CCad tool broker owns native tool execution observations.
- [ ] Approval manager owns approval observations.
- [ ] Staging/transaction layer owns proposal/diff/transaction observations.
- [ ] DRC/ERC subsystem owns verification observations.
- [ ] Do not let Qt widgets create business-logic traces directly.
- [ ] Qt may attach UI action metadata/events to an existing turn but must not become the observability source of truth.

### Langfuse inspection acceptance test

A single real Agent prompt must be manually inspected in Langfuse before this section is complete.

- [ ] Start one new CCad conversation.
- [ ] Submit one prompt that requires project context.
- [ ] Confirm exactly one Agent-turn trace is created for that prompt.
- [ ] Confirm its Langfuse session ID matches the CCad thread ID.
- [ ] Confirm context assembly is visible underneath the trace.
- [ ] Confirm memory retrieval is visible if enabled.
- [ ] Confirm supervisor/router/other real graph nodes are visible.
- [ ] Confirm every provider generation is visible as a generation observation.
- [ ] Confirm token usage is visible where the provider supplies it.
- [ ] Confirm cost appears only when reliable pricing is available.
- [ ] Confirm every tool call is visible and correctly parented.
- [ ] Confirm read-only tool calls remain inside the same trace.
- [ ] Use a mutation prompt that requires proposal/approval.
- [ ] Confirm proposal staging is visible.
- [ ] Confirm approval wait is visible.
- [ ] Approve the proposal.
- [ ] Confirm approval continuation remains in the same trace.
- [ ] Confirm transaction commit is visible.
- [ ] Confirm DRC/ERC verification is visible.
- [ ] Confirm final Agent generation is visible.
- [ ] Confirm the trace terminates with the actual final outcome.
- [ ] Confirm no extra top-level trace was created for any model call.
- [ ] Confirm no extra top-level trace was created for any tool call.
- [ ] Confirm no extra top-level trace was created after approval resume.
- [ ] Confirm a second user prompt creates a second trace inside the same Langfuse session.
- [ ] Confirm the Langfuse session view therefore reconstructs the complete CCad chat across prompt-level traces.
- [ ] Inspect all observation inputs/outputs/metadata for credential leakage.
- [ ] Verify provider secrets are absent.
- [ ] Verify Langfuse secret/public credential material is absent from observation payloads.
- [ ] Verify authorization headers are absent.
- [ ] Verify raw project contents are absent when content tracing is disabled.
- [ ] Verify raw prompt/tool arguments are absent when their opt-ins are disabled.
- [ ] Save the trace ID/URL and exact tested CCad SHA as validation evidence.

### Trace tree quality gate

Do not mark Langfuse observability complete merely because data appears in Langfuse.

- [ ] One prompt must read visually as one coherent tree.
- [ ] A developer must be able to answer “why did this tool run?” from its parent observations.
- [ ] A developer must be able to see which model generation requested a tool.
- [ ] A developer must be able to see which tool result returned to the Agent.
- [ ] A developer must be able to distinguish reasoning/model latency from tool latency.
- [ ] A developer must be able to distinguish active compute time from human approval waiting time.
- [ ] A developer must be able to inspect retries and their causes.
- [ ] A developer must be able to inspect the proposal/approval/transaction sequence.
- [ ] A developer must be able to determine the exact terminal failure stage without reading local logs.
- [ ] A developer must be able to correlate the trace with the CCad thread, turn, proposal, transaction, and project revision without exposing sensitive project contents.
- [ ] There must be no unexplained sibling/top-level observations caused by broken OTel context propagation.

## Native Qt UI implementation discipline — reference SPA is authoritative

The SPA has already been produced. Do not redesign it again. Its purpose is to stop autonomous UI invention by the coding agent.

- [ ] Treat the approved SPA as a visual/behavioral specification, not as source code to embed.
- [ ] Do not introduce React, WebView, Electron, Node, npm, browser runtime, or web dependencies into CCad to reproduce the SPA.
- [ ] Rebuild each reference interaction using the existing Qt widget/framework architecture.
- [ ] Reuse existing CCad/KiCad-derived theme tokens, icon infrastructure, spacing, docks, tabs, menus, dialogs, and editor views where possible.
- [ ] Do not independently “improve”, simplify, or reinterpret the approved layout without an explicit product decision.
- [ ] Do not replace approved UI with generic AI/SaaS dashboard patterns.
- [ ] Do not add decorative status cards, telemetry dashboards, progress chips, assistant avatars, pills, or sidebars absent from the reference merely because they are common in AI products.
- [ ] Do not omit a reference control merely because its backend is not yet implemented.
- [ ] When a required backend is missing, implement the backend or keep the control explicitly disabled/unavailable; do not replace it with a fake action.
- [ ] Do not wire reference controls to placeholder chat messages.
- [ ] Do not recreate visible UI state by parsing prose from the model.
- [ ] Drive UI state from typed backend events/models.
- [ ] Preserve the existing native editor, Layers/Objects surface, toolbars, status bar, menus, and application identity around the Agent UI.
- [ ] Use the reference SPA only to determine information architecture, layout hierarchy, interaction behavior, visibility rules, and state transitions.
- [ ] Use the current native CCad codebase to determine actual implementation classes, signals/slots, models, tools, transactions, project state, and backend ownership.
- [ ] For every SPA control, document:
  - [ ] corresponding Qt widget/class.
  - [ ] semantic UI-map ID.
  - [ ] signal.
  - [ ] slot/controller.
  - [ ] backend RPC/tool/state dependency.
  - [ ] enabled/disabled condition.
  - [ ] loading state.
  - [ ] error state.
  - [ ] persistence requirement.
  - [ ] required test.
- [ ] Verify each completed native screen side-by-side against the SPA reference.
- [ ] Capture a native screenshot at the same approximate window dimensions as its SPA reference.
- [ ] Inspect spacing, hierarchy, visibility, icon semantics, typography, control grouping, disabled states, and dock proportions.
- [ ] Do not accept “functionally similar” if the native implementation has obviously drifted into another layout.
- [ ] Do not accept visually matching UI when controls are backed by stubs.
- [ ] Require both visual parity and backend truthfulness before marking a UI slice complete.

## UI stub elimination gate

Before declaring the Agent UI complete, audit every interactive element.

- [ ] Clicking every visible button must invoke a real action, open a real surface, or truthfully report unavailable.
- [ ] Every checkbox/toggle must change canonical state consumed by the runtime.
- [ ] Every dropdown must use canonical state and valid options.
- [ ] Every status indicator must be produced from authoritative state.
- [ ] Every list must be populated from real data or explicitly labelled example/empty state.
- [ ] Every progress indicator must reflect measurable work.
- [ ] Every count must derive from actual data.
- [ ] Every install button must have a real installation backend.
- [ ] Every Enable toggle must actually affect runtime capability.
- [ ] Every Save operation must persist and read back successfully.
- [ ] Every Reset/Delete operation must actually perform its documented destructive action.
- [ ] Every preview must derive from staged project data.
- [ ] Every Approve action must commit exactly what was previewed.
- [ ] Every Reject/Cancel action must demonstrably avoid mutation.
- [ ] Every Undo/Revert action must operate on native project history/transaction state.
- [ ] Remove all remaining production strings such as:
  - [ ] `Stand-in`.
  - [ ] `TODO` used behind visible working-looking controls.
  - [ ] `mock`.
  - [ ] `fake`.
  - [ ] `placeholder`.
  - [ ] fabricated `ok:true` responses.
  - [ ] fabricated completion prose.
- [ ] Search the full Agent-related source tree for these patterns before the final parity gate.
