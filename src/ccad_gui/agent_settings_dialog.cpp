#include "ccad_gui/agent_settings_dialog.hpp"
#include "ccad_gui/agent_panel.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QJsonArray>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QFormLayout>
#include <QCheckBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QPointer>
#include <QTimer>

AgentSettingsDialog::AgentSettingsDialog(AgentPanel* agent_panel, QWidget* parent)
    : QDialog(parent), agent_panel_(agent_panel) {
  setWindowTitle("Agent Settings");
  setMinimumSize(700, 500);

  setStyleSheet(R"(
    QDialog {
      background-color: #0d1117;
      color: #e6edf3;
      font-family: 'Inter', sans-serif;
    }
    QListWidget {
      background-color: #0d1117;
      border: 1px solid #30363d;
      border-radius: 6px;
      color: #e6edf3;
      font-size: 13px;
    }
    QListWidget::item {
      padding: 10px;
    }
    QListWidget::item:selected {
      background-color: rgba(138, 43, 226, 0.15);
      border-left: 3px solid #8a2be2;
    }
    QLabel {
      color: #e6edf3;
      font-size: 13px;
    }
    QLineEdit, QTextEdit, QComboBox {
      background-color: #010409;
      border: 1px solid #30363d;
      border-radius: 6px;
      padding: 8px;
      color: #e6edf3;
    }
    QPushButton {
      background-color: #21262d;
      border: 1px solid #30363d;
      border-radius: 6px;
      color: #e6edf3;
      padding: 8px 16px;
    }
    QPushButton:hover {
      background-color: #30363d;
      border-color: #8a2be2;
    }
    QPushButton#primaryButton {
      background-color: #8a2be2;
      border: none;
      color: #ffffff;
    }
    QPushButton#primaryButton:hover {
      background-color: #9b4dff;
    }
    QGroupBox {
      border: 1px solid #30363d;
      border-radius: 6px;
      margin-top: 1ex;
      padding: 12px;
    }
    QGroupBox::title {
      subcontrol-origin: margin;
      subcontrol-position: top center;
      padding: 0 5px;
      color: #8a2be2;
    }
  )");

  setupUi();

  if (agent_panel_) {
      agent_panel_->setConfigStateCallback([this](const QJsonObject& config) {
          this->applyConfigState(config);
      });
      agent_panel_->setProviderStateCallback([this](const QJsonObject& state) {
          if (!provider_status_label_) return;
          const bool ready = state["execution_enabled"].toBool(false);
          const QString error = state["error"].toString();
          provider_status_label_->setText(
              ready ? "Provider test: ready (network not probed)"
                    : (error.isEmpty() ? "Provider test: unavailable"
                                       : "Provider test: " + error));
      });
      agent_panel_->setMarketplaceCatalogCallback([this](const QJsonObject& catalog) {
          this->applyMarketplaceCatalog(catalog);
      });
  }

  loadCurrentSettings();
}

AgentSettingsDialog::~AgentSettingsDialog() = default;

void AgentSettingsDialog::setupUi() {
  auto* base_layout = new QVBoxLayout(this);

  auto* content_layout = new QHBoxLayout();

  category_list_ = new QListWidget(this);
  category_list_->setObjectName("control:categoryList");
  category_list_->setFixedWidth(180);
  category_list_->addItem("General");
  category_list_->addItem("Configuration");
  category_list_->addItem("Personalisation");
  category_list_->addItem("MCP");
  category_list_->addItem("API & Providers");
  category_list_->addItem("Plugins");
  category_list_->addItem("Workflows");

  stacked_widget_ = new QStackedWidget(this);

  auto* general_tab = new QWidget();
  createGeneralTab(general_tab);
  stacked_widget_->addWidget(general_tab);

  auto* config_tab = new QWidget();
  createConfigurationTab(config_tab);
  stacked_widget_->addWidget(config_tab);

  auto* person_tab = new QWidget();
  createPersonalisationTab(person_tab);
  stacked_widget_->addWidget(person_tab);

  auto* mcp_tab = new QWidget();
  createMCPTab(mcp_tab);
  stacked_widget_->addWidget(mcp_tab);

  auto* api_tab = new QWidget();
  createAPIProvidersTab(api_tab);
  stacked_widget_->addWidget(api_tab);

  auto* plugins_tab = new QWidget();
  createPluginsTab(plugins_tab);
  stacked_widget_->addWidget(plugins_tab);

  auto* workflows_tab = new QWidget();
  createWorkflowsTab(workflows_tab);
  stacked_widget_->addWidget(workflows_tab);

  content_layout->addWidget(category_list_);
  content_layout->addWidget(stacked_widget_);
  base_layout->addLayout(content_layout);

  auto* btn_layout = new QHBoxLayout();
  btn_layout->addStretch();
  cancel_btn_ = new QPushButton("Cancel", this);
  auto* save_btn = new QPushButton("Save Preferences", this);
  save_btn->setObjectName("action:primaryButton");
  btn_layout->addWidget(cancel_btn_);
  btn_layout->addWidget(save_btn);

  base_layout->addLayout(btn_layout);

  connect(category_list_, &QListWidget::currentRowChanged, stacked_widget_, &QStackedWidget::setCurrentIndex);
  category_list_->setCurrentRow(0);

  connect(cancel_btn_, &QPushButton::clicked, this, &QDialog::reject);
  connect(save_btn, &QPushButton::clicked, this, &AgentSettingsDialog::saveAllSettings);
}

