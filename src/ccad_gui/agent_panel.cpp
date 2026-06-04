#include "ccad_gui/agent_panel.hpp"

#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include <utility>

namespace {

int countUiMapNodes(const QString& json) {
  return json.count("\"id\":");
}

QString extractUiEpoch(const QString& json) {
  const QString key = "\"ui_epoch\":";
  int start = json.indexOf(key);
  if (start < 0) {
    return {};
  }
  start += key.size();
  int end = start;
  while (end < json.size() && json.at(end).isDigit()) {
    ++end;
  }
  return json.mid(start, end - start);
}

QJsonObject parsedObject(const QString& json) {
  QJsonParseError error{};
  const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8(), &error);
  if (error.error != QJsonParseError::NoError || !document.isObject()) {
    return {};
  }
  return document.object();
}

QString resultSummaryFromJson(const QString& json, const QString& fallback) {
  const QJsonObject object = parsedObject(json);
  if (object.isEmpty()) {
    return fallback;
  }
  if (object.contains("ok")) {
    if (object.value("ok").toBool(false)) {
      const QString method = object.value("method").toString();
      return method.isEmpty() ? QString("Result OK") : QString("Result OK ") + method;
    }
    const QString error = object.value("error").toString();
    return error.isEmpty() ? QString("Result Error") : QString("Result Error ") + error;
  }
  if (object.contains("performed")) {
    const QString reason = object.value("reason").toString();
    if (object.value("performed").toBool(false)) {
      return reason.isEmpty() ? QString("Result Performed")
                              : QString("Result Performed ") + reason;
    }
    return reason.isEmpty() ? QString("Result Not performed")
                            : QString("Result Not performed ") + reason;
  }
  if (object.contains("error")) {
    return QString("Result Error ") + object.value("error").toString();
  }
  return fallback;
}

QString contextValue(const QString& value, const QString& fallback) {
  const QString trimmed = value.trimmed();
  return trimmed.isEmpty() ? fallback : trimmed;
}

}  // namespace

