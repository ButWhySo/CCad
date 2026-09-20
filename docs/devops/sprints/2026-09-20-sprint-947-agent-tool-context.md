# Sprint 947: truthful agent execution and typed tool-context bridge

## Scope

Remove fabricated completion from the background `AgentRunner`, then continue
the active Agent tool/context work from a real GUI-map and transaction surface.
This sprint follows the supplied Agent UI parity contract in
`docs/product/agent-ui-parity-spec.md`.

## Completed slice: no executor means no execution

`AgentRunner` is a queue and scheduling primitive, not an execution engine by
itself. Previously, a runner without a `TaskExecutor` slept and marked a task
successful. That could report a completed read-only task even though no CCad
tool had run. The fallback is removed. A task without its orchestrator-owned
executor now terminates as `task_executor_unavailable`, makes no tool call,
makes no project change, and increments the goal failure count.

The runner contract tests now prove both paths. One injects an explicit test
executor and preserves the queued task identity/result; the other verifies the
no-executor terminal failure. Queue restoration supplies a real executor before
expecting a restored task to complete.

## Verification

With the Qt MinGW runtime on `PATH`, `ctest --test-dir build-qt -C Release -R
'^(agent_runner|job_manager)$' --output-on-failure` passed on 2026-09-20.

The Qt Agent-panel regression test also proves that a saved non-secret
provider/model selection is rendered before the Python backend starts. This
fixes the visual `Model: Auto` first frame without reading a Windows-vault
credential. The official UI-map target harness was rerun after the rebuild:
its Agent screenshot shows the persisted `gemini-2.5-flash` label, the child
backend ready, and a vault-loaded credential status with no stderr output.

Several old offline tests were aligned with the current provider contracts,
not changed to mask runtime errors. They now cover the public Cerebras catalog
endpoint, the explicit local Ollama catalog, OpenRouter's `openrouter/free`
default, a zero-retry normal chat policy, the deferred provider activation
boundary, and the grouped transient-provider probe dispatch.

## Next bounded slice

The Python provider graph currently publishes a small handwritten tool subset,
while the live Qt UI map already publishes a typed method catalog including
schemas and safety flags. Replace that split catalog with broker-mediated,
catalog-derived read and proposal tools. Read calls must remain immediate;
mutations must still create one native approval bound to the typed request.
This is not complete until a real provider can inspect the exact catalog and
context, invoke a read method, and stage rather than execute a mutation.