void AgentSettingsDialog::createGeneralTab(QWidget* parent_widget) {
  auto* layout = new QVBoxLayout(parent_widget);
  layout->addWidget(new QLabel("<b>General Settings</b>", parent_widget));
  layout->addWidget(new QLabel("General workspace settings.", parent_widget));
  layout->addStretch();
}

void AgentSettingsDialog::createConfigurationTab(QWidget* parent_widget) {
  auto* layout = new QVBoxLayout(parent_widget);
  layout->addWidget(new QLabel("<b>Configuration</b>", parent_widget));

  auto* form = new QFormLayout();

  provider_combo_ = new QComboBox(parent_widget);
  provider_combo_->setObjectName("control:providerCombo");
  provider_combo_->addItem("OpenAI", "openai");
  provider_combo_->addItem("Anthropic", "anthropic");
  provider_combo_->addItem("Google Gemini", "google_gemini");
  provider_combo_->addItem("OpenAI-compatible", "openai_compatible");
  provider_combo_->addItem("Local model server", "local_model");
  form->addRow("Provider:", provider_combo_);

  model_input_ = new QLineEdit(parent_widget);
  model_input_->setObjectName("control:modelInput");
  model_input_->setPlaceholderText("Current model selected");
  form->addRow("Model:", model_input_);

  sandbox_cb_ = new QCheckBox("Sandbox Mode", parent_widget);
  sandbox_cb_->setObjectName("control:sandboxCb");
  approval_cb_ = new QCheckBox("Approval Policy", parent_widget);
  approval_cb_->setObjectName("control:approvalCb");
  form->addRow("Security:", sandbox_cb_);
  form->addRow("", approval_cb_);

  project_name_ = new QLineEdit(parent_widget);
  project_name_->setObjectName("control:projectNameInput");
  project_path_ = new QLineEdit(parent_widget);
  project_path_->setObjectName("control:projectPathInput");
  trust_level_ = new QComboBox(parent_widget);
  trust_level_->setObjectName("control:trustLevelCombo");
  trust_level_->addItems({"Trusted", "Untrusted"});
  form->addRow("Project Name:", project_name_);
  form->addRow("Project Path:", project_path_);
  form->addRow("Trust Level:", trust_level_);

  auto* mem_group = new QGroupBox("Generate Memories", parent_widget);
  auto* mem_layout = new QVBoxLayout(mem_group);
  stm_cb_ = new QCheckBox("STM (handles stm)", parent_widget);
  stm_cb_->setObjectName("control:stmCb");
  ltm_cb_ = new QCheckBox("LTM (handles ltm)", parent_widget);
  ltm_cb_->setObjectName("control:ltmCb");
  episodic_cb_ = new QCheckBox("Episodic (handles episodic)", parent_widget);
  episodic_cb_->setObjectName("control:episodicCb");
  mem_layout->addWidget(stm_cb_);
  mem_layout->addWidget(ltm_cb_);
  mem_layout->addWidget(episodic_cb_);
  form->addRow(mem_group);

  hooks_combo_ = new QComboBox(parent_widget);
  hooks_combo_->setObjectName("control:hooksCombo");
  hooks_combo_->addItems({"Active Prompts", "Selected Prompts"});
  form->addRow("Hooks:", hooks_combo_);

  layout->addLayout(form);
  layout->addStretch();
}

