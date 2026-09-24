#include "ccad_gui/agent_settings_dialog.hpp"
#include "ccad_gui/agent_panel.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QJsonArray>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QSizePolicy>
#include <QSignalBlocker>
#include <QSettings>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QFormLayout>
#include <QCheckBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QPointer>
#include <QTimer>
#include <QInputDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QTableWidget>
#include <QHeaderView>

#ifdef Q_OS_WIN
#include <windows.h>
#include <lmcons.h>
#include <wincred.h>
#endif

namespace {

QStringList modelsForProvider(const QString& provider) {
  if (provider == "openai") {
    return {"gpt-5.1", "gpt-5", "gpt-5-mini", "gpt-4.1", "gpt-4.1-mini"};
  }
  if (provider == "anthropic") {
    return {"claude-opus-5", "claude-sonnet-5",
            "claude-haiku-4-5-20251001"};
  }
  if (provider == "google_gemini") {
    return {"gemini-3.8-flash", "gemini-3.7-flash", "gemini-3.6-flash",
            "gemini-3.5-flash", "gemini-3.1-pro-preview",
            "gemini-2.5-flash", "gemini-2.5-flash-lite", "gemini-2.5-pro"};
  }
  if (provider == "openai_compatible") {
    return {"Custom model (type below)"};
  }
  if (provider == "openrouter") {
    return {"openrouter/free", "openrouter/auto", "Custom model (type below)"};
  }
  if (provider == "cerebras") {
    return {"gpt-oss-120b", "qwen-3.8-27b"};
  }
  if (provider == "ollama") {
    return {"qwen3", "Custom model (type below)"};
  }
  return {"local-model", "Custom model (type below)"};
}

bool looksLikeConcatenatedPreset(const QString& model) {
  const QString candidate = model.trimmed();
  if (candidate.isEmpty()) return false;
  const QStringList providers = {"openai", "anthropic", "google_gemini", "cerebras"};
  for (const QString& provider : providers) {
    for (const QString& preset : modelsForProvider(provider)) {
      // Provider/model changes must never preserve a stale preset prefix.
      // Reject any value containing a known preset plus another token.
      if (candidate.contains(preset) && candidate != preset) return true;
    }
  }
  return false;
}

QString modelDetailsForProvider(const QString& provider) {
  if (provider == "openai") return "OpenAI API | startup presets; Refresh models fetches your catalog | text/image | tool calling | key: OPENAI_API_KEY";
  if (provider == "anthropic") return "Anthropic API | startup presets; Refresh models fetches your catalog | Claude family | tool use | key: ANTHROPIC_API_KEY";
  if (provider == "google_gemini") return "Google Gemini API | startup presets; Refresh models fetches your catalog | multimodal | long context | key: GEMINI_API_KEY";
  if (provider == "cerebras") return "Cerebras API | startup snapshot; Refresh models fetches current catalog | fast inference | key: CEREBRAS_API_KEY";
  if (provider == "openai_compatible") return "OpenAI-compatible endpoint | custom base URL and model";
  if (provider == "openrouter") return "OpenRouter API | dynamic model catalog | key: OPENROUTER_API_KEY";
  if (provider == "ollama") return "Ollama local API | installed models from localhost:11434 | no API key by default | tool calling depends on selected model";
  return "Local model endpoint | custom model ID and endpoint required";
}

bool authorizeSecretReveal(QWidget* parent) {
#ifdef Q_OS_WIN
  wchar_t user[UNLEN + 1] = {};
  DWORD user_size = UNLEN + 1;
  if (!GetUserNameW(user, &user_size)) {
    return false;
  }
  bool accepted = false;
  const QString account = QString::fromWCharArray(user);
  QString password = QInputDialog::getText(
      parent, QStringLiteral("CCad API key"),
      QStringLiteral("Enter the Windows password for '%1' to reveal this key:").arg(account),
      QLineEdit::Password, QString(), &accepted, Qt::WindowFlags(), Qt::ImhNone);
  if (!accepted || password.isEmpty()) return false;
  HANDLE token = nullptr;
  std::wstring password_wide = password.toStdWString();
  const BOOL valid = LogonUserW(
      user, nullptr, password_wide.c_str(), LOGON32_LOGON_INTERACTIVE,
      LOGON32_PROVIDER_DEFAULT, &token);
  SecureZeroMemory(password_wide.data(),
                   password_wide.size() * sizeof(wchar_t));
  password.fill(QChar(u'\0'));
  if (token) CloseHandle(token);
  return valid == TRUE;
#else
  Q_UNUSED(parent);
  return false;
#endif
}

QString credentialTarget(const QString& provider) {
  return QStringLiteral("CCad/provider/") + provider;
}

QString loadStoredSecret(const QString& provider) {
#ifdef Q_OS_WIN
  PCREDENTIALW credential = nullptr;
  const std::wstring target = credentialTarget(provider).toStdWString();
  if (CredReadW(target.c_str(), CRED_TYPE_GENERIC, 0, &credential) && credential) {
    const QString value = QString::fromUtf8(
        reinterpret_cast<const char*>(credential->CredentialBlob),
        static_cast<int>(credential->CredentialBlobSize));
    CredFree(credential);
    return value;
  }
#else
  Q_UNUSED(provider);
#endif
  return {};
}

bool storeSecret(const QString& provider, const QString& secret) {
#ifdef Q_OS_WIN
  const std::wstring target = credentialTarget(provider).toStdWString();
  if (secret.isEmpty()) {
    return CredDeleteW(target.c_str(), CRED_TYPE_GENERIC, 0) != FALSE ||
           GetLastError() == ERROR_NOT_FOUND;
  }
  const QByteArray bytes = secret.toUtf8();
  if (bytes.size() > CRED_MAX_CREDENTIAL_BLOB_SIZE) return false;
  CREDENTIALW credential{};
  credential.Type = CRED_TYPE_GENERIC;
  credential.TargetName = const_cast<LPWSTR>(target.c_str());
  credential.CredentialBlobSize = static_cast<DWORD>(bytes.size());
  credential.CredentialBlob = reinterpret_cast<LPBYTE>(const_cast<char*>(bytes.constData()));
  credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
  return CredWriteW(&credential, 0) != FALSE;
#else
  Q_UNUSED(provider);
  Q_UNUSED(secret);
  return false;
#endif
}

}  // namespace

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
    QListWidget::item:selected:focus {
      background-color: rgba(167, 113, 230, 0.24);
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
    QLineEdit:focus, QTextEdit:focus, QComboBox:focus {
      background-color: #171321;
      border-color: #8a2be2;
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
    QPushButton:focus {
      background-color: #32283e;
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
    QPushButton#primaryButton:focus {
      background-color: #a765f4;
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
      agent_panel_->setMemoryStateCallback([this](const QJsonObject& state) {
          this->applyMemoryState(state);
      });
      agent_panel_->setProviderStateCallback([](const QJsonObject& state) {
          // Ambient backend state may describe the restored active provider.
          // It is deliberately not used to complete a selected-provider check.
          Q_UNUSED(state);
      });
      agent_panel_->setProviderTestResultCallback([this](const QJsonObject& state) {
          if (!provider_status_label_) return;
          const bool ready = state["execution_enabled"].toBool(false);
          const QString error = state["error"].toString();
          const QString category = state["error_category"].toString();
          const QHash<QString, QString> guidance = {
              {"missing_api_key", "API key is missing"},
              {"authentication", "API key was rejected"},
              {"payment_required", "provider payment, credits, or project billing is required"},
              {"rate_limited", "provider rate limit reached"},
              {"permission_denied", "provider denied this account or model"},
              {"model_not_found", "selected model was not found"},
              {"timeout", "provider request timed out"},
              {"dependency", "provider dependency is missing"},
              {"provider_unavailable", "provider is unavailable"},
          };
          const QString explanation = guidance.value(
              category, error.isEmpty() ? QStringLiteral("provider is unavailable") : error);
          provider_status_label_->setText(
              ready ? "Provider validation: ready (network not probed)"
                    : "Provider validation: " + explanation);
      });
      agent_panel_->setProviderConnectionResultCallback([this](const QJsonObject& state) {
          if (!provider_status_label_) return;
          if (state["connected"].toBool(false)) {
            const QString preview = state["response_preview"].toString().left(180);
            provider_status_label_->setText("Live connection: success — " + preview);
            return;
          }
          const QString category = state["error_category"].toString("provider_unavailable");
          const QHash<QString, QString> guidance = {
              {"payment_required", "payment, credits, or project billing is required"},
              {"quota_exhausted", "provider quota or credits are exhausted; check quota and billing"},
              {"rate_limited", "provider rate limit reached; wait before retrying"},
              {"authentication", "API key was rejected"},
              {"permission_denied", "account or project cannot use this model"},
              {"model_not_found", "selected model was not found"},
              {"timeout", "provider request timed out"},
              {"missing_api_key", "API key is missing"},
          };
          QString detail = guidance.value(category, category);
          const int http_status = state["http_status"].toInt(0);
          if (http_status > 0) detail += " (HTTP " + QString::number(http_status) + ")";
          const int retry_after_seconds = state["retry_after_seconds"].toInt(0);
          if (retry_after_seconds > 0)
            detail += "; retry after " + QString::number(retry_after_seconds) + " seconds";
          provider_status_label_->setText("Live connection failed: " + detail);
      });
      agent_panel_->setProviderSecretResultCallback([this](const QJsonObject& state) {
          if (!provider_status_label_) return;
          const bool ready = state["execution_enabled"].toBool(false);
          provider_status_label_->setText(
              ready ? "Key saved; selected provider is ready"
                    : "Key saved, but provider setup failed: " +
                          state["error_category"].toString("provider_unavailable"));
      });
      agent_panel_->setMarketplaceCatalogCallback([this](const QJsonObject& catalog) {
          this->applyMarketplaceCatalog(catalog);
      });
      agent_panel_->setModelCatalogCallback([this](const QJsonObject& catalog) {
          this->applyModelCatalog(catalog);
      });
      agent_panel_->setMcpStatusCallback([this](const QJsonObject& status) {
          if (!mcp_runtime_status_label_) return;
          const QString runtime = status["runtime"].toString("unknown");
          const bool process_execution = status["process_execution"].toBool(false);
          mcp_runtime_status_label_->setText(
              QString("Runtime: %1 | %2 server(s) | process execution %3")
                  .arg(runtime)
                  .arg(status["servers"].toArray().size())
                  .arg(process_execution ? "enabled" : "disabled"));
      });
      agent_panel_->setObservabilityStateCallback([this](const QJsonObject& state) {
          if (!langfuse_status_label_) return;
          if (!state["enabled"].toBool(false)) {
              langfuse_status_label_->setText("Langfuse: disabled");
          } else if (state["exporter_initialized"].toBool(false)) {
              QString status = "Langfuse: ready | export " +
                  state["last_export"].toString("not_run") +
                  " | last test " + state["last_test"].toString("not_run");
              if (state.contains("exported_span_count")) {
                  status += " | spans " +
                      QString::number(state["exported_span_count"].toInt());
              }
              if (state.contains("trace_id")) {
                  status += " | trace " + state["trace_id"].toString();
              }
              langfuse_status_label_->setText(status);
          } else {
              langfuse_status_label_->setText(
                  "Langfuse: " + state["reason"].toString("not configured"));
          }
      });
  }

  loadCurrentSettings();
}

AgentSettingsDialog::~AgentSettingsDialog() {
  // Responses arrive asynchronously from the agent bridge. Do not leave
  // callbacks capturing this dialog after accept() destroys it.
  if (agent_panel_) {
    agent_panel_->setConfigStateCallback({});
    agent_panel_->setMemoryStateCallback({});
    agent_panel_->setProviderStateCallback({});
    agent_panel_->setProviderTestResultCallback({});
    agent_panel_->setProviderConnectionResultCallback({});
    agent_panel_->setProviderSecretResultCallback({});
    agent_panel_->setMarketplaceCatalogCallback({});
    agent_panel_->setModelCatalogCallback({});
    agent_panel_->setMcpStatusCallback({});
    agent_panel_->setObservabilityStateCallback({});
  }
  if (memory_dialog_) {
    memory_dialog_->disconnect(this);
    memory_dialog_->close();
    memory_dialog_.clear();
  }
}

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
  category_list_->addItem("Observability");
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

  auto* observability_tab = new QWidget();
  createObservabilityTab(observability_tab);
  stacked_widget_->addWidget(observability_tab);

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
  cancel_btn_->setObjectName("action:cancelSettingsButton");
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
  auto* form = new QFormLayout();
  theme_combo_ = new QComboBox(parent_widget);
  theme_combo_->setObjectName("control:themeCombo");
  theme_combo_->addItems({"Dark", "Light", "System"});
  form->addRow("Theme:", theme_combo_);
  grid_combo_ = new QComboBox(parent_widget);
  grid_combo_->setObjectName("control:gridCombo");
  grid_combo_->addItems({"0.5 mm", "1.0 mm", "2.5 mm", "5.0 mm", "10.0 mm", "Hidden"});
  grid_combo_->setToolTip("Metric grid spacing, independent of display units");
  const QSettings local_settings("CCad", "Agent");
  if (local_settings.contains("grid")) {
    grid_combo_->setCurrentText(local_settings.value("grid").toString());
    grid_user_modified_ = true;
  }
  connect(grid_combo_, &QComboBox::currentTextChanged, this,
          [this](const QString&) { grid_user_modified_ = true; });
  form->addRow("Canvas grid spacing:", grid_combo_);
  autosave_cb_ = new QCheckBox("Save project changes automatically", parent_widget);
  autosave_cb_->setObjectName("control:autosaveCb");
  form->addRow("Editing:", autosave_cb_);
  restore_session_cb_ = new QCheckBox("Restore the last project and chat session", parent_widget);
  restore_session_cb_->setObjectName("control:restoreSessionCb");
  form->addRow("Startup:", restore_session_cb_);
  layout->addLayout(form);
  layout->addWidget(new QLabel("These preferences are stored in the per-user CCad agent configuration, not in the board file.", parent_widget));
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
  provider_combo_->addItem("OpenRouter", "openrouter");
  provider_combo_->addItem("Cerebras", "cerebras");
  provider_combo_->addItem("Ollama (local)", "ollama");
  provider_combo_->addItem("Local model server", "local_model");
  form->addRow("Provider:", provider_combo_);

  model_combo_ = new QComboBox(parent_widget);
  model_combo_->setObjectName("control:modelCombo");
  // Known providers are strict dropdowns. Free-form model IDs belong only to
  // endpoint-backed providers; editable preset boxes caused concatenated IDs.
  model_combo_->setEditable(true);
  model_combo_->setInsertPolicy(QComboBox::NoInsert);
  model_input_ = model_combo_->lineEdit();
  model_input_->setObjectName("control:modelInput");
  // Curated providers start as strict dropdowns. Endpoint-backed providers
  // become editable only when provider selection changes below.
  model_input_->setReadOnly(true);
  model_input_->setPlaceholderText("Choose a model or type a custom model ID");
  model_combo_->addItems(modelsForProvider(provider_combo_->currentData().toString()));
  form->addRow("Model:", model_combo_);
  auto* refresh_models = new QPushButton("Refresh models", parent_widget);
  refresh_models->setObjectName("action:refreshModelCatalog");
  refresh_models->setToolTip("Explicitly fetch the selected provider model catalog; never runs automatically");
  refresh_models->setEnabled(provider_combo_->currentData().toString() != "openai_compatible" &&
                             provider_combo_->currentData().toString() != "local_model");
  connect(refresh_models, &QPushButton::clicked, this, [this]() {
    if (!agent_panel_ || !provider_combo_) return;
    agent_panel_->sendJsonRpc("agent.list_models", QJsonObject{
        {"provider", provider_combo_->currentData().toString()}});
  });
  form->addRow(QString(), refresh_models);
  model_details_ = new QLabel(parent_widget);
  model_details_->setObjectName("label:modelDetails");
  model_details_->setWordWrap(true);
  model_details_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
  model_details_->setMinimumHeight(42);
  model_details_->setStyleSheet("color: #8b949e; padding: 2px 0 6px 0;");
  form->addRow("Details:", model_details_);

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

  hooks_combo_ = new QComboBox(parent_widget);
  hooks_combo_->setObjectName("control:hooksCombo");
  hooks_combo_->addItems({"Active Prompts", "Selected Prompts"});
  form->addRow("Hooks:", hooks_combo_);

  layout->addLayout(form);
  layout->addStretch();

  connect(provider_combo_, &QComboBox::currentIndexChanged, this, [this, refresh_models]() {
    if (!model_combo_ || !provider_combo_) return;
    const QString current = model_input_ ? model_input_->text().trimmed() : QString();
    const bool current_was_provider_preset = model_combo_->findText(current) >= 0;
    const bool current_was_malformed_preset = looksLikeConcatenatedPreset(current);
    model_combo_->blockSignals(true);
    model_combo_->clear();
    const QStringList models = modelsForProvider(provider_combo_->currentData().toString());
    if (refresh_models) refresh_models->setEnabled(provider_combo_->currentData().toString() != "openai_compatible" &&
                                                    provider_combo_->currentData().toString() != "local_model");
    model_combo_->addItems(models);
    const bool custom_model_provider = provider_combo_->currentData().toString() == "openai_compatible" ||
                                        provider_combo_->currentData().toString() == "openrouter" ||
                                        provider_combo_->currentData().toString() == "ollama" ||
                                        provider_combo_->currentData().toString() == "local_model";
    if (model_input_) model_input_->setReadOnly(!custom_model_provider);
    const int matching = model_combo_->findText(current);
    if (custom_model_provider && !current.isEmpty() && !current_was_provider_preset &&
        !current_was_malformed_preset && matching < 0) {
      model_combo_->setEditText(current);
    } else if (matching >= 0 && !current_was_provider_preset) {
      model_combo_->setCurrentIndex(matching);
    } else if (!models.isEmpty()) {
      model_combo_->setCurrentIndex(0);
    }
    model_combo_->blockSignals(false);
    if (model_details_) model_details_->setText(modelDetailsForProvider(provider_combo_->currentData().toString()));
    if (api_key_input_) api_key_input_->setText(loadStoredSecret(provider_combo_->currentData().toString()));
    if (resolved_config_preview_) {
      resolved_config_preview_->setPlainText(QString("[agent]\nprovider = \"%1\"\nmodel = \"%2\"\n\n[security]\nsandbox = %3\napproval = %4")
          .arg(provider_combo_->currentData().toString(), model_input_ ? model_input_->text() : QString(),
               sandbox_cb_ && sandbox_cb_->isChecked() ? "true" : "false",
               approval_cb_ && approval_cb_->isChecked() ? "true" : "false"));
    }
  });
  model_combo_->setCurrentIndex(0);
  if (model_input_) model_input_->setReadOnly(true);
  model_details_->setText(modelDetailsForProvider(provider_combo_->currentData().toString()));

  resolved_config_preview_ = new QTextEdit(parent_widget);
  resolved_config_preview_->setObjectName("control:resolvedConfigPreview");
  resolved_config_preview_->setReadOnly(true);
  resolved_config_preview_->setMaximumHeight(120);
  resolved_config_preview_->setPlaceholderText("Resolved configuration preview");
  form->addRow("Resolved config:", resolved_config_preview_);
  resolved_config_preview_->setPlainText(QString("[agent]\nprovider = \"%1\"\nmodel = \"%2\"\n\n[security]\nsandbox = %3\napproval = %4")
      .arg(provider_combo_->currentData().toString(), model_input_->text(),
           sandbox_cb_->isChecked() ? "true" : "false",
           approval_cb_->isChecked() ? "true" : "false"));
  connect(model_input_, &QLineEdit::textChanged, this, [this](const QString&) {
    if (!resolved_config_preview_ || !provider_combo_ || !model_input_) return;
    resolved_config_preview_->setPlainText(QString("[agent]\nprovider = \"%1\"\nmodel = \"%2\"\n\n[security]\nsandbox = %3\napproval = %4")
        .arg(provider_combo_->currentData().toString(), model_input_->text(),
             sandbox_cb_ && sandbox_cb_->isChecked() ? "true" : "false",
             approval_cb_ && approval_cb_->isChecked() ? "true" : "false"));
  });
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
  stm_cb_ = new QCheckBox("Short-term memory — current task/session only", mem_group);
  stm_cb_->setObjectName("control:stmCb");
  stm_cb_->setToolTip("Temporary working memory. Cleared when this agent process ends.");
  ltm_cb_ = new QCheckBox("Conversation memory — this chat thread", mem_group);
  ltm_cb_->setObjectName("control:ltmCb");
  ltm_cb_->setToolTip("Durable records scoped to the current conversation thread.");
  episodic_cb_ = new QCheckBox("Episodic memory — across this local user’s chats", mem_group);
  episodic_cb_->setObjectName("control:episodicCb");
  episodic_cb_->setToolTip("Durable local-user memories shared across projects on this device.");
  mem_layout->addWidget(stm_cb_);
  mem_layout->addWidget(ltm_cb_);
  mem_layout->addWidget(episodic_cb_);
  memory_status_label_ = new QLabel("Memory state: waiting for backend", mem_group);
  memory_status_label_->setObjectName("label:memoryState");
  memory_status_label_->setWordWrap(false);
  memory_status_label_->setToolTip("Per tier: enabled state, runtime-loaded / persistent count. Unsafe legacy records are hidden.");
  mem_layout->addWidget(memory_status_label_);
  auto* memory_actions = new QHBoxLayout();
  auto* manage_btn = new QPushButton("Manage memories", mem_group);
  manage_btn->setObjectName("action:agent_memory_manage");
  connect(manage_btn, &QPushButton::clicked, this, &AgentSettingsDialog::openMemoryManager);
  memory_actions->addWidget(manage_btn);
  auto* reset_btn = new QPushButton("Reset memories…", mem_group);
  reset_btn->setObjectName("action:agent_memory_reset");
  connect(reset_btn, &QPushButton::clicked, this, [this, reset_btn]() {
    const auto answer = QMessageBox::warning(
        this, "Reset memories", "Delete all persisted Agent memories?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes || !agent_panel_) return;
    if (agent_panel_->sendJsonRpc("agent.memory_reset", QJsonObject{{"confirmed", true}})) {
      reset_btn->setEnabled(false);
      QTimer::singleShot(1000, reset_btn, [reset_btn]() { reset_btn->setEnabled(true); });
    }
  });
  memory_actions->addWidget(reset_btn);
  memory_actions->addStretch();
  mem_layout->addLayout(memory_actions);
  const auto setMemoryTier = [this](const QString& tier, bool enabled) {
    if (memory_status_label_) memory_status_label_->setText("Memory state: updating " + tier + "…");
    if (!agent_panel_ || !agent_panel_->sendJsonRpc("agent.memory_set_enabled",
          QJsonObject{{"tier", tier}, {"enabled", enabled}})) {
      if (memory_status_label_) memory_status_label_->setText("Memory state: backend unavailable; preference unchanged");
    }
  };
  connect(stm_cb_, &QCheckBox::toggled, this,
          [setMemoryTier](bool enabled) { setMemoryTier("stm", enabled); });
  connect(ltm_cb_, &QCheckBox::toggled, this,
          [setMemoryTier](bool enabled) { setMemoryTier("ltm", enabled); });
  connect(episodic_cb_, &QCheckBox::toggled, this,
          [setMemoryTier](bool enabled) { setMemoryTier("episodic", enabled); });
  form->addRow(mem_group);

  layout->addLayout(form);
  layout->addStretch();
}

void AgentSettingsDialog::createMCPTab(QWidget* parent_widget) {
  auto* layout = new QVBoxLayout(parent_widget);
  layout->addWidget(new QLabel("<b>MCP Servers</b><br>Configure local stdio servers used by the agent. Changes are saved with the rest of Agent Settings.<br><i>Saving does not connect or launch a server; runtime status is reported separately.</i>", parent_widget));
  mcp_runtime_status_label_ = new QLabel("Runtime: checking...", parent_widget);
  mcp_runtime_status_label_->setObjectName("label:mcpRuntimeStatus");
  layout->addWidget(mcp_runtime_status_label_);
  mcp_servers_table_ = new QTableWidget(parent_widget);
  mcp_servers_table_->setObjectName("control:mcpServersTable");
  mcp_servers_table_->setColumnCount(5);
  mcp_servers_table_->setHorizontalHeaderLabels({"Name", "Command", "Arguments", "Port", "Enabled"});
  mcp_servers_table_->horizontalHeader()->setStretchLastSection(true);
  mcp_servers_table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  mcp_servers_table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  mcp_servers_table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
  mcp_servers_table_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
  mcp_servers_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  layout->addWidget(mcp_servers_table_);
  auto* buttons = new QHBoxLayout();
  auto* add = new QPushButton("Add server", parent_widget);
  add->setObjectName("action:addMcpServerBtn");
  auto* remove = new QPushButton("Remove selected", parent_widget);
  remove->setObjectName("action:removeMcpServerBtn");
  buttons->addWidget(add);
  buttons->addWidget(remove);
  buttons->addStretch();
  layout->addLayout(buttons);
  connect(add, &QPushButton::clicked, this, [this]() {
    const int row = mcp_servers_table_->rowCount();
    mcp_servers_table_->insertRow(row);
    for (int col = 0; col < 4; ++col)
      mcp_servers_table_->setItem(row, col, new QTableWidgetItem(col == 3 ? QStringLiteral("0") : QString()));
    auto* enabled = new QTableWidgetItem();
    enabled->setCheckState(Qt::Checked);
    mcp_servers_table_->setItem(row, 4, enabled);
    mcp_servers_table_->setCurrentCell(row, 0);
  });
  connect(remove, &QPushButton::clicked, this, [this]() {
    const int row = mcp_servers_table_->currentRow();
    if (row >= 0) mcp_servers_table_->removeRow(row);
  });
}

void AgentSettingsDialog::createAPIProvidersTab(QWidget* parent_widget) {
  auto* layout = new QVBoxLayout(parent_widget);
  layout->addWidget(new QLabel("<b>API & Providers</b>", parent_widget));
  layout->addWidget(new QLabel("Provider keys persist in Windows Credential Manager. They are masked and never written to project files, config JSON, or logs.", parent_widget));
  api_key_input_ = new QLineEdit(parent_widget);
  api_key_input_->setObjectName("control:apiKeyInput");
  api_key_input_->setEchoMode(QLineEdit::Password);
  api_key_input_->setPlaceholderText("API key (stored in OS credential vault)");
  api_key_input_->setToolTip("Stored in Windows Credential Manager, never in project/config/logs.");
  api_key_input_->setText(loadStoredSecret(provider_combo_->currentData().toString()));
  layout->addWidget(api_key_input_);
  provider_target_label_ = new QLabel(parent_widget);
  provider_target_label_->setObjectName("label:providerTestTarget");
  provider_target_label_->setProperty("agentRole", "noticeCard");
  layout->addWidget(provider_target_label_);
  provider_status_label_ = new QLabel("Provider validation: not run", parent_widget);
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
      const QString provider_id = provider_combo_ ? provider_combo_->currentData().toString()
                                                    : QString();
      api_key_input_->setPlaceholderText(provider_id == QStringLiteral("ollama")
          ? QStringLiteral("No API key required for default localhost")
          : QStringLiteral("API key (stored in Windows Credential Manager)"));
    }
  };
  connect(provider_combo_, &QComboBox::currentTextChanged, this, [refresh_target](const QString&) { refresh_target(); });
  connect(model_input_, &QLineEdit::textChanged, this, [refresh_target](const QString&) { refresh_target(); });
  auto* reveal_key = new QCheckBox("Show key", parent_widget);
  reveal_key->setObjectName("control:showApiKeyCb");
  reveal_key->setToolTip("Temporarily reveal the session key on screen");
  connect(reveal_key, &QCheckBox::toggled, this, [this, reveal_key](bool visible) {
    if (api_key_input_) {
      if (visible) {
        if (!authorizeSecretReveal(this)) {
          QSignalBlocker blocker(reveal_key);
          reveal_key->setChecked(false);
          api_key_input_->setEchoMode(QLineEdit::Password);
          return;
        }
      }
      api_key_input_->setEchoMode(visible ? QLineEdit::Normal : QLineEdit::Password);
    }
  });
  layout->addWidget(reveal_key);
  layout->addWidget(new QLabel("Key is restored from Windows Credential Manager for this provider when CCad starts.", parent_widget));
  auto* set_key = new QPushButton("Set key", parent_widget);
  set_key->setObjectName("action:setProviderKeyBtn");
  set_key->setToolTip("Store this provider key in Windows Credential Manager and activate the selected provider and model");
  connect(set_key, &QPushButton::clicked, this, [this]() {
    if (!api_key_input_ || !agent_panel_) return;
    const QString secret = api_key_input_->text();
    if (secret.isEmpty()) {
      if (provider_status_label_) provider_status_label_->setText("Provider key is required");
      return;
    }
    const QString provider = provider_combo_ ? provider_combo_->currentData().toString()
                                             : QStringLiteral("openai");
    const QString model = model_input_ ? model_input_->text().trimmed() : QString();
    if (!storeSecret(provider, secret)) {
      if (provider_status_label_) {
        provider_status_label_->setText("Windows Credential Manager rejected this key; it was not activated.");
      }
      return;
    }
    // Provider and model are safe preferences; the key travels only through
    // the private secret IPC and Windows Credential Manager.
    agent_panel_->sendJsonRpc("agent.set_config", QJsonObject{
        {"provider", provider}, {"model", model}});
    agent_panel_->setProviderSecret(provider, secret);
    if (provider_status_label_) provider_status_label_->setText("Saving key and activating provider...");
  });
  layout->addWidget(set_key);
  auto* test_provider = new QPushButton("Validate local setup (no network)", parent_widget);
  test_provider->setObjectName("action:testProviderBtn");
  test_provider->setToolTip("Checks local adapter setup only; it does not validate the key or network");
  connect(test_provider, &QPushButton::clicked, this, [this]() {
    if (provider_status_label_) {
      provider_status_label_->setText("Provider validation: running...");
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
    }
    QPointer<AgentSettingsDialog> dialog_guard(this);
    QTimer::singleShot(5000, this, [dialog_guard]() {
      if (dialog_guard && dialog_guard->provider_status_label_ &&
          dialog_guard->provider_status_label_->text() == "Provider validation: running...") {
        dialog_guard->provider_status_label_->setText("Provider validation: no response");
      }
    });
  });
  layout->addWidget(test_provider);
  auto* test_connection = new QPushButton("Test live connection (uses quota)", parent_widget);
  test_connection->setObjectName("action:testProviderConnectionBtn");
  test_connection->setToolTip("Sends exactly one minimal provider request, does not execute tools, and never retries");
  connect(test_connection, &QPushButton::clicked, this, [this]() {
    if (!agent_panel_ || !api_key_input_) return;
    if (provider_status_label_) provider_status_label_->setText("Live connection: sending one request...");
    const QString provider = provider_combo_ ? provider_combo_->currentData().toString()
                                             : QStringLiteral("openai");
    QJsonObject config;
    config.insert("provider", provider);
    if (model_input_) config.insert("model", model_input_->text().trimmed());
    config.insert("secret", api_key_input_->text());
    agent_panel_->sendJsonRpc("agent.test_provider_connection", config);
  });
  layout->addWidget(test_connection);
  auto* clear_key = new QPushButton("Remove saved key", parent_widget);
  clear_key->setObjectName("action:clearProviderKeyBtn");
  clear_key->setToolTip("Remove selected provider key from Windows Credential Manager and this agent process");
  connect(clear_key, &QPushButton::clicked, this, [this]() {
    const QString provider = provider_combo_ ? provider_combo_->currentData().toString()
                                             : QStringLiteral("openai");
    if (!storeSecret(provider, QString())) {
      if (provider_status_label_) provider_status_label_->setText("Could not remove key from Windows Credential Manager.");
      return;
    }
    if (agent_panel_) agent_panel_->setProviderSecret(provider, QString());
    if (api_key_input_) api_key_input_->clear();
  });
  layout->addWidget(clear_key);
  refresh_target();
  layout->addStretch();
}

