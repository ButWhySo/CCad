"""Keep provider presets, docs, and compatibility claims synchronized."""

from pathlib import Path

root = Path(__file__).resolve().parents[1]
settings = (root / "src" / "ccad_gui" / "agent_settings_dialog.cpp").read_text(encoding="utf-8")
orchestrator = (root / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
compat = (root / "docs" / "research" / "provider-model-compatibility.md").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")

official_qwen = "qwen-3-235b-a22b-instruct-2507"
production_model = "gpt-oss-120b"
stale_model = "qwen-3.8-27b"
gemini_25_text = ("gemini-2.5-flash", "gemini-2.5-flash-lite", "gemini-2.5-pro")

for document in (settings, compat, readme):
    assert official_qwen in document
    assert production_model in document
    assert stale_model not in document
assert official_qwen in orchestrator
for model in gemini_25_text:
    assert model in settings
    assert model in compat
for document in (settings, orchestrator, compat, readme):
    assert stale_model not in document
print("PASS provider catalog/runtime/docs consistency; no network")
