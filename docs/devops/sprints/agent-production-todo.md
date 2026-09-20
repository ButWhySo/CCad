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
- [ ] Add screenshot/evidence capture with viewport, layer, and selection metadata.
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
