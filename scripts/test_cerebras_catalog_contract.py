"""Verify Cerebras public model refresh is explicit, bounded, and keyless."""

from pathlib import Path

source = (Path(__file__).resolve().parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert "def fetch_cerebras_models():" in source
assert 'source_url = "https://api.cerebras.ai/public/v1/models"' in source
assert '"User-Agent": "CCad/1.0 (+https://github.com/ButWhySo/CCad)"' in source
assert '"network_access": "explicit_refresh"' in source
assert '"source_kind": "provider_api"' in source
assert 'method == "agent.list_models"' in source
assert '"cerebras": "explicit_refresh"' in source
assert "urllib.request.urlopen(request, timeout=timeout)" in source
assert "invalid_catalog_shape" in source
print("PASS explicit Cerebras catalog refresh contract; no network")
