# CCad honest status review

## Purpose

This document records a capability review that deliberately separates working product behavior from scaffolding, planned work, and marketing language. It is a planning baseline, not a claim of external validation or investment readiness.

## Current product boundary

CCad is a solo-developer prototype for an LLM-native desktop PCB workflow. Its source of truth is a typed C++ kernel with a JSON project representation, a CLI mutation/query surface, and an optional Qt review/editor surface. The working physical model includes board outline, layers, pads, vias, tracks, zones, keepouts, placement regions, imported footprints, and geometric DRC checks. The logical layer includes schematic primitives, netlist-style connectivity, and ERC-oriented diagnostics.

Routing is manual or agent-directed track/via authoring plus route-request bookkeeping. It is not a production autorouter. Zone refill is an early deterministic slice, not a KiCad-equivalent solver with complete thermal-spoke, island, and optimization behavior. Fabrication and KiCad evidence workflows can delegate to `kicad-cli`; that makes CCad an orchestration/front-end layer for those workflows, not yet an independent fabrication engine.

The Qt GUI is a useful review surface and early editor shell. It has PCB rendering, layer/object panels, schematic tabs and early actions, semantic UI-map access, agent chat, local session metadata, approval UI, and diagnostics. It is not yet feature-complete KiCad parity, especially for schematic editing, advanced placement, routing, zone solving, 3D, manufacturing, and polished long-running agent interaction.

## Agent and harness boundary

The current agent work is primarily infrastructure: JSON-RPC discovery, native tool schemas, approval gates, UI-map tools, project context, diagnostics, session/checkpoint metadata, audit artifacts, and visual-validation harnesses. Native tool adapters now cover multiple PCB and schematic mutations, but a provider model loop is not yet enabled. Agent Settings has a masked process-memory-only credential field, while provider network execution remains disabled. `HttpClient` remains a stub, so a key does not yet make a remote model callable.

### Status correction after Sprints 568–573

The preceding paragraph is the pre-runtime baseline and must not be read as saying no orchestration code exists. `src/ccad_agent/orchestrator.py` now uses LangGraph for supervisor/router/librarian graph execution, supports an explicit offline `mock` provider, emits correlated tool calls, accepts redacted broker `tool_result` acknowledgements, and attaches opt-in Langfuse/LangSmith callbacks to graph runs. A live subprocess smoke has proven mock chat, one `ui.place_via` ToolNode pass, provider-key handoff, provider reset, and tool-result acknowledgement.

This still falls short of a usable autonomous agent. The mock provider is deterministic test infrastructure, not intelligence; real provider construction is not authenticated or network-verified; AgentPanel currently executes the C++ broker result but Python only acknowledges it rather than resuming a pending graph run; there is no durable async worker, budget enforcement, sandbox, context fabric, failure router, or conditional signoff gate. Thus the accurate classification is “early executable orchestration slice,” not “no orchestration,” and not “production agent.”

The harness must be treated as a first-class runtime boundary. It selects context, exposes tools, enforces permissions, records observations and failures, persists resumable state, and verifies results. Model requests may be stateless; CCad session state must therefore live in explicit project revisions, context snapshots, tool results, diagnostics, UI epochs, approvals, and evidence artifacts. Human edits and agent edits must enter one revision/event stream so the agent can observe user changes rather than operate on stale context.

## Consequences for roadmap

Near-term work must prioritize one secure provider runtime, one canonical context assembler, truthful readiness/error states, and end-to-end tests proving that a human edit is observed by the next agent turn. Schematic usability should advance as a real editor workflow, not as more method catalogs. MCP is a compatibility surface for external harnesses, not a substitute for the kernel contract; add stdio/server lifecycle only after schemas, permissions, context snapshots, and audit records are stable.

Every feature claim should identify whether it is native CCad behavior, delegated KiCad evidence, local harness metadata, or planned work. Demos should show one understandable engineering task with intent, mutation, failure, repair, and proof; they should not present a command list as product capability.

## Review-derived backlog

Implement provider transport and secret-store integration without project-file or trace leakage; replace the empty HTTP client; define provider-neutral request/response and tool-call contracts; consume broker results and resume pending LangGraph runs; add context-budget and revision-delta assembly; connect AgentPanel to real approved tool execution; add human-edit observation tests; build a usable schematic canvas/editor; improve agent chat layout and activity timeline; add input guardrails, failure classification, retry/circuit-breaker, budget enforcement, sandboxing, and intent-conditioned verification; add MCP stdio/server configuration with lifecycle and approval semantics; deepen router and zone-fill behavior; implement native manufacturing/3D/import/export capabilities or label delegated paths explicitly; add release artifacts, reproducible setup, and external-user validation.

## Diagram review disposition

The June orchestration Mermaid is aspirational, not reverse-engineered current architecture. Generic nodes such as accessibility/citation checks, context fabric, secret vault, sandbox, failure router, and side-effect gate have no shipped CCad counterpart. `Tool Registry`, `EDAReview`, `VisualQA`, `Evidence/Checkpoint`, and `BuildCheck` are partial; LangGraph execution and opt-in tracing are now real but early. The diagram must be relabeled “target architecture” and each node mapped to a source file and test before being used in a demo. Approval authority must sit before every mutating tool call, while final verification remains a separate intent-conditioned gate; learning/memory writes and budget stop conditions must be drawn explicitly.
