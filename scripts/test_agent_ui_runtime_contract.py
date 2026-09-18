"""No-GUI source contract for chat stream and provider runtime fixes."""

from pathlib import Path


root = Path(__file__).parents[1]
panel = (root / "src" / "ccad_gui" / "agent_panel.cpp").read_text(encoding="utf-8")
header = (root / "src" / "ccad_gui" / "agent_panel.hpp").read_text(encoding="utf-8")
settings = (root / "src" / "ccad_gui" / "agent_settings_dialog.cpp").read_text(encoding="utf-8")
review = (root / "src" / "ccad_gui" / "review_window.cpp").read_text(encoding="utf-8")

assert "QTextBrowser* chat_stream_" in header
assert 'setObjectName("control:agent_chat_stream")' in panel
assert "cursor.insertText(prefix +" in panel
assert "setTextInteractionFlags(Qt::TextSelectableByMouse" in panel
assert "repo_src + \"/ccad_agent/venv/Scripts/python.exe\"" in panel
assert 'agent_env.insert("PYTHONNOUSERSITE", "1")' in panel
assert "LogonUserW(" in settings
assert "Windows password for '%1'" in settings
assert "password.fill(QChar(u'\\0'))" in settings
assert "CredUIPromptForCredentialsW" not in settings
assert 'sendJsonRpc("agent.list_models"' in settings
assert 'action:refreshModelCatalog' in settings
assert 'provider_models' in panel
assert 'setModelCatalogCallback' in settings
assert 'applyModelCatalog' in settings
assert 'catalog["ok"].toBool(false)' in settings
assert 'catalog["models"].toArray()' in settings
assert 'refresh_models->setEnabled(provider_combo_->currentData().toString() == "openrouter")' in settings
assert 'combo_selection_selected' in review
assert 'object->value("value").toString()' in review
assert 'object->value("text").toString()' in review
print("PASS agent UI/provider runtime source contract; no GUI launched")
