"""Keep provider presets, docs, and compatibility claims synchronized."""

from pathlib import Path

root = Path(__file__).resolve().parents[1]
settings = (root / "src" / "ccad_gui" / "agent_settings_dialog.cpp").read_text(encoding="utf-8")
orchestrator = (root / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
compat = (root / "docs" / "research" / "provider-model-compatibility.md").read_text(encoding="utf-8")
parity = (root / "docs" / "product" / "agent-ui-parity-spec.md").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")

production_model = "gpt-oss-120b"
small_production_model = "qwen-3.8-27b"
stale_model = "llama3.1-8b"
stale_preview_model = "zai-glm-4.7"
stale_anthropic_model = "claude-fable-5-1"
gemini_25_text = ("gemini-2.5-flash", "gemini-2.5-flash-lite", "gemini-2.5-pro")

for document in (settings, compat, parity, readme):
    assert production_model in document
    assert stale_model not in document
assert small_production_model in settings
assert small_production_model in parity
assert small_production_model in settings
assert "current public model catalog" in compat
assert stale_anthropic_model not in settings
assert stale_anthropic_model not in compat
assert production_model in orchestrator
for model in gemini_25_text:
    assert model in settings
    assert model in compat
for document in (settings, orchestrator, compat, parity, readme):
    assert stale_model not in document
    assert stale_preview_model not in document
print("PASS provider catalog/runtime/docs consistency; no network")
