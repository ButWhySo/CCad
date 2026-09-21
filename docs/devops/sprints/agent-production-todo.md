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
- [x] Qt Release build and full CTest gate (91/91).
- [x] Bounded real-provider catalog tool call and broker result.

## Provider, context, and memory

- [x] Restore persisted provider, model, and OS-vault credential before first chat turn.
- [ ] Verify every provider/model integration against official documentation and classify failures safely.
- [ ] Build bounded context from project, PCB, schematic, selection, coordinates, layers, nets, rules, libraries, tool state, conversation, and memories.
- [ ] Report safe metadata for the exact context package sent on each turn.
- [ ] Implement STM, project-long-term, and episodic memory: retrieval, scope, ranking, update, deletion, reset, expiry, compaction, and secret redaction.
- [ ] Publish the exhaustive memory/context lifecycle report.

## Typed CCad tool surface

- [ ] Generate one typed registry from real CLI, core transactions, UI-map, DRC/ERC, library/catalog, schematic, PCB, routing, export, inspection, memory, and evidence surfaces.
- [ ] Give each supported command schemas, examples, validation, side-effect class, context needs, and result shape.
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
- [ ] Store tracing credentials separately in Windows Credential Manager; never in args, project/config files, logs, prompts, screenshots, or git.
- [ ] Initialize Langfuse before first traced graph call and attach official callback to every invocation.
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
- [ ] Fix live Agent Settings opening and modeless dialog discovery through the UI map.
- [x] Make quick DRC execute authoritative DRC or remove the chip; do not merely insert `/drc`.
- [x] Map `/drc` to authoritative `project.drc`, return its diagnostics, and never leave a chat turn at “Running DRC checks…”.
- [ ] Preserve 429/quota/rate-limit categories from provider SDK exceptions; never relabel them `provider_unavailable`.
- [x] Remove the stale Gemini adapter notice that says no provider was contacted after a configured provider has initialized or completed a real call.
- [ ] Bind one immutable proposal/call ID to one approval and one execution; reject duplicate/replayed approval results.
- [ ] Never narrate a failed UI gesture or transaction as a completed board change; surface the authoritative failure reason.
- [ ] Make collapsed Agent dock restorable through a mapped action and release its unused dock space.
- [ ] Expose Layers/Objects child tabs as stable UI-map targets.
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
- [ ] Remove false "activated and hooked into context" claims when installation has not occurred.
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
- [ ] Store Langfuse secret key in the OS credential vault, never config/project/logs/prompts.
- [ ] Update LangChain integration to the current `langfuse.langchain.CallbackHandler` API.
- [ ] Pin a tested Langfuse SDK major/minor range instead of unconstrained `langfuse>=2.30.0`.
- [ ] Replace import-time-only tracer/callback initialization with reconfigurable `LangfuseRuntime`.
- [ ] Add `agent.langfuse_set_config`, `agent.langfuse_set_secret`, `agent.langfuse_status`, and `agent.langfuse_test`.
- [ ] Use durable Agent thread ID as Langfuse session ID.
- [ ] Trace agent run, prompt assembly, model calls, routing, tool calls, approvals, transactions, verification, DRC/ERC, retries, cancellation, and failures.
- [ ] Record provider/model/token/cost/latency when available.
- [ ] Implement Langfuse `mask_otel_spans` redaction before export.
- [ ] Default prompt contents, raw tool arguments, screenshots, and project contents to OFF.
- [ ] Flush Langfuse on explicit test, application shutdown, and bounded process termination.
- [ ] Prove one opt-in real Langfuse trace contains the expected hierarchy and no secrets.
