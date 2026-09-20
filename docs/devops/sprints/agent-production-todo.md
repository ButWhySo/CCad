# CCad Agent production TODO

This is the repository-tracked execution plan supplied on 2026-09-20. Check a
box only after implementation, source-level contract coverage, real runtime
evidence where applicable, and same-commit documentation have been reviewed.

## Update protocol and active slice

Every implementation slice must update this file in the same commit. A task
stays unchecked when any part of its stated end-to-end evidence is missing;
partial work is recorded here rather than represented as completion.

**2026-09-20, Sprint 947 — committed and pushed as `d1917eb`.** The native no-executor path now
fails truthfully with `task_executor_unavailable`, and the Agent panel restores
the persisted provider/model before backend startup. The offline contract gate
passed 57 scripts. The explicit Qt runtime completed all 91 CTest cases in
runtime-safe intervals; the official UI-map target harness visually confirmed
the restored model header and reachable Agent Settings controls. The remaining
Sprint 947 passed its staged-diff secret scan and was pushed after the gates.
Provider-first-turn activation, complete context, dynamic tools, memory lifecycle, tracing,
and real mutation proof remain unchecked because their full stated criteria are
not yet met.

**2026-09-20, Sprint 948 — in progress.** AgentPanel now reads the live native
`agent.methods` catalog and registers every catalog entry with its published
description, JSON input schema, and read-only/mutation policy. This removes the
second hand-maintained C++ method list. The Qt panel test passed and the
official target harness reported every initial/resized target found with empty
stderr; its Agent screenshot was visually inspected. Python still exposes a
fixed LangChain tool list, so model-facing dynamic composition remains
unchecked until that side uses this registry too.

**Next slice, not started.** Replace the Python fixed lists only together with
validated catalog transport, `StructuredTool` construction from native input
schemas, the existing broker call-id/approval path, ToolNode rebuild, and a
real provider tool-call proof. A sender-only IPC change is not retained.

## Provider, context, and memory

- [ ] Restore persisted provider, model, and OS-vault credential before first chat turn. Sprint 947 restores the visible provider/model before Python startup; an end-to-end first live chat turn is still required.
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
- [ ] Add screenshot/evidence capture with viewport, layer, and selection metadata.
- [ ] Add UI-map inspection and mapped-action tools; never fixed-coordinate scripts for normal operation.
- [ ] Add exact PCB/schematic state inspection and typed placement/edit transactions with units, snap, net, geometry, rules, and validation.
- [ ] Let models compose real tools dynamically; remove fixed workflow execution paths. Sprint 948 removes the fixed C++ registration list; Python LangChain binding remains to be made catalog-driven.

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

- [ ] Write contract tests before behavior changes. Sprint 947 added no-executor and persisted-selection coverage; this remains a continuous delivery requirement.
- [ ] Build Qt toolchain, run targeted/full CTest, official UI-map/mouse-keyboard harness, inspect screenshots and stdout/stderr, and perform bounded real provider tests. Sprint 947: all 91 native CTest cases, 57 offline contracts, and the UI-map visual run passed; the next bounded real-provider turn is pending the complete dynamic tool/context surface.
- [ ] Run redacted repository and staged-diff secret scans before every commit. Repository scan is in progress for Sprint 947; staged-diff scan follows selective staging.
- [ ] Update architecture, feature, CLI, methodology, provider, memory, tracing, autorouter, backlog, and progress docs in the same commit.
- [ ] Commit only verified source/tests/docs; never keys, vault data, local config, logs, screenshots, generated boards, or unrelated user files.
