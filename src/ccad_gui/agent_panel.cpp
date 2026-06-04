#include "ccad_gui/agent_panel.hpp"

#include <QHBoxLayout>
#include <QFrame>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
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

QFrame* makePanelSection(const QString& object_name, QWidget* parent) {
  auto* section = new QFrame(parent);
  section->setObjectName(object_name);
  section->setProperty("agentRole", "section");
  section->setFrameShape(QFrame::NoFrame);
  section->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
  return section;
}

QLabel* makeSectionTitle(const QString& text, QWidget* parent) {
  auto* label = new QLabel(text, parent);
  label->setProperty("agentRole", "sectionTitle");
  return label;
}

QLabel* makeChip(const QString& object_name, const QString& text, QWidget* parent) {
  auto* label = new QLabel(text, parent);
  label->setObjectName(object_name);
  label->setProperty("agentRole", "chip");
  label->setTextInteractionFlags(Qt::TextSelectableByMouse);
  return label;
}

}  // namespace

AgentPanel::AgentPanel(QWidget* parent) : QWidget(parent) {
  setObjectName("agentPanel");
  setStyleSheet(R"(
    QWidget#agentPanel {
      background: #111827;
      color: #e5e7eb;
    }
    QScrollArea#agentScrollArea {
      background: #111827;
      border: 0;
    }
    QFrame[agentRole="section"] {
      background: #1f2937;
      border: 1px solid #374151;
      border-radius: 6px;
      padding: 4px;
    }
    QLabel {
      color: #e5e7eb;
    }
    QLabel[agentRole="panelTitle"] {
      color: #f9fafb;
      font-size: 15px;
      font-weight: 700;
    }
    QLabel[agentRole="sectionTitle"] {
      color: #f9fafb;
      font-weight: 600;
    }
    QLabel[agentRole="chip"] {
      background: #374151;
      color: #dbeafe;
      border: 1px solid #4b5563;
      border-radius: 4px;
      padding: 3px 6px;
    }
    QLabel#agentProjectLabel,
    QLabel#agentEpochLabel,
    QLabel#agentStatusLabel,
    QLabel#agentResultStateLabel {
      color: #f9fafb;
      font-weight: 600;
    }
    QLabel#agentWorkspaceLabel,
    QLabel#agentDiagnosticsLabel,
    QLabel#agentTaskStateLabel,
    QLabel#agentEvidenceLabel,
    QLabel#agentApprovalStatusLabel {
      color: #cbd5e1;
    }
    QLineEdit,
    QPlainTextEdit#agentOutput {
      background: #0f172a;
      color: #e5e7eb;
      border: 1px solid #475569;
      border-radius: 4px;
      padding: 5px;
      selection-background-color: #2563eb;
    }
    QPushButton {
      background: #263244;
      color: #f8fafc;
      border: 1px solid #475569;
      border-radius: 4px;
      padding: 5px 8px;
    }
    QPushButton:hover {
      background: #334155;
    }
    QPushButton:pressed {
      background: #1d4ed8;
    }
  )");

  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(0, 0, 0, 0);
  root->setSpacing(0);

  auto* scroll = new QScrollArea(this);
  scroll->setObjectName("agentScrollArea");
  scroll->setWidgetResizable(true);
  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  auto* content = new QWidget(scroll);
  content->setObjectName("panel:agent_workspace");
  auto* content_layout = new QVBoxLayout(content);
  content_layout->setContentsMargins(6, 6, 6, 6);
  content_layout->setSpacing(6);

  auto* header = makePanelSection("panel:agent_header", content);
  auto* header_layout = new QVBoxLayout(header);
  header_layout->setContentsMargins(6, 6, 6, 6);
  header_layout->setSpacing(3);
  auto* title_row = new QHBoxLayout();
  title_row->setSpacing(8);
  session_title_label_ = new QLabel("CCad Agent", header);
  session_title_label_->setObjectName("label:agent_session_title");
  session_title_label_->setProperty("agentRole", "panelTitle");
  title_row->addWidget(session_title_label_, 1);
  model_chip_label_ = makeChip("label:agent_model_chip", "Model: local", header);
  mode_chip_label_ = makeChip("label:agent_mode_chip", "Mode: plan", header);
  title_row->addWidget(model_chip_label_);
  title_row->addWidget(mode_chip_label_);
  header_layout->addLayout(title_row);
  status_label_ = new QLabel("Agent ready", header);
  status_label_->setObjectName("agentStatusLabel");
  status_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  status_label_->setWordWrap(true);
  header_layout->addWidget(status_label_);
  content_layout->addWidget(header);

  auto* context_section = makePanelSection("panel:agent_context", content);
  auto* context_layout = new QVBoxLayout(context_section);
  context_layout->setContentsMargins(6, 6, 6, 6);
  context_layout->setSpacing(3);
  context_layout->addWidget(makeSectionTitle("Workspace", context_section));
  auto* context_meta_row = new QHBoxLayout();
  context_meta_row->setSpacing(8);
  project_label_ = new QLabel("Project none", context_section);
  project_label_->setObjectName("agentProjectLabel");
  project_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  epoch_label_ = new QLabel("UI map epoch 0", context_section);
  epoch_label_->setObjectName("agentEpochLabel");
  epoch_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  context_meta_row->addWidget(project_label_, 1);
  context_meta_row->addWidget(epoch_label_);
  context_layout->addLayout(context_meta_row);
  workspace_label_ = new QLabel("View unknown | Layer -- | Net -- | Tool default", context_section);
  workspace_label_->setObjectName("agentWorkspaceLabel");
  workspace_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  workspace_label_->setWordWrap(true);
  context_layout->addWidget(workspace_label_);
  auto* result_row = new QHBoxLayout();
  result_row->setSpacing(8);
  diagnostics_label_ = new QLabel("Diagnostics not checked", context_section);
  diagnostics_label_->setObjectName("agentDiagnosticsLabel");
  diagnostics_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  result_state_label_ = new QLabel("Result Idle", context_section);
  result_state_label_->setObjectName("agentResultStateLabel");
  result_state_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  result_row->addWidget(diagnostics_label_, 1);
  result_row->addWidget(result_state_label_);
  context_layout->addLayout(result_row);
  content_layout->addWidget(context_section);

  auto* stream_section = makePanelSection("panel:agent_command_stream", content);
  auto* stream_layout = new QVBoxLayout(stream_section);
  stream_layout->setContentsMargins(6, 6, 6, 6);
  stream_layout->setSpacing(4);
  stream_layout->addWidget(makeSectionTitle("Command", stream_section));
  auto* preset_row = new QHBoxLayout();
  preset_row->setSpacing(6);
  auto* harness_button = new QPushButton("Context", stream_section);
  harness_button->setObjectName("action:agent_preset_harness_context");
  harness_button->setAccessibleName("Run agent harness context preset");
  auto* diagnostics_button = new QPushButton("Diagnostics", stream_section);
  diagnostics_button->setObjectName("action:agent_preset_project_diagnostics");
  diagnostics_button->setAccessibleName("Run project diagnostics preset");
  auto* tool_guide_button = new QPushButton("Tool Guide", stream_section);
  tool_guide_button->setObjectName("action:agent_preset_tool_guide");
  tool_guide_button->setAccessibleName("Run agent tool guide preset");
  auto* clear_button = new QPushButton("Clear", stream_section);
  clear_button->setObjectName("action:agent_clear_output");
  clear_button->setAccessibleName("Clear agent output");
  preset_row->addWidget(harness_button);
  preset_row->addWidget(diagnostics_button);
  preset_row->addWidget(tool_guide_button);
  preset_row->addStretch(1);
  preset_row->addWidget(clear_button);
  stream_layout->addLayout(preset_row);

  auto* action_row = new QHBoxLayout();
  action_row->setSpacing(6);
  auto* action_label = new QLabel("Action", stream_section);
  action_id_input_ = new QLineEdit("action:zoom_in", stream_section);
  action_id_input_->setObjectName("control:agent_action_id");
  action_id_input_->setAccessibleName("Agent action ID");
  action_id_input_->setMinimumWidth(0);
  auto* refresh_button = new QPushButton("Refresh Map", stream_section);
  refresh_button->setObjectName("action:agent_refresh_map");
  refresh_button->setAccessibleName("Refresh agent UI map");
  auto* trigger_button = new QPushButton("Trigger", stream_section);
  trigger_button->setObjectName("action:agent_trigger_safe");
  trigger_button->setAccessibleName("Trigger safe agent action");
  action_row->addWidget(action_label);
  action_row->addWidget(action_id_input_, 1);
  action_row->addWidget(refresh_button);
  action_row->addWidget(trigger_button);
  stream_layout->addLayout(action_row);

  auto* live_row = new QHBoxLayout();
  live_row->setSpacing(6);
  auto* live_method_label = new QLabel("Query", stream_section);
  live_method_input_ = new QLineEdit("ui.find", stream_section);
  live_method_input_->setObjectName("control:agent_live_method");
  live_method_input_->setAccessibleName("Agent live query method");
  live_method_input_->setMinimumWidth(0);
  live_payload_input_ =
      new QLineEdit("{\"query\":\"add\",\"role\":\"action\",\"limit\":8}", stream_section);
  live_payload_input_->setObjectName("control:agent_live_payload");
  live_payload_input_->setAccessibleName("Agent live query payload");
  live_payload_input_->setMinimumWidth(0);
  auto* live_query_button = new QPushButton("Run", stream_section);
  live_query_button->setObjectName("action:agent_live_query");
  live_query_button->setAccessibleName("Run agent live query");
  live_row->addWidget(live_method_label);
  live_row->addWidget(live_method_input_);
  live_row->addWidget(live_payload_input_, 1);
  live_row->addWidget(live_query_button);
  stream_layout->addLayout(live_row);

  output_ = new QPlainTextEdit(stream_section);
  output_->setObjectName("agentOutput");
  output_->setReadOnly(true);
  output_->setMinimumHeight(64);
  output_->setMaximumHeight(88);
  output_->setPlainText("{}");
  stream_layout->addWidget(output_);

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

  auto* task_section = makePanelSection("panel:agent_task_list", content);
  auto* task_layout = new QVBoxLayout(task_section);
  task_layout->setContentsMargins(6, 6, 6, 6);
  task_layout->setSpacing(4);
  task_layout->addWidget(makeSectionTitle("Plan", task_section));
  auto* goal_row = new QHBoxLayout();
  goal_row->setSpacing(6);
  auto* goal_label = new QLabel("Goal", task_section);
  goal_input_ = new QLineEdit(task_section);
  goal_input_->setObjectName("control:agent_goal");
  goal_input_->setAccessibleName("Agent task goal");
  goal_input_->setPlaceholderText("Task goal");
  goal_input_->setMinimumWidth(0);
  auto* stage_goal_button = new QPushButton("Stage", task_section);
  stage_goal_button->setObjectName("action:agent_stage_goal");
  stage_goal_button->setAccessibleName("Stage agent task goal");
  goal_row->addWidget(goal_label);
  goal_row->addWidget(goal_input_, 1);
  goal_row->addWidget(stage_goal_button);
  task_layout->addLayout(goal_row);
  task_state_label_ = new QLabel("Active tasks: context ready | DRC ready | visual proof pending",
                                  task_section);
  task_state_label_->setObjectName("agentTaskStateLabel");
  task_state_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  task_state_label_->setWordWrap(true);
  task_layout->addWidget(task_state_label_);
  content_layout->addWidget(task_section);

  connect(stage_goal_button, &QPushButton::clicked, this, [this]() { stageGoal(); });
  connect(goal_input_, &QLineEdit::returnPressed, this, [this]() { stageGoal(); });

  auto* evidence_section = makePanelSection("panel:agent_evidence_tray", content);
  auto* evidence_layout = new QVBoxLayout(evidence_section);
  evidence_layout->setContentsMargins(6, 6, 6, 6);
  evidence_layout->setSpacing(4);
  evidence_layout->addWidget(makeSectionTitle("Pinned Evidence", evidence_section));
  auto* evidence_row = new QHBoxLayout();
  evidence_row->setSpacing(6);
  evidence_label_ = new QLabel("Evidence 0 pinned | no cards yet", evidence_section);
  evidence_label_->setObjectName("agentEvidenceLabel");
  evidence_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  evidence_label_->setWordWrap(true);
  auto* pin_evidence_button = new QPushButton("Pin", evidence_section);
  pin_evidence_button->setObjectName("action:agent_pin_evidence");
  pin_evidence_button->setAccessibleName("Pin current agent output as evidence");
  auto* clear_evidence_button = new QPushButton("Clear", evidence_section);
  clear_evidence_button->setObjectName("action:agent_clear_evidence");
  clear_evidence_button->setAccessibleName("Clear pinned agent evidence");
  evidence_row->addWidget(evidence_label_, 1);
  evidence_row->addWidget(pin_evidence_button);
  evidence_row->addWidget(clear_evidence_button);
  evidence_layout->addLayout(evidence_row);

  connect(pin_evidence_button, &QPushButton::clicked, this, [this]() { pinEvidence(); });
  connect(clear_evidence_button, &QPushButton::clicked, this, [this]() { clearEvidence(); });

  auto* approval_section = makePanelSection("panel:agent_approval_card", content);
  auto* approval_layout = new QVBoxLayout(approval_section);
  approval_layout->setContentsMargins(6, 6, 6, 6);
  approval_layout->setSpacing(4);
  approval_layout->addWidget(makeSectionTitle("Approval Pending", approval_section));
  auto* approval_request_row = new QHBoxLayout();
  approval_request_row->setSpacing(6);
  approval_request_input_ = new QLineEdit(approval_section);
  approval_request_input_->setObjectName("control:agent_approval_request");
  approval_request_input_->setAccessibleName("Agent approval request");
  approval_request_input_->setPlaceholderText("Approval request");
  approval_request_input_->setMinimumWidth(0);
  auto* request_approval_button = new QPushButton("Request", approval_section);
  request_approval_button->setObjectName("action:agent_request_approval");
  request_approval_button->setAccessibleName("Request agent approval");
  approval_request_row->addWidget(approval_request_input_, 1);
  approval_request_row->addWidget(request_approval_button);
  approval_layout->addLayout(approval_request_row);

  auto* approval_status_row = new QHBoxLayout();
  approval_status_row->setSpacing(6);
  approval_status_label_ = new QLabel("Approvals 0 pending", approval_section);
  approval_status_label_->setObjectName("agentApprovalStatusLabel");
  approval_status_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  approval_status_label_->setWordWrap(true);
  auto* approve_button = new QPushButton("Accept", approval_section);
  approve_button->setObjectName("action:agent_approve_next");
  approve_button->setAccessibleName("Accept pending agent approval");
  auto* decline_button = new QPushButton("Decline", approval_section);
  decline_button->setObjectName("action:agent_decline_next");
  decline_button->setAccessibleName("Decline pending agent approval");
  auto* cancel_button = new QPushButton("Cancel", approval_section);
  cancel_button->setObjectName("action:agent_cancel_approval");
  cancel_button->setAccessibleName("Cancel pending agent approval");
  auto* clear_approvals_button = new QPushButton("Clear", approval_section);
  clear_approvals_button->setObjectName("action:agent_clear_approvals");
  clear_approvals_button->setAccessibleName("Clear agent approval state");
  approval_status_row->addStretch(1);
  approval_status_row->addWidget(approve_button);
  approval_status_row->addWidget(decline_button);
  approval_status_row->addWidget(cancel_button);
  approval_status_row->addWidget(clear_approvals_button);
  approval_layout->addLayout(approval_status_row);
  approval_layout->addWidget(approval_status_label_);
  content_layout->addWidget(approval_section);
  content_layout->addWidget(evidence_section);
  content_layout->addWidget(stream_section, 1);
  content_layout->addStretch(1);

  connect(request_approval_button, &QPushButton::clicked, this, [this]() { requestApproval(); });
  connect(approval_request_input_, &QLineEdit::returnPressed, this,
          [this]() { requestApproval(); });
  connect(approve_button, &QPushButton::clicked, this, [this]() { approveNextApproval(); });
  connect(decline_button, &QPushButton::clicked, this, [this]() { declineNextApproval(); });
  connect(cancel_button, &QPushButton::clicked, this, [this]() { cancelApproval(); });
  connect(clear_approvals_button, &QPushButton::clicked, this, [this]() { clearApprovals(); });

  scroll->setWidget(content);
  root->addWidget(scroll, 1);

  auto* command_bar = makePanelSection("panel:agent_command_bar", this);
  auto* command_bar_layout = new QVBoxLayout(command_bar);
  command_bar_layout->setContentsMargins(6, 6, 6, 6);
  command_bar_layout->setSpacing(4);
  auto* footer_action_row = new QHBoxLayout();
  footer_action_row->setSpacing(6);
  auto* footer_context_button = new QPushButton("Request Context", command_bar);
  footer_context_button->setObjectName("action:agent_footer_request_context");
  footer_context_button->setAccessibleName("Request agent workspace context");
  auto* footer_drc_button = new QPushButton("Trigger DRC", command_bar);
  footer_drc_button->setObjectName("action:agent_footer_trigger_drc");
  footer_drc_button->setAccessibleName("Trigger agent diagnostics query");
  footer_action_row->addWidget(footer_context_button);
  footer_action_row->addWidget(footer_drc_button);
  command_bar_layout->addLayout(footer_action_row);
  auto* prompt_row = new QHBoxLayout();
  prompt_row->setSpacing(6);
  command_input_ = new QLineEdit(command_bar);
  command_input_->setObjectName("control:agent_command_input");
  command_input_->setAccessibleName("Agent command prompt");
  command_input_->setPlaceholderText("Ask the agent to inspect, route, validate, or explain");
  command_input_->setMinimumWidth(0);
  auto* submit_command_button = new QPushButton("Send", command_bar);
  submit_command_button->setObjectName("action:agent_submit_command");
  submit_command_button->setAccessibleName("Submit agent command");
  prompt_row->addWidget(command_input_, 1);
  prompt_row->addWidget(submit_command_button);
  command_bar_layout->addLayout(prompt_row);
  root->addWidget(command_bar);

  connect(footer_context_button, &QPushButton::clicked, this,
          [this]() { runHarnessContextPreset(); });
  connect(footer_drc_button, &QPushButton::clicked, this, [this]() { runDiagnosticsPreset(); });
  connect(submit_command_button, &QPushButton::clicked, this, [this]() { submitCommand(); });
  connect(command_input_, &QLineEdit::returnPressed, this, [this]() { submitCommand(); });
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

