"""No-GUI source contract for chat stream and provider runtime fixes."""

from pathlib import Path


root = Path(__file__).parents[1]
panel = (root / "src" / "ccad_gui" / "agent_panel.cpp").read_text(encoding="utf-8")
header = (root / "src" / "ccad_gui" / "agent_panel.hpp").read_text(encoding="utf-8")
settings = (root / "src" / "ccad_gui" / "agent_settings_dialog.cpp").read_text(encoding="utf-8")
review = (root / "src" / "ccad_gui" / "review_window.cpp").read_text(encoding="utf-8")
main = (root / "src" / "ccad_gui" / "main.cpp").read_text(encoding="utf-8")
orchestrator = (root / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")

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
assert 'Key is restored from Windows Credential Manager for this provider when CCad starts.' in settings
assert 'action:setProviderKeyBtn' in settings
assert 'Key saved; selected provider is ready' in settings
assert 'agent_panel_->setProviderSecret(provider, secret);' in settings
assert 'agent_panel_->sendJsonRpc("agent.set_config", QJsonObject{' in settings
assert 'setProviderSecretResultCallback' in header
assert 'provider_secret_result_cb_' in header
assert '"provider_secret_result"' in panel
assert 'setProviderSecretResultCallback' in settings
assert 'provider_selector_->addItem(spec.label, spec.id);' in panel
assert 'provider_selector_->findData(provider)' in panel
assert 'control:providerStateSelector' in panel
assert 'Validate local setup (no network)' in settings
assert 'it does not validate the key or network' in settings
assert 'setProviderTestResultCallback' in header
assert 'provider_test_result_cb_' in header
assert '"provider_test_result"' in panel
assert 'setProviderTestResultCallback' in settings
assert 'setProviderConnectionResultCallback' in header
assert 'provider_connection_result_cb_' in header
assert '"provider_connection_result"' in panel
assert 'Test live connection (uses quota)' in settings
assert 'agent.test_provider_connection' in settings
assert 'Langfuse: ready | export ' in settings
assert 'exported_span_count' in settings
assert 'trace_id' in settings
assert 'telemetry_runtime.flush_turn()' in orchestrator
assert 'method": "observability_state"' in orchestrator
assert 'payment, credits, or project billing is required' in settings
assert 'https://api.cerebras.ai/public/v1/models' in orchestrator
assert 'CCad/1.0 (+https://github.com/ButWhySo/CCad)' in orchestrator
assert 'connection_attempted = True' in orchestrator
assert '"request_count": 1 if connection_attempted else 0' in orchestrator
assert 'return "payment_required"' in orchestrator
assert '"provider_test_result"' in orchestrator
assert '"network_access": "not_probed"' in orchestrator
assert 'def initialize_agent_process():' in orchestrator
assert 'if __name__ == "__main__":\n    initialize_agent_process()' in orchestrator
assert 'provider payment, credits, or project billing is required' in settings
assert 'provider rate limit reached' in settings
assert 'provider quota or credits are exhausted; check quota and billing' in settings
assert 'retry_after_seconds' in settings
assert 'retry_after_seconds' in orchestrator
assert 'selected model was not found' in settings
assert 'API key was rejected' in settings
assert settings.count('sendJsonRpc("agent.set_config", config)') == 2
assert 'QJsonObject{{"grid", grid_combo_->currentText()}}' not in settings
assert 'combo_selection_selected' in review
assert 'object->value("value").toString()' in review
assert 'object->value("text").toString()' in review
assert 'Optional exact combo item data value.' in review
assert 'Optional exact combo visible text.' in review
assert 'CCAD_AGENT_DEFER_PROVIDER_INIT' in panel
assert 'storedProviderSecret(configured_provider)' in panel
assert 'openrouter/free' in settings
assert 'provider_combo_->addItem("Ollama (local)", "ollama")' in settings
assert 'fetch_ollama_models' in orchestrator
assert '"network_access": "explicit_local_refresh"' in orchestrator
for target in ("label:providerTestTarget", "label:providerTestStatus",
               "action:testProviderBtn", "action:cancelSettingsButton"):
    assert target in main
assert 'name.startsWith("sprint972-provider")' in main
assert 'Provider validation: ready (network not probed)' in settings
assert 'provider quota or credits are exhausted; check quota and billing' in settings
assert 'connection_retry_after = provider_retry_after_seconds(error)' in orchestrator
assert '"retry_after_seconds": connection_retry_after' in orchestrator
assert 'provider_local_validation_ready' in main
assert 'id.startsWith("label:")' in (root / "src" / "ccad_gui" / "review_window.cpp").read_text(encoding="utf-8")
print("PASS agent UI/provider runtime source contract; no GUI launched")
