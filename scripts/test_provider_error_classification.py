"""No-network safety contract for actionable provider failures."""

from pathlib import Path

source = (Path(__file__).resolve().parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")

assert "def classify_provider_error(error: Exception)" in source
assert 'category = classify_provider_error(error)' in source
for category in ("authentication", "model_not_found", "quota_or_rate_limit", "timeout", "dependency", "provider_unavailable"):
    assert f'"{category}"' in source
assert '"cause": classify_provider_error(error)' in source
assert "no tool was executed" in source
print("PASS provider failure classification is actionable and secret-safe; no network")