AgentPanel::AgentPanel(QWidget* parent) : QWidget(parent) {
  setObjectName("agentPanel");
  setStyleSheet(R"(
    QWidget#agentPanel {
      background: #f8fafc;
      color: #111827;
    }
    QLabel#agentProjectLabel,
    QLabel#agentEpochLabel,
    QLabel#agentStatusLabel,
    QLabel#agentResultStateLabel {
      color: #111827;
      font-weight: 600;
    }
    QLabel#agentWorkspaceLabel,
    QLabel#agentDiagnosticsLabel {
      color: #334155;
    }
    QLineEdit,
    QPlainTextEdit#agentOutput {
      background: #ffffff;
      color: #111827;
      border: 1px solid #cbd5e1;
      border-radius: 4px;
      padding: 4px;
    }
  )");

  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(8, 8, 8, 8);
  root->setSpacing(6);

  auto* status_row = new QHBoxLayout();
  status_row->setSpacing(12);
  project_label_ = new QLabel("Project none", this);
  project_label_->setObjectName("agentProjectLabel");
  epoch_label_ = new QLabel("UI map epoch 0", this);
  epoch_label_->setObjectName("agentEpochLabel");
  status_label_ = new QLabel("Agent ready", this);
  status_label_->setObjectName("agentStatusLabel");
  status_row->addWidget(project_label_);
  status_row->addWidget(epoch_label_);
  status_row->addWidget(status_label_, 1);
  root->addLayout(status_row);

  workspace_label_ = new QLabel("View unknown | Layer -- | Net -- | Tool default", this);
  workspace_label_->setObjectName("agentWorkspaceLabel");
  workspace_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  workspace_label_->setWordWrap(true);
  root->addWidget(workspace_label_);

  auto* evidence_row = new QHBoxLayout();
  evidence_row->setSpacing(12);
  diagnostics_label_ = new QLabel("Diagnostics not checked", this);
  diagnostics_label_->setObjectName("agentDiagnosticsLabel");
  diagnostics_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  result_state_label_ = new QLabel("Result Idle", this);
  result_state_label_->setObjectName("agentResultStateLabel");
  result_state_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  evidence_row->addWidget(diagnostics_label_);
  evidence_row->addWidget(result_state_label_, 1);
  root->addLayout(evidence_row);

  auto* preset_row = new QHBoxLayout();
  preset_row->setSpacing(6);
  auto* harness_button = new QPushButton("Context", this);
  harness_button->setObjectName("action:agent_preset_harness_context");
  harness_button->setAccessibleName("Run agent harness context preset");
  auto* diagnostics_button = new QPushButton("Diagnostics", this);
  diagnostics_button->setObjectName("action:agent_preset_project_diagnostics");
  diagnostics_button->setAccessibleName("Run project diagnostics preset");
  auto* tool_guide_button = new QPushButton("Tool Guide", this);
  tool_guide_button->setObjectName("action:agent_preset_tool_guide");
  tool_guide_button->setAccessibleName("Run agent tool guide preset");
  auto* clear_button = new QPushButton("Clear", this);
  clear_button->setObjectName("action:agent_clear_output");
  clear_button->setAccessibleName("Clear agent output");
  preset_row->addWidget(harness_button);
  preset_row->addWidget(diagnostics_button);
  preset_row->addWidget(tool_guide_button);
  preset_row->addStretch(1);
  preset_row->addWidget(clear_button);
  root->addLayout(preset_row);

  auto* action_row = new QHBoxLayout();
  action_row->setSpacing(6);
  auto* action_label = new QLabel("Action ID", this);
  action_id_input_ = new QLineEdit("action:zoom_in", this);
  action_id_input_->setObjectName("control:agent_action_id");
  action_id_input_->setAccessibleName("Agent action ID");
  auto* refresh_button = new QPushButton("Refresh Map", this);
  refresh_button->setObjectName("action:agent_refresh_map");
  refresh_button->setAccessibleName("Refresh agent UI map");
  auto* trigger_button = new QPushButton("Trigger Safe", this);
  trigger_button->setObjectName("action:agent_trigger_safe");
  trigger_button->setAccessibleName("Trigger safe agent action");
  action_row->addWidget(action_label);
  action_row->addWidget(action_id_input_, 1);
  action_row->addWidget(refresh_button);
  action_row->addWidget(trigger_button);
  root->addLayout(action_row);

  auto* live_row = new QHBoxLayout();
  live_row->setSpacing(6);
  auto* live_method_label = new QLabel("Method", this);
  live_method_input_ = new QLineEdit("ui.find", this);
  live_method_input_->setObjectName("control:agent_live_method");
  live_method_input_->setAccessibleName("Agent live query method");
  live_payload_input_ =
      new QLineEdit("{\"query\":\"add\",\"role\":\"action\",\"limit\":8}", this);
  live_payload_input_->setObjectName("control:agent_live_payload");
  live_payload_input_->setAccessibleName("Agent live query payload");
  auto* live_query_button = new QPushButton("Live Query", this);
  live_query_button->setObjectName("action:agent_live_query");
  live_query_button->setAccessibleName("Run agent live query");
  live_row->addWidget(live_method_label);
  live_row->addWidget(live_method_input_);
  live_row->addWidget(live_payload_input_, 1);
  live_row->addWidget(live_query_button);
  root->addLayout(live_row);

  output_ = new QPlainTextEdit(this);
  output_->setObjectName("agentOutput");
  output_->setReadOnly(true);
  output_->setMinimumHeight(72);
  output_->setPlainText("{}");
  root->addWidget(output_, 1);

  connect(refresh_button, &QPushButton::clicked, this, [this]() { refreshUiMap(); });
  connect(trigger_button, &QPushButton::clicked, this, [this]() { triggerSafeAction(); });
  connect(action_id_input_, &QLineEdit::returnPressed, this,
          [this]() { triggerSafeAction(); });
  connect(live_query_button, &QPushButton::clicked, this, [this]() { runLiveQuery(); });
  connect(live_method_input_, &QLineEdit::returnPressed, this, [this]() { runLiveQuery(); });
  connect(live_payload_input_, &QLineEdit::returnPressed, this, [this]() { runLiveQuery(); });
  connect(harness_button, &QPushButton::clicked, this, [this]() { runHarnessContextPreset(); });
  connect(diagnostics_button, &QPushButton::clicked, this, [this]() { runDiagnosticsPreset(); });
  connect(tool_guide_button, &QPushButton::clicked, this, [this]() { runToolGuidePreset(); });
  connect(clear_button, &QPushButton::clicked, this, [this]() { clearOutput(); });
}

void AgentPanel::setUiMapProvider(UiMapProvider provider) {
  ui_map_provider_ = std::move(provider);
}

void AgentPanel::setSafeActionTrigger(SafeActionTrigger trigger) {
  safe_action_trigger_ = std::move(trigger);
}

void AgentPanel::setLiveQueryProvider(LiveQueryProvider provider) {
  live_query_provider_ = std::move(provider);
}

void AgentPanel::setProjectContext(const QString& project_label, const int ui_map_epoch) {
  project_label_->setText("Project " + (project_label.isEmpty() ? QString("none") : project_label));
  epoch_label_->setText("UI map epoch " + QString::number(ui_map_epoch));
}