void AgentPanel::setGoalText(const QString& goal) {
  goal_input_->setText(goal);
}

void AgentPanel::setCommandText(const QString& command) {
  command_input_->setText(command);
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

void AgentPanel::stageGoal() {
  const QString trimmed_goal = goal_input_->text().trimmed();
  if (trimmed_goal.isEmpty()) {
    staged_goal_.clear();
    task_state_label_->setText("Task idle");
    status_label_->setText("Goal required");
    result_state_label_->setText("Result Error empty_goal");
    return;
  }
  staged_goal_ = trimmed_goal;
  task_state_label_->setText("Goal staged: " + staged_goal_);
  status_label_->setText("Goal staged");
  result_state_label_->setText("Result Goal staged");
}

void AgentPanel::pinEvidence() {
  QString entry = "Evidence " + QString::number(evidence_entries_.size() + 1);
  const QString goal = staged_goal_.trimmed().isEmpty() ? goal_input_->text().trimmed() : staged_goal_;
  if (!goal.isEmpty()) {
    entry += " | goal " + goal;
  }
  const QString method = live_method_input_->text().trimmed();
  if (!method.isEmpty()) {
    entry += " | method " + method;
  }
  const QString result_state = result_state_label_->text().trimmed();
  if (!result_state.isEmpty()) {
    entry += " | " + result_state;
  }
  QString output_summary = output_->toPlainText().simplified();
  if (!output_summary.isEmpty()) {
    if (output_summary.size() > 240) {
      output_summary = output_summary.left(240) + "...";
    }
    entry += " | output " + output_summary;
  }
  evidence_entries_.append(entry);
  while (evidence_entries_.size() > 8) {
    evidence_entries_.removeFirst();
  }
  evidence_label_->setText("Evidence " + QString::number(evidence_entries_.size()) + " pinned");
  status_label_->setText("Evidence pinned");
  result_state_label_->setText("Result Evidence pinned");
}

void AgentPanel::clearEvidence() {
  evidence_entries_.clear();
  evidence_label_->setText("Evidence 0 pinned");
  status_label_->setText("Evidence cleared");
  result_state_label_->setText("Result Evidence cleared");
}

void AgentPanel::setApprovalRequestText(const QString& request) {
  approval_request_input_->setText(request);
}

void AgentPanel::requestApproval() {
  const QString trimmed_request = approval_request_input_->text().trimmed();
  if (trimmed_request.isEmpty()) {
    pending_approval_request_.clear();
    approval_last_decision_ = "none";
    approval_status_label_->setText("Approvals 0 pending");
    status_label_->setText("Approval request required");
    result_state_label_->setText("Result Approval required");
    return;
  }
  pending_approval_request_ = trimmed_request;
  approval_last_decision_ = "pending";
  approval_status_label_->setText("Approval pending: " + pending_approval_request_);
  status_label_->setText("Approval pending");
  result_state_label_->setText("Result Approval pending");
}

void AgentPanel::submitCommand() {
  const QString trimmed_command = command_input_->text().trimmed();
  if (trimmed_command.isEmpty()) {
    staged_command_.clear();
    status_label_->setText("Command required");
    result_state_label_->setText("Result Error empty_command");
    output_->setPlainText("{\"error\":\"empty_command\"}\n");
    return;
  }

  staged_command_ = trimmed_command;
  task_state_label_->setText("Command staged: " + staged_command_);
  status_label_->setText("Command staged");
  result_state_label_->setText("Result Command staged");

  QJsonObject event;
  event.insert("schema_version", 1);
  event.insert("event", "agent_command_staged");
  event.insert("command", staged_command_);
  const QString goal = staged_goal_.isEmpty() ? goal_input_->text().trimmed() : staged_goal_;
  if (!goal.isEmpty()) {
    event.insert("goal", goal);
  }
  event.insert("workspace", workspaceText());
  event.insert("diagnostics", diagnosticsText());
  output_->setPlainText(QString::fromUtf8(QJsonDocument(event).toJson(QJsonDocument::Compact)) +
                        "\n");
}

void AgentPanel::approveNextApproval() {
  if (pending_approval_request_.trimmed().isEmpty()) {
    approval_status_label_->setText("Approvals 0 pending");
    status_label_->setText("No approval pending");
    result_state_label_->setText("Result No approval pending");
    return;
  }
  const QString request = pending_approval_request_;
  pending_approval_request_.clear();
  approval_last_decision_ = "accept";
  approval_status_label_->setText("Approval accepted: " + request);
  status_label_->setText("Approval accepted");
  result_state_label_->setText("Result Approval accepted");
}

void AgentPanel::declineNextApproval() {
  if (pending_approval_request_.trimmed().isEmpty()) {
    approval_status_label_->setText("Approvals 0 pending");
    status_label_->setText("No approval pending");
    result_state_label_->setText("Result No approval pending");
    return;
  }
  const QString request = pending_approval_request_;
  pending_approval_request_.clear();
  approval_last_decision_ = "decline";
  approval_status_label_->setText("Approval declined: " + request);
  status_label_->setText("Approval declined");
  result_state_label_->setText("Result Approval declined");
}

void AgentPanel::cancelApproval() {
  if (pending_approval_request_.trimmed().isEmpty()) {
    approval_status_label_->setText("Approvals 0 pending");
    status_label_->setText("No approval pending");
    result_state_label_->setText("Result No approval pending");
    return;
  }
  const QString request = pending_approval_request_;
  pending_approval_request_.clear();
  approval_last_decision_ = "cancel";
  approval_status_label_->setText("Approval canceled: " + request);
  status_label_->setText("Approval canceled");
  result_state_label_->setText("Result Approval canceled");
}

void AgentPanel::clearApprovals() {
  pending_approval_request_.clear();
  approval_request_input_->clear();
  approval_last_decision_ = "none";
  approval_status_label_->setText("Approvals 0 pending");
  status_label_->setText("Approvals cleared");
  result_state_label_->setText("Result Approvals cleared");
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

QString AgentPanel::goalText() const {
  return goal_input_->text();
}

QString AgentPanel::commandText() const {
  return command_input_->text();
}

QString AgentPanel::taskStateText() const {
  return task_state_label_->text();
}

QString AgentPanel::evidenceText() const {
  return evidence_label_->text();
}

QString AgentPanel::approvalRequestText() const {
  return approval_request_input_->text();
}

QString AgentPanel::approvalStatusText() const {
  return approval_status_label_->text();
}

int AgentPanel::pendingApprovalCount() const {
  return pending_approval_request_.trimmed().isEmpty() ? 0 : 1;
}

QString AgentPanel::outputText() const {
  return output_->toPlainText();
}

QString AgentPanel::workspaceStateJson() const {
  QJsonArray evidence;
  for (const QString& entry : evidence_entries_) {
    evidence.append(entry);
  }

  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("workspace_kind", "ccad_agent_workspace_state");
  response.insert("panel_layout", "vertical_agent_workspace");
  response.insert("session_title", session_title_label_->text());
  response.insert("model_label", model_chip_label_->text());
  response.insert("mode_label", mode_chip_label_->text());
  response.insert("command", staged_command_.isEmpty() ? commandText().trimmed() : staged_command_);
  response.insert("command_input", commandText());
  response.insert("goal", staged_goal_.isEmpty() ? goal_input_->text().trimmed() : staged_goal_);
  response.insert("task_state", taskStateText());
  response.insert("evidence_count", evidence_entries_.size());
  response.insert("evidence", evidence);
  response.insert("approval_pending_count", pendingApprovalCount());
  response.insert("approval_request", pending_approval_request_);
  response.insert("approval_input", approvalRequestText());
  response.insert("approval_status", approvalStatusText());
  response.insert("approval_last_decision", approval_last_decision_);
  response.insert("project", projectText());
  response.insert("ui_epoch", epochText());
  response.insert("workspace", workspaceText());
  response.insert("diagnostics", diagnosticsText());
  response.insert("status", statusText());
  response.insert("result_state", resultStateText());
  response.insert("action_id", actionIdText());
  response.insert("live_method", liveMethodText());
  response.insert("live_payload", livePayloadText());
  return QString::fromUtf8(QJsonDocument(response).toJson(QJsonDocument::Compact)) + "\n";
}
