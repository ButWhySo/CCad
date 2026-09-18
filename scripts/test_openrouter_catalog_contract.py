"""No-network contract for explicit OpenRouter model refresh."""

from pathlib import Path

source = (Path(__file__).resolve().parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert "def fetch_openrouter_models():" in source
assert '"https://openrouter.ai/api/v1/models"' in source
assert 'method == "agent.list_models"' in source
assert '"network_access": "explicit_refresh"' in source
assert "urllib.request.urlopen(request, timeout=timeout)" in source
assert "invalid_catalog_shape" in source
assert "except ValueError:" in source
print("PASS explicit OpenRouter catalog refresh contract; no network")