void AgentPanel::setWorkspaceContext(const QString& active_view,
                                     const QString& active_layer,
                                     const QString& active_net,
                                     const QString& interaction_mode,
                                     const int error_count,
                                     const int warning_count) {
  workspace_label_->setText("View " + contextValue(active_view, "unknown") +
                            " | Layer " + contextValue(active_layer, "--") +
                            " | Net " + contextValue(active_net, "--") +
                            " | Tool " + contextValue(interaction_mode, "default"));
  if (error_count >= 0 && warning_count >= 0) {
    diagnostics_label_->setText("Diagnostics " + QString::number(error_count) +
                                " errors / " + QString::number(warning_count) +
                                " warnings");
  } else {
    diagnostics_label_->setText("Diagnostics use preset");
  }
}

void AgentPanel::setActionId(const QString& action_id) {
  action_id_input_->setText(action_id);
}

void AgentPanel::setLiveQuery(const QString& method, const QString& payload) {
  live_method_input_->setText(method);
  live_payload_input_->setText(payload);
}

void AgentPanel::refreshUiMap() {
  if (!ui_map_provider_) {
    status_label_->setText("UI map unavailable");
    output_->setPlainText("{\"error\":\"ui_map_unavailable\"}");
    result_state_label_->setText("Result Error ui_map_unavailable");
    return;
  }
  const QString json = ui_map_provider_();
  output_->setPlainText(json);
  const QString epoch = extractUiEpoch(json);
  if (!epoch.isEmpty()) {
    epoch_label_->setText("UI map epoch " + epoch);
  }
  status_label_->setText("UI map nodes " + QString::number(countUiMapNodes(json)));
  result_state_label_->setText("Result Map refreshed");
}

void AgentPanel::triggerSafeAction() {
  if (!safe_action_trigger_) {
    status_label_->setText("Safe trigger unavailable");
    output_->setPlainText("{\"error\":\"safe_trigger_unavailable\"}");
    result_state_label_->setText("Result Error safe_trigger_unavailable");
    return;
  }
  const QString action_id = action_id_input_->text().trimmed();
  if (action_id.isEmpty()) {
    status_label_->setText("Action ID required");
    output_->setPlainText("{\"error\":\"empty_action_id\"}");
    result_state_label_->setText("Result Error empty_action_id");
    return;
  }
  const QString result = safe_action_trigger_(action_id);
  output_->setPlainText(result);
  status_label_->setText("Safe action " + action_id);
  result_state_label_->setText(resultSummaryFromJson(result, "Result Safe action"));
}

void AgentPanel::runLiveQuery() {
  if (!live_query_provider_) {
    status_label_->setText("Live query unavailable");
    output_->setPlainText("{\"error\":\"live_query_unavailable\"}");
    result_state_label_->setText("Result Error live_query_unavailable");
    return;
  }
  const QString method = live_method_input_->text().trimmed();
  if (method.isEmpty()) {
    status_label_->setText("Live method required");
    output_->setPlainText("{\"error\":\"empty_live_method\"}");
    result_state_label_->setText("Result Error empty_live_method");
    return;
  }
  const QString payload = live_payload_input_->text().trimmed();
  const QString result = live_query_provider_(method, payload.isEmpty() ? "{}" : payload);
  output_->setPlainText(result);
  const QString epoch = extractUiEpoch(result);
  if (!epoch.isEmpty()) {
    epoch_label_->setText("UI map epoch " + epoch);
  }
  status_label_->setText("Live query " + method);
  result_state_label_->setText(resultSummaryFromJson(result, "Result Live query"));
}

void AgentPanel::runHarnessContextPreset() {
  setLiveQuery("agent.harness_context", "{}");
  runLiveQuery();
}

void AgentPanel::runDiagnosticsPreset() {
  setLiveQuery("project.diagnostics", "{}");
  runLiveQuery();
}

void AgentPanel::runToolGuidePreset() {
  setLiveQuery("agent.tool_guide", "{\"method_name\":\"ui.route_track\"}");
  runLiveQuery();
}

void AgentPanel::clearOutput() {
  output_->clear();
  status_label_->setText("Output cleared");
  result_state_label_->setText("Result Idle");
}

QString AgentPanel::projectText() const {
  return project_label_->text();
}

QString AgentPanel::epochText() const {
  return epoch_label_->text();
}

QString AgentPanel::statusText() const {
  return status_label_->text();
}

QString AgentPanel::workspaceText() const {
  return workspace_label_->text();
}

QString AgentPanel::diagnosticsText() const {
  return diagnostics_label_->text();
}

QString AgentPanel::resultStateText() const {
  return result_state_label_->text();
}

QString AgentPanel::actionIdText() const {
  return action_id_input_->text();
}

QString AgentPanel::liveMethodText() const {
  return live_method_input_->text();
}

QString AgentPanel::livePayloadText() const {
  return live_payload_input_->text();
}

QString AgentPanel::outputText() const {
  return output_->toPlainText();
}
