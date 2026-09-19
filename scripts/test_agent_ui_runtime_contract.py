"""No-GUI source contract for chat stream and provider runtime fixes."""

from pathlib import Path


root = Path(__file__).parents[1]
panel = (root / "src" / "ccad_gui" / "agent_panel.cpp").read_text(encoding="utf-8")
header = (root / "src" / "ccad_gui" / "agent_panel.hpp").read_text(encoding="utf-8")
settings = (root / "src" / "ccad_gui" / "agent_settings_dialog.cpp").read_text(encoding="utf-8")
review = (root / "src" / "ccad_gui" / "review_window.cpp").read_text(encoding="utf-8")
main = (root / "src" / "ccad_gui" / "main.cpp").read_text(encoding="utf-8")

assert "QTextBrowser* chat_stream_" in header
assert 'setObjectName("control:agent_chat_stream")' in panel
assert 'if (!chat_stream_) return;' in panel
assert 'chat_history_layout_->addWidget(container);' not in panel
assert 'agentRole="toolCard"' not in panel
assert 'agentRole="chatBubbleUser"' not in panel
assert 'duplicate_backend_notice' in panel
assert 'collapse_btn->setText(QString::fromUtf8("\\xE2\\x88\\x92"));' in panel
assert 'role == "agent"' in panel
review = (root / "src" / "ccad_gui" / "review_window.cpp").read_text(encoding="utf-8")
assert 'agentRole="chatBubbleUser"' not in review
assert 'agentRole="toolCard"' not in review
assert '"method": "backend_state"' in (root / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert 'backend_provider_initialized_' in panel
assert '"backend_ready"' in panel
assert 'chat_history_layout_->addWidget(bubble);' not in panel
assert 'appendChatMessage("agent", activity_text);' in panel
assert 'chat_scroll_area_' not in panel
assert 'chat_history_layout_' not in panel
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
assert 'allowed_objects' in (root / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert 'warnings.filterwarnings' in (root / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert 'startup presets; Refresh models fetches your catalog' in settings
assert 'model_details_->setMinimumHeight(42);' in settings
assert 'startup snapshot; Refresh models fetches current catalog' in settings
assert 'if (!model_combo_ || !provider_combo_' in settings
assert 'agent_panel_->setModelCatalogCallback({});' in settings
assert 'control:mcpServersTable' in settings
assert 'action:addMcpServerBtn' in settings
assert 'action:removeMcpServerBtn' in settings
assert 'Agent Settings is modeless and therefore a separate top-level window.' in review
assert 'QWidget* capture_window = window;' in (root / "src" / "ccad_gui" / "main.cpp").read_text(encoding="utf-8")
assert 'control:categoryList' in (root / "src" / "ccad_gui" / "main.cpp").read_text(encoding="utf-8")
assert 'config["mcp_servers"] = servers;' in settings
assert 'mcp_servers_table_->setRowCount(0);' in settings
assert 'mcp_list->addItem("Server: chrome-devtools' not in settings
assert 'Saving does not connect or launch a server' in settings
assert 'label:mcpRuntimeStatus' in settings
assert 'agent.mcp_status' in settings
assert 'agent.mcp_plan' in (root / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert 'setMcpStatusCallback' in settings
assert 'agent_panel_->sendJsonRpc("agent.mcp_status", QJsonObject());' in settings
for target in ("action:settingsBtn", "control:providerCombo", "control:modelCombo",
               "control:apiKeyInput", "control:mcpServersTable",
               "action:addMcpServerBtn", "action:removeMcpServerBtn"):
    assert target in main
assert 'setModelCatalogCallback' in settings
assert 'applyModelCatalog' in settings
assert 'catalog["ok"].toBool(false)' in settings
assert 'catalog["models"].toArray()' in settings
assert 'provider_combo_->currentData().toString() != "openai_compatible" &&' in settings
assert 'provider_combo_->currentData().toString() != "local_model"' in settings
assert 'model_input_->setReadOnly(true)' in settings
assert 'model_input_->setReadOnly(!custom_model_provider)' in settings
assert '(!custom_model_provider && matching < 0)' in settings
assert 'model_combo_->setCurrentIndex(0)' in settings
assert 'api_key_input_ = new QLineEdit(parent_widget);' in settings
assert 'layout->addWidget(api_key_input_);' in settings
assert 'Session key is held in memory and stored only in the OS credential vault.' in settings
assert 'action:setProviderKeyBtn' in settings
assert 'Key saved; selected provider is active' in settings
assert 'agent_panel_->setProviderSecret(provider, secret);' in settings
assert 'agent_panel_->sendJsonRpc("agent.set_config", QJsonObject{' in settings
assert 'Validate Provider Setup' in settings
assert 'without sending a request or consuming quota' in settings
assert 'setProviderTestResultCallback' in header
assert 'provider_test_result_cb_' in header
assert '"provider_test_result"' in panel
assert 'setProviderTestResultCallback' in settings
orchestrator = (root / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert '"provider_test_result"' in orchestrator
assert '"network_access": "not_probed"' in orchestrator
assert 'quota or rate limit reached' in settings
assert 'selected model was not found' in settings
assert 'API key was rejected' in settings
assert settings.count('sendJsonRpc("agent.set_config", config)') == 1
assert 'QJsonObject{{"grid", grid_combo_->currentText()}}' not in settings
assert 'combo_selection_selected' in review
assert 'object->value("value").toString()' in review
assert 'object->value("text").toString()' in review
assert 'Optional exact combo item data value.' in review
assert 'Optional exact combo visible text.' in review
print("PASS agent UI/provider runtime source contract; no GUI launched")