void AgentSettingsDialog::createObservabilityTab(QWidget* parent_widget) {
  auto* layout = new QVBoxLayout(parent_widget);
  auto* observability_intro = new QLabel(
      "<b>Langfuse observability</b><br>Exports are disabled until both keys are stored and tracing is enabled. "
      "Keys stay in Windows Credential Manager; project files, settings JSON, chat, screenshots, and logs never contain them.",
      parent_widget);
  observability_intro->setWordWrap(true);
  layout->addWidget(observability_intro);

  langfuse_enabled_cb_ = new QCheckBox("Enable Langfuse tracing", parent_widget);
  langfuse_enabled_cb_->setObjectName("control:langfuseEnabledCb");
  layout->addWidget(langfuse_enabled_cb_);
  auto* form = new QFormLayout();
  langfuse_public_key_input_ = new QLineEdit(parent_widget);
  langfuse_public_key_input_->setObjectName("control:langfusePublicKeyInput");
  langfuse_public_key_input_->setEchoMode(QLineEdit::Password);
  langfuse_public_key_input_->setPlaceholderText("pk-lf-… (Windows Credential Manager)");
  langfuse_public_key_input_->setText(loadStoredSecret("langfuse_public"));
  form->addRow("Public key", langfuse_public_key_input_);
  langfuse_secret_key_input_ = new QLineEdit(parent_widget);
  langfuse_secret_key_input_->setObjectName("control:langfuseSecretKeyInput");
  langfuse_secret_key_input_->setEchoMode(QLineEdit::Password);
  langfuse_secret_key_input_->setPlaceholderText("sk-lf-… (Windows Credential Manager)");
  langfuse_secret_key_input_->setText(loadStoredSecret("langfuse_secret"));
  form->addRow("Secret key", langfuse_secret_key_input_);
  langfuse_base_url_input_ = new QLineEdit(parent_widget);
  langfuse_base_url_input_->setObjectName("control:langfuseBaseUrlInput");
  langfuse_base_url_input_->setPlaceholderText("https://cloud.langfuse.com");
  form->addRow("Base URL", langfuse_base_url_input_);
  langfuse_environment_input_ = new QLineEdit("development", parent_widget);
  langfuse_environment_input_->setObjectName("control:langfuseEnvironmentInput");
  form->addRow("Environment", langfuse_environment_input_);
  langfuse_service_name_input_ = new QLineEdit("ccad-agent", parent_widget);
  langfuse_service_name_input_->setObjectName("control:langfuseServiceNameInput");
  form->addRow("Service name", langfuse_service_name_input_);
  layout->addLayout(form);
  langfuse_status_label_ = new QLabel("Langfuse: checking runtime state…", parent_widget);
  langfuse_status_label_->setObjectName("label:langfuseStatus");
  langfuse_status_label_->setWordWrap(true);
  layout->addWidget(langfuse_status_label_);
  auto* test = new QPushButton("Test Langfuse export", parent_widget);
  test->setObjectName("action:testLangfuseExportBtn");
  test->setToolTip("Flushes one redacted test span using the currently entered settings.");
  connect(test, &QPushButton::clicked, this, [this]() {
    if (!agent_panel_) return;
    const QJsonObject config{{"observability", QJsonObject{
        {"enabled", langfuse_enabled_cb_ && langfuse_enabled_cb_->isChecked()},
        {"backend", "langfuse"},
        {"base_url", langfuse_base_url_input_ ? langfuse_base_url_input_->text().trimmed() : QString()},
        {"environment", langfuse_environment_input_ ? langfuse_environment_input_->text().trimmed() : QStringLiteral("development")},
        {"service_name", langfuse_service_name_input_ ? langfuse_service_name_input_->text().trimmed() : QStringLiteral("ccad-agent")},
    }}};
    const bool config_sent = agent_panel_->sendJsonRpc("agent.set_config", config);
    const bool secret_sent = agent_panel_->sendJsonRpc("agent.langfuse_set_secret", QJsonObject{
        {"public_key", langfuse_public_key_input_ ? langfuse_public_key_input_->text() : QString()},
        {"secret_key", langfuse_secret_key_input_ ? langfuse_secret_key_input_->text() : QString()},
    });
    const bool test_sent = agent_panel_->sendJsonRpc("agent.langfuse_test", QJsonObject());
    if (!config_sent || !secret_sent || !test_sent) {
      if (langfuse_status_label_) {
        langfuse_status_label_->setText("Langfuse: agent backend unavailable");
      }
      return;
    }
    if (langfuse_status_label_) langfuse_status_label_->setText("Langfuse: test running...");
    QPointer<AgentSettingsDialog> dialog_guard(this);
    QTimer::singleShot(12000, this, [dialog_guard]() {
      if (dialog_guard && dialog_guard->langfuse_status_label_ &&
          dialog_guard->langfuse_status_label_->text() == "Langfuse: test running...") {
        dialog_guard->langfuse_status_label_->setText("Langfuse: test response timeout");
      }
    });
  });
  layout->addWidget(test);
  auto* remove = new QPushButton("Remove Langfuse credentials", parent_widget);
  remove->setObjectName("action:clearLangfuseCredentialsBtn");
  connect(remove, &QPushButton::clicked, this, [this]() {
    if (!storeSecret("langfuse_public", QString()) || !storeSecret("langfuse_secret", QString())) {
      if (langfuse_status_label_) langfuse_status_label_->setText("Langfuse: credential removal failed");
      return;
    }
    if (langfuse_public_key_input_) langfuse_public_key_input_->clear();
    if (langfuse_secret_key_input_) langfuse_secret_key_input_->clear();
    if (agent_panel_) agent_panel_->sendJsonRpc("agent.langfuse_set_secret", QJsonObject());
  });
  layout->addWidget(remove);
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
      // The panel receives this durable, non-secret state during startup.
      // Apply it synchronously so Settings never exposes defaults while an
      // asynchronous refresh is still in transit.
      if (!agent_panel_->cachedConfigState().isEmpty()) {
          applyConfigState(agent_panel_->cachedConfigState());
      }
      agent_panel_->sendJsonRpc("agent.get_config", QJsonObject());
      agent_panel_->sendJsonRpc("agent.memory_state", QJsonObject());
      agent_panel_->sendJsonRpc("agent.langfuse_set_secret", QJsonObject{
          {"public_key", langfuse_public_key_input_ ? langfuse_public_key_input_->text() : QString()},
          {"secret_key", langfuse_secret_key_input_ ? langfuse_secret_key_input_->text() : QString()},
      });
      agent_panel_->sendJsonRpc("agent.langfuse_status", QJsonObject());
      agent_panel_->sendJsonRpc("agent.get_marketplace_catalog", QJsonObject());
      agent_panel_->sendJsonRpc("agent.mcp_status", QJsonObject());
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
        const QString loaded_model = config["model"].toString().trimmed();
        const QString provider = provider_combo_ ? provider_combo_->currentData().toString() : QString();
        const bool custom_model_provider = provider == "openai_compatible" ||
                                           provider == "openrouter" || provider == "ollama" ||
                                           provider == "local_model";
        const int matching = model_combo_ ? model_combo_->findText(loaded_model) : -1;
        if (looksLikeConcatenatedPreset(loaded_model) ||
            (!custom_model_provider && matching < 0)) {
            if (model_combo_ && model_combo_->count() > 0)
                model_combo_->setCurrentIndex(0);
        } else {
            model_input_->setText(loaded_model);
        }
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
    if (theme_combo_ && config.contains("theme")) theme_combo_->setCurrentText(config["theme"].toString());
    if (grid_combo_ && config.contains("grid")) {
        const QString loaded_grid = config["grid"].toString();
        const bool untouched = last_loaded_grid_.isEmpty() ||
                               grid_combo_->currentText() == last_loaded_grid_;
        if (untouched && !grid_user_modified_) {
            const QSignalBlocker blocker(grid_combo_);
            grid_combo_->setCurrentText(loaded_grid);
        }
        last_loaded_grid_ = loaded_grid;
    }
    if (autosave_cb_ && config.contains("autosave")) autosave_cb_->setChecked(config["autosave"].toBool());
    if (restore_session_cb_ && config.contains("restore_session")) restore_session_cb_->setChecked(config["restore_session"].toBool());
    if (project_name_ && config.contains("project_name")) project_name_->setText(config["project_name"].toString());
    if (project_path_ && config.contains("project_path")) project_path_->setText(config["project_path"].toString());
    if (trust_level_ && config.contains("trust_level")) trust_level_->setCurrentText(config["trust_level"].toString());
    const QJsonObject memory = config.value("memory").toObject();
    if (stm_cb_ && memory.contains("stm")) { const QSignalBlocker blocker(stm_cb_); stm_cb_->setChecked(memory["stm"].toBool()); }
    if (ltm_cb_ && memory.contains("ltm")) { const QSignalBlocker blocker(ltm_cb_); ltm_cb_->setChecked(memory["ltm"].toBool()); }
    if (episodic_cb_ && memory.contains("episodic")) { const QSignalBlocker blocker(episodic_cb_); episodic_cb_->setChecked(memory["episodic"].toBool()); }
    const QJsonObject personalisation = config.value("personalisation").toObject();
    if (follow_up_ && personalisation.contains("follow_up")) follow_up_->setText(personalisation["follow_up"].toString());
    if (context_window_ && personalisation.contains("show_context_usage")) context_window_->setChecked(personalisation["show_context_usage"].toBool());
    if (inline_detached_ && personalisation.contains("chat_mode")) inline_detached_->setCurrentText(personalisation["chat_mode"].toString());
    if (agent_personality_ && personalisation.contains("agent_personality")) agent_personality_->setCurrentText(personalisation["agent_personality"].toString());
    if (custom_instructions_ && personalisation.contains("custom_instructions")) custom_instructions_->setPlainText(personalisation["custom_instructions"].toString());
    const QJsonObject observability = config.value("observability").toObject();
    if (langfuse_enabled_cb_ && observability.contains("enabled")) langfuse_enabled_cb_->setChecked(observability["enabled"].toBool());
    if (langfuse_base_url_input_ && observability.contains("base_url")) langfuse_base_url_input_->setText(observability["base_url"].toString());
    if (langfuse_environment_input_ && observability.contains("environment")) langfuse_environment_input_->setText(observability["environment"].toString());
    if (langfuse_service_name_input_ && observability.contains("service_name")) langfuse_service_name_input_->setText(observability["service_name"].toString());
    if (mcp_servers_table_ && config.contains("mcp_servers")) {
        const QJsonArray servers = config["mcp_servers"].toArray();
        mcp_servers_table_->setRowCount(0);
        for (const QJsonValue& value : servers) {
            const QJsonObject server = value.toObject();
            const int row = mcp_servers_table_->rowCount();
            mcp_servers_table_->insertRow(row);
            mcp_servers_table_->setItem(row, 0, new QTableWidgetItem(server["name"].toString()));
            mcp_servers_table_->setItem(row, 1, new QTableWidgetItem(server["command"].toString()));
            QString args_text;
            if (server["args"].isArray()) {
                QStringList args;
                for (const QJsonValue& arg : server["args"].toArray()) args << arg.toString();
                args_text = args.join(' ');
            } else {
                args_text = server["args"].toString();
            }
            mcp_servers_table_->setItem(row, 2, new QTableWidgetItem(args_text));
            mcp_servers_table_->setItem(row, 3, new QTableWidgetItem(QString::number(server["port"].toInt(0))));
            auto* enabled = new QTableWidgetItem();
            enabled->setCheckState(server.value("enabled").toBool(true) ? Qt::Checked : Qt::Unchecked);
            mcp_servers_table_->setItem(row, 4, enabled);
        }
    }
}