void AgentSettingsDialog::createPersonalisationTab(QWidget* parent_widget) {
  auto* layout = new QVBoxLayout(parent_widget);
  layout->addWidget(new QLabel("<b>Personalisation</b>", parent_widget));

  auto* form = new QFormLayout();

  follow_up_ = new QLineEdit(parent_widget);
  follow_up_->setObjectName("control:followUpInput");
  form->addRow("Follow up behaviour:", follow_up_);

  context_window_ = new QCheckBox("Show context window usage in chat", parent_widget);
  context_window_->setObjectName("control:contextWindowCb");
  form->addRow("", context_window_);

  inline_detached_ = new QComboBox(parent_widget);
  inline_detached_->setObjectName("control:chatModeCombo");
  inline_detached_->addItems({"Inline", "Detached"});
  form->addRow("Chat mode:", inline_detached_);

  agent_personality_ = new QComboBox(parent_widget);
  agent_personality_->setObjectName("control:agentPersonalityCombo");
  agent_personality_->addItems({"Default", "Senior EE", "Super Power"});
  form->addRow("Agent Personality:", agent_personality_);

  custom_instructions_ = new QTextEdit(parent_widget);
  custom_instructions_->setObjectName("control:customInstructionsText");
  custom_instructions_->setPlaceholderText("Custom instructions...");
  form->addRow("Instructions:", custom_instructions_);

  auto* mem_group = new QGroupBox("Memory Settings", parent_widget);
  auto* mem_layout = new QVBoxLayout(mem_group);
  mem_layout->addWidget(new QLabel("STM -> Goal/Task set specific"));
  mem_layout->addWidget(new QLabel("LTM -> Chat/convo specific"));
  mem_layout->addWidget(new QLabel("Episodic -> Global of all chats & convo just like humans"));
  auto* reset_btn = new QPushButton("Reset Memories", parent_widget);
  mem_layout->addWidget(reset_btn);
  form->addRow(mem_group);

  layout->addLayout(form);
  layout->addStretch();
}

void AgentSettingsDialog::createMCPTab(QWidget* parent_widget) {
  auto* layout = new QVBoxLayout(parent_widget);
  layout->addWidget(new QLabel("<b>MCP Servers</b>", parent_widget));

  auto* mcp_list = new QListWidget(parent_widget);
  mcp_list->addItem("Server: chrome-devtools\nPath: ...\nArgs: ...\nPorts: ...");
  mcp_list->addItem("Server: filesystem\nPath: ...\nArgs: ...\nPorts: ...");
  layout->addWidget(mcp_list);
}

