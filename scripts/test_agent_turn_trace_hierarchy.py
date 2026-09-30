"""Verify per-turn Langfuse nesting with the real SDK and a local exporter."""
# pyright: reportMissingImports=false

from __future__ import annotations

import json
import pathlib
import sys
from typing import Any, cast

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))

from langfuse import Langfuse
from opentelemetry.sdk.resources import Resource
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export.in_memory_span_exporter import InMemorySpanExporter

from telemetry import TelemetryRuntime
from context_broker import memory_retrieval_trace_metadata


def main() -> None:
    orchestrator_source = (ROOT / "src" / "ccad_agent" / "orchestrator.py").read_text(
        encoding="utf-8")
    assert "finish=not waiting_for_tool_result" in orchestrator_source
    assert 'status().get("turn_state") == "active"' in orchestrator_source
    assert '"approval.resolve", "span"' in orchestrator_source
    assert 'turn_is_terminal = not bool(resumed_snapshot.next)' in orchestrator_source

    disabled_runtime = TelemetryRuntime()
    disabled_runtime._last_trace_id = "stale-trace"
    assert not disabled_runtime.start_agent_turn("thread-disabled")
    assert disabled_runtime.status()["turn_state"] == "active"
    assert disabled_runtime.status()["trace_id"] == "unavailable"
    assert not disabled_runtime.finish_agent_turn("failed")
    assert disabled_runtime.status()["turn_state"] == "failed"
    assert disabled_runtime.current_trace()["trace_id"] == "unavailable"

    exporter = InMemorySpanExporter()
    provider = TracerProvider(resource=Resource({"service.name": "ccad-test"}))
    client = Langfuse(
        public_key="pk-lf-local-contract",
        secret_key="local-contract-secret",
        base_url="http://localhost:3000",
        tracer_provider=provider,
        span_exporter=exporter,
        flush_interval=1,
        tracing_enabled=True,
    )
    runtime = TelemetryRuntime()
    runtime._provider = provider
    runtime._exporter = cast(Any, exporter)
    runtime._langfuse_client = client
    test_exporter = cast(Any, exporter)
    test_exporter.last_result = None
    test_exporter.last_span_count = 0
    runtime._development_logging = False

    assert runtime.run_in_turn_context(
        runtime.start_agent_turn,
        "thread-contract",
        {"workflow": "contract", "turn_id": "turn-contract"},
        input_data={"prompt_sha256": "a" * 64, "prompt_chars": 12},
    )
    root_trace = runtime.current_trace()["trace_id"]
    active_status = runtime.status()
    assert active_status["turn_state"] == "active", active_status
    assert active_status["trace_id"] == root_trace, active_status
    assert active_status["last_export"] == "pending", active_status
    retrieval_metadata = memory_retrieval_trace_metadata({
        "cache_hit": False, "memory_chars": 42, "memory_token_budget": 1000,
        "memories": [{"id": "private-memory-id", "content": "private memory text"}],
        "memory_retrieval_status": {
            "status": "ready", "channels": {"lexical": "ready",
                                                "semantic": "disabled"}},
        "manifest": {"tiers": {"ltm": {"enabled": True}},
                     "available_tier_count": 1,
                     "semantic_retrieval_ready": False},
        "memory_retrieval": [{"entry_id": "private-memory-id",
                               "inclusion_channels": ["automatic_retrieval"]}],
    })
    def assemble_context() -> None:
        with runtime.observation("context.assemble", "chain"):
            with runtime.observation("memory.retrieve", "retriever",
                                     retrieval_metadata):
                pass
            with runtime.observation("context.package", "span"):
                pass
        with runtime.session("thread-contract"), runtime.observation(
                "agent-turn", "agent"):
            pass

    runtime.run_in_turn_context(assemble_context)
    # A LangGraph checkpoint can return control to Qt while waiting for tool
    # approval. Flushing that partial run must not end its root: the later
    # broker execution and graph resume belong to the same user turn.
    paused = runtime.run_in_turn_context(runtime.flush_turn, finish=False)
    assert paused["turn_state"] == "awaiting_tool_result", paused
    assert paused["trace_id"] == root_trace
    assert runtime.current_trace()["trace_id"] == root_trace

    # Unrelated main-loop requests run outside a paused turn and cannot become
    # accidental children of its still-open root.
    with runtime.observation("unrelated.status.request", "span") as unrelated:
        assert unrelated is not None
        unrelated_trace = str(unrelated.trace_id)
    assert unrelated_trace != root_trace

    def resume_tool_and_close() -> dict[str, Any]:
        with runtime.observation("approval.resolve", "span", {"decision": "approved"}):
            with runtime.observation("dispatch-native-tool", "tool", {"result": "performed"}):
                pass
        return runtime.flush_turn()

    flushed = runtime.run_in_turn_context(resume_tool_and_close)
    assert flushed["turn_state"] == "completed", flushed
    assert flushed["last_export"] == "queued"
    assert flushed["trace_id"] == root_trace
    assert runtime.current_trace()["trace_id"] == root_trace
    assert provider.force_flush(timeout_millis=3000)

    spans = exporter.get_finished_spans()
    by_name = {span.name: span for span in spans}
    expected = {"agent.turn", "context.assemble", "memory.retrieve",
                "context.package", "agent-turn", "approval.resolve",
                "dispatch-native-tool", "unrelated.status.request"}
    assert expected <= by_name.keys(), sorted(by_name)
    trace_id = int(root_trace, 16)

    def span_context(name: str):
        context = by_name[name].get_span_context()
        assert context is not None
        return context

    contexts = {name: span_context(name) for name in expected}
    assert contexts["unrelated.status.request"].trace_id != trace_id
    assert {context.trace_id for name, context in contexts.items()
            if name != "unrelated.status.request"} == {trace_id}

    def parent_span_id(name: str) -> int:
        parent = by_name[name].parent
        assert parent is not None
        return parent.span_id

    assert parent_span_id("context.assemble") == contexts["agent.turn"].span_id
    assert parent_span_id("memory.retrieve") == contexts["context.assemble"].span_id
    assert parent_span_id("context.package") == contexts["context.assemble"].span_id
    assert parent_span_id("agent-turn") == contexts["agent.turn"].span_id
    assert parent_span_id("approval.resolve") == contexts["agent.turn"].span_id
    assert parent_span_id("dispatch-native-tool") == contexts["approval.resolve"].span_id
    assert by_name["unrelated.status.request"].parent is None

    memory_attrs = by_name["memory.retrieve"].attributes or {}
    assert memory_attrs["langfuse.observation.metadata.retrieval_status"] == "ready"
    assert memory_attrs["langfuse.observation.metadata.retrieval_lexical_status"] == "ready"
    assert memory_attrs["langfuse.observation.metadata.retrieval_semantic_status"] == "disabled"
    assert memory_attrs["langfuse.observation.metadata.retrieved_record_count"] == "1"
    assert memory_attrs["langfuse.observation.metadata.candidate_automatic_retrieval_count"] == "1"
    assert "private-memory-id" not in str(memory_attrs)
    assert "private memory text" not in str(memory_attrs)

    root = by_name["agent.turn"]
    root_attrs = root.attributes or {}
    assert json.loads(str(root_attrs["langfuse.observation.input"])) == {
        "prompt_sha256": "a" * 64, "prompt_chars": 12,
    }, root_attrs
    assert json.loads(str(root_attrs["langfuse.observation.output"])) == {
        "terminal_state": "completed",
    }, root_attrs
    for span in spans:
        if span.name == "unrelated.status.request":
            continue
        attrs = span.attributes or {}
        assert attrs.get("langfuse.session.id", attrs.get("session.id")) == "thread-contract", (span.name, attrs)
        assert attrs["langfuse.trace.name"] == "ccad.agent.turn", (span.name, attrs)
        assert attrs["langfuse.trace.metadata.ccad_turn_id"] == "turn-contract", (span.name, attrs)

    client.shutdown()
    provider.shutdown()
    print("agent turn trace hierarchy contract passed")


if __name__ == "__main__":
    main()