void AgentSettingsDialog::applyMemoryState(const QJsonObject& state) {
  QJsonObject tiers = state.value("tiers").toObject();
  if (tiers.isEmpty() && state.contains("tier"))
    tiers.insert(state.value("tier").toString(), state);
  QStringList summary;
  const auto applyTier = [&tiers, &summary](const QString& tier,
                                                  QCheckBox* checkbox) {
    const QJsonObject item = tiers.value(tier).toObject();
    if (item.isEmpty()) return;
    const bool enabled = item.value("enabled").toBool(false);
    if (checkbox) {
      const QSignalBlocker blocker(checkbox);
      checkbox->setChecked(enabled);
    }
    const QString persistent_count = item.value("persistent_count_known").toBool(true)
        ? QString::number(item.value("persistent_entries").toInt()) : QString("?");
    QString tier_summary = QString("%1 %2 %3/%4")
                   .arg(tier.toUpper(), enabled ? "on" : "off")
                   .arg(item.value("runtime_entries").toInt())
                   .arg(persistent_count);
    const QString storage_error = item.value("storage_error").toString();
    if (!storage_error.isEmpty())
      tier_summary += QString(" (%1)").arg(storage_error);
    const int unsafe = item.value("unsafe_persistent_entries_omitted").toInt();
    if (unsafe > 0)
      tier_summary += QString(" !%1 hidden").arg(unsafe);
    summary << tier_summary;
  };
  applyTier("stm", stm_cb_);
  applyTier("ltm", ltm_cb_);
  applyTier("episodic", episodic_cb_);
  if (memory_status_label_ && !summary.isEmpty())
    memory_status_label_->setText(summary.join("  |  "));

  const QJsonArray entries = state.value("entries").toArray();
  if (memory_entries_ && state.contains("entries") && memory_dialog_) {
    const QString selected = memory_entries_->currentItem()
                                 ? memory_entries_->currentItem()->data(Qt::UserRole).toString()
                                 : QString();
    memory_entries_->clear();
    for (const QJsonValue& value : entries) {
      const QJsonObject entry = value.toObject();
      const QString label = QString("%1 memory  ·  record %2")
          .arg(entry.value("tier").toString(), entry.value("id").toString().right(8));
      auto* row = new QListWidgetItem(label, memory_entries_);
      row->setData(Qt::UserRole, entry.value("id").toString());
      row->setData(Qt::UserRole + 1, entry.value("content").toString());
      row->setData(Qt::UserRole + 2, entry.value("tier").toString());
      row->setData(Qt::UserRole + 3, entry.value("scope").toString());
      row->setData(Qt::UserRole + 4, entry.value("title").toString());
      if (row->data(Qt::UserRole).toString() == selected)
        memory_entries_->setCurrentItem(row);
    }
  }
}

