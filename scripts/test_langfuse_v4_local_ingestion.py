"""Exercise Langfuse v4 OTLP and LangChain callback against a local receiver."""
# pyright: reportMissingImports=false
from __future__ import annotations

import base64
import pathlib
import sys
import uuid
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from threading import Thread
from typing import Any

from opentelemetry.proto.collector.trace.v1.trace_service_pb2 import ExportTraceServiceRequest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))

from telemetry import TelemetryRuntime


class Receiver(BaseHTTPRequestHandler):
    requests: list[tuple[str, dict[str, str], bytes]] = []

    def do_POST(self) -> None:
        body = self.rfile.read(int(self.headers.get("Content-Length", "0")))
        type(self).requests.append((
            self.path,
            {key.lower(): value for key, value in self.headers.items()},
            body,
        ))
        self.send_response(200)
        self.send_header("Content-Type", "application/x-protobuf")
        self.end_headers()
        self.wfile.write(b"")

    def log_message(self, format: str, *_args: Any) -> None:
        return


def attributes(span) -> dict[str, Any]:
    values = {}
    for item in span.attributes:
        value = item.value
        if value.HasField("string_value"):
            values[item.key] = value.string_value
        elif value.HasField("bool_value"):
            values[item.key] = value.bool_value
        elif value.HasField("int_value"):
            values[item.key] = value.int_value
        elif value.HasField("array_value"):
            values[item.key] = tuple(element.string_value for element in value.array_value.values)
    return values


def main() -> None:
    Receiver.requests.clear()
    server = ThreadingHTTPServer(("127.0.0.1", 0), Receiver)
    worker = Thread(target=server.serve_forever, daemon=True)
    worker.start()
    port = server.server_address[1]
    public_key = "local-public-contract-id"
    secret_key = "local-ingestion-contract-sentinel"
    runtime = TelemetryRuntime()
    try:
        state = runtime.configure({
            "enabled": True,
            "base_url": f"http://127.0.0.1:{port}",
            "environment": "validation",
            "service_name": "ccad-agent-contract",
        }, {"public_key": public_key, "secret_key": secret_key})
        assert state["exporter_initialized"], state
        runtime._development_logging = False
        assert runtime.run_in_turn_context(runtime.start_agent_turn,
            "thread-local-contract",
            {"turn_id": "turn-local-contract"},
            input_data={"prompt_sha256": "b" * 64, "prompt_chars": 9},
        )
        def emit_turn_observations() -> None:
            callback = runtime.callbacks()[0]
            callback_run_id = uuid.uuid4()
            callback.on_chain_start(
                {"name": "ccad_callback_contract"},
                {"request_kind": "contract"},
                run_id=callback_run_id,
                metadata={"ccad_contract": "local"},
            )
            callback.on_chain_end(
                {"result_kind": "observed"}, run_id=callback_run_id)
            with runtime.observation("tool.call", "tool") as observation:
                assert observation is not None
                observation.update(output={"status": "observed"})
            with runtime.observation(
                "metadata.contract",
                metadata={"workflow": True, "result": 12, "tool": "x" * 240,
                          "nested": {"ignored": True}},
            ):
                pass
            assert runtime.finish_agent_turn()

        runtime.run_in_turn_context(emit_turn_observations)
        assert runtime._provider.force_flush(timeout_millis=6000)

        assert len(Receiver.requests) == 1, len(Receiver.requests)
        path, headers, body = Receiver.requests[0]
        assert path == "/api/public/otel/v1/traces", path
        assert headers["x-langfuse-ingestion-version"] == "4", headers
        expected_auth = "Basic " + base64.b64encode(
            f"{public_key}:{secret_key}".encode()).decode()
        assert headers["authorization"] == expected_auth
        assert "application/x-protobuf" in headers["content-type"]
        assert secret_key.encode() not in body

        request = ExportTraceServiceRequest.FromString(body)
        spans = [span for resource in request.resource_spans
                 for scope in resource.scope_spans for span in scope.spans]
        by_name = {span.name: span for span in spans}
        assert set(by_name) == {
            "agent.turn", "ccad_callback_contract", "tool.call",
            "metadata.contract"}, sorted(by_name)
        assert sum(span.name == "agent.turn" for span in spans) == 1
        root = by_name["agent.turn"]
        root_attrs = attributes(root)
        assert root_attrs.get("langfuse.observation.input", "").find("b" * 64) >= 0, root_attrs
        # These short in-process spans can begin and end within one clock tick.
        # Equal timestamps are valid; the contract is that export completed
        # without an end time preceding the start time.
        assert root.start_time_unix_nano > 0, root
        assert root.end_time_unix_nano >= root.start_time_unix_nano, root
        trace_ids = {span.trace_id for span in spans}
        assert len(trace_ids) == 1
        assert by_name["tool.call"].parent_span_id == root.span_id
        assert by_name["ccad_callback_contract"].parent_span_id == root.span_id
        for span in spans:
            attrs = attributes(span)
            assert attrs.get("langfuse.session.id", attrs.get("session.id")) == "thread-local-contract"
            assert attrs["langfuse.trace.name"] == "ccad.agent.turn"
            assert attrs["langfuse.trace.metadata.ccad_turn_id"] == "turn-local-contract"
        metadata_attrs = attributes(by_name["metadata.contract"])
        assert metadata_attrs["langfuse.observation.metadata.workflow"] == "true", metadata_attrs
        assert metadata_attrs["langfuse.observation.metadata.result"] == "12", metadata_attrs
        assert len(metadata_attrs["langfuse.observation.metadata.tool"]) == 200, metadata_attrs
        assert "langfuse.observation.metadata.nested" not in metadata_attrs, metadata_attrs
        print("Langfuse v4 local OTLP ingestion contract passed")
    finally:
        runtime.shutdown()
        server.shutdown()
        server.server_close()
        worker.join(timeout=2)


if __name__ == "__main__":
    main()
