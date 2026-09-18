"""No-network contract: provider defaults must match current preset catalog."""

from pathlib import Path

root = Path(__file__).resolve().parents[1]
orchestrator = (root / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
config = (root / "src" / "ccad_agent" / "config.py").read_text(encoding="utf-8")

assert 'model_name = "claude-opus-5"' in orchestrator
assert 'model_name = "gemini-3.8-flash"' in orchestrator
assert 'model_name = model_name or "gpt-5.1"' in orchestrator
assert '"model": "gpt-5.1"' in config
assert '"method": "backend_state"' in orchestrator
assert '"provider_initialized": bool(provider_initialized)' in orchestrator
for stale in ("claude-3-opus-20240229", "gemini-1.5-pro-latest"):
    assert stale not in orchestrator
print("PASS provider runtime defaults match current model catalog; no network")
