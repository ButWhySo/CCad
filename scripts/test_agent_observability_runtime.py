"""Offline contract for runtime Langfuse configuration and redaction."""

import os
import pathlib
import sys

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
