"""Offline contract for runtime Langfuse configuration and redaction."""

import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))

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
    }, {"public_key": "pk-lf-test-not-real", "secret_key": "sk-lf-test-not-real"})
    assert configured["configured"] is True
    assert configured["exporter_initialized"] is True, configured
    assert len(runtime.callbacks()) == 1
    runtime.shutdown()

    protected = redact({
        "secret_key": "sk-lf-private-value",
        "authorization": "Bearer value",
        "normal": "pk-lf-public-value",
        "nested": ["AIzaGoogleKey", "safe"],
    })
    assert protected["secret_key"] == "[REDACTED]"
    assert protected["authorization"] == "[REDACTED]"
    assert protected["normal"] == "[REDACTED]"
    assert protected["nested"][0] == "[REDACTED]"
    assert protected["nested"][1] == "safe"
    print("agent observability runtime contract passed")


if __name__ == "__main__":
    main()
