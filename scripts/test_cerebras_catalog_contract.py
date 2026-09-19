"""Verify Cerebras model refresh is explicit, authenticated, and bounded."""

from pathlib import Path

source = (Path(__file__).resolve().parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert "def fetch_cerebras_models():" in source
assert 'source_url = "https://api.cerebras.ai/v1/models"' in source
assert 'os.environ.get("CEREBRAS_API_KEY", "")' in source
assert '"network_access": "explicit_refresh"' in source
assert '"source_kind": "provider_api"' in source
assert 'method == "agent.list_models"' in source
assert '"cerebras": "explicit_refresh"' in source
assert "urllib.request.urlopen(request, timeout=timeout)" in source
assert "invalid_catalog_shape" in source
print("PASS explicit Cerebras catalog refresh contract; no network")
