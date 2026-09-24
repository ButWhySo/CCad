"""No-network contract for explicit OpenRouter model refresh."""

from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = "\n".join((root / "src" / "ccad_agent" / name).read_text(encoding="utf-8")
                   for name in ("orchestrator.py", "model_catalog.py", "method_catalog.py"))
assert "def fetch_openrouter_models():" in source
assert '"https://openrouter.ai/api/v1/models"' in source
assert 'method == "agent.list_models"' in source
assert '"name": "agent.list_models"' in source
assert '"network_access": "explicit_refresh"' in source
assert '"providers": ["openai", "anthropic", "google_gemini", "openrouter", "cerebras", "ollama"]' in source
assert '"response": {"method": "provider_models"' in source
assert "urllib.request.urlopen(request, timeout=catalog_timeout_seconds())" in source
assert "invalid_catalog_shape" in source
assert "except ValueError:" in source
assert '"error": "unsupported_provider"' in source
assert '"error_detail": "model catalog is unavailable for this provider"' in source
assert '"network_access": "explicit_refresh"' in source
print("PASS explicit OpenRouter catalog refresh contract; no network")