void AgentSettingsDialog::createAPIProvidersTab(QWidget* parent_widget) {
  auto* layout = new QVBoxLayout(parent_widget);
  layout->addWidget(new QLabel("<b>API & Providers</b>", parent_widget));
  layout->addWidget(new QLabel("Enter provider key for this session only. Key is masked and never written to project files, config JSON, or logs.", parent_widget));
  provider_target_label_ = new QLabel(parent_widget);
  provider_target_label_->setObjectName("label:providerTestTarget");
  provider_target_label_->setProperty("agentRole", "noticeCard");
  layout->addWidget(provider_target_label_);
  provider_status_label_ = new QLabel("Provider test: not run", parent_widget);
  provider_status_label_->setObjectName("label:providerTestStatus");
  provider_status_label_->setProperty("agentRole", "noticeCard");
  layout->addWidget(provider_status_label_);
  const auto refresh_target = [this]() {
    const QString provider = provider_combo_ ? provider_combo_->currentText() : QStringLiteral("OpenAI");
    const QString model = model_input_ && !model_input_->text().trimmed().isEmpty()
                              ? model_input_->text().trimmed() : QStringLiteral("Auto");
    if (provider_target_label_) {
      provider_target_label_->setText("Test target: " + provider + " / " + model);
    }
    if (api_key_input_) {
      api_key_input_->setPlaceholderText(provider + " key (kept in memory)");
    }
  };
  connect(provider_combo_, &QComboBox::currentTextChanged, this, [refresh_target](const QString&) { refresh_target(); });
  connect(model_input_, &QLineEdit::textChanged, this, [refresh_target](const QString&) { refresh_target(); });
  api_key_input_ = new QLineEdit(parent_widget);
  api_key_input_->setObjectName("control:apiKeyInput");
  api_key_input_->setEchoMode(QLineEdit::Password);
  api_key_input_->setPlaceholderText("API key (kept in memory)");
  layout->addWidget(api_key_input_);
  auto* reveal_key = new QCheckBox("Show key", parent_widget);
  reveal_key->setObjectName("control:showApiKeyCb");
  reveal_key->setToolTip("Temporarily reveal the session key on screen");
  connect(reveal_key, &QCheckBox::toggled, this, [this](bool visible) {
    if (api_key_input_) {
      api_key_input_->setEchoMode(visible ? QLineEdit::Normal : QLineEdit::Password);
    }
  });
  layout->addWidget(reveal_key);
  auto* test_provider = new QPushButton("Test Provider", parent_widget);
  test_provider->setObjectName("action:testProviderBtn");
  test_provider->setToolTip("Initialize the selected provider for this session without sending a prompt");
  connect(test_provider, &QPushButton::clicked, this, [this]() {
    if (provider_status_label_) {
      provider_status_label_->setText("Provider test: running...");
    }
    if (agent_panel_ && api_key_input_) {
      const QString provider = provider_combo_ ? provider_combo_->currentData().toString()
                                               : QStringLiteral("openai");
      // Test the exact selection currently visible in Settings. This updates
      // the live child process only; the Save action remains the persistence
      // boundary for non-secret preferences.
      QJsonObject config;
      config.insert("provider", provider);
      if (model_input_) config.insert("model", model_input_->text().trimmed());
      config.insert("secret", api_key_input_->text());
      agent_panel_->sendJsonRpc("agent.test_provider", config);
      QPointer<AgentSettingsDialog> dialog_guard(this);
      QTimer::singleShot(5000, this, [dialog_guard]() {
        if (dialog_guard && dialog_guard->provider_status_label_ &&
            dialog_guard->provider_status_label_->text() == "Provider test: running...") {
          dialog_guard->provider_status_label_->setText("Provider test: no response");
        }
      });
    }
  });
  layout->addWidget(test_provider);
  auto* clear_key = new QPushButton("Clear session key", parent_widget);
  clear_key->setObjectName("action:clearProviderKeyBtn");
  clear_key->setToolTip("Remove selected provider key from this session and agent process");
  connect(clear_key, &QPushButton::clicked, this, [this]() {
    const QString provider = provider_combo_ ? provider_combo_->currentData().toString()
                                             : QStringLiteral("openai");
    if (agent_panel_) agent_panel_->setProviderSecret(provider, QString());
    if (api_key_input_) api_key_input_->clear();
  });
  layout->addWidget(clear_key);
  refresh_target();
  auto* btn = new QPushButton("Test Export (OTel/Langfuse)", parent_widget);
  btn->setObjectName("action:testExportBtn");
  connect(btn, &QPushButton::clicked, this, [this]() {
      agent_panel_->sendJsonRpc("agent.test_export", QJsonObject());
  });
  layout->addWidget(btn);
  layout->addStretch();
}

void AgentSettingsDialog::createPluginsTab(QWidget* parent_widget) {
  auto* layout = new QVBoxLayout(parent_widget);
  layout->addWidget(new QLabel("<b>Plugins</b>", parent_widget));
  plugins_list_ = new QListWidget(parent_widget);
  layout->addWidget(plugins_list_);
}

void AgentSettingsDialog::createWorkflowsTab(QWidget* parent_widget) {
  auto* layout = new QVBoxLayout(parent_widget);
  layout->addWidget(new QLabel("<b>Workflows</b>", parent_widget));

  workflows_list_ = new QListWidget(parent_widget);
  layout->addWidget(workflows_list_);

  auto* form = new QFormLayout();
  auto* hooks = new QLabel("pre tool, post tool, post prompt, pre exit/end");
  form->addRow("Hooks:", hooks);

  system_prompt_ = new QTextEdit(parent_widget);
  system_prompt_->setPlaceholderText("System prompt...");
  form->addRow("System Prompt:", system_prompt_);

  dev_prompt_ = new QTextEdit(parent_widget);
  dev_prompt_->setPlaceholderText("Dev prompt (experimental, for developers)... e.g. tool calls specific");
  form->addRow("Dev Prompt:", dev_prompt_);

  layout->addLayout(form);
}

void AgentSettingsDialog::loadCurrentSettings() {
  if (agent_panel_) {
      agent_panel_->sendJsonRpc("agent.get_config", QJsonObject());
      agent_panel_->sendJsonRpc("agent.get_marketplace_catalog", QJsonObject());
  }
}

