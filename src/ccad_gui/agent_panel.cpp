#include "ccad_gui/agent_panel.hpp"

#include "ccad_core/agent_policy.hpp"

#include <QByteArray>
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollArea>
#include <QSize>
#include <QSizePolicy>
#include <QStyle>
#include <QStringList>
#include <QVBoxLayout>

#include <algorithm>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

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

QStringList splitPolicyCommandLine(const QString& command) {
  QStringList tokens;
  QString token;
  bool in_quote = false;
  QChar quote_char;
  bool escaping = false;
  for (const QChar ch : command.trimmed()) {
    if (escaping) {
      token.append(ch);
      escaping = false;
      continue;
    }
    if (ch == '\\') {
      escaping = true;
      continue;
    }
    if (in_quote) {
      if (ch == quote_char) {
        in_quote = false;
      } else {
        token.append(ch);
      }
      continue;
    }
    if (ch == '\'' || ch == '"') {
      in_quote = true;
      quote_char = ch;
      continue;
    }
    if (ch.isSpace()) {
      if (!token.isEmpty()) {
        tokens.append(token);
        token.clear();
      }
      continue;
    }
    token.append(ch);
  }
  if (!token.isEmpty()) {
    tokens.append(token);
  }
  return tokens;
}

bool isCcadExecutableToken(const QString& token) {
  const QString file_name = QFileInfo(token).fileName().toLower();
  return file_name == "ccad" || file_name == "ccad.exe";
}

bool isKnownCcadCommandHead(const QString& token) {
  static const QStringList commands = {"help",      "validate", "drc", "inspect",
                                       "diff",      "init",     "project",
                                       "pcb",       "sch",      "schematic",
                                       "lib",       "agent"};
  return commands.contains(token);
}

std::vector<std::string> policyArgsFromCommand(const QString& command,
                                               QStringList* normalized_args) {
  QStringList tokens = splitPolicyCommandLine(command);
  bool explicit_ccad = false;
  if (!tokens.isEmpty() && isCcadExecutableToken(tokens.front())) {
    explicit_ccad = true;
    tokens.removeFirst();
  }
  if (tokens.isEmpty()) {
    if (normalized_args != nullptr) {
      normalized_args->clear();
    }
    return {};
  }
  if (!explicit_ccad && !isKnownCcadCommandHead(tokens.front())) {
    if (normalized_args != nullptr) {
      normalized_args->clear();
    }
    return {};
  }
  std::vector<std::string> args;
  args.reserve(static_cast<std::size_t>(tokens.size()));
  for (const QString& token : tokens) {
    args.push_back(token.toStdString());
  }
  if (normalized_args != nullptr) {
    *normalized_args = tokens;
  }
  return args;
}

QJsonObject readJsonFileObject(const QString& path, QString* error_message) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    if (error_message != nullptr) {
      *error_message = "failed to open session file";
    }
    return {};
  }
  QJsonParseError parse_error{};
  const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parse_error);
  if (parse_error.error != QJsonParseError::NoError || !document.isObject()) {
    if (error_message != nullptr) {
      *error_message = "malformed session JSON";
    }
    return {};
  }
  return document.object();
}

bool writeJsonFileObject(const QString& path,
                         const QJsonObject& object,
                         QString* error_message) {
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    if (error_message != nullptr) {
      *error_message = "failed to write session file";
    }
    return false;
  }
  file.write(QJsonDocument(object).toJson(QJsonDocument::Compact));
  if (!file.commit()) {
    if (error_message != nullptr) {
      *error_message = "failed to commit session file";
    }
    return false;
  }
  return true;
}

QString checkpointResourceUri(const QString& session_id, const QString& checkpoint_id) {
  return "ccad-agent-checkpoint:" + session_id + "/" + checkpoint_id;
}

QString nextGuiCheckpointId(const QJsonArray& checkpoints) {
  int sequence = checkpoints.size() + 1;
  while (true) {
    const QString candidate = "gui-checkpoint-" + QString::number(sequence);
    bool exists = false;
    for (const QJsonValue& value : checkpoints) {
      if (value.isObject() &&
          value.toObject().value("checkpoint_id").toString() == candidate) {
        exists = true;
        break;
      }
    }
    if (!exists) {
      return candidate;
    }
    ++sequence;
  }
}

