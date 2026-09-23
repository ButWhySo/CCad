"""No-network proof for the emitted provider failure event."""

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))
import orchestrator  # noqa: E402


class AuthenticationError(RuntimeError):
    status_code = 401


class RateLimitError(RuntimeError):
    status_code = 429


class GeminiQuotaError(RuntimeError):
    pass


class StatusError(RuntimeError):
    def __init__(self, status_code, message):
        super().__init__(message)
        self.status_code = status_code


captured = []
orchestrator.emit = captured.append
redaction_probe = "sk-" + "fixture-value-not-a-credential"
orchestrator.emit_provider_failure(
    "cerebras", AuthenticationError(f"invalid api key {redaction_probe}"))
event = captured[0]
params = event["params"]
assert event["method"] == "provider_state"
assert params["provider"] == "cerebras"
assert params["error_category"] == "authentication"
assert params["secret_value_visible"] is False
assert all(redaction_probe not in json.dumps(item)
           for item in captured)
source = (ROOT / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert '"kind": "provider_error"' in source
assert 'failure_category = classify_provider_error(error)' in source
assert '"category": failure_category' in source
assert '"secret_value_visible": False' in source

captured.clear()
orchestrator.emit_provider_failure("cerebras", RateLimitError("not exposing details"))
assert captured[0]["params"]["error_category"] == "rate_limited"
assert orchestrator.provider_http_status(RateLimitError("safe")) == 429

captured.clear()
orchestrator.emit_provider_failure(
    "google_gemini",
    GeminiQuotaError("ResourceExhausted: 429 You exceeded your current quota"),
)
assert captured[0]["params"]["error_category"] == "quota_exhausted"

# LangGraph/LangChain can wrap the original provider exception. Classification
# must retain the nested quota/status rather than collapsing it to unavailable.
try:
    raise GeminiQuotaError(
        f"ResourceExhausted: 429 You exceeded your current quota; key={redaction_probe}")
except GeminiQuotaError as cause:
    wrapped = RuntimeError("model generation failed")
    wrapped.__cause__ = cause
assert orchestrator.classify_provider_error(wrapped) == "quota_exhausted"
assert orchestrator.provider_http_status(wrapped) is None
user_message = orchestrator.provider_error_user_message(wrapped)
assert "quota" in user_message.lower()
assert "provider_unavailable" not in user_message
assert redaction_probe not in user_message
wrapped_status = RuntimeError("request failed")
wrapped_status.__cause__ = RateLimitError("HTTP rate limit")
assert orchestrator.classify_provider_error(wrapped_status) == "rate_limited"
assert orchestrator.provider_http_status(wrapped_status) == 429

connection_error = ConnectionError("connection refused")
assert orchestrator.classify_provider_error(connection_error) == "connection_error"
assert "connection" in orchestrator.provider_error_user_message(connection_error).lower()
for error, category in (
        (RuntimeError(f"API key missing: {redaction_probe}"), "missing_api_key"),
        (StatusError(401, "credential rejected"), "authentication"),
        (StatusError(403, "access denied"), "permission_denied"),
        (StatusError(404, "unknown model"), "model_not_found"),
        (StatusError(402, "billing required"), "payment_required"),
        (StatusError(429, "too many requests"), "rate_limited"),
        (TimeoutError("deadline exceeded"), "timeout"),
        (ImportError("adapter absent"), "dependency"),
        (RuntimeError("opaque failure"), "provider_unavailable")):
    assert orchestrator.classify_provider_error(error) == category
    assert category in orchestrator.provider_error_user_message(error)
print("PASS provider failure event is classified and redacted; no network")
