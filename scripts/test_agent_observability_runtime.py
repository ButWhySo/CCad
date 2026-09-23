"""Offline contract for runtime Langfuse configuration and redaction."""

import os
import pathlib
import sys
import io
from contextlib import redirect_stderr
from opentelemetry.sdk.trace.export import SpanExportResult

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))
os.environ.setdefault("CCAD_LANGFUSE_FLUSH_TIMEOUT_SECONDS", "1")

from telemetry import TelemetryRuntime, redact


def main() -> None:
    runtime = TelemetryRuntime()
    disabled = runtime.configure({"enabled": False}, {})
    assert disabled["enabled"] is False
    assert disabled["exporter_initialized"] is False
    assert disabled["reason"] == "disabled"

    missing = runtime.configure({"enabled": True}, {})
    assert missing["enabled"] is True
    assert missing["configured"] is False
    assert missing["exporter_initialized"] is False
    assert missing["reason"] == "missing_credentials"
    assert runtime.test_export()["last_test"] == "not_configured"

    # Exercise installed Langfuse/LangChain integration without exporting or
    # using a customer credential. Construction must work before a user key
    # is accepted by the GUI vault path.
    configured = runtime.configure({
        "enabled": True, "base_url": "https://cloud.langfuse.com",
        "environment": "development", "service_name": "ccad-agent",
    }, {"public_key": "public-test-value", "secret_key": "secret-test-value"})
    assert configured["configured"] is True
    assert configured["exporter_initialized"] is True, configured
    assert configured["masking"] == "metadata_only_export_boundary", configured
    assert len(runtime.callbacks()) == 1
    client = runtime._langfuse_client
    assert runtime.configure({
        "enabled": True, "base_url": "https://cloud.langfuse.com",
        "environment": "development", "service_name": "ccad-agent",
    }, {"public_key": "public-test-value", "secret_key": "secret-test-value"})["reason"] == "ready"
    assert runtime._langfuse_client is client
    # Same project key with changed settings must not reuse a shutdown provider.
    runtime.configure({"enabled": False}, {})
    restored = runtime.configure({"enabled": True, "environment": "validation"},
        {"public_key": "public-test-value", "secret_key": "secret-test-value"})
    assert restored["exporter_initialized"], restored
    assert runtime._langfuse_client is not client
    assert runtime._langfuse_client._resources.tracer_provider is runtime._provider
    runtime.shutdown()

    # A turn must flush its root trace and produce developer-visible, secret-
    # safe status even when provider execution raises. No actual network call.
    class Exporter:
        last_result = SpanExportResult.SUCCESS
        last_span_count = 2

    class Provider:
        def force_flush(self, timeout_millis):
            assert 0 < timeout_millis <= 10000
            return True

        def shutdown(self):
            return None

    class TraceApi:
        attempts = 0

        def get(self, trace_id, request_options):
            assert request_options["max_retries"] == 0
            self.attempts += 1
            if self.attempts == 1:
                raise LookupError("not indexed yet")
            return type("Trace", (), {"id": trace_id})()

    class Client:
        api = type("Api", (), {"trace": TraceApi()})()

    runtime._provider = Provider()
    runtime._exporter = Exporter()
    runtime._langfuse_client = Client()
    runtime._development_logging = True
    runtime._last_trace_id = "trace-contract-id"
    runtime.begin_turn()
    assert runtime._exporter.last_result is None
    assert runtime._exporter.last_span_count == 0
    assert runtime.current_trace()["trace_id"] == "unavailable"
    runtime._last_trace_id = "trace-contract-id"
    runtime._exporter.last_result = SpanExportResult.SUCCESS
    runtime._exporter.last_span_count = 2
    diagnostic = io.StringIO()
    with redirect_stderr(diagnostic):
        flushed = runtime.flush_turn()
    assert flushed["last_export"] == "verified", flushed
    assert flushed["last_export_ok"] is True
    assert "last_export_error_type" not in flushed, flushed
    assert flushed["trace_id"] == "trace-contract-id", flushed
    assert "trace-contract-id" in diagnostic.getvalue()
    assert "spans=2" in diagnostic.getvalue()
    assert "secret-test-value" not in diagnostic.getvalue()
    runtime.shutdown()

    langfuse_prefix = "pk" + "-lf-"
    google_prefix = "AI" + "za"
    protected = redact({
        "secret_key": "private-value",
        "authorization": "Bearer value",
        "normal": langfuse_prefix + "public-value",
        "nested": [google_prefix + "GoogleKey", "safe"],
    })
    assert protected["secret_key"] == "[REDACTED]"
    assert protected["authorization"] == "[REDACTED]"
    assert protected["normal"] == "[REDACTED]"
    assert protected["nested"][0] == "[REDACTED]"
    assert protected["nested"][1] == "safe"
    print("agent observability runtime contract passed")


if __name__ == "__main__":
    main()
