"""No-network OpenRouter provider contract."""

from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = (root / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
gui = (root / "src" / "ccad_gui" / "agent_settings_dialog.cpp").read_text(encoding="utf-8")
panel = (root / "src" / "ccad_gui" / "agent_panel.cpp").read_text(encoding="utf-8")
cli = (root / "src" / "ccad_cli" / "agent_provider_config.cpp").read_text(encoding="utf-8")

assert 'provider == "openrouter"' in source
assert 'base_url = "https://openrouter.ai/api/v1"' in source
assert 'model_name = model_name or "openrouter/auto"' in source
assert '"openrouter": "OPENROUTER_API_KEY"' in source
assert '"OpenRouter", "openrouter"' in gui
assert '"openrouter/auto"' in gui
assert '"OPENROUTER_API_KEY"' in panel
assert '"openrouter_api"' in cli
print("PASS OpenRouter endpoint/key/model contract; no network")
