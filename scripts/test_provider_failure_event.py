"""No-network proof for the emitted provider failure event."""

import json
import sys
from datetime import datetime, timezone, timedelta
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


class GoogleGrpcStatusError(RuntimeError):
    def __init__(self, message="RESOURCE_EXHAUSTED"):
        super().__init__(message)

    class Code:
        name = "RESOURCE_EXHAUSTED"
    code = Code()


class OpenAICompatibleError(RuntimeError):
    def __init__(self, status_code, body_code, message):
        super().__init__(message)
        self.status_code = status_code
        self.body = {"error": {"code": body_code, "message": message}}
        self.response = type("Response", (), {
            "status_code": status_code,
            "headers": {"retry-after": "17"},
            "json": lambda _self: self.body,
        })()


for error, category in (
        (OpenAICompatibleError(429, "insufficient_quota", "credits exhausted"), "quota_exhausted"),
        (OpenAICompatibleError(429, "rate_limit_exceeded", "slow down"), "rate_limited"),
        (StatusError(402, "billing_error"), "payment_required")):
    catalog = orchestrator.catalog_failure(
        "provider-test", error, "https://invalid.example/models", "explicit_refresh")
    assert catalog["error"] == category
    assert catalog["http_status"] == error.status_code


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
    "openai", OpenAICompatibleError(429, "rate_limit_exceeded", "slow down"))
assert captured[0]["params"]["http_status"] == 429
assert captured[0]["params"]["retry_after_seconds"] == 17

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

# SDK response metadata, not unsafe raw text, controls quota/payment diagnosis.
assert orchestrator.classify_provider_error(
    OpenAICompatibleError(429, "insufficient_quota", "quota reached")) == "quota_exhausted"
assert orchestrator.classify_provider_error(
    OpenAICompatibleError(429, "rate_limit_exceeded", "slow down")) == "rate_limited"
assert orchestrator.classify_provider_error(
    GoogleGrpcStatusError("RESOURCE_EXHAUSTED")) == "quota_or_rate_limit"
assert orchestrator.classify_provider_error(
    GoogleGrpcStatusError("RESOURCE_EXHAUSTED: rate limit exceeded")) == "rate_limited"
assert orchestrator.classify_provider_error(
    GoogleGrpcStatusError("RESOURCE_EXHAUSTED: daily quota exceeded")) == "quota_exhausted"
assert orchestrator.classify_provider_error(
    GeminiQuotaError("ResourceExhausted: 429 RESOURCE_EXHAUSTED")) == "quota_or_rate_limit"
assert "does not distinguish quota from request rate" in orchestrator.provider_error_user_message(
    GoogleGrpcStatusError("RESOURCE_EXHAUSTED"))
assert orchestrator.provider_retry_after_seconds(
    OpenAICompatibleError(429, "rate_limit_exceeded", "slow down")) == 17
assert orchestrator.provider_retry_after_seconds(
    OpenAICompatibleError(429, "rate_limit_exceeded", f"{redaction_probe}")) == 17
http_date = (datetime.now(timezone.utc) + timedelta(seconds=29)).strftime("%a, %d %b %Y %H:%M:%S GMT")
date_retry = type("DateRetry", (RuntimeError,), {
    "response": type("R", (), {"headers": {"Retry-After": http_date}})()})()
assert 0 <= orchestrator.provider_retry_after_seconds(date_retry) <= 30
assert orchestrator.provider_http_status(
    type("GrpcHttpStatus", (RuntimeError,), {"status": 429})()) == 429
assert orchestrator.provider_http_status(
    type("ResponseWrapped", (RuntimeError,), {"response": type("R", (), {"status_code": 402})()})()) == 402
wrapped_status = RuntimeError("request failed")
wrapped_status.__cause__ = RateLimitError("HTTP rate limit")
assert orchestrator.classify_provider_error(wrapped_status) == "rate_limited"
assert orchestrator.provider_http_status(wrapped_status) == 429

# Exercise the explicit Settings connection-result protocol at its provider
# boundary without making an HTTP request or changing persistent settings.
connection_error = OpenAICompatibleError(429, "rate_limit_exceeded", "slow down")
connection_previous = {
    "llm": orchestrator.llm,
    "init_provider": orchestrator.init_provider,
    "provider_connection_probe": orchestrator.provider_connection_probe,
    "emit": orchestrator.emit,
}
connection_events = []
def fail_connection_probe(_llm):
    raise connection_error

try:
    orchestrator.llm = object()
    orchestrator.init_provider = lambda: True
    orchestrator.provider_connection_probe = fail_connection_probe
    orchestrator.emit = connection_events.append
    orchestrator.handle_provider_and_state_request({
        "method": "agent.test_provider_connection",
        "params": {"provider": "openai_compatible", "model": "offline-contract"},
    }, None)
finally:
    orchestrator.llm = connection_previous["llm"]
    orchestrator.init_provider = connection_previous["init_provider"]
    orchestrator.provider_connection_probe = connection_previous["provider_connection_probe"]
    orchestrator.emit = connection_previous["emit"]
connection_result = next(event for event in connection_events
                         if event.get("method") == "provider_connection_result")
assert connection_result["params"]["error_category"] == "rate_limited"
assert connection_result["params"]["http_status"] == 429
assert connection_result["params"]["retry_after_seconds"] == 17
assert connection_result["params"]["network_access"] == "explicit_one_request"
assert connection_result["params"]["request_count"] == 1
assert connection_result["params"]["tool_executed"] is False
assert connection_result["params"]["secret_value_visible"] is False
assert "slow down" not in json.dumps(connection_events)

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
