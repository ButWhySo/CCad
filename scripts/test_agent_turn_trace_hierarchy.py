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
    disabled_runtime = TelemetryRuntime()
    disabled_runtime._last_trace_id = "stale-trace"
    assert not disabled_runtime.start_agent_turn("thread-disabled")
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

    assert runtime.start_agent_turn(
        "thread-contract",
        {"workflow": "contract", "turn_id": "turn-contract"},
        input_data={"prompt_sha256": "a" * 64, "prompt_chars": 12},
    )
    root_trace = runtime.current_trace()["trace_id"]
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
    with runtime.observation("context.assemble", "chain"):
        with runtime.observation("memory.retrieve", "retriever",
                                 retrieval_metadata):
            pass
        with runtime.observation("context.package", "span"):
            pass
    with runtime.session("thread-contract"), runtime.observation("agent-turn", "agent"):
        pass
    flushed = runtime.flush_turn()
    assert flushed["last_export"] == "queued"
    assert flushed["trace_id"] == root_trace
    assert not runtime.finish_agent_turn()
    assert provider.force_flush(timeout_millis=3000)

    spans = exporter.get_finished_spans()
    by_name = {span.name: span for span in spans}
    expected = {"agent.turn", "context.assemble", "memory.retrieve",
                "context.package", "agent-turn"}
    assert expected <= by_name.keys(), sorted(by_name)
    trace_id = int(root_trace, 16)

    def span_context(name: str):
        context = by_name[name].get_span_context()
        assert context is not None
        return context

    contexts = {name: span_context(name) for name in expected}
    assert {context.trace_id for context in contexts.values()} == {trace_id}

    def parent_span_id(name: str) -> int:
        parent = by_name[name].parent
        assert parent is not None
        return parent.span_id

    assert parent_span_id("context.assemble") == contexts["agent.turn"].span_id
    assert parent_span_id("memory.retrieve") == contexts["context.assemble"].span_id
    assert parent_span_id("context.package") == contexts["context.assemble"].span_id
    assert parent_span_id("agent-turn") == contexts["agent.turn"].span_id

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
    assert json.loads(root_attrs["langfuse.observation.input"]) == {
        "prompt_sha256": "a" * 64, "prompt_chars": 12,
    }, root_attrs
    assert json.loads(root_attrs["langfuse.observation.output"]) == {
        "terminal_state": "closed",
    }, root_attrs
    for span in spans:
        attrs = span.attributes or {}
        assert attrs.get("langfuse.session.id", attrs.get("session.id")) == "thread-contract", (span.name, attrs)
        assert attrs["langfuse.trace.name"] == "ccad.agent.turn", (span.name, attrs)
        assert attrs["langfuse.trace.metadata.ccad_turn_id"] == "turn-contract", (span.name, attrs)

    client.shutdown()
    provider.shutdown()
    print("agent turn trace hierarchy contract passed")


if __name__ == "__main__":
    main()
