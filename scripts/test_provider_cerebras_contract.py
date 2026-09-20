"""No-network Cerebras contract test; never consumes provider quota."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")

assert 'provider == "cerebras"' in source
assert 'base_url = "https://api.cerebras.ai/v1"' in source
assert '"X-Cerebras-3rd-Party-Integration": "langgraph"' in source
assert 'CCAD_CEREBRAS_REASONING_EFFORT' in source
assert '"CCAD_CEREBRAS_REASONING_EFFORT"' in source[source.index("test_env_names"):source.index("saved_test_env")]
assert '"qwen-3.8-27b": {"none", "low", "medium", "high"}' in source
assert 'model_name = model_name or "gpt-oss-120b"' in source
assert 'os.environ.get("CCAD_CEREBRAS_MODEL") or model_name' in source
assert '"cerebras": "CEREBRAS_API_KEY"' in source
assert '"cerebras": "CCAD_CEREBRAS_MODEL"' in source
assert 'Never multiply quota/credit failures' in source
assert 'if status == 402 or any(marker in text for marker in' in source
assert 'if status == 429 or any(marker in text for marker in' in source
print("PASS Cerebras endpoint, key alias, and production fallback contract; no network")
