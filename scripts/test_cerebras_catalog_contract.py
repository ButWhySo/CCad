"""Verify Cerebras public model refresh is explicit, bounded, and keyless."""

from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = "\n".join((root / "src" / "ccad_agent" / name).read_text(encoding="utf-8")
                   for name in ("orchestrator.py", "model_catalog.py", "method_catalog.py"))
assert "def fetch_cerebras_models():" in source
assert 'source_url = "https://api.cerebras.ai/public/v1/models"' in source
assert '"User-Agent": "CCad/1.0 (+https://github.com/ButWhySo/CCad)"' in source
assert '"network_access": "explicit_refresh"' in source
assert '"source_kind": "provider_api"' in source
assert 'method == "agent.list_models"' in source
assert '"cerebras": "explicit_refresh"' in source
assert "urllib.request.urlopen(request, timeout=catalog_timeout_seconds())" in source
assert "invalid_catalog_shape" in source
print("PASS explicit Cerebras catalog refresh contract; no network")
