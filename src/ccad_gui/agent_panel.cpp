#include "ccad_gui/agent_panel.hpp"

#include <QHBoxLayout>
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
    QLabel#agentStatusLabel {
      color: #111827;
      font-weight: 600;
    }
    QLineEdit#agentActionIdInput,
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

  auto* action_row = new QHBoxLayout();
  action_row->setSpacing(6);
  auto* action_label = new QLabel("Action ID", this);
  action_id_input_ = new QLineEdit("action:zoom_in", this);
  action_id_input_->setObjectName("agentActionIdInput");
  auto* refresh_button = new QPushButton("Refresh Map", this);
  refresh_button->setObjectName("agentRefreshMapButton");
  auto* trigger_button = new QPushButton("Trigger Safe", this);
  trigger_button->setObjectName("agentTriggerSafeButton");
  action_row->addWidget(action_label);
  action_row->addWidget(action_id_input_, 1);
  action_row->addWidget(refresh_button);
  action_row->addWidget(trigger_button);
  root->addLayout(action_row);

  output_ = new QPlainTextEdit(this);
  output_->setObjectName("agentOutput");
  output_->setReadOnly(true);
  output_->setMinimumHeight(96);
  output_->setPlainText("{}");
  root->addWidget(output_, 1);

  connect(refresh_button, &QPushButton::clicked, this, [this]() { refreshUiMap(); });
  connect(trigger_button, &QPushButton::clicked, this, [this]() { triggerSafeAction(); });
  connect(action_id_input_, &QLineEdit::returnPressed, this,
          [this]() { triggerSafeAction(); });
}

void AgentPanel::setUiMapProvider(UiMapProvider provider) {
  ui_map_provider_ = std::move(provider);
}

void AgentPanel::setSafeActionTrigger(SafeActionTrigger trigger) {
  safe_action_trigger_ = std::move(trigger);
}

void AgentPanel::setProjectContext(const QString& project_label, const int ui_map_epoch) {
  project_label_->setText("Project " + (project_label.isEmpty() ? QString("none") : project_label));
  epoch_label_->setText("UI map epoch " + QString::number(ui_map_epoch));
}

void AgentPanel::setActionId(const QString& action_id) {
  action_id_input_->setText(action_id);
}

void AgentPanel::refreshUiMap() {
  if (!ui_map_provider_) {
    status_label_->setText("UI map unavailable");
    output_->setPlainText("{\"error\":\"ui_map_unavailable\"}");
    return;
  }
  const QString json = ui_map_provider_();
  output_->setPlainText(json);
  const QString epoch = extractUiEpoch(json);
  if (!epoch.isEmpty()) {
    epoch_label_->setText("UI map epoch " + epoch);
  }
  status_label_->setText("UI map nodes " + QString::number(countUiMapNodes(json)));
}

void AgentPanel::triggerSafeAction() {
  if (!safe_action_trigger_) {
    status_label_->setText("Safe trigger unavailable");
    output_->setPlainText("{\"error\":\"safe_trigger_unavailable\"}");
    return;
  }
  const QString action_id = action_id_input_->text().trimmed();
  if (action_id.isEmpty()) {
    status_label_->setText("Action ID required");
    output_->setPlainText("{\"error\":\"empty_action_id\"}");
    return;
  }
  const QString result = safe_action_trigger_(action_id);
  output_->setPlainText(result);
  status_label_->setText("Safe action " + action_id);
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

QString AgentPanel::actionIdText() const {
  return action_id_input_->text();
}

QString AgentPanel::outputText() const {
  return output_->toPlainText();
}
