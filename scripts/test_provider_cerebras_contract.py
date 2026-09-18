"""No-network Cerebras contract test; never consumes provider quota."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")

assert 'provider == "cerebras"' in source
assert 'base_url = "https://api.cerebras.ai/v1"' in source
assert 'model_name = model_name or "qwen-3-235b-a22b-instruct-2507"' in source
assert 'os.environ.get("CCAD_CEREBRAS_MODEL") or model_name' in source
assert '"cerebras": "CEREBRAS_API_KEY"' in source
assert '"cerebras": "CCAD_CEREBRAS_MODEL"' in source
assert 'Never multiply quota/credit failures' in source
assert 'status in (402, 403, 429)' in source
print("PASS Cerebras endpoint, key alias, and Qwen fallback contract; no network")