void AgentSettingsDialog::openMemoryManager() {
  if (!agent_panel_) return;
  if (memory_dialog_) {
    memory_dialog_->raise();
    memory_dialog_->activateWindow();
    return;
  }
  auto* dialog = new QDialog(this);
  memory_dialog_ = dialog;
  dialog->setObjectName("dialog:agentMemoryManager");
  dialog->setWindowTitle("Manage Agent memories");
  dialog->setMinimumSize(660, 480);
  auto* root = new QVBoxLayout(dialog);
  auto* entries = new QListWidget(dialog);
  memory_entries_ = entries;
  entries->setObjectName("control:memoryEntries");
  root->addWidget(entries, 1);
  auto* form = new QFormLayout();
  memory_tier_ = new QComboBox(dialog);
  memory_tier_->setObjectName("control:memoryTier");
  memory_tier_->addItem("Short-term (task)", "stm");
  memory_tier_->addItem("Conversation (thread)", "ltm");
  memory_tier_->addItem("Episodic (local user)", "episodic");
  memory_title_ = new QLineEdit(dialog);
  memory_title_->setObjectName("control:memoryTitle");
  memory_scope_ = new QLineEdit(dialog);
  memory_scope_->setObjectName("control:memoryScope");
  memory_content_ = new QTextEdit(dialog);
  memory_content_->setObjectName("control:memoryContent");
  memory_content_->setMaximumHeight(100);
  form->addRow("Tier:", memory_tier_);
  form->addRow("Title:", memory_title_);
  form->addRow("Scope:", memory_scope_);
  form->addRow("Content:", memory_content_);
  root->addLayout(form);
  auto* buttons = new QHBoxLayout();
  auto* add = new QPushButton("New", dialog);
  add->setObjectName("action:addMemory");
  auto* save = new QPushButton("Save memory", dialog);
  save->setObjectName("action:saveMemory");
  auto* remove = new QPushButton("Delete…", dialog);
  remove->setObjectName("action:deleteMemory");
  auto* close = new QPushButton("Close", dialog);
  close->setObjectName("action:closeMemoryManager");
  buttons->addWidget(add);
  buttons->addWidget(save);
  buttons->addWidget(remove);
  buttons->addStretch();
  buttons->addWidget(close);
  root->addLayout(buttons);
  connect(entries, &QListWidget::currentItemChanged, this,
          [this](QListWidgetItem* current) {
    if (!current) return;
    if (memory_content_) memory_content_->setPlainText(current->data(Qt::UserRole + 1).toString());
    if (memory_title_) memory_title_->setText(current->data(Qt::UserRole + 4).toString());
    if (memory_scope_) memory_scope_->setText(current->data(Qt::UserRole + 3).toString());
    const int tier = memory_tier_ ? memory_tier_->findData(current->data(Qt::UserRole + 2)) : -1;
    if (tier >= 0) memory_tier_->setCurrentIndex(tier);
  });
  connect(add, &QPushButton::clicked, this, [this]() {
    if (memory_entries_) memory_entries_->clearSelection();
    if (memory_content_) memory_content_->clear();
    if (memory_title_) memory_title_->clear();
    if (memory_scope_) memory_scope_->setText("conversation");
  });
  connect(save, &QPushButton::clicked, this, [this]() {
    if (!agent_panel_ || !memory_tier_ || !memory_content_ || !memory_scope_) return;
    QJsonObject params{{"tier", memory_tier_->currentData().toString()},
                       {"scope", memory_scope_->text().trimmed()},
                       {"title", memory_title_ ? memory_title_->text().trimmed() : QString()},
                       {"content", memory_content_->toPlainText()}};
    const auto* current = memory_entries_ ? memory_entries_->currentItem() : nullptr;
    const bool update = current && !current->data(Qt::UserRole).toString().isEmpty();
    if (update) {
      params["id"] = current->data(Qt::UserRole).toString();
      agent_panel_->sendJsonRpc("agent.memory_update", params);
    } else {
      agent_panel_->sendJsonRpc("agent.memory_add", params);
    }
    agent_panel_->sendJsonRpc("agent.memory_list", QJsonObject{});
  });
  connect(remove, &QPushButton::clicked, this, [this]() {
    const auto* current = memory_entries_ ? memory_entries_->currentItem() : nullptr;
    if (!current || !agent_panel_) return;
    if (QMessageBox::warning(this, "Delete memory", "Permanently delete this memory record?",
                             QMessageBox::Yes | QMessageBox::No,
                             QMessageBox::No) != QMessageBox::Yes) return;
    agent_panel_->sendJsonRpc("agent.memory_delete",
                              QJsonObject{{"id", current->data(Qt::UserRole).toString()},
                                          {"confirmed", true}});
    agent_panel_->sendJsonRpc("agent.memory_list", QJsonObject{});
  });
  connect(close, &QPushButton::clicked, dialog, &QDialog::close);
  connect(dialog, &QObject::destroyed, this, [this]() {
    memory_dialog_.clear();
    memory_entries_ = nullptr;
    memory_content_ = nullptr;
    memory_title_ = nullptr;
    memory_scope_ = nullptr;
    memory_tier_ = nullptr;
  });
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->show();
  agent_panel_->sendJsonRpc("agent.memory_list", QJsonObject{});
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

void AgentSettingsDialog::applyModelCatalog(const QJsonObject& catalog) {
    if (!model_combo_ || !provider_combo_ || catalog["provider"].toString() !=
        provider_combo_->currentData().toString()) return;
    if (!catalog["ok"].toBool(false)) {
        if (model_details_) model_details_->setText(
            "Model refresh unavailable: " + catalog["error"].toString());
        return;
    }
    const QJsonArray rows = catalog["models"].toArray();
    QString current = model_input_ ? model_input_->text().trimmed() : QString();
    QStringList ids;
    for (const QJsonValue& row : rows) {
        const QString id = row.toObject()["id"].toString().trimmed();
        if (!id.isEmpty()) ids << id;
    }
    if (ids.isEmpty()) return;
    model_combo_->blockSignals(true);
    model_combo_->clear();
    model_combo_->addItems(ids);
    const int match = model_combo_->findText(current);
    model_combo_->setCurrentIndex(match >= 0 ? match : 0);
    model_combo_->blockSignals(false);
    if (model_details_) model_details_->setText(
        QString("Refreshed %1 models from %2").arg(ids.size()).arg(catalog["provider"].toString()));
}

void AgentSettingsDialog::saveAllSettings() {
  QJsonObject config;

  if (provider_combo_) config["provider"] = provider_combo_->currentData().toString();
  if (model_input_) config["model"] = model_input_->text();
  if (theme_combo_) config["theme"] = theme_combo_->currentText();
  if (grid_combo_) config["grid"] = grid_combo_->currentText();
  if (grid_combo_) {
    QSettings local_settings("CCad", "Agent");
    local_settings.setValue("grid", grid_combo_->currentText());
  }
  if (autosave_cb_) config["autosave"] = autosave_cb_->isChecked();
  if (restore_session_cb_) config["restore_session"] = restore_session_cb_->isChecked();
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

  QJsonObject observability;
  if (langfuse_enabled_cb_) observability["enabled"] = langfuse_enabled_cb_->isChecked();
  observability["backend"] = "langfuse";
  if (langfuse_base_url_input_) observability["base_url"] = langfuse_base_url_input_->text().trimmed();
  if (langfuse_environment_input_) observability["environment"] = langfuse_environment_input_->text().trimmed();
  if (langfuse_service_name_input_) observability["service_name"] = langfuse_service_name_input_->text().trimmed();
  config["observability"] = observability;

  if (system_prompt_) config["system_prompt"] = system_prompt_->toPlainText();
  if (dev_prompt_) config["dev_prompt"] = dev_prompt_->toPlainText();

  if (mcp_servers_table_) {
    QJsonArray servers;
    for (int row = 0; row < mcp_servers_table_->rowCount(); ++row) {
      auto textAt = [this, row](int column) {
        const auto* item = mcp_servers_table_->item(row, column);
        return item ? item->text().trimmed() : QString();
      };
      const QString name = textAt(0);
      const QString command = textAt(1);
      if (name.isEmpty() || command.isEmpty()) continue;
      QJsonObject server;
      server["name"] = name;
      server["command"] = command;
      QJsonArray args;
      const QStringList arg_tokens = textAt(2).split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
      for (const QString& token : arg_tokens) args.append(token);
      server["args"] = args;
      bool port_ok = false;
      const int port = textAt(3).toInt(&port_ok);
      server["port"] = port_ok && port >= 0 ? port : 0;
      const auto* enabled = mcp_servers_table_->item(row, 4);
      server["enabled"] = enabled && enabled->checkState() == Qt::Checked;
      servers.append(server);
    }
    config["mcp_servers"] = servers;
  }

  if (agent_panel_ && api_key_input_ && !api_key_input_->text().isEmpty()) {
    const QString provider = provider_combo_ ? provider_combo_->currentData().toString() : "openai";
    if (!storeSecret(provider, api_key_input_->text())) {
      QMessageBox::critical(this, "Agent Settings",
                            "Windows Credential Manager rejected this key. Settings were not saved.");
      return;
    }
    agent_panel_->setProviderSecret(provider, api_key_input_->text());
  }
  if (agent_panel_) {
    const QString public_key = langfuse_public_key_input_ ? langfuse_public_key_input_->text() : QString();
    const QString secret_key = langfuse_secret_key_input_ ? langfuse_secret_key_input_->text() : QString();
    if ((!public_key.isEmpty() && !storeSecret("langfuse_public", public_key)) ||
        (!secret_key.isEmpty() && !storeSecret("langfuse_secret", secret_key))) {
      QMessageBox::critical(this, "Agent Settings",
                            "Windows Credential Manager rejected Langfuse credentials. Settings were not saved.");
      return;
    }
    agent_panel_->sendJsonRpc("agent.langfuse_set_secret", QJsonObject{
        {"public_key", public_key}, {"secret_key", secret_key}});
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
