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


captured = []
orchestrator.emit = captured.append
orchestrator.emit_provider_failure(
    "cerebras", AuthenticationError("invalid api key sk-secret-must-not-leak"))
event = captured[0]
params = event["params"]
assert event["method"] == "provider_state"
assert params["provider"] == "cerebras"
assert params["error_category"] == "authentication"
assert params["secret_value_visible"] is False
assert all("sk-secret-must-not-leak" not in json.dumps(item)
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
print("PASS provider failure event is classified and redacted; no network")