AgentPanel::AgentSessionMetadata metadataFromSessionObject(const QJsonObject& object,
                                                           QString* error_message) {
  AgentPanel::AgentSessionMetadata metadata;
  const QString kind = object.value("session_kind").toString();
  if (!kind.isEmpty() && kind != "ccad_agent_session") {
    if (error_message != nullptr) {
      *error_message = "not a CCad agent session";
    }
    return metadata;
  }
  metadata.session_id = object.value("session_id").toString().trimmed();
  if (metadata.session_id.isEmpty()) {
    if (error_message != nullptr) {
      *error_message = "session_id missing";
    }
    return metadata;
  }
  metadata.thread_id = object.value("thread_id").toString().trimmed();
  if (metadata.thread_id.isEmpty()) {
    metadata.thread_id = metadata.session_id;
  }
  metadata.title = object.value("title").toString().trimmed();
  metadata.project_path = object.value("project_path").toString().trimmed();
  const QJsonArray checkpoints = object.value("checkpoints").toArray();
  metadata.checkpoint_count = checkpoints.size();
  metadata.replayable = metadata.checkpoint_count > 0;
  if (!checkpoints.isEmpty() && checkpoints.last().isObject()) {
    metadata.latest_checkpoint_id =
        checkpoints.last().toObject().value("checkpoint_id").toString().trimmed();
  }
  return metadata;
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

QJsonObject resultObjectFromAgentOutput(const QJsonObject& root) {
  const QJsonValue result = root.value("result");
  if (result.isObject()) {
    return result.toObject();
  }
  return root;
}

QString methodFromAgentOutput(const QJsonObject& root, const QString& fallback) {
  const QString method = root.value("method").toString().trimmed();
  return method.isEmpty() ? fallback.trimmed() : method;
}

QString evidenceKindForMethod(const QString& method) {
  if (method == "ui.screenshot") {
    return "screenshot";
  }
  if (method == "project.drc") {
    return "drc_report";
  }
  if (method == "project.erc") {
    return "erc_report";
  }
  if (method == "project.diagnostics") {
    return "diagnostics_report";
  }
  if (method == "project.review") {
    return "review_report";
  }
  return "tool_result";
}

QString evidenceTitleForMethod(const QString& method, const QString& kind) {
  if (method.isEmpty()) {
    return kind == "tool_result" ? QString("Agent Evidence") : kind;
  }
  return method;
}

int intValueFromJson(const QJsonObject& object, const QString& key) {
  const QJsonValue value = object.value(key);
  if (value.isDouble()) {
    return value.toInt();
  }
  if (value.isString()) {
    bool ok = false;
    const int parsed = value.toString().toInt(&ok);
    if (ok) {
      return parsed;
    }
  }
  return -1;
}

QString stringValueFromJson(const QJsonObject& object, const QStringList& keys) {
  for (const QString& key : keys) {
    const QString value = object.value(key).toString().trimmed();
    if (!value.isEmpty()) {
      return value;
    }
  }
  return {};
}

QString summarizeEvidenceCard(const QString& kind,
                              const QString& result_state,
                              const QJsonObject& result,
                              const QString& output_text) {
  const int diagnostic_count = intValueFromJson(result, "diagnostic_count");
  const int error_count = intValueFromJson(result, "error_count");
  const int warning_count = intValueFromJson(result, "warning_count");
  if (kind == "screenshot") {
    const int width = intValueFromJson(result, "width");
    const int height = intValueFromJson(result, "height");
    const QString path = stringValueFromJson(result, {"artifact_path", "path", "target_path"});
    QString summary = "Screenshot";
    if (width >= 0 && height >= 0) {
      summary += " " + QString::number(width) + " x " + QString::number(height);
    }
    if (!path.isEmpty()) {
      summary += " | " + path;
    }
    return summary;
  }
  if (kind == "drc_report" || kind == "erc_report") {
    QString summary = kind == "drc_report" ? "DRC report" : "ERC report";
    if (diagnostic_count >= 0) {
      summary += " | diagnostics " + QString::number(diagnostic_count);
    }
    if (error_count >= 0 || warning_count >= 0) {
      summary += " | " + QString::number(std::max(error_count, 0)) + " errors / " +
                 QString::number(std::max(warning_count, 0)) + " warnings";
    }
    return summary;
  }
  if (kind == "diagnostics_report") {
    QString summary = "Project diagnostics";
    const int erc_count = intValueFromJson(result, "erc_count");
    const int drc_count = intValueFromJson(result, "drc_count");
    if (erc_count >= 0) {
      summary += " | ERC " + QString::number(erc_count);
    }
    if (drc_count >= 0) {
      summary += " | DRC " + QString::number(drc_count);
    }
    if (error_count >= 0 || warning_count >= 0) {
      summary += " | " + QString::number(std::max(error_count, 0)) + " errors / " +
                 QString::number(std::max(warning_count, 0)) + " warnings";
    }
    return summary;
  }

  const QString trimmed_state = result_state.trimmed();
  if (!trimmed_state.isEmpty()) {
    return trimmed_state;
  }
  QString output_summary = output_text.simplified();
  if (output_summary.size() > 160) {
    output_summary = output_summary.left(160) + "...";
  }
  return output_summary.isEmpty() ? QString("Pinned agent output") : output_summary;
}

QString contextValue(const QString& value, const QString& fallback) {
  const QString trimmed = value.trimmed();
  return trimmed.isEmpty() ? fallback : trimmed;
}

struct AgentProviderSpec {
  QString id;
  QString label;
  QString access_path;
  QStringList env_vars;
  QString model_env;
  QString security_note;
};

const QVector<AgentProviderSpec>& agentProviderSpecs() {
  static const QVector<AgentProviderSpec> specs = {
      {"openai",
       "OpenAI API",
       "official_api",
       {"OPENAI_API_KEY", "OPENAI_ORG_ID", "OPENAI_PROJECT_ID", "CCAD_OPENAI_MODEL"},
       "CCAD_OPENAI_MODEL",
       "Use environment or secret-manager credentials only."},
      {"anthropic",
       "Anthropic Claude API",
       "anthropic_api",
       {"ANTHROPIC_API_KEY", "ANTHROPIC_BASE_URL", "CCAD_ANTHROPIC_MODEL"},
       "CCAD_ANTHROPIC_MODEL",
       "Do not automate consumer Claude browser sessions."},
      {"google_gemini",
       "Google Gemini API",
       "google_gemini_api",
       {"GEMINI_API_KEY", "GOOGLE_API_KEY", "CCAD_GEMINI_MODEL"},
       "CCAD_GEMINI_MODEL",
       "Treat Gemini keys like passwords and restrict them where possible."},
      {"openai_compatible",
       "OpenAI-compatible API",
       "openai_compatible_api",
       {"CCAD_OPENAI_COMPATIBLE_API_KEY", "CCAD_OPENAI_COMPATIBLE_BASE_URL",
        "CCAD_OPENAI_COMPATIBLE_MODEL"},
       "CCAD_OPENAI_COMPATIBLE_MODEL",
       "Use only provider-approved compatible endpoints."},
      {"local_model_server",
       "Local model server",
       "local_model_server",
       {"CCAD_LOCAL_MODEL_BASE_URL", "CCAD_LOCAL_MODEL_NAME", "CCAD_LOCAL_MODEL_API_KEY"},
       "CCAD_LOCAL_MODEL_NAME",
       "Keep localhost model access explicit and auditable."}};
  return specs;
}

AgentProviderSpec providerSpecForId(const QString& id) {
  for (const AgentProviderSpec& spec : agentProviderSpecs()) {
    if (spec.id == id) {
      return spec;
    }
  }
  return agentProviderSpecs().front();
}

AgentProviderSpec currentProviderSpec(const QComboBox* selector) {
  if (selector == nullptr) {
    return agentProviderSpecs().front();
  }
  const QString id = selector->currentData().toString().trimmed();
  return providerSpecForId(id.isEmpty() ? "openai" : id);
}

bool environmentVariablePresent(const QString& name) {
  const QByteArray bytes = name.toUtf8();
  const char* value = std::getenv(bytes.constData());
  return value != nullptr && value[0] != '\0';
}

bool providerEnvironmentPresent(const AgentProviderSpec& spec) {
  for (const QString& env_var : spec.env_vars) {
    if (environmentVariablePresent(env_var)) {
      return true;
    }
  }
  return false;
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

QPushButton* makeIconButton(const QString& object_name,
                            const QString& accessible_name,
                            const QIcon& icon,
                            QWidget* parent) {
  auto* button = new QPushButton(parent);
  button->setObjectName(object_name);
  button->setAccessibleName(accessible_name);
  button->setToolTip(accessible_name);
  button->setIcon(icon);
  button->setIconSize(QSize(16, 16));
  button->setProperty("agentRole", "iconButton");
  button->setFixedSize(28, 26);
  button->setFocusPolicy(Qt::StrongFocus);
  return button;
}

QFrame* makePlanRow(const QString& object_name,
                    const QString& title,
                    const QString& detail,
                    const QString& state,
                    const int progress,
                    QWidget* parent) {
  auto* row = new QFrame(parent);
  row->setObjectName(object_name);
  row->setProperty("agentRole", "planRow");
  auto* layout = new QVBoxLayout(row);
  layout->setContentsMargins(6, 5, 6, 5);
  layout->setSpacing(3);

  auto* title_row = new QHBoxLayout();
  title_row->setSpacing(6);
  auto* title_label = new QLabel(title, row);
  title_label->setProperty("agentRole", "planTitle");
  title_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
  auto* state_label = new QLabel(state, row);
  state_label->setProperty("agentRole", "planState");
  state_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
  title_row->addWidget(title_label, 1);
  title_row->addWidget(state_label);
  layout->addLayout(title_row);

  auto* detail_label = new QLabel(detail, row);
  detail_label->setProperty("agentRole", "planDetail");
  detail_label->setWordWrap(true);
  detail_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
  layout->addWidget(detail_label);

  auto* progress_bar = new QProgressBar(row);
  progress_bar->setObjectName(object_name + ":progress");
  progress_bar->setRange(0, 100);
  progress_bar->setValue(progress);
  progress_bar->setTextVisible(false);
  progress_bar->setFixedHeight(6);
  layout->addWidget(progress_bar);
  return row;
}

}  // namespace

AgentPanel::AgentPanel(QWidget* parent) : QWidget(parent) {
  setObjectName("agentPanel");
  setStyleSheet(R"(
    QWidget#agentPanel {
      background: #14171c;
      color: #e9edf3;
    }
    QScrollArea#agentScrollArea {
      background: #14171c;
      border: 0;
    }
    QFrame[agentRole="section"] {
      background: #20242b;
      border: 1px solid #363d48;
      border-radius: 6px;
      padding: 4px;
    }
    QFrame[agentRole="sessionStrip"] {
      background: #191d23;
      border: 1px solid #333b47;
      border-radius: 6px;
      padding: 2px;
    }
    QFrame[agentRole="modeStrip"] {
      background: #242932;
      border: 1px solid #3b4350;
      border-radius: 6px;
      padding: 2px;
    }
    QFrame[agentRole="tabStrip"] {
      background: #171b21;
      border: 1px solid #303743;
      border-radius: 6px;
      padding: 3px;
    }
    QFrame[agentRole="statusRail"] {
      background: #101820;
      border: 1px solid #35516b;
      border-radius: 6px;
      padding: 4px;
    }
    QFrame[agentRole="commandComposer"] {
      background: #151a21;
      border: 1px solid #415166;
      border-radius: 6px;
      padding: 4px;
    }
    QFrame[agentRole="planDeck"],
    QFrame[agentRole="evidenceLane"],
    QFrame[agentRole="approvalLane"] {
      background: #161b22;
      border: 1px solid #334155;
      border-radius: 6px;
      padding: 4px;
    }
    QLabel {
      color: #e9edf3;
    }
    QLabel[agentRole="panelTitle"] {
      color: #f8fafc;
      font-size: 15px;
      font-weight: 700;
    }
    QLabel[agentRole="sectionTitle"] {
      color: #f8fafc;
      font-weight: 600;
    }
    QLabel[agentRole="chip"] {
      background: #2b3038;
      color: #dfe7f2;
      border: 1px solid #444c59;
      border-radius: 4px;
      padding: 2px 5px;
    }
    QLabel[agentRole="permissionChip"] {
      background: #203629;
      color: #9ae6b4;
      border: 1px solid #3a6b4b;
      border-radius: 4px;
      padding: 2px 5px;
      font-weight: 600;
    }
    QLabel[agentRole="runStateChip"] {
      background: #17253a;
      color: #a7d2ff;
      border: 1px solid #355a86;
      border-radius: 4px;
      padding: 2px 5px;
      font-weight: 700;
    }
    QLabel[agentRole="tabChip"] {
      background: #252b34;
      color: #d7deea;
      border: 1px solid #404856;
      border-radius: 4px;
      padding: 4px 8px;
      font-weight: 600;
    }
    QLabel[agentRole="tabChipSelected"] {
      background: #1d2d3d;
      color: #9fd1ff;
      border: 1px solid #4d83b6;
      border-radius: 4px;
      padding: 4px 8px;
      font-weight: 700;
    }
    QFrame[agentRole="evidenceCard"] {
      background: #171c23;
      border: 1px solid #3c4552;
      border-radius: 6px;
      padding: 4px;
    }
    QFrame[agentRole="activityCard"] {
      background: #181d23;
      border: 1px solid #354050;
      border-radius: 6px;
      padding: 4px;
    }
    QFrame[agentRole="planRow"] {
      background: #161b22;
      border: 1px solid #303a48;
      border-radius: 6px;
      padding: 3px;
    }
    QLabel[agentRole="planTitle"] {
      color: #f8fafc;
      font-weight: 650;
    }
    QLabel[agentRole="planState"] {
      color: #8bd5a7;
      font-weight: 650;
    }
    QLabel[agentRole="planDetail"] {
      color: #b8c2d0;
    }
    QProgressBar {
      background: #0f1318;
      border: 0;
      border-radius: 3px;
    }
    QProgressBar::chunk {
      background: #69b7ff;
      border-radius: 3px;
    }
    QLabel[agentRole="evidenceKind"] {
      color: #9fd1ff;
      font-weight: 600;
    }
    QLabel[agentRole="evidenceTitle"] {
      color: #f8fafc;
      font-weight: 700;
    }
    QLabel[agentRole="evidenceMeta"] {
      color: #a7b3c5;
    }
    QLabel#agentProjectLabel,
    QLabel#agentEpochLabel,
    QLabel#agentStatusLabel,
    QLabel#agentResultStateLabel {
      color: #f8fafc;
      font-weight: 600;
    }
    QLabel#agentWorkspaceLabel,
    QLabel#agentDiagnosticsLabel,
    QLabel#agentTaskStateLabel,
    QLabel#agentEvidenceLabel,
    QLabel#agentApprovalStatusLabel {
      color: #cbd3df;
    }
    QComboBox,
    QLineEdit,
    QPlainTextEdit[agentRole="rawOutput"] {
      background: #101318;
      color: #e9edf3;
      border: 1px solid #444c59;
      border-radius: 4px;
      padding: 5px;
      selection-background-color: #2f6fb0;
    }
    QPushButton {
      background: #252b34;
      color: #f8fafc;
      border: 1px solid #444c59;
      border-radius: 4px;
      padding: 5px 8px;
    }
    QPushButton[agentRole="iconButton"] {
      background: #222832;
      border: 1px solid #404856;
      padding: 3px;
    }
    QPushButton[agentTraceAction="true"] {
      background: #173552;
      border: 1px solid #4d83b6;
      padding: 3px;
    }
    QPushButton[agentTraceAction="true"]:hover {
      background: #21486d;
      border: 1px solid #69b7ff;
    }
    QPushButton:hover {
      background: #303743;
    }
    QPushButton:pressed {
      background: #295b89;
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

  auto* header = makePanelSection("panel:agent_session_strip", content);
  header->setProperty("agentRole", "sessionStrip");
  auto* header_layout = new QVBoxLayout(header);
  header_layout->setContentsMargins(5, 5, 5, 5);
  header_layout->setSpacing(3);
  auto* title_row = new QHBoxLayout();
  title_row->setSpacing(8);
  session_title_label_ = new QLabel("CCad Agent", header);
  session_title_label_->setObjectName("label:agent_session_title");
  session_title_label_->setProperty("agentRole", "panelTitle");
  title_row->addWidget(session_title_label_, 1);
  auto* header_context_button =
      makeIconButton("action:agent_header_request_context", "Request agent workspace context",
                     style()->standardIcon(QStyle::SP_BrowserReload), header);
  auto* header_drc_button =
      makeIconButton("action:agent_header_trigger_drc", "Trigger agent diagnostics query",
                     style()->standardIcon(QStyle::SP_MessageBoxWarning), header);
  auto* header_clear_button =
      makeIconButton("action:agent_header_clear_output", "Clear agent output",
                     style()->standardIcon(QStyle::SP_DialogResetButton), header);
  title_row->addWidget(header_context_button);
  title_row->addWidget(header_drc_button);
  title_row->addWidget(header_clear_button);
  header_layout->addLayout(title_row);

  auto* mode_strip = makePanelSection("panel:agent_mode_strip", header);
  mode_strip->setProperty("agentRole", "modeStrip");
  auto* mode_strip_layout = new QHBoxLayout(mode_strip);
  mode_strip_layout->setContentsMargins(3, 3, 3, 3);
  mode_strip_layout->setSpacing(4);
  model_chip_label_ = makeChip("label:agent_model_chip", "Model: local", header);
  mode_chip_label_ = makeChip("label:agent_mode_chip", "Mode: plan", header);
  permission_chip_label_ = makeChip("label:agent_permission_chip", "Policy: local-only", header);
  permission_chip_label_->setProperty("agentRole", "permissionChip");
  mode_strip_layout->addWidget(model_chip_label_);
  mode_strip_layout->addWidget(mode_chip_label_);
  mode_strip_layout->addWidget(permission_chip_label_);
  mode_strip_layout->addStretch(1);
  header_layout->addWidget(mode_strip);

  auto* trace_strip = makePanelSection("panel:agent_trace_strip", header);
  trace_strip->setProperty("agentRole", "modeStrip");
  auto* trace_strip_layout = new QHBoxLayout(trace_strip);
  trace_strip_layout->setContentsMargins(3, 3, 3, 3);
  trace_strip_layout->setSpacing(4);
  trace_chip_label_ = makeChip("label:agent_trace_chip", "Trace: local-off", trace_strip);
  session_chip_label_ = makeChip("label:agent_session_chip", "Session: local", trace_strip);
  trace_strip_layout->addWidget(trace_chip_label_);
  trace_strip_layout->addWidget(session_chip_label_);
  trace_strip_layout->addStretch(1);
  header_layout->addWidget(trace_strip);

  auto* provider_controls = makePanelSection("panel:agent_provider_controls", header);
  provider_controls->setProperty("agentRole", "modeStrip");
  auto* provider_layout = new QVBoxLayout(provider_controls);
  provider_layout->setContentsMargins(3, 3, 3, 3);
  provider_layout->setSpacing(4);
  auto* provider_title_row = new QHBoxLayout();
  provider_title_row->setSpacing(4);
  auto* provider_title = new QLabel("Provider", provider_controls);
  provider_title->setProperty("agentRole", "sectionTitle");
  auto* provider_refresh_button =
      makeIconButton("action:agent_provider_refresh_status", "Refresh provider readiness",
                     style()->standardIcon(QStyle::SP_BrowserReload), provider_controls);
  provider_title_row->addWidget(provider_title);
  provider_title_row->addWidget(provider_refresh_button);
  provider_title_row->addStretch(1);
  provider_layout->addLayout(provider_title_row);

  auto* provider_row = new QVBoxLayout();
  provider_row->setSpacing(4);
  provider_selector_ = new QComboBox(provider_controls);
  provider_selector_->setObjectName("control:agent_provider_family");
  provider_selector_->setAccessibleName("Agent provider family");
  provider_selector_->setToolTip("Agent provider family");
  provider_selector_->setMinimumWidth(0);
  for (const AgentProviderSpec& spec : agentProviderSpecs()) {
    provider_selector_->addItem(spec.label, spec.id);
  }
  provider_model_input_ = new QLineEdit(provider_controls);
  provider_model_input_->setObjectName("control:agent_provider_model");
  provider_model_input_->setAccessibleName("Agent provider model hint");
  provider_model_input_->setPlaceholderText("model hint");
  provider_model_input_->setMinimumWidth(0);
  provider_row->addWidget(provider_selector_);
  provider_row->addWidget(provider_model_input_);
  provider_layout->addLayout(provider_row);

  provider_status_label_ =
      makeChip("label:agent_provider_status", "Provider: env unchecked", provider_controls);
  provider_env_label_ =
      makeChip("label:agent_provider_env", "Env: OPENAI_API_KEY unchecked", provider_controls);
  provider_execution_status_label_ =
      makeChip("label:agent_provider_execution_status", "Execution: disabled", provider_controls);
  provider_status_label_->setWordWrap(true);
  provider_env_label_->setWordWrap(true);
  provider_execution_status_label_->setWordWrap(true);
  provider_layout->addWidget(provider_status_label_);
  provider_layout->addWidget(provider_env_label_);
  provider_layout->addWidget(provider_execution_status_label_);
  header_layout->addWidget(provider_controls);

  auto* trace_links = makePanelSection("panel:agent_trace_links", header);
  trace_links->setProperty("agentRole", "modeStrip");
  auto* trace_links_layout = new QVBoxLayout(trace_links);
  trace_links_layout->setContentsMargins(3, 3, 3, 3);
  trace_links_layout->setSpacing(4);
  auto* trace_links_title_row = new QHBoxLayout();
  trace_links_title_row->setSpacing(4);
  auto* trace_links_title = new QLabel("Trace Links", trace_links);
  trace_links_title->setProperty("agentRole", "sectionTitle");
  auto* new_trace_button =
      makeIconButton("action:agent_new_trace_context", "Create local trace context",
                     style()->standardIcon(QStyle::SP_BrowserReload), trace_links);
  new_trace_button->setProperty("agentTraceAction", true);
  new_trace_button->setFixedSize(32, 28);
  trace_links_title_row->addWidget(trace_links_title);
  trace_links_title_row->addWidget(new_trace_button);
  trace_links_title_row->addStretch(1);
  trace_links_layout->addLayout(trace_links_title_row);
  trace_id_label_ = makeChip("label:agent_trace_id", "Trace ID: none", trace_links);
  span_id_label_ = makeChip("label:agent_span_id", "Span ID: none", trace_links);
  trace_status_label_ =
      makeChip("label:agent_trace_status", "Trace status: local_off", trace_links);
  trace_export_status_label_ =
      makeChip("label:agent_trace_export_status", "Export: disabled", trace_links);
  trace_id_label_->setWordWrap(true);
  span_id_label_->setWordWrap(true);
  trace_status_label_->setWordWrap(true);
  trace_export_status_label_->setWordWrap(true);
  trace_links_layout->addWidget(trace_id_label_);
  trace_links_layout->addWidget(span_id_label_);
  trace_links_layout->addWidget(trace_status_label_);
  trace_links_layout->addWidget(trace_export_status_label_);
  header_layout->addWidget(trace_links);

  auto* session_binding = makePanelSection("panel:agent_session_binding", header);
  session_binding->setProperty("agentRole", "modeStrip");
  auto* session_binding_layout = new QVBoxLayout(session_binding);
  session_binding_layout->setContentsMargins(3, 3, 3, 3);
  session_binding_layout->setSpacing(4);
  auto* session_path_row = new QHBoxLayout();
  session_path_row->setSpacing(4);
  session_path_input_ = new QLineEdit(session_binding);
  session_path_input_->setObjectName("control:agent_session_path");
  session_path_input_->setPlaceholderText("Local .ccad-agent-session.json");
  session_path_input_->setAccessibleName("Agent session file path");
  auto* load_session_button =
      makeIconButton("action:agent_load_session", "Load local agent session file",
                     style()->standardIcon(QStyle::SP_DialogOpenButton), session_binding);
  auto* checkpoint_session_button =
      makeIconButton("action:agent_checkpoint_session",
                     "Append local agent session checkpoint",
                     style()->standardIcon(QStyle::SP_DialogSaveButton), session_binding);
  session_path_row->addWidget(session_path_input_, 1);
  session_path_row->addWidget(load_session_button);
  session_path_row->addWidget(checkpoint_session_button);
  session_binding_layout->addLayout(session_path_row);
  session_status_label_ = new QLabel("Session not bound | local JSON", session_binding);
  session_status_label_->setObjectName("label:agent_session_status");
  session_status_label_->setProperty("agentRole", "evidenceMeta");
  session_status_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  session_status_label_->setWordWrap(true);
  session_binding_layout->addWidget(session_status_label_);
  header_layout->addWidget(session_binding);

  auto* policy_surface = makePanelSection("panel:agent_policy_surface", header);
  policy_surface->setProperty("agentRole", "modeStrip");
  auto* policy_layout = new QVBoxLayout(policy_surface);
  policy_layout->setContentsMargins(3, 3, 3, 3);
  policy_layout->setSpacing(4);
  auto* policy_row = new QHBoxLayout();
  policy_row->setSpacing(4);
  policy_decision_label_ = new QLabel("Policy: not classified", policy_surface);
  policy_decision_label_->setObjectName("label:agent_policy_decision");
  policy_decision_label_->setProperty("agentRole", "chip");
  policy_decision_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  policy_risk_label_ = new QLabel("Risk: none", policy_surface);
  policy_risk_label_->setObjectName("label:agent_policy_risk");
  policy_risk_label_->setProperty("agentRole", "chip");
  policy_risk_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  policy_dry_run_checkbox_ = new QCheckBox("dry", policy_surface);
  policy_dry_run_checkbox_->setObjectName("control:agent_policy_dry_run");
  policy_dry_run_checkbox_->setAccessibleName("Preview command policy as dry run");
  policy_dry_run_checkbox_->setToolTip("Preview command policy as dry run");
  policy_dry_run_checkbox_->setProperty("agentRole", "chip");
  auto* policy_preview_button =
      makeIconButton("action:agent_policy_preview", "Preview command policy",
                     style()->standardIcon(QStyle::SP_DialogApplyButton), policy_surface);
  policy_row->addWidget(policy_decision_label_, 1);
  policy_row->addWidget(policy_risk_label_);
  policy_row->addWidget(policy_dry_run_checkbox_);
  policy_row->addWidget(policy_preview_button);
  policy_layout->addLayout(policy_row);
  header_layout->addWidget(policy_surface);

  auto* run_controls = makePanelSection("panel:agent_run_controls", header);
  run_controls->setProperty("agentRole", "modeStrip");
  auto* run_controls_layout = new QHBoxLayout(run_controls);
  run_controls_layout->setContentsMargins(3, 3, 3, 3);
  run_controls_layout->setSpacing(4);
  run_state_chip_label_ = makeChip("label:agent_run_state_chip", "Run: idle", run_controls);
  run_state_chip_label_->setProperty("agentRole", "runStateChip");
  auto* pause_run_button =
      makeIconButton("action:agent_pause_run", "Pause local agent run",
                     style()->standardIcon(QStyle::SP_MediaPause), run_controls);
  auto* resume_run_button =
      makeIconButton("action:agent_resume_run", "Resume local agent run",
                     style()->standardIcon(QStyle::SP_MediaPlay), run_controls);
  auto* stop_run_button =
      makeIconButton("action:agent_stop_run", "Stop local agent run",
                     style()->standardIcon(QStyle::SP_MediaStop), run_controls);
  run_controls_layout->addWidget(run_state_chip_label_);
  run_controls_layout->addWidget(pause_run_button);
  run_controls_layout->addWidget(resume_run_button);
  run_controls_layout->addWidget(stop_run_button);
  run_controls_layout->addStretch(1);
  header_layout->addWidget(run_controls);

  auto* run_queue = makePanelSection("panel:agent_run_queue", header);
  run_queue->setProperty("agentRole", "modeStrip");
  auto* run_queue_layout = new QVBoxLayout(run_queue);
  run_queue_layout->setContentsMargins(3, 3, 3, 3);
  run_queue_layout->setSpacing(4);
  auto* run_queue_title_row = new QHBoxLayout();
  run_queue_title_row->setSpacing(4);
  auto* run_queue_title = new QLabel("Run Queue", run_queue);
  run_queue_title->setProperty("agentRole", "sectionTitle");
  auto* cancel_queue_button =
      makeIconButton("action:agent_cancel_run_queue", "Cancel local run queue",
                     style()->standardIcon(QStyle::SP_DialogCancelButton), run_queue);
  auto* clear_queue_button =
      makeIconButton("action:agent_clear_run_queue", "Clear local run queue",
                     style()->standardIcon(QStyle::SP_DialogResetButton), run_queue);
  run_queue_title_row->addWidget(run_queue_title);
  run_queue_title_row->addWidget(cancel_queue_button);
  run_queue_title_row->addWidget(clear_queue_button);
  run_queue_title_row->addStretch(1);
  run_queue_layout->addLayout(run_queue_title_row);
  run_queue_status_label_ =
      makeChip("label:agent_run_queue_status", "Queue: idle", run_queue);
  run_queue_counts_label_ =
      makeChip("label:agent_run_queue_counts", "Steps: 0/3 done | 0 failed | 3 queued",
               run_queue);
  run_queue_current_step_label_ =
      makeChip("label:agent_run_queue_current_step", "Current: Collect evidence", run_queue);
  run_queue_status_label_->setWordWrap(true);
  run_queue_counts_label_->setWordWrap(true);
  run_queue_current_step_label_->setWordWrap(true);
  run_queue_layout->addWidget(run_queue_status_label_);
  run_queue_layout->addWidget(run_queue_counts_label_);
  run_queue_layout->addWidget(run_queue_current_step_label_);
  header_layout->insertWidget(3, run_queue);

  auto* tab_strip = makePanelSection("panel:agent_tab_strip", header);
  tab_strip->setProperty("agentRole", "tabStrip");
  auto* tab_strip_layout = new QHBoxLayout(tab_strip);
  tab_strip_layout->setContentsMargins(3, 3, 3, 3);
  tab_strip_layout->setSpacing(4);
  auto* command_tab = new QLabel("Command", tab_strip);
  command_tab->setObjectName("tab:agent_command");
  command_tab->setProperty("agentRole", "tabChipSelected");
  auto* evidence_tab = new QLabel("Evidence", tab_strip);
  evidence_tab->setObjectName("tab:agent_evidence");
  evidence_tab->setProperty("agentRole", "tabChip");
  auto* approvals_tab = new QLabel("Approvals", tab_strip);
  approvals_tab->setObjectName("tab:agent_approvals");
  approvals_tab->setProperty("agentRole", "tabChip");
  tab_strip_layout->addWidget(command_tab);
  tab_strip_layout->addWidget(evidence_tab);
  tab_strip_layout->addWidget(approvals_tab);
  tab_strip_layout->addStretch(1);
  header_layout->addWidget(tab_strip);

  auto* status_rail = makePanelSection("panel:agent_status_rail", header);
  status_rail->setProperty("agentRole", "statusRail");
  auto* status_rail_layout = new QHBoxLayout(status_rail);
  status_rail_layout->setContentsMargins(5, 4, 5, 4);
  status_rail_layout->setSpacing(6);
  status_label_ = new QLabel("Agent ready", status_rail);
  status_label_->setObjectName("agentStatusLabel");
  status_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  status_label_->setWordWrap(true);
  auto* status_tail = new QLabel("local only", status_rail);
  status_tail->setProperty("agentRole", "evidenceMeta");
  status_tail->setTextInteractionFlags(Qt::TextSelectableByMouse);
  status_rail_layout->addWidget(status_label_, 1);
  status_rail_layout->addWidget(status_tail);
  header_layout->addWidget(status_rail);
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

  auto* stream_section = makePanelSection("panel:agent_command_stream", content);
  auto* stream_layout = new QVBoxLayout(stream_section);
  stream_layout->setContentsMargins(6, 6, 6, 6);
  stream_layout->setSpacing(4);
  stream_layout->addWidget(makeSectionTitle("Command", stream_section));
  auto* command_composer = makePanelSection("panel:agent_command_composer", stream_section);
  command_composer->setProperty("agentRole", "commandComposer");
  auto* command_composer_layout = new QVBoxLayout(command_composer);
  command_composer_layout->setContentsMargins(5, 5, 5, 5);
  command_composer_layout->setSpacing(5);
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
  command_composer_layout->addLayout(preset_row);

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
  command_composer_layout->addLayout(action_row);

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
  command_composer_layout->addLayout(live_row);
  stream_layout->addWidget(command_composer);

  stream_layout->addWidget(makeSectionTitle("Raw JSON", stream_section));
  output_ = new QPlainTextEdit(stream_section);
  output_->setObjectName("panel:agent_raw_output");
  output_->setProperty("agentRole", "rawOutput");
  output_->setReadOnly(true);
  output_->setMinimumHeight(64);
  output_->setMaximumHeight(88);
  output_->setPlainText("{}");
  stream_layout->addWidget(output_);

  connect(refresh_button, &QPushButton::clicked, this, [this]() { refreshUiMap(); });
  connect(trigger_button, &QPushButton::clicked, this, [this]() { triggerSafeAction(); });
  connect(header_context_button, &QPushButton::clicked, this,
          [this]() { runHarnessContextPreset(); });
  connect(header_drc_button, &QPushButton::clicked, this, [this]() { runDiagnosticsPreset(); });
  connect(header_clear_button, &QPushButton::clicked, this, [this]() { clearOutput(); });
  connect(load_session_button, &QPushButton::clicked, this, [this]() {
    QString path = session_path_input_->text().trimmed();
    if (path.isEmpty()) {
      path = QFileDialog::getOpenFileName(
          this, "Load CCad Agent Session", "",
          "CCad Agent Session (*.ccad-agent-session.json);;JSON Files (*.json)");
    }
    if (!path.isEmpty()) {
      bindSessionFile(path);
    }
  });
  connect(checkpoint_session_button, &QPushButton::clicked, this,
          [this]() { checkpointSession(); });
  connect(provider_refresh_button, &QPushButton::clicked, this,
          [this]() { refreshProviderStatus(); });
  connect(provider_selector_, &QComboBox::currentIndexChanged, this,
          [this](int) { updateProviderControls(); });
  connect(provider_model_input_, &QLineEdit::textChanged, this,
          [this](const QString&) { updateProviderControls(); });
  connect(policy_preview_button, &QPushButton::clicked, this,
          [this]() { previewCommandPolicy(); });
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

  auto* active_plan_section = makePanelSection("panel:agent_active_plan", task_section);
  auto* active_plan_layout = new QVBoxLayout(active_plan_section);
  active_plan_layout->setContentsMargins(6, 6, 6, 6);
  active_plan_layout->setSpacing(4);
  active_plan_layout->addWidget(makeSectionTitle("Active Plan", active_plan_section));
  auto* plan_deck = makePanelSection("panel:agent_plan_deck", active_plan_section);
  plan_deck->setProperty("agentRole", "planDeck");
  auto* plan_deck_layout = new QVBoxLayout(plan_deck);
  plan_deck_layout->setContentsMargins(5, 5, 5, 5);
  plan_deck_layout->setSpacing(4);
  plan_deck_layout->addWidget(makePlanRow("panel:agent_plan_row_1", "Read workspace context",
                                          "Project, layer, net, selection, and diagnostics are cached for the next action.",
                                          "ready", 100, plan_deck));
  plan_deck_layout->addWidget(makePlanRow("panel:agent_plan_row_2", "Collect evidence",
                                          "Use DRC/ERC reports, screenshots, and pinned artifacts before proposing edits.",
                                          "active", 62, plan_deck));
  plan_deck_layout->addWidget(makePlanRow("panel:agent_plan_row_3", "Apply bounded changes",
                                          "Mutating tools require policy checks and explicit verification artifacts.",
                                          "queued", 18, plan_deck));
  active_plan_layout->addWidget(plan_deck);
  task_layout->addWidget(active_plan_section);

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

  connect(stage_goal_button, &QPushButton::clicked, this, [this]() { stageGoal(); });
  connect(goal_input_, &QLineEdit::returnPressed, this, [this]() { stageGoal(); });
  connect(pause_run_button, &QPushButton::clicked, this, [this]() { pauseRun(); });
  connect(resume_run_button, &QPushButton::clicked, this, [this]() { resumeRun(); });
  connect(stop_run_button, &QPushButton::clicked, this, [this]() { stopRun(); });
  connect(cancel_queue_button, &QPushButton::clicked, this, [this]() { cancelRunQueue(); });
  connect(clear_queue_button, &QPushButton::clicked, this, [this]() { clearRunQueue(); });
  connect(new_trace_button, &QPushButton::clicked, this, [this]() { createLocalTraceContext(); });

  content_layout->addWidget(task_section);

  auto* activity_section = makePanelSection("panel:agent_activity_stream", content);
  auto* activity_layout = new QVBoxLayout(activity_section);
  activity_layout->setContentsMargins(6, 6, 6, 6);
  activity_layout->setSpacing(4);
  activity_layout->addWidget(makeSectionTitle("Activity", activity_section));
  activity_events_layout_ = new QVBoxLayout();
  activity_events_layout_->setContentsMargins(0, 0, 0, 0);
  activity_events_layout_->setSpacing(4);
  activity_layout->addLayout(activity_events_layout_);
  addActivityEvent("session", "Agent ready", "Local workspace loaded", "agent.workspace");

  auto* evidence_section = makePanelSection("panel:agent_evidence_tray", content);
  auto* evidence_layout = new QVBoxLayout(evidence_section);
  evidence_layout->setContentsMargins(6, 6, 6, 6);
  evidence_layout->setSpacing(4);
  evidence_layout->addWidget(makeSectionTitle("Pinned Evidence", evidence_section));
  auto* evidence_lane = makePanelSection("panel:agent_evidence_lane", evidence_section);
  evidence_lane->setProperty("agentRole", "evidenceLane");
  auto* evidence_lane_layout = new QVBoxLayout(evidence_lane);
  evidence_lane_layout->setContentsMargins(5, 5, 5, 5);
  evidence_lane_layout->setSpacing(4);
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
  evidence_lane_layout->addLayout(evidence_row);
  evidence_layout->addWidget(evidence_lane);
  evidence_cards_layout_ = new QVBoxLayout();
  evidence_cards_layout_->setContentsMargins(0, 0, 0, 0);
  evidence_cards_layout_->setSpacing(4);
  evidence_layout->addLayout(evidence_cards_layout_);

  connect(pin_evidence_button, &QPushButton::clicked, this, [this]() { pinEvidence(); });
  connect(clear_evidence_button, &QPushButton::clicked, this, [this]() { clearEvidence(); });

  auto* approval_section = makePanelSection("panel:agent_approval_card", content);
  auto* approval_layout = new QVBoxLayout(approval_section);
  approval_layout->setContentsMargins(6, 6, 6, 6);
  approval_layout->setSpacing(4);
  approval_layout->addWidget(makeSectionTitle("Approval Pending", approval_section));
  auto* approval_lane = makePanelSection("panel:agent_approval_lane", approval_section);
  approval_lane->setProperty("agentRole", "approvalLane");
  auto* approval_lane_layout = new QVBoxLayout(approval_lane);
  approval_lane_layout->setContentsMargins(5, 5, 5, 5);
  approval_lane_layout->setSpacing(4);
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
  approval_lane_layout->addLayout(approval_request_row);

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
  approval_lane_layout->addLayout(approval_status_row);
  approval_lane_layout->addWidget(approval_status_label_);
  approval_layout->addWidget(approval_lane);
  content_layout->addWidget(evidence_section);
  content_layout->addWidget(approval_section);
  content_layout->addWidget(context_section);
  content_layout->addWidget(activity_section);
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
  updateRunQueueLabels();
  updateProviderControls();
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

QJsonObject AgentPanel::providerStateObject() const {
  const AgentProviderSpec spec = currentProviderSpec(provider_selector_);
  const bool env_present = providerEnvironmentPresent(spec);
  QJsonArray env_vars;
  for (const QString& env_var : spec.env_vars) {
    env_vars.append(env_var);
  }

  QJsonObject provider;
  provider.insert("provider_panel_available", provider_selector_ != nullptr &&
                                                  provider_model_input_ != nullptr);
  provider.insert("provider_id", spec.id);
  provider.insert("provider_label", spec.label);
  provider.insert("provider_access_path", spec.access_path);
  provider.insert("provider_model_hint",
                  provider_model_input_ == nullptr ? QString()
                                                   : provider_model_input_->text().trimmed());
  provider.insert("provider_model_env", spec.model_env);
  provider.insert("provider_api_key_env",
                  spec.env_vars.isEmpty() ? QString() : spec.env_vars.front());
  provider.insert("provider_env_vars", env_vars);
  provider.insert("provider_env_present", env_present);
  provider.insert("provider_configured", env_present);
  provider.insert("provider_status", env_present ? "env_present" : "env_missing");
  provider.insert("provider_status_method", "agent.provider_status");
  provider.insert("provider_config_schema_method", "agent.provider_config_schema");
  provider.insert("provider_config_template_method", "agent.provider_config_template");
  provider.insert("provider_execution_enabled", false);
  provider.insert("provider_secret_value_visible", false);
  provider.insert("provider_network_probe_enabled", false);
  provider.insert("provider_browser_account_automation_enabled", false);
  provider.insert("provider_project_file_secret_storage", false);
  provider.insert("provider_secret_value_policy", "never_emit_secret_values");
  provider.insert("provider_readiness_policy", "env_presence_only_no_network_probe");
  provider.insert("provider_security_note", spec.security_note);
  return provider;
}

void AgentPanel::updateProviderControls() {
  const AgentProviderSpec spec = currentProviderSpec(provider_selector_);
  provider_env_present_ = providerEnvironmentPresent(spec);
  provider_status_ = provider_env_present_ ? "env_present" : "env_missing";

  const QString primary_env = spec.env_vars.isEmpty() ? QString("env") : spec.env_vars.front();
  if (model_chip_label_ != nullptr) {
    model_chip_label_->setText("Model: " + spec.label + " off");
  }
  if (provider_status_label_ != nullptr) {
    provider_status_label_->setText("Provider: " + spec.label + " | " + provider_status_);
  }
  if (provider_env_label_ != nullptr) {
    provider_env_label_->setText("Env: " + primary_env +
                                 (provider_env_present_ ? " present" : " missing") +
                                 " | values hidden");
  }
  if (provider_execution_status_label_ != nullptr) {
    provider_execution_status_label_->setText("Execution: disabled | no network probe");
  }
}

void AgentPanel::updateRunQueueLabels() {
  if (run_queue_status_label_ != nullptr) {
    run_queue_status_label_->setText("Queue: " + run_queue_status_ + " | local metadata");
  }
  if (run_queue_counts_label_ != nullptr) {
    run_queue_counts_label_->setText(
        "Steps: " + QString::number(run_queue_completed_count_) + "/" +
        QString::number(run_steps_total_) + " done | " +
        QString::number(run_queue_failed_count_) + " failed | " +
        QString::number(run_queue_depth_) + " queued");
  }
  if (run_queue_current_step_label_ != nullptr) {
    const QString step = run_queue_current_step_.trimmed().isEmpty()
                             ? QString("none")
                             : run_queue_current_step_;
    run_queue_current_step_label_->setText("Current: " + step);
  }
}

QJsonObject AgentPanel::runQueueStateObject() const {
  QJsonArray steps;
  auto append_step = [&steps](const QString& id,
                              const QString& title,
                              const QString& kind,
                              const QString& state) {
    QJsonObject step;
    step.insert("id", id);
    step.insert("title", title);
    step.insert("kind", kind);
    step.insert("state", state);
    steps.append(step);
  };
  append_step("queue-step-1", "Read workspace context", "context", "queued");
  append_step("queue-step-2", "Collect evidence", "evidence", "active");
  append_step("queue-step-3", "Apply bounded changes", "tool_plan", "queued");

  QJsonObject queue;
  queue.insert("run_queue_available", true);
  queue.insert("run_queue_id", run_queue_id_);
  queue.insert("run_queue_thread_id", durable_thread_id_);
  queue.insert("run_queue_session_id", durable_session_id_);
  queue.insert("run_queue_status", run_queue_status_);
  queue.insert("run_queue_depth", run_queue_depth_);
  queue.insert("run_queue_completed_count", run_queue_completed_count_);
  queue.insert("run_queue_failed_count", run_queue_failed_count_);
  queue.insert("run_steps_total", run_steps_total_);
  queue.insert("run_step_current", run_queue_current_step_);
  queue.insert("run_step_current_index", run_queue_current_step_.trimmed().isEmpty() ? 0 : 2);
  queue.insert("run_queue_cancelable", run_queue_cancelable_);
  queue.insert("run_queue_provider_execution_enabled", false);
  queue.insert("run_queue_worker_thread_enabled", false);
  queue.insert("run_queue_trace_export_enabled", false);
  queue.insert("run_queue_external_process_enabled", false);
  queue.insert("run_queue_project_mutation_enabled", false);
  queue.insert("run_queue_persistence", "local_in_memory_checkpoint_shape");
  queue.insert("run_queue_policy", "local_metadata_only_no_provider_calls");
  queue.insert("run_queue_steps", steps);
  return queue;
}

void AgentPanel::cancelRunQueue() {
  run_queue_status_ = "canceled";
  run_queue_cancelable_ = false;
  run_state_ = "stopped";
  if (run_state_chip_label_ != nullptr) {
    run_state_chip_label_->setText("Run: stopped");
  }
  updateRunQueueLabels();

  QJsonObject event = runQueueStateObject();
  event.insert("schema_version", 1);
  event.insert("event", "agent_run_queue_canceled");
  event.insert("provider_execution_performed", false);
  event.insert("worker_thread_started", false);
  event.insert("trace_export_performed", false);
  output_->setPlainText(QString::fromUtf8(QJsonDocument(event).toJson(QJsonDocument::Compact)) +
                        "\n");
  status_label_->setText("Run queue canceled");
  result_state_label_->setText("Result Run queue canceled");
  addActivityEvent("run", "Queue canceled",
                   "Local queue canceled; provider execution remains disabled.",
                   "agent.run_queue.cancel");
}

void AgentPanel::clearRunQueue() {
  run_queue_status_ = "cleared";
  run_queue_current_step_.clear();
  run_queue_depth_ = 0;
  run_queue_completed_count_ = 0;
  run_queue_failed_count_ = 0;
  run_steps_total_ = 0;
  run_queue_cancelable_ = false;
  updateRunQueueLabels();

  QJsonObject event = runQueueStateObject();
  event.insert("schema_version", 1);
  event.insert("event", "agent_run_queue_cleared");
  event.insert("provider_execution_performed", false);
  event.insert("worker_thread_started", false);
  event.insert("trace_export_performed", false);
  output_->setPlainText(QString::fromUtf8(QJsonDocument(event).toJson(QJsonDocument::Compact)) +
                        "\n");
  status_label_->setText("Run queue cleared");
  result_state_label_->setText("Result Run queue cleared");
  addActivityEvent("run", "Queue cleared",
                   "Local queue metadata cleared; no queued external work existed.",
                   "agent.run_queue.clear");
}

void AgentPanel::refreshProviderStatus() {
  updateProviderControls();
  QJsonObject event = providerStateObject();
  event.insert("schema_version", 1);
  event.insert("event", "agent_provider_status_refreshed");
  event.insert("secret_values_present", false);
  event.insert("network_probe_performed", false);
  output_->setPlainText(QString::fromUtf8(QJsonDocument(event).toJson(QJsonDocument::Compact)) +
                        "\n");
  status_label_->setText("Provider status refreshed");
  result_state_label_->setText("Result Provider local status");
  addActivityEvent("provider", "Provider status refreshed",
                   event.value("provider_label").toString() + " | " +
                       event.value("provider_status").toString(),
                   "agent.provider_status");
}

QJsonObject AgentPanel::policyStateObject() const {
  QJsonArray args;
  for (const QString& arg : policy_args_) {
    args.append(arg);
  }
  QJsonObject policy;
  policy.insert("policy_decision", policy_decision_);
  policy.insert("policy_risk_level", policy_risk_level_);
  policy.insert("policy_approval_required", policy_approval_required_);
  policy.insert("policy_approval_reason", policy_approval_reason_);
  policy.insert("policy_dry_run", policy_dry_run_);
  policy.insert("policy_would_execute", policy_would_execute_);
  policy.insert("policy_read_only", policy_read_only_);
  policy.insert("policy_mutates_project", policy_mutates_project_);
  policy.insert("policy_mutates_files", policy_mutates_files_);
  policy.insert("policy_command", policy_command_);
  policy.insert("policy_args", args);
  return policy;
}

void AgentPanel::classifyCommandPolicy(const QString& command, const bool record_activity) {
  const QString trimmed_command = command.trimmed();
  policy_command_ = trimmed_command;
  policy_dry_run_ = policy_dry_run_checkbox_ != nullptr && policy_dry_run_checkbox_->isChecked();
  std::vector<std::string> args = policyArgsFromCommand(trimmed_command, &policy_args_);
  if (args.empty()) {
    policy_decision_ = trimmed_command.isEmpty() ? "not_classified" : "not_cli_command";
    policy_risk_level_ = "none";
    policy_approval_required_ = false;
    policy_approval_reason_.clear();
    policy_would_execute_ = false;
    policy_read_only_ = false;
    policy_mutates_project_ = false;
    policy_mutates_files_ = false;
    policy_decision_label_->setText("Policy: " + policy_decision_);
    policy_risk_label_->setText("Risk: none");
    if (record_activity && !trimmed_command.isEmpty()) {
      addActivityEvent("policy", "Policy not classified",
                       "Command text is not a CCad CLI-shaped command", "agent.policy_check");
    }
    return;
  }

  const ccad::AgentCommandPolicy policy =
      ccad::classifyAgentCommandPolicy(args, policy_dry_run_);
  policy_decision_ = QString::fromStdString(policy.decision);
  policy_risk_level_ = QString::fromStdString(policy.risk_level);
  policy_approval_required_ = policy.approval_required;
  policy_approval_reason_ = QString::fromStdString(policy.approval_reason);
  policy_would_execute_ = policy.would_execute;
  policy_read_only_ = policy.read_only;
  policy_mutates_project_ = policy.mutates_project;
  policy_mutates_files_ = policy.mutates_files;
  policy_decision_label_->setText("Policy: " + policy_decision_);
  policy_risk_label_->setText("Risk: " + policy_risk_level_);

  if (policy_approval_required_ && !policy_dry_run_) {
    pending_approval_request_ =
        "Approve command: " + trimmed_command + " | " + policy_approval_reason_;
    approval_last_decision_ = "pending";
    approval_status_label_->setText("Approval pending: " + pending_approval_request_);
  }
  if (record_activity) {
    addActivityEvent("policy", "Policy " + policy_decision_,
                     policy_risk_level_ + " | " + policy_approval_reason_,
                     "agent.policy_check");
  }
}

void AgentPanel::previewCommandPolicy() {
  classifyCommandPolicy(commandText(), true);
  QJsonObject event = policyStateObject();
  event.insert("schema_version", 1);
  event.insert("event", "agent_command_policy_preview");
  output_->setPlainText(QString::fromUtf8(QJsonDocument(event).toJson(QJsonDocument::Compact)) +
                        "\n");
  status_label_->setText("Policy previewed");
  result_state_label_->setText("Result Policy " + policy_decision_);
}

void AgentPanel::setSessionFilePath(const QString& path) {
  session_path_input_->setText(path);
}

void AgentPanel::applySessionMetadata(const AgentSessionMetadata& metadata, const QString& path) {
  session_file_path_ = QFileInfo(path).absoluteFilePath();
  durable_session_id_ = metadata.session_id;
  durable_thread_id_ = metadata.thread_id;
  latest_checkpoint_id_ = metadata.latest_checkpoint_id;
  session_checkpoint_count_ = metadata.checkpoint_count;
  session_replayable_ = metadata.replayable;
  durable_session_bound_ = true;
  session_path_input_->setText(session_file_path_);
  const QString compact_id = durable_session_id_.isEmpty() ? "local" : durable_session_id_;
  session_chip_label_->setText("Session: " + compact_id);
  session_status_label_->setText(
      "Session " + compact_id + " | thread " + durable_thread_id_ + " | checkpoints " +
      QString::number(session_checkpoint_count_));
  status_label_->setText("Session loaded");
  result_state_label_->setText("Result Session loaded");
  addActivityEvent("session", "Session loaded",
                   compact_id + " | checkpoints " + QString::number(session_checkpoint_count_),
                   "agent.session_state");
}

void AgentPanel::resetSessionBinding(const QString& status) {
  session_file_path_.clear();
  durable_session_id_.clear();
  durable_thread_id_.clear();
  latest_checkpoint_id_.clear();
  session_checkpoint_count_ = 0;
  session_replayable_ = false;
  durable_session_bound_ = false;
  session_chip_label_->setText("Session: local");
  session_status_label_->setText(status);
}

void AgentPanel::bindSessionFile(const QString& path) {
  const QString trimmed_path = path.trimmed();
  if (trimmed_path.isEmpty()) {
    resetSessionBinding("Session path required");
    status_label_->setText("Session path required");
    result_state_label_->setText("Result Error session_path_required");
    addActivityEvent("error", "Session path required",
                     "No local session file was selected", "agent.session_state");
    return;
  }

  QString error_message;
  const QJsonObject object = readJsonFileObject(trimmed_path, &error_message);
  if (object.isEmpty()) {
    resetSessionBinding(error_message);
    status_label_->setText("Session load failed");
    result_state_label_->setText("Result Error session_load_failed");
    addActivityEvent("error", "Session load failed", error_message,
                     "agent.session_state");
    return;
  }

  const AgentSessionMetadata metadata = metadataFromSessionObject(object, &error_message);
  if (metadata.session_id.isEmpty()) {
    resetSessionBinding(error_message);
    status_label_->setText("Session load failed");
    result_state_label_->setText("Result Error session_load_failed");
    addActivityEvent("error", "Session load failed", error_message,
                     "agent.session_state");
    return;
  }

  applySessionMetadata(metadata, trimmed_path);
}

void AgentPanel::checkpointSession() {
  QString path = session_file_path_.trimmed();
  if (path.isEmpty()) {
    path = session_path_input_->text().trimmed();
  }
  if (!durable_session_bound_) {
    if (!path.isEmpty()) {
      bindSessionFile(path);
    }
    if (!durable_session_bound_) {
      status_label_->setText("Session checkpoint skipped");
      result_state_label_->setText("Result Error session_not_bound");
      addActivityEvent("error", "Session checkpoint skipped",
                       "No valid local session is bound", "agent.checkpoint");
      return;
    }
  }

  QString error_message;
  QJsonObject object = readJsonFileObject(path, &error_message);
  if (object.isEmpty()) {
    resetSessionBinding(error_message);
    status_label_->setText("Session checkpoint failed");
    result_state_label_->setText("Result Error session_checkpoint_failed");
    addActivityEvent("error", "Session checkpoint failed", error_message,
                     "agent.checkpoint");
    return;
  }

  const QString session_id = object.value("session_id").toString().trimmed();
  if (session_id.isEmpty()) {
    resetSessionBinding("session_id missing");
    status_label_->setText("Session checkpoint failed");
    result_state_label_->setText("Result Error session_checkpoint_failed");
    addActivityEvent("error", "Session checkpoint failed", "session_id missing",
                     "agent.checkpoint");
    return;
  }

  QJsonArray checkpoints = object.value("checkpoints").toArray();
  const QString checkpoint_id = nextGuiCheckpointId(checkpoints);
  const QString created_at = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
  QString summary = statusText().trimmed();
  if (!resultStateText().trimmed().isEmpty()) {
    summary += " | " + resultStateText().trimmed();
  }
  if (!staged_goal_.trimmed().isEmpty()) {
    summary += " | goal " + staged_goal_.trimmed();
  }
  if (summary.trimmed().isEmpty()) {
    summary = "GUI metadata checkpoint";
  }

  QJsonObject checkpoint;
  checkpoint.insert("checkpoint_id", checkpoint_id);
  checkpoint.insert("sequence", checkpoints.size() + 1);
  checkpoint.insert("kind", "gui_checkpoint");
  checkpoint.insert("summary", summary);
  checkpoint.insert("artifact_path", QJsonValue());
  checkpoint.insert("created_at", created_at);
  checkpoint.insert("resource_uri", checkpointResourceUri(session_id, checkpoint_id));
  checkpoints.append(checkpoint);
  object.insert("updated_at", created_at);
  object.insert("checkpoint_count", checkpoints.size());
  object.insert("checkpoints", checkpoints);
  if (!writeJsonFileObject(path, object, &error_message)) {
    status_label_->setText("Session checkpoint failed");
    result_state_label_->setText("Result Error session_checkpoint_failed");
    addActivityEvent("error", "Session checkpoint failed", error_message,
                     "agent.checkpoint");
    return;
  }

  const AgentSessionMetadata metadata = metadataFromSessionObject(object, &error_message);
  applySessionMetadata(metadata, path);
  status_label_->setText("Session checkpointed");
  result_state_label_->setText("Result Session checkpointed");
  output_->setPlainText(
      "{\"schema_version\":1,\"checkpoint_added\":true,\"checkpoint_id\":\"" +
      checkpoint_id + "\",\"checkpoint_count\":" + QString::number(checkpoints.size()) + "}\n");
  addActivityEvent("session", "Session checkpointed",
                   checkpoint_id + " | local metadata only", "agent.checkpoint");
}

void AgentPanel::refreshUiMap() {
  if (!ui_map_provider_) {
    status_label_->setText("UI map unavailable");
    output_->setPlainText("{\"error\":\"ui_map_unavailable\"}");
    result_state_label_->setText("Result Error ui_map_unavailable");
    addActivityEvent("error", "UI map unavailable", "No UI-map provider is connected",
                     "ui.map");
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
  addActivityEvent("context", "UI map refreshed",
                   "Nodes " + QString::number(countUiMapNodes(json)), "ui.map");
}

void AgentPanel::triggerSafeAction() {
  if (!safe_action_trigger_) {
    status_label_->setText("Safe trigger unavailable");
    output_->setPlainText("{\"error\":\"safe_trigger_unavailable\"}");
    result_state_label_->setText("Result Error safe_trigger_unavailable");
    addActivityEvent("error", "Safe trigger unavailable",
                     "No safe action trigger is connected", "ui.trigger_safe");
    return;
  }
  const QString action_id = action_id_input_->text().trimmed();
  if (action_id.isEmpty()) {
    status_label_->setText("Action ID required");
    output_->setPlainText("{\"error\":\"empty_action_id\"}");
    result_state_label_->setText("Result Error empty_action_id");
    addActivityEvent("error", "Action ID required", "Safe trigger was not run",
                     "ui.trigger_safe");
    return;
  }
  const QString result = safe_action_trigger_(action_id);
  output_->setPlainText(result);
  status_label_->setText("Safe action " + action_id);
  result_state_label_->setText(resultSummaryFromJson(result, "Result Safe action"));
  addActivityEvent("action", "Safe action", action_id, "ui.trigger_safe");
}

void AgentPanel::runLiveQuery() {
  if (!live_query_provider_) {
    status_label_->setText("Live query unavailable");
    output_->setPlainText("{\"error\":\"live_query_unavailable\"}");
    result_state_label_->setText("Result Error live_query_unavailable");
    addActivityEvent("error", "Live query unavailable",
                     "No live query provider is connected", "agent.query");
    return;
  }
  const QString method = live_method_input_->text().trimmed();
  if (method.isEmpty()) {
    status_label_->setText("Live method required");
    output_->setPlainText("{\"error\":\"empty_live_method\"}");
    result_state_label_->setText("Result Error empty_live_method");
    addActivityEvent("error", "Live method required", "Live query was not run",
                     "agent.query");
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
  addActivityEvent("tool", "Live query", result_state_label_->text(), method);
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
  addActivityEvent("session", "Output cleared", "Raw JSON stream cleared", "agent.clear");
}

void AgentPanel::updateRunState(const QString& state,
                                const QString& title,
                                const QString& detail) {
  run_state_ = contextValue(state, "idle");
  run_queue_status_ = run_state_;
  if (run_queue_status_ == "stopped") {
    run_queue_cancelable_ = false;
  } else if (run_queue_status_ == "running" || run_queue_status_ == "paused") {
    run_queue_cancelable_ = true;
  }
  if (run_state_chip_label_ != nullptr) {
    run_state_chip_label_->setText("Run: " + run_state_);
  }
  updateRunQueueLabels();
  status_label_->setText(title);
  result_state_label_->setText("Result " + title);
  addActivityEvent("run", title, detail, "agent.run");
}

void AgentPanel::pauseRun() {
  updateRunState("paused", "Run paused", "Local run state paused; no provider call was made.");
}

void AgentPanel::resumeRun() {
  updateRunState("running", "Run resumed", "Local run state resumed; durable runner binding is pending.");
}

void AgentPanel::stopRun() {
  updateRunState("stopped", "Run stopped", "Local run state stopped; queued provider work remains disabled.");
}

void AgentPanel::createLocalTraceContext() {
  ++trace_sequence_;
  trace_id_ = "ccad-local-trace-" + QString::number(trace_sequence_);
  span_id_ = "ccad-local-span-" + QString::number(trace_sequence_);
  trace_status_ = "local_ready";
  trace_export_status_ = "export_disabled";

  if (trace_chip_label_ != nullptr) {
    trace_chip_label_->setText("Trace: local-" + QString::number(trace_sequence_));
  }
  if (trace_id_label_ != nullptr) {
    trace_id_label_->setText("Trace ID: " + trace_id_);
  }
  if (span_id_label_ != nullptr) {
    span_id_label_->setText("Span ID: " + span_id_);
  }
  if (trace_status_label_ != nullptr) {
    trace_status_label_->setText("Trace status: " + trace_status_);
  }
  if (trace_export_status_label_ != nullptr) {
    trace_export_status_label_->setText("Export: disabled");
  }

  QJsonObject event;
  event.insert("schema_version", 1);
  event.insert("event", "agent_trace_context_ready");
  event.insert("trace_id", trace_id_);
  event.insert("span_id", span_id_);
  event.insert("trace_status", trace_status_);
  event.insert("trace_export_status", trace_export_status_);
  event.insert("trace_export_enabled", false);
  event.insert("trace_link_available", false);
  event.insert("trace_backend", "local_metadata_only");
  event.insert("trace_session_id", durable_session_id_);
  event.insert("trace_thread_id", durable_thread_id_);
  event.insert("trace_content_policy",
               "metadata_only_no_prompt_tool_or_design_payloads");
  output_->setPlainText(QString::fromUtf8(QJsonDocument(event).toJson(QJsonDocument::Compact)) +
                        "\n");
  status_label_->setText("Trace context ready");
  result_state_label_->setText("Result Trace context ready");
  addActivityEvent("trace", "Trace context ready",
                   trace_id_ + " | export disabled | local metadata only", "agent.trace");
}

void AgentPanel::stageGoal() {
  const QString trimmed_goal = goal_input_->text().trimmed();
  if (trimmed_goal.isEmpty()) {
    staged_goal_.clear();
    task_state_label_->setText("Task idle");
    status_label_->setText("Goal required");
    result_state_label_->setText("Result Error empty_goal");
    addActivityEvent("error", "Goal required", "Task goal was cleared", "agent.goal");
    return;
  }
  staged_goal_ = trimmed_goal;
  task_state_label_->setText("Goal staged: " + staged_goal_);
  status_label_->setText("Goal staged");
  result_state_label_->setText("Result Goal staged");
  addActivityEvent("plan", "Goal staged", staged_goal_, "agent.goal");
}

QJsonObject AgentPanel::activityEventJson(const ActivityEvent& event) const {
  QJsonObject object;
  object.insert("id", event.id);
  object.insert("kind", event.kind);
  object.insert("title", event.title);
  object.insert("detail", event.detail);
  object.insert("method", event.method);
  object.insert("created_at", event.created_at);
  return object;
}

void AgentPanel::addActivityEvent(const QString& kind,
                                  const QString& title,
                                  const QString& detail,
                                  const QString& method) {
  ActivityEvent event;
  event.id = "act-" + QString::number(++activity_sequence_);
  event.kind = contextValue(kind, "event");
  event.title = contextValue(title, "Agent event");
  event.detail = detail.trimmed();
  event.method = method.trimmed();
  event.created_at = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
  activity_events_.append(event);
  while (activity_events_.size() > 8) {
    activity_events_.removeFirst();
  }
  renderActivityEvents();
}

void AgentPanel::renderActivityEvents() {
  if (activity_events_layout_ == nullptr) {
    return;
  }
  while (QLayoutItem* item = activity_events_layout_->takeAt(0)) {
    if (QWidget* widget = item->widget()) {
      delete widget;
    }
    delete item;
  }

  for (int i = 0; i < activity_events_.size(); ++i) {
    const ActivityEvent& event = activity_events_.at(i);
    auto* frame = new QFrame(this);
    frame->setObjectName("card:agent_activity_" + QString::number(i + 1));
    frame->setProperty("agentRole", "activityCard");
    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(6, 5, 6, 5);
    layout->setSpacing(2);

    auto* title_row = new QHBoxLayout();
    title_row->setSpacing(6);
    auto* kind_label = new QLabel(event.kind, frame);
    kind_label->setObjectName("label:agent_activity_" + QString::number(i + 1) + "_kind");
    kind_label->setProperty("agentRole", "evidenceKind");
    auto* title_label = new QLabel(event.title, frame);
    title_label->setObjectName("label:agent_activity_" + QString::number(i + 1) + "_title");
    title_label->setProperty("agentRole", "evidenceTitle");
    title_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    title_row->addWidget(kind_label);
    title_row->addWidget(title_label, 1);
    layout->addLayout(title_row);

    if (!event.detail.isEmpty()) {
      auto* detail_label = new QLabel(event.detail, frame);
      detail_label->setObjectName("label:agent_activity_" + QString::number(i + 1) + "_detail");
      detail_label->setWordWrap(true);
      detail_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
      layout->addWidget(detail_label);
    }

    const QString meta = event.method.isEmpty() ? event.id : event.method + " | " + event.id;
    auto* meta_label = new QLabel(meta, frame);
    meta_label->setObjectName("label:agent_activity_" + QString::number(i + 1) + "_meta");
    meta_label->setProperty("agentRole", "evidenceMeta");
    meta_label->setWordWrap(true);
    meta_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(meta_label);

    activity_events_layout_->addWidget(frame);
  }
}

void AgentPanel::renderEvidenceCards() {
  if (evidence_cards_layout_ == nullptr) {
    return;
  }
  while (QLayoutItem* item = evidence_cards_layout_->takeAt(0)) {
    if (QWidget* widget = item->widget()) {
      delete widget;
    }
    delete item;
  }

  for (int i = 0; i < evidence_cards_.size(); ++i) {
    const EvidenceCard& card = evidence_cards_.at(i);
    auto* frame = new QFrame(this);
    frame->setObjectName("card:agent_evidence_" + QString::number(i + 1));
    frame->setProperty("agentRole", "evidenceCard");
    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(6, 5, 6, 5);
    layout->setSpacing(2);

    auto* title_row = new QHBoxLayout();
    title_row->setSpacing(6);
    auto* kind_label = new QLabel(card.kind, frame);
    kind_label->setObjectName("label:agent_evidence_" + QString::number(i + 1) + "_kind");
    kind_label->setProperty("agentRole", "evidenceKind");
    auto* title_label = new QLabel(card.title, frame);
    title_label->setObjectName("label:agent_evidence_" + QString::number(i + 1) + "_title");
    title_label->setProperty("agentRole", "evidenceTitle");
    title_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    title_row->addWidget(kind_label);
    title_row->addWidget(title_label, 1);
    layout->addLayout(title_row);

    auto* summary_label = new QLabel(card.summary, frame);
    summary_label->setObjectName("label:agent_evidence_" + QString::number(i + 1) + "_summary");
    summary_label->setWordWrap(true);
    summary_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(summary_label);

    QString meta = card.method;
    if (!card.artifact_path.isEmpty()) {
      meta += meta.isEmpty() ? card.artifact_path : " | " + card.artifact_path;
    }
    if (!card.id.isEmpty()) {
      meta += meta.isEmpty() ? card.id : " | " + card.id;
    }
    auto* meta_label = new QLabel(meta, frame);
    meta_label->setObjectName("label:agent_evidence_" + QString::number(i + 1) + "_meta");
    meta_label->setProperty("agentRole", "evidenceMeta");
    meta_label->setWordWrap(true);
    meta_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(meta_label);

    evidence_cards_layout_->addWidget(frame);
  }
}

void AgentPanel::pinEvidence() {
  const QJsonObject root = parsedObject(output_->toPlainText());
  const QJsonObject result = resultObjectFromAgentOutput(root);
  const QString method = methodFromAgentOutput(root, live_method_input_->text());
  const QString kind = evidenceKindForMethod(method);

  EvidenceCard card;
  card.id = "ev-" + QString::number(++evidence_sequence_);
  card.kind = kind;
  card.title = evidenceTitleForMethod(method, kind);
  card.method = method;
  card.artifact_path =
      stringValueFromJson(result, {"artifact_path", "path", "target_path", "report_path",
                                   "screenshot_path"});
  card.created_at = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
  card.trace_id = root.value("trace_id").toString();
  card.span_id = root.value("span_id").toString();
  card.source = "agent_panel";
  card.diagnostic_count = intValueFromJson(result, "diagnostic_count");
  card.error_count = intValueFromJson(result, "error_count");
  card.warning_count = intValueFromJson(result, "warning_count");
  card.drc_count = intValueFromJson(result, "drc_count");
  card.erc_count = intValueFromJson(result, "erc_count");
  card.width = intValueFromJson(result, "width");
  card.height = intValueFromJson(result, "height");
  card.summary =
      summarizeEvidenceCard(kind, result_state_label_->text(), result, output_->toPlainText());

  const QString goal = staged_goal_.trimmed().isEmpty() ? goal_input_->text().trimmed() : staged_goal_;
  if (!goal.isEmpty()) {
    card.summary += " | goal " + goal;
  }

  evidence_cards_.append(card);
  while (evidence_cards_.size() > 8) {
    evidence_cards_.removeFirst();
  }
  renderEvidenceCards();
  evidence_label_->setText("Evidence " + QString::number(evidence_cards_.size()) +
                           " pinned | last " + card.kind);
  status_label_->setText("Evidence pinned");
  result_state_label_->setText("Result Evidence pinned");
  addActivityEvent("evidence", "Evidence pinned", card.title + " | " + card.kind,
                   "agent.evidence");
}

void AgentPanel::clearEvidence() {
  evidence_cards_.clear();
  renderEvidenceCards();
  evidence_label_->setText("Evidence 0 pinned | no cards yet");
  status_label_->setText("Evidence cleared");
  result_state_label_->setText("Result Evidence cleared");
  addActivityEvent("evidence", "Evidence cleared", "Pinned evidence queue reset",
                   "agent.evidence");
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
    addActivityEvent("error", "Approval request required",
                     "Approval lane returned to idle", "agent.approval");
    return;
  }
  pending_approval_request_ = trimmed_request;
  approval_last_decision_ = "pending";
  approval_status_label_->setText("Approval pending: " + pending_approval_request_);
  status_label_->setText("Approval pending");
  result_state_label_->setText("Result Approval pending");
  addActivityEvent("approval", "Approval pending", pending_approval_request_,
                   "agent.approval");
}

void AgentPanel::submitCommand() {
  const QString trimmed_command = command_input_->text().trimmed();
  if (trimmed_command.isEmpty()) {
    staged_command_.clear();
    status_label_->setText("Command required");
    result_state_label_->setText("Result Error empty_command");
    output_->setPlainText("{\"error\":\"empty_command\"}\n");
    addActivityEvent("error", "Command required", "No command was staged", "agent.command");
    return;
  }

  staged_command_ = trimmed_command;
  classifyCommandPolicy(staged_command_, true);
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
  const QJsonObject policy = policyStateObject();
  for (auto it = policy.begin(); it != policy.end(); ++it) {
    event.insert(it.key(), it.value());
  }
  event.insert("workspace", workspaceText());
  event.insert("diagnostics", diagnosticsText());
  output_->setPlainText(QString::fromUtf8(QJsonDocument(event).toJson(QJsonDocument::Compact)) +
                        "\n");
  addActivityEvent("command", "Command staged", staged_command_, "agent.command");
}

void AgentPanel::approveNextApproval() {
  if (pending_approval_request_.trimmed().isEmpty()) {
    approval_status_label_->setText("Approvals 0 pending");
    status_label_->setText("No approval pending");
    result_state_label_->setText("Result No approval pending");
    addActivityEvent("approval", "No approval pending", "Accept skipped",
                     "agent.approval");
    return;
  }
  const QString request = pending_approval_request_;
  pending_approval_request_.clear();
  approval_last_decision_ = "accept";
  approval_status_label_->setText("Approval accepted: " + request);
  status_label_->setText("Approval accepted");
  result_state_label_->setText("Result Approval accepted");
  addActivityEvent("approval", "Approval accepted", request, "agent.approval");
}

void AgentPanel::declineNextApproval() {
  if (pending_approval_request_.trimmed().isEmpty()) {
    approval_status_label_->setText("Approvals 0 pending");
    status_label_->setText("No approval pending");
    result_state_label_->setText("Result No approval pending");
    addActivityEvent("approval", "No approval pending", "Decline skipped",
                     "agent.approval");
    return;
  }
  const QString request = pending_approval_request_;
  pending_approval_request_.clear();
  approval_last_decision_ = "decline";
  approval_status_label_->setText("Approval declined: " + request);
  status_label_->setText("Approval declined");
  result_state_label_->setText("Result Approval declined");
  addActivityEvent("approval", "Approval declined", request, "agent.approval");
}

void AgentPanel::cancelApproval() {
  if (pending_approval_request_.trimmed().isEmpty()) {
    approval_status_label_->setText("Approvals 0 pending");
    status_label_->setText("No approval pending");
    result_state_label_->setText("Result No approval pending");
    addActivityEvent("approval", "No approval pending", "Cancel skipped",
                     "agent.approval");
    return;
  }
  const QString request = pending_approval_request_;
  pending_approval_request_.clear();
  approval_last_decision_ = "cancel";
  approval_status_label_->setText("Approval canceled: " + request);
  status_label_->setText("Approval canceled");
  result_state_label_->setText("Result Approval canceled");
  addActivityEvent("approval", "Approval canceled", request, "agent.approval");
}

void AgentPanel::clearApprovals() {
  pending_approval_request_.clear();
  approval_request_input_->clear();
  approval_last_decision_ = "none";
  approval_status_label_->setText("Approvals 0 pending");
  status_label_->setText("Approvals cleared");
  result_state_label_->setText("Result Approvals cleared");
  addActivityEvent("approval", "Approvals cleared", "Approval lane reset",
                   "agent.approval");
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

QString AgentPanel::sessionPathText() const {
  return session_path_input_->text();
}

QString AgentPanel::sessionStatusText() const {
  return session_status_label_->text();
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

QJsonObject AgentPanel::evidenceCardJson(const EvidenceCard& card) const {
  QJsonObject object;
  object.insert("id", card.id);
  object.insert("kind", card.kind);
  object.insert("title", card.title);
  object.insert("summary", card.summary);
  object.insert("method", card.method);
  object.insert("artifact_path", card.artifact_path);
  object.insert("created_at", card.created_at);
  object.insert("trace_id", card.trace_id);
  object.insert("span_id", card.span_id);
  object.insert("source", card.source);
  if (card.diagnostic_count >= 0) {
    object.insert("diagnostic_count", card.diagnostic_count);
  }
  if (card.error_count >= 0) {
    object.insert("error_count", card.error_count);
  }
  if (card.warning_count >= 0) {
    object.insert("warning_count", card.warning_count);
  }
  if (card.drc_count >= 0) {
    object.insert("drc_count", card.drc_count);
  }
  if (card.erc_count >= 0) {
    object.insert("erc_count", card.erc_count);
  }
  if (card.width >= 0) {
    object.insert("width", card.width);
  }
  if (card.height >= 0) {
    object.insert("height", card.height);
  }
  return object;
}

QString AgentPanel::workspaceStateJson() const {
  QJsonArray evidence;
  QJsonArray evidence_cards;
  QJsonArray activity_events;
  QJsonArray plan_items;
  for (const ActivityEvent& event : activity_events_) {
    activity_events.append(activityEventJson(event));
  }
  for (const EvidenceCard& card : evidence_cards_) {
    QString legacy_summary = card.title;
    if (!card.summary.isEmpty()) {
      legacy_summary += " | " + card.summary;
    }
    if (!card.method.isEmpty()) {
      legacy_summary += " | method " + card.method;
    }
    evidence.append(legacy_summary);
    evidence_cards.append(evidenceCardJson(card));
  }
  auto append_plan_item = [&plan_items](const QString& id,
                                        const QString& title,
                                        const QString& state,
                                        const QString& detail,
                                        const int progress) {
    QJsonObject item;
    item.insert("id", id);
    item.insert("title", title);
    item.insert("state", state);
    item.insert("detail", detail);
    item.insert("progress", progress);
    plan_items.append(item);
  };
  append_plan_item("plan-1", "Read workspace context", "ready",
                   "Project, layer, net, selection, and diagnostics are cached for the next action.",
                   100);
  append_plan_item("plan-2", "Collect evidence", "active",
                   "Use DRC/ERC reports, screenshots, and pinned artifacts before proposing edits.",
                   62);
  append_plan_item("plan-3", "Apply bounded changes", "queued",
                   "Mutating tools require policy checks and explicit verification artifacts.",
                   18);

  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("workspace_kind", "ccad_agent_workspace_state");
  response.insert("evidence_manifest_kind", "ccad_agent_evidence_manifest");
  response.insert("panel_layout", "vertical_agent_workspace");
  response.insert("visual_style", "agent_reference_panel_v4");
  response.insert("workspace_layout_version", 4);
  response.insert("active_agent_tab", "command");
  response.insert("visible_sections",
                  QJsonArray{"session_strip",
                             "mode_strip",
                             "trace_strip",
                             "run_queue",
                             "trace_links",
                             "provider_controls",
                             "session_binding",
                             "policy_surface",
                             "run_controls",
                             "status_rail",
                             "command_stream",
                             "command_composer",
                             "task_list",
                             "active_plan",
                             "plan_deck",
                             "activity_stream",
                             "pinned_evidence",
                             "evidence_lane",
                             "approval_card",
                             "approval_lane",
                             "command_bar"});
  response.insert("session_title", session_title_label_->text());
  response.insert("model_label", model_chip_label_->text());
  response.insert("mode_label", mode_chip_label_->text());
  response.insert("permission_label", permission_chip_label_->text());
  response.insert("trace_label", trace_chip_label_->text());
  response.insert("trace_id", trace_id_);
  response.insert("span_id", span_id_);
  response.insert("trace_status", trace_status_);
  response.insert("trace_export_status", trace_export_status_);
  response.insert("trace_backend", "local_metadata_only");
  response.insert("trace_export_enabled", false);
  response.insert("trace_link", "");
  response.insert("trace_link_available", false);
  response.insert("trace_session_id", durable_session_id_);
  response.insert("trace_thread_id", durable_thread_id_);
  response.insert("trace_content_policy",
                  "metadata_only_no_prompt_tool_or_design_payloads");
  response.insert("session_label", session_chip_label_->text());
  response.insert("durable_session_bound", durable_session_bound_);
  response.insert("session_file_path", session_file_path_);
  response.insert("session_path_input", sessionPathText());
  response.insert("durable_session_id", durable_session_id_);
  response.insert("thread_id", durable_thread_id_);
  response.insert("checkpoint_count", session_checkpoint_count_);
  response.insert("latest_checkpoint_id", latest_checkpoint_id_);
  response.insert("replayable", session_replayable_);
  response.insert("session_status", sessionStatusText());
  response.insert("run_state", run_state_);
  response.insert("run_label", run_state_chip_label_->text());
  const QJsonObject queue = runQueueStateObject();
  for (auto it = queue.begin(); it != queue.end(); ++it) {
    response.insert(it.key(), it.value());
  }
  response.insert("plan_item_count", plan_items.size());
  response.insert("plan_items", plan_items);
  response.insert("command", staged_command_.isEmpty() ? commandText().trimmed() : staged_command_);
  response.insert("command_input", commandText());
  response.insert("goal", staged_goal_.isEmpty() ? goal_input_->text().trimmed() : staged_goal_);
  response.insert("task_state", taskStateText());
  response.insert("activity_event_count", activity_events_.size());
  response.insert("activity_events", activity_events);
  response.insert("evidence_count", evidence_cards_.size());
  response.insert("evidence", evidence);
  response.insert("evidence_cards", evidence_cards);
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
  const QJsonObject provider = providerStateObject();
  for (auto it = provider.begin(); it != provider.end(); ++it) {
    response.insert(it.key(), it.value());
  }
  const QJsonObject policy = policyStateObject();
  for (auto it = policy.begin(); it != policy.end(); ++it) {
    response.insert(it.key(), it.value());
  }
  return QString::fromUtf8(QJsonDocument(response).toJson(QJsonDocument::Compact)) + "\n";
}