void AgentSettingsDialog::applyConfigState(const QJsonObject& config) {
    if (provider_combo_ && config.contains("provider")) {
        const QString provider_id = config["provider"].toString();
        const int index = provider_combo_->findData(provider_id);
        if (index >= 0) {
            provider_combo_->setCurrentIndex(index);
        } else {
            // Backward compatibility for configs written before IDs existed.
            provider_combo_->setCurrentText(provider_id);
        }
    }
    if (model_input_ && config.contains("model")) {
        model_input_->setText(config["model"].toString());
    }
    if (sandbox_cb_ && config.contains("sandbox_mode")) {
        sandbox_cb_->setChecked(config["sandbox_mode"].toBool());
    }
    if (approval_cb_ && config.contains("approval_policy")) {
        approval_cb_->setChecked(config["approval_policy"].toBool());
    }
    if (system_prompt_ && config.contains("system_prompt")) {
        system_prompt_->setPlainText(config["system_prompt"].toString());
    }
    if (dev_prompt_ && config.contains("dev_prompt")) {
        dev_prompt_->setPlainText(config["dev_prompt"].toString());
    }
}

void AgentSettingsDialog::applyMarketplaceCatalog(const QJsonObject& catalog) {
    if (plugins_list_ && catalog.contains("plugins")) {
        plugins_list_->clear();
        QJsonArray plugins = catalog["plugins"].toArray();
        for (const auto& val : plugins) {
            QJsonObject p = val.toObject();
            QString text = p["name"].toString() + " - " + p["description"].toString();
            auto* item = new QListWidgetItem(text, plugins_list_);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(p["installed"].toBool() ? Qt::Checked : Qt::Unchecked);
            item->setData(Qt::UserRole, p["id"].toString());
        }
    }
    if (workflows_list_ && catalog.contains("workflows")) {
        workflows_list_->clear();
        QJsonArray workflows = catalog["workflows"].toArray();
        for (const auto& val : workflows) {
            QJsonObject w = val.toObject();
            QString text = w["name"].toString() + " - " + w["description"].toString();
            auto* item = new QListWidgetItem(text, workflows_list_);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(w["installed"].toBool() ? Qt::Checked : Qt::Unchecked);
            item->setData(Qt::UserRole, w["id"].toString());
        }
    }
}

void AgentSettingsDialog::saveAllSettings() {
  QJsonObject config;

  if (provider_combo_) config["provider"] = provider_combo_->currentData().toString();
  if (model_input_) config["model"] = model_input_->text();
  if (sandbox_cb_) config["sandbox_mode"] = sandbox_cb_->isChecked();
  if (approval_cb_) config["approval_policy"] = approval_cb_->isChecked();
  if (project_name_) config["project_name"] = project_name_->text();
  if (project_path_) config["project_path"] = project_path_->text();
  if (trust_level_) config["trust_level"] = trust_level_->currentText();

  QJsonObject memory;
  if (stm_cb_) memory["stm"] = stm_cb_->isChecked();
  if (ltm_cb_) memory["ltm"] = ltm_cb_->isChecked();
  if (episodic_cb_) memory["episodic"] = episodic_cb_->isChecked();
  config["memory"] = memory;

  QJsonObject personalisation;
  if (follow_up_) personalisation["follow_up"] = follow_up_->text();
  if (context_window_) personalisation["show_context_usage"] = context_window_->isChecked();
  if (inline_detached_) personalisation["chat_mode"] = inline_detached_->currentText();
  if (agent_personality_) personalisation["agent_personality"] = agent_personality_->currentText();
  if (custom_instructions_) personalisation["custom_instructions"] = custom_instructions_->toPlainText();
  config["personalisation"] = personalisation;

  if (system_prompt_) config["system_prompt"] = system_prompt_->toPlainText();
  if (dev_prompt_) config["dev_prompt"] = dev_prompt_->toPlainText();

  if (agent_panel_ && api_key_input_) {
    agent_panel_->setProviderSecret(provider_combo_ ? provider_combo_->currentData().toString() : "openai",
                                    api_key_input_->text());
  }

  if (plugins_list_) {
    QJsonArray installed_plugins;
    for (int i = 0; i < plugins_list_->count(); ++i) {
      QListWidgetItem* item = plugins_list_->item(i);
      if (item->checkState() == Qt::Checked) {
        installed_plugins.append(item->data(Qt::UserRole).toString());
      }
    }
    config["installed_plugins"] = installed_plugins;
  }

  if (workflows_list_) {
    QJsonArray active_workflows;
    for (int i = 0; i < workflows_list_->count(); ++i) {
      QListWidgetItem* item = workflows_list_->item(i);
      if (item->checkState() == Qt::Checked) {
        active_workflows.append(item->data(Qt::UserRole).toString());
      }
    }
    config["active_workflows"] = active_workflows;
  }

  agent_panel_->sendJsonRpc("agent.set_config", config);
  accept();
}
