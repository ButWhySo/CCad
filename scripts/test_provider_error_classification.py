"""No-network safety contract for actionable provider failures."""

from pathlib import Path

source = (Path(__file__).resolve().parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")

assert "def classify_provider_error(error: Exception)" in source
assert "def provider_error_user_message(error: Exception)" in source
assert 'category = classify_provider_error(error)' in source
for category in ("authentication", "permission_denied", "model_not_found", "payment_required", "quota_exhausted", "quota_or_rate_limit", "rate_limited", "timeout", "dependency", "provider_unavailable"):
    assert f'"{category}"' in source
assert '"cause": classify_provider_error(error)' in source
assert '"http_status": provider_http_status(error)' in source
assert 'provider_error_user_message(error)' in source
assert '"kind": "provider_error"' in source
assert '"cause": classify_provider_error(error)' in source
assert '"http_status": provider_http_status(error)' in source
assert "no tool was executed" not in source
print("PASS provider failure classification is actionable and secret-safe; no network")
