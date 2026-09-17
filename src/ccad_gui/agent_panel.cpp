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
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollArea>
#include <QSize>
#include <QSizePolicy>
#include <QStyle>
#include <QVBoxLayout>
#include <QListWidget>
#include <QKeyEvent>
#include <QTimer>
#include <QPoint>
#include <QPixmap>

#include "ccad_gui/agent_icons.hpp"
#include "ccad_gui/agent_marketplace_dialog.hpp"
#include "ccad_gui/agent_settings_dialog.hpp"

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
  metadata.queue_state = object.value("run_queue_state").toObject();
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







}  // namespace

AgentPanel::AgentPanel(QWidget* parent) : QWidget(parent), orchestrator_(std::make_unique<ccad::AgentOrchestrator>()) {
  setObjectName("agentPanel");
  setStyleSheet(R"(
    QWidget#agentPanel {
      background-color: #1e1e1e;
      color: #e3e3e3;
      font-family: "Inter", "Segoe UI", sans-serif;
    }
    QScrollArea#agentScrollArea {
      background-color: transparent;
      border: none;
    }
    QWidget#agentScrollContainer {
      background-color: transparent;
    }
    QFrame[agentRole="section"] {
      background-color: transparent;
    }
    QFrame[agentRole="chatBubbleAgent"] {
      background-color: transparent;
      border-left: 3px solid #8a2be2;
      border-radius: 6px;
      margin: 6px 4px;
      padding: 10px 14px;
    }
    QFrame[agentRole="noticeCard"] {
      background-color: #2b2417;
      border: 1px solid #8b6f35;
      border-left: 3px solid #d29922;
      border-radius: 7px;
      margin: 5px 4px;
      padding: 6px 10px;
    }
    QFrame[agentRole="chatBubbleUser"] {
      background-color: #2d2f36;
      border-radius: 14px;
      margin: 6px 4px;
      padding: 10px 14px;
    }
    QTextBrowser {
      background-color: transparent;
      color: #e3e3e3;
      border: none;
      font-family: "Inter", "Segoe UI", sans-serif;
      font-size: 14px;
      line-height: 1.5;
    }
    QFrame[agentRole="toolCard"] {
      background-color: #25262b;
      border: 1px solid #3d3f4b;
      border-radius: 10px;
      padding: 10px 14px;
      margin: 4px 2px;
    }
    QLabel[agentRole="toolTitle"] {
      color: #a5b4fc;
      font-family: "JetBrains Mono", "Consolas", monospace;
      font-size: 12px;
    }
    QTextEdit#chatInput {
      background-color: #25262b;
      border: 1px solid #3d3f4b;
      border-radius: 14px;
      padding: 12px 16px;
      color: #ffffff;
      font-size: 14px;
    }
    QTextEdit#chatInput:focus {
      border: 1px solid #8a2be2;
      background-color: #2a2b32;
    }
    QPushButton[agentRole="iconButton"] {
      background-color: transparent;
      border: none;
      border-radius: 8px;
      color: #9ca3af;
      padding: 6px;
    }
    QPushButton[agentRole="iconButton"]:hover {
      background-color: #374151;
      color: #ffffff;
    }
    QPushButton[agentRole="iconButtonPrimary"] {
      background-color: #8a2be2;
      border-radius: 8px;
      color: #ffffff;
    }
    QPushButton[agentRole="iconButtonPrimary"]:hover {
      background-color: #7b1fa2;
    }
    QLabel[agentRole="panelTitle"] {
      color: #ffffff;
      font-size: 16px;
      font-weight: 600;
    }
    QLabel[agentRole="chip"] {
      background-color: #374151;
      color: #d1d5db;
      border-radius: 10px;
      padding: 2px 8px;
      font-size: 11px;
      font-weight: 500;
    }
  )");

  auto* main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(0, 0, 0, 0);
  main_layout->setSpacing(0);

  // --- Helper to load SVG icon ---
  auto load_svg_icon = [](const QString& svg) {
    QPixmap pixmap;
    pixmap.loadFromData(svg.toUtf8(), "SVG");
    return QIcon(pixmap);
  };

  // --- TOP BAR ---
  auto* top_bar = new QFrame(this);
  top_bar->setProperty("agentRole", "section");
  auto* top_layout = new QHBoxLayout(top_bar);
  top_layout->setContentsMargins(8, 8, 8, 8);
  
  auto* back_btn = new QPushButton(top_bar);
  back_btn->setObjectName("action:agent_menu");
  back_btn->setToolTip("Open agent navigation");
  back_btn->setIcon(load_svg_icon(ccad_icons::icon_menu)); // Stand-in for back
  back_btn->setProperty("agentRole", "iconButton");
  back_btn->setFixedSize(24, 24);
  
  auto* title = new QLabel("Chat", top_bar);
  title->setProperty("agentRole", "panelTitle");
  title->setAlignment(Qt::AlignCenter);

  auto* templates_btn = new QPushButton(top_bar);
  templates_btn->setObjectName("action:agent_templates");
  templates_btn->setToolTip("Insert prompt template");
  templates_btn->setIcon(load_svg_icon(ccad_icons::icon_templates));
  templates_btn->setProperty("agentRole", "iconButton");
  templates_btn->setFixedSize(24, 24);
  connect(templates_btn, &QPushButton::clicked, this, [this]() {
    if (chat_input_) {
      chat_input_->insertPlainText("/");
      chat_input_->setFocus();
    }
  });

  auto* settings_btn = new QPushButton(top_bar);
  settings_btn->setObjectName("action:settingsBtn");
  settings_btn->setToolTip("Agent settings");
  settings_btn->setIcon(load_svg_icon(ccad_icons::icon_settings));
  settings_btn->setProperty("agentRole", "iconButton");
  settings_btn->setFixedSize(24, 24);
  connect(settings_btn, &QPushButton::clicked, this, [this]() {
    AgentSettingsDialog dialog(this);
    dialog.exec();
  });

  auto* close_btn = new QPushButton(top_bar);
  close_btn->setObjectName("action:agent_close");
  close_btn->setToolTip("Close agent panel");
  close_btn->setIcon(load_svg_icon(ccad_icons::icon_close));
  close_btn->setProperty("agentRole", "iconButton");
  close_btn->setFixedSize(24, 24);
  connect(close_btn, &QPushButton::clicked, this, [this]() {
    this->hide();
  });

  top_layout->addWidget(back_btn);
  top_layout->addStretch();
  top_layout->addWidget(title);
  top_layout->addStretch();
  top_layout->addWidget(templates_btn);
  top_layout->addWidget(settings_btn);
  top_layout->addWidget(close_btn);

  main_layout->addWidget(top_bar);

  // --- CHAT HISTORY ---
  chat_scroll_area_ = new QScrollArea(this);
  chat_scroll_area_->setObjectName("agentScrollArea");
  chat_scroll_area_->setWidgetResizable(true);
  chat_scroll_area_->viewport()->setAutoFillBackground(false);
  chat_scroll_area_->viewport()->setStyleSheet("background-color: transparent;");
  auto* chat_container = new QWidget();
  chat_container->setObjectName("agentScrollContainer");
  chat_container->setStyleSheet("background-color: transparent;");
  chat_history_layout_ = new QVBoxLayout(chat_container);
  chat_history_layout_->setAlignment(Qt::AlignTop);
  chat_scroll_area_->setWidget(chat_container);
  
  main_layout->addWidget(chat_scroll_area_, 1);

  // --- COMPOSER ---
  auto* composer_container = new QFrame(this);
  composer_container->setProperty("agentRole", "section");
  auto* composer_layout = new QVBoxLayout(composer_container);
  composer_layout->setContentsMargins(8, 8, 8, 8);
  
  chat_input_ = new QTextEdit(composer_container);
  chat_input_->setObjectName("control:agent_chat_input");
  chat_input_->setPlaceholderText("Message Agent...");
  chat_input_->setFixedHeight(60);
  connect(chat_input_, &QTextEdit::textChanged, this, [this]() {
    const int doc_height = chat_input_->document()->size().height();
    const int new_height = std::clamp(static_cast<int>(doc_height + 16), 60, 200);
    chat_input_->setFixedHeight(new_height);
  });
  composer_layout->addWidget(chat_input_);
  
  auto* actions_layout = new QHBoxLayout();
  auto* paperclip_btn = new QPushButton(composer_container);
  paperclip_btn->setObjectName("action:agent_attach");
  paperclip_btn->setToolTip("Attach file");
  paperclip_btn->setIcon(load_svg_icon(ccad_icons::icon_attach));
  paperclip_btn->setProperty("agentRole", "iconButton");
  paperclip_btn->setFixedSize(24, 24);
  connect(paperclip_btn, &QPushButton::clicked, this, [this]() {
    const QString file = QFileDialog::getOpenFileName(this, "Attach File");
    if (!file.isEmpty()) {
      chat_input_->insertPlainText(QString(" [Attached: %1] ").arg(file));
      chat_input_->setFocus();
    }
  });
  
  auto* marketplace_btn = new QPushButton(composer_container);
  marketplace_btn->setObjectName("action:agent_marketplace");
  marketplace_btn->setToolTip("Browse agent tools and providers");
  marketplace_btn->setIcon(load_svg_icon(ccad_icons::icon_menu));
  marketplace_btn->setProperty("agentRole", "iconButton");
  marketplace_btn->setFixedSize(24, 24);
  connect(marketplace_btn, &QPushButton::clicked, this, [this]() {
    auto* dialog = new AgentMarketplaceDialog(this, this);
    dialog->show();
  });
  
  auto* context_circle = new QPushButton(composer_container);
  context_circle->setObjectName("action:agent_context_refresh");
  context_circle->setToolTip("Refresh project context");
  context_circle->setIcon(load_svg_icon(ccad_icons::icon_settings)); // Reusing settings as context pie stand-in
  context_circle->setProperty("agentRole", "iconButton");
  context_circle->setFixedSize(24, 24);
  
  auto* context_label = new QLabel("1.2k / 128k context", composer_container);
  context_label->setObjectName("control:contextLabel");
  context_label->setStyleSheet("color: #8b949e; font-size: 11px;");
  
  connect(context_circle, &QPushButton::clicked, this, [this]() {
    appendChatMessage("agent", "*Refreshing AI context map...*");
  });

  auto* stt_btn = new QPushButton(composer_container);
  stt_btn->setObjectName("action:agent_voice");
  stt_btn->setToolTip("Voice input");
  stt_btn->setIcon(load_svg_icon(ccad_icons::icon_mic));
  stt_btn->setProperty("agentRole", "iconButton");
  stt_btn->setFixedSize(32, 24);
  connect(stt_btn, &QPushButton::clicked, this, [this]() {
    chat_input_->insertPlainText("[STT Recording...]");
  });
  
  auto* send_btn = new QPushButton(composer_container);
  send_btn->setIcon(load_svg_icon(ccad_icons::icon_send));
  send_btn->setProperty("agentRole", "iconButtonPrimary");
  send_btn->setObjectName("action:agent_submit_chat");
  send_btn->setToolTip("Send message");
  send_btn->setProperty("target_id", "action:agent_submit_chat");
  send_btn->setFixedSize(32, 32);
  connect(send_btn, &QPushButton::clicked, this, &AgentPanel::submitChat);

  actions_layout->addWidget(paperclip_btn);
  actions_layout->addWidget(marketplace_btn);
  actions_layout->addWidget(context_circle);
  actions_layout->addWidget(context_label);
  actions_layout->addWidget(stt_btn);
  actions_layout->addStretch();
  actions_layout->addWidget(send_btn);

  composer_layout->addLayout(actions_layout);
  main_layout->addWidget(composer_container);

  // --- STATUS HEADER ---
  auto* status_container = new QFrame(this);
  status_container->setProperty("agentRole", "section");
  auto* status_layout = new QVBoxLayout(status_container);
  status_layout->setContentsMargins(8, 0, 8, 4);
  status_layout->setSpacing(4);

  auto* info_row = new QHBoxLayout();
  session_title_label_ = new QLabel("New Session", status_container);
  session_title_label_->setProperty("agentRole", "panelTitle");
  session_title_label_->setStyleSheet("font-size: 12px; font-weight: normal; color: #9ca3af;");
  
  model_chip_label_ = new QLabel("Model: Auto", status_container);
  model_chip_label_->setProperty("agentRole", "chip");
  mode_chip_label_ = new QLabel("Mode: Copilot", status_container);
  mode_chip_label_->setProperty("agentRole", "chip");
  permission_chip_label_ = new QLabel("Permissions: Standard", status_container);
  permission_chip_label_->setProperty("agentRole", "chip");
  
  info_row->addWidget(session_title_label_);
  info_row->addStretch();
  info_row->addWidget(model_chip_label_);
  info_row->addWidget(mode_chip_label_);
  info_row->addWidget(permission_chip_label_);
  status_layout->addLayout(info_row);

  auto* trace_row = new QHBoxLayout();
  trace_chip_label_ = new QLabel("Trace: Off", status_container);
  trace_chip_label_->setProperty("agentRole", "chip");
  session_chip_label_ = new QLabel("Session: None", status_container);
  session_chip_label_->setProperty("agentRole", "chip");
  trace_id_label_ = new QLabel("TraceID: -", status_container);
  trace_id_label_->setProperty("agentRole", "chip");
  span_id_label_ = new QLabel("SpanID: -", status_container);
  span_id_label_->setProperty("agentRole", "chip");
  
  trace_row->addWidget(trace_chip_label_);
  trace_row->addWidget(session_chip_label_);
  trace_row->addWidget(trace_id_label_);
  trace_row->addWidget(span_id_label_);
  trace_row->addStretch();
  status_layout->addLayout(trace_row);

  auto* exec_row = new QHBoxLayout();
  run_state_chip_label_ = new QLabel("Run: Idle", status_container);
  run_state_chip_label_->setProperty("agentRole", "chip");
  run_queue_status_label_ = new QLabel("Queue: Empty", status_container);
  run_queue_status_label_->setProperty("agentRole", "chip");
  run_queue_counts_label_ = new QLabel("Counts: 0/0", status_container);
  run_queue_counts_label_->setProperty("agentRole", "chip");
  run_queue_current_step_label_ = new QLabel("Step: None", status_container);
  run_queue_current_step_label_->setProperty("agentRole", "chip");
  
  exec_row->addWidget(run_state_chip_label_);
  exec_row->addWidget(run_queue_status_label_);
  exec_row->addWidget(run_queue_counts_label_);
  exec_row->addWidget(run_queue_current_step_label_);
  exec_row->addStretch();
  status_layout->addLayout(exec_row);

  // Hidden labels that might still be accessed programmatically
  trace_status_label_ = new QLabel(this); trace_status_label_->hide();
  trace_export_status_label_ = new QLabel(this); trace_export_status_label_->hide();
  session_status_label_ = new QLabel(this); session_status_label_->hide();
  policy_decision_label_ = new QLabel(this); policy_decision_label_->hide();
  policy_risk_label_ = new QLabel(this); policy_risk_label_->hide();
  provider_status_label_ = new QLabel(this); provider_status_label_->hide();
  provider_env_label_ = new QLabel(this); provider_env_label_->hide();
  provider_execution_status_label_ = new QLabel(this); provider_execution_status_label_->hide();
  project_label_ = new QLabel(this); project_label_->hide();
  epoch_label_ = new QLabel(this); epoch_label_->hide();
  status_label_ = new QLabel(this); status_label_->setObjectName("status:agent_run"); status_label_->hide();
  workspace_label_ = new QLabel(this); workspace_label_->hide();
  diagnostics_label_ = new QLabel(this); diagnostics_label_->hide();
  result_state_label_ = new QLabel(this); result_state_label_->setObjectName("status:agent_result"); result_state_label_->hide();
  task_state_label_ = new QLabel(this); task_state_label_->hide();
  evidence_label_ = new QLabel(this); evidence_label_->hide();
  approval_status_label_ = new QLabel(this); approval_status_label_->hide();
  
  action_id_input_ = new QLineEdit(this); action_id_input_->hide();
  session_path_input_ = new QLineEdit(this); session_path_input_->hide();
  provider_selector_ = new QComboBox(this); provider_selector_->hide();
  provider_model_input_ = new QLineEdit(this); provider_model_input_->hide();
  live_method_input_ = new QLineEdit(this); live_method_input_->hide();
  live_payload_input_ = new QLineEdit(this); live_payload_input_->hide();
  goal_input_ = new QLineEdit(this); goal_input_->hide();
  command_input_ = new QLineEdit(this); command_input_->hide();
  approval_request_input_ = new QLineEdit(this); approval_request_input_->hide();
  policy_dry_run_checkbox_ = new QCheckBox(this); policy_dry_run_checkbox_->hide();
  output_ = new QPlainTextEdit(this); output_->hide();

  main_layout->insertWidget(1, status_container);



  slash_popup_ = new QListWidget(this);
  slash_popup_->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
  slash_popup_->setStyleSheet("QListWidget { background-color: #161b22; color: #c9d1d9; border: 1px solid #30363d; border-radius: 4px; padding: 4px; font-family: 'Segoe UI'; font-size: 13px; } QListWidget::item:selected { background-color: #30363d; }");
  slash_popup_->hide();
  connect(slash_popup_, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
    executeSlashCommand(item->text());
  });

  chat_input_->installEventFilter(this);

  if (orchestrator_) {
    orchestrator_->set_progress_callback([this](const ccad::AgentGoal& goal) {
        QMetaObject::invokeMethod(this, [this, goal]() {
            // Marshall background thread update to UI thread
            QString status_str = QString::fromStdString(ccad::goal_status_string(goal.status));
            QString detail_str = QString("Completed %1/%2").arg(goal.completed_count).arg(goal.total_count);
            this->updateRunState(status_str, "Agent Thread", detail_str);
        });
    });
  }

  startPythonBackend();
}

AgentPanel::~AgentPanel() {
  if (python_process_) {
    if (python_process_->state() == QProcess::Running) {
      python_process_->terminate();
      if (!python_process_->waitForFinished(500)) {
        python_process_->kill();
        python_process_->waitForFinished(500);
      }
    }
  }
}

#include <QTextBrowser>

void AgentPanel::appendChatMessage(const QString& role, const QString& text) {
  auto* container = new QWidget();
  auto* container_layout = new QHBoxLayout(container);
  container_layout->setContentsMargins(0, 4, 0, 4);

  auto* bubble = new QFrame(container);
  const bool notice = text.startsWith("Agent provider ") || text.startsWith("Provider ");
  bubble->setProperty("agentRole", notice ? "noticeCard" : (role == "agent" ? "chatBubbleAgent" : "chatBubbleUser"));
  auto* layout = new QVBoxLayout(bubble);
  layout->setContentsMargins(4, 4, 4, 4);
  
  if (text.startsWith("<TOOL>")) {
      QString tool_text = text.mid(6);
      bubble->setProperty("agentRole", "toolCard");
      auto* header_layout = new QHBoxLayout();
      auto* icon = new QLabel("</>", bubble);
      icon->setProperty("agentRole", "toolTitle");
      auto* title = new QLabel(tool_text, bubble);
      title->setProperty("agentRole", "toolTitle");
      header_layout->addWidget(icon);
      header_layout->addWidget(title);
      header_layout->addStretch();
      layout->addLayout(header_layout);
  } else {
      auto* browser = new QTextBrowser(bubble);
      browser->setOpenExternalLinks(true);
      browser->setMarkdown(text);
      browser->setStyleSheet("background-color: transparent; border: none;");
      browser->setFrameShape(QFrame::NoFrame);
      browser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      browser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      browser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
      browser->document()->setDocumentMargin(0.0);
      browser->document()->setTextWidth(-1);
      
      // Keep background transparent so bubble color shows
      browser->viewport()->setAutoFillBackground(false);

      browser->document()->adjustSize();
      int docHeight = browser->document()->size().height() + 10;
      browser->setMinimumHeight(docHeight);
      browser->setMaximumHeight(docHeight);
      layout->addWidget(browser);
  }
  
  if (role == "user") {
      container_layout->addStretch();
      container_layout->addWidget(bubble, 3);
  } else {
      container_layout->addWidget(bubble, 3);
      container_layout->addStretch();
  }

  chat_history_layout_->addWidget(container);
}

void AgentPanel::renderChatChecklist() {
  // Renders activity events as a checklist inline.
  if (activity_events_.isEmpty()) return;
  auto* bubble = new QFrame();
  bubble->setProperty("agentRole", "chatBubbleAgent");
  auto* layout = new QVBoxLayout(bubble);
  layout->setContentsMargins(8, 8, 8, 8);
  
  for (const auto& ev : activity_events_) {
    auto* cb = new QCheckBox(ev.title, bubble);
    cb->setChecked(true);
    cb->setEnabled(false);
    layout->addWidget(cb);
  }
  chat_history_layout_->addWidget(bubble);
}

void AgentPanel::startPythonBackend() {
  python_process_ = new QProcess(this);
  QString python_path = "python";
  if (QFile::exists("src/ccad_agent/venv/Scripts/python.exe")) {
    python_path = "src/ccad_agent/venv/Scripts/python.exe";
  } else if (QFile::exists("src/ccad_agent/venv/bin/python")) {
    python_path = "src/ccad_agent/venv/bin/python";
  }
  python_process_->setProgram(python_path);
  python_process_->setArguments({"src/ccad_agent/orchestrator.py"});
  connect(python_process_, &QProcess::readyReadStandardOutput, this, &AgentPanel::handlePythonOutput);
  connect(python_process_, &QProcess::readyReadStandardError, this, &AgentPanel::handlePythonError);
  python_process_->start();
}

void AgentPanel::handlePythonOutput() {
  if (!python_process_) return;
  while (python_process_->canReadLine()) {
    QByteArray line = python_process_->readLine().trimmed();
    if (line.isEmpty()) continue;
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(line, &err);
    if (err.error == QJsonParseError::NoError && doc.isObject()) {
      QJsonObject obj = doc.object();
      if (obj.contains("method") && obj["method"].toString() == "tool_call") {
        QJsonObject params = obj["params"].toObject();
        QString tool = params["tool"].toString();
        QString args = QJsonDocument(params["args"].toObject()).toJson(QJsonDocument::Compact);
        const QString call_id = params["call_id"].toString();
        appendChatMessage("agent", "<TOOL>" + tool + " " + args);
        QJsonObject result;
        result["jsonrpc"] = "2.0";
        result["id"] = call_id.isEmpty() ? QString("agent-tool-call") : call_id;
        result["method"] = "tool_result";

        if (orchestrator_) {
            ccad::OrchestratorConfig cfg;
            std::string res_str = orchestrator_->execute_tool(tool.toStdString(), args.toStdString(), cfg);
            QJsonParseError res_err;
            QJsonDocument res_doc = QJsonDocument::fromJson(QString::fromStdString(res_str).toUtf8(), &res_err);
            if (res_err.error == QJsonParseError::NoError && res_doc.isObject()) {
                result["result"] = res_doc.object();
            } else {
                result["result"] = QString::fromStdString(res_str);
            }
            if (QString::fromStdString(res_str).contains("\"error\":\"approval_required\"")) {
              pending_tool_name_ = tool;
              pending_tool_args_ = args;
              pending_tool_call_id_ = call_id.isEmpty() ? QStringLiteral("agent-tool-call") : call_id;
              setApprovalRequestText("Agent tool: " + tool + " " + args);
              requestApproval();
            }
        } else {
            result["error"] = QJsonObject{{"code", -32601}, {"message", "ToolBroker not initialized"}};
        }
        
        python_process_->write(QJsonDocument(result).toJson(QJsonDocument::Compact) + "\n");
      } else if (obj.contains("method") && obj["method"].toString() == "message") {
        appendChatMessage("agent", obj["params"].toObject()["text"].toString());
      } else if (obj.contains("method") && obj["method"].toString() == "config_state") {
        if (config_state_cb_) config_state_cb_(obj["params"].toObject());
      } else if (obj.contains("method") && obj["method"].toString() == "provider_state") {
        const QJsonObject params = obj["params"].toObject();
        provider_status_ = params["configured"].toBool(false)
                               ? (params["execution_enabled"].toBool(false)
                                      ? QStringLiteral("configured_memory_only")
                                      : QStringLiteral("credential_received_provider_unavailable"))
                               : QStringLiteral("env_missing");
        updateProviderControls();
      } else if (obj.contains("method") && obj["method"].toString() == "thread_state") {
        const QJsonObject params = obj["params"].toObject();
        const bool resumable = params["resumable"].toBool(false);
        const QString status = resumable ? QStringLiteral("Checkpoint resumable")
                                         : QStringLiteral("Checkpoint unavailable");
        status_label_->setText(status);
        result_state_label_->setText("Result " + status);
        if (resumable) {
          addActivityEvent("session", status,
                           "Thread " + params["thread_id"].toString() +
                               " | checkpoint " + params["checkpoint_id"].toString(),
                           "agent.resume_thread");
        } else {
          addActivityEvent("session", status,
                           params["reason"].toString("No checkpoint backend"),
                           "agent.resume_thread");
        }
      } else if (obj.contains("method") && obj["method"].toString() == "thread_resumed") {
        const QJsonObject params = obj["params"].toObject();
        const QString thread_id = params["thread_id"].toString();
        const QString call_id = params["call_id"].toString();
        const QString detail = QStringLiteral("Thread %1 resumed%2")
                                   .arg(thread_id.isEmpty() ? QStringLiteral("ccad-local") : thread_id,
                                        call_id.isEmpty() ? QString() : QStringLiteral(" | call ") + call_id);
        status_label_->setText("Run resumed");
        result_state_label_->setText("Result resumed");
        addActivityEvent("session", "Checkpoint run resumed", detail,
                         "agent.resume_thread");
      } else if (obj.contains("method") && obj["method"].toString() == "marketplace_catalog") {
        if (marketplace_catalog_cb_) marketplace_catalog_cb_(obj["params"].toObject());
      } else if (obj.contains("method") && obj["method"].toString() == "generated_component") {
        if (component_wizard_cb_) component_wizard_cb_(obj["params"].toObject());
      } else if (obj.contains("method") && obj["method"].toString() == "telemetry") {
        QJsonObject params = obj["params"].toObject();
        if (params.contains("run_state") && run_state_chip_label_) {
          run_state_chip_label_->setText("Run: " + params["run_state"].toString());
        }
        if (params.contains("trace_id") && trace_id_label_) {
          trace_id_label_->setText("TraceID: " + params["trace_id"].toString());
          trace_chip_label_->setText("Trace: Active");
        }
        if (params.contains("span_id") && span_id_label_) {
          span_id_label_->setText("SpanID: " + params["span_id"].toString());
        }
      }
    }
  }
}

void AgentPanel::handlePythonError() {
  if (!python_process_) return;
  QByteArray error = python_process_->readAllStandardError();
  // Optional: log to output
}

void AgentPanel::submitChat() {
  QString text = chat_input_->toPlainText().trimmed();
  if (text.isEmpty()) return;
  
  appendChatMessage("user", text);
  chat_input_->clear();

  if (python_process_ && python_process_->state() == QProcess::Running) {
    QJsonObject payload;
    payload["jsonrpc"] = "2.0";
    payload["method"] = "human_message";
    QJsonObject params;
    params["text"] = text;
    if (context_provider_) {
        params["context"] = QString::fromStdString(context_provider_());
    }
    payload["params"] = params;
    python_process_->write(QJsonDocument(payload).toJson(QJsonDocument::Compact) + "\n");
  }
}

void AgentPanel::sendJsonRpc(const QString& method, const QJsonObject& params) {
  if (python_process_ && python_process_->state() == QProcess::Running) {
    QJsonObject payload;
    payload["jsonrpc"] = "2.0";
    payload["method"] = method;
    payload["params"] = params;
    python_process_->write(QJsonDocument(payload).toJson(QJsonDocument::Compact) + "\n");
  }
}

bool AgentPanel::eventFilter(QObject* obj, QEvent* event) {
  if (obj == chat_input_) {
    if (event->type() == QEvent::KeyPress) {
      auto* key_event = static_cast<QKeyEvent*>(event);
      if (slash_popup_->isVisible()) {
        if (key_event->key() == Qt::Key_Up) {
          int row = slash_popup_->currentRow();
          if (row > 0) slash_popup_->setCurrentRow(row - 1);
          return true;
        } else if (key_event->key() == Qt::Key_Down) {
          int row = slash_popup_->currentRow();
          if (row < slash_popup_->count() - 1) slash_popup_->setCurrentRow(row + 1);
          return true;
        } else if (key_event->key() == Qt::Key_Enter || key_event->key() == Qt::Key_Return) {
          if (auto* item = slash_popup_->currentItem()) {
            executeSlashCommand(item->text());
          }
          return true;
        } else if (key_event->key() == Qt::Key_Escape) {
          hideSlashPopup();
          return true;
        }
      } else {
        if (key_event->key() == Qt::Key_Return && !(key_event->modifiers() & Qt::ShiftModifier)) {
          submitChat();
          return true;
        }
      }
    } else if (event->type() == QEvent::KeyRelease) {
      QString text = chat_input_->toPlainText();
      if (text.startsWith("/")) {
        filterSlashCommands();
      } else {
        hideSlashPopup();
      }
    }
  }
  return QWidget::eventFilter(obj, event);
}

void AgentPanel::showSlashPopup() {
  if (!slash_popup_->isVisible()) {
    QPoint bottom_left = chat_input_->mapToGlobal(QPoint(0, 0));
    slash_popup_->setFixedWidth(chat_input_->width());
    slash_popup_->move(bottom_left.x(), bottom_left.y() - slash_popup_->height() - 4);
    slash_popup_->show();
  }
}

void AgentPanel::hideSlashPopup() {
  slash_popup_->hide();
  slash_popup_->clear();
}

void AgentPanel::filterSlashCommands() {
  QString text = chat_input_->toPlainText().mid(1).trimmed().toLower();
  QStringList all_commands = {"/commands", "/workflow:use:", "/workflow:chaining phase:", "/workflow:chaining state:", "/hooks:", "/set:", "/compact context", "/cc", "/schedule:", "/help", "/drc", "/route", "/place", "/design", "/explain", "/clear", "/marketplace", "/settings"};
  slash_popup_->clear();
  for (const QString& cmd : all_commands) {
    if (text.isEmpty() || cmd.mid(1).toLower().startsWith(text)) {
      slash_popup_->addItem(cmd);
    }
  }
  if (slash_popup_->count() > 0) {
    slash_popup_->setCurrentRow(0);
    slash_popup_->setFixedHeight(qMin(slash_popup_->count() * 28 + 4, 150));
    showSlashPopup();
  } else {
    hideSlashPopup();
  }
}

void AgentPanel::executeSlashCommand(const QString& cmd) {
  hideSlashPopup();
  chat_input_->setPlainText(cmd + " ");
  QTextCursor cursor = chat_input_->textCursor();
  cursor.movePosition(QTextCursor::End);
  chat_input_->setTextCursor(cursor);
  chat_input_->setFocus();
}
void AgentPanel::setUiMapProvider(UiMapProvider provider) {
  ui_map_provider_ = std::move(provider);
}

void AgentPanel::setSafeActionTrigger(SafeActionTrigger trigger) {
  safe_action_trigger_ = std::move(trigger);
}

void AgentPanel::setLiveQueryProvider(LiveQueryProvider provider) {
  live_query_provider_ = std::move(provider);
  
  if (orchestrator_ && live_query_provider_) {
      auto register_ui_tool = [this](const std::string& name, ccad::TaskRisk risk) {
          orchestrator_->register_tool({
              name, "UI Map Tool", risk, "{}",
              [this, name](const std::string& args) -> std::string {
                  if (live_query_provider_) {
                      return live_query_provider_(QString::fromStdString(name), QString::fromStdString(args)).toStdString();
                  }
                  return "{\"error\":\"no provider\"}";
              }
          });
      };
      
      register_ui_tool("ui.place_via", ccad::TaskRisk::LowMutation);
      register_ui_tool("ui.add_track", ccad::TaskRisk::LowMutation);
      register_ui_tool("ui.route_track", ccad::TaskRisk::LowMutation);
      register_ui_tool("ui.add_polygon", ccad::TaskRisk::LowMutation);
      register_ui_tool("ui.add_zone", ccad::TaskRisk::LowMutation);
      register_ui_tool("ui.place_footprint", ccad::TaskRisk::LowMutation);
      register_ui_tool("lib.catalog_info", ccad::TaskRisk::ReadOnly);
      register_ui_tool("lib.catalog_search", ccad::TaskRisk::ReadOnly);
      register_ui_tool("ui.place_symbol", ccad::TaskRisk::LowMutation);
      register_ui_tool("ui.add_wire", ccad::TaskRisk::LowMutation);
      register_ui_tool("ui.add_label", ccad::TaskRisk::LowMutation);
      register_ui_tool("ui.add_keepout", ccad::TaskRisk::LowMutation);
      register_ui_tool("ui.draw_graphic", ccad::TaskRisk::LowMutation);
      register_ui_tool("ui.place_text", ccad::TaskRisk::LowMutation);
      register_ui_tool("ui.delete_object", ccad::TaskRisk::LowMutation);
      register_ui_tool("ui.trigger_safe", ccad::TaskRisk::LowMutation);
      register_ui_tool("ui.click", ccad::TaskRisk::LowMutation);
      register_ui_tool("ui.double_click", ccad::TaskRisk::LowMutation);
      register_ui_tool("ui.type_text", ccad::TaskRisk::LowMutation);
      register_ui_tool("ui.key", ccad::TaskRisk::LowMutation);
      register_ui_tool("ui.select_canvas_object", ccad::TaskRisk::ReadOnly);
      register_ui_tool("ui.get_selection", ccad::TaskRisk::ReadOnly);
      register_ui_tool("ui.active_layer", ccad::TaskRisk::ReadOnly);
      register_ui_tool("ui.set_active_layer", ccad::TaskRisk::ReadOnly);
      register_ui_tool("ui.active_net", ccad::TaskRisk::ReadOnly);
      register_ui_tool("ui.set_active_net", ccad::TaskRisk::ReadOnly);
      register_ui_tool("project.review", ccad::TaskRisk::ReadOnly);
      register_ui_tool("action.drc", ccad::TaskRisk::ReadOnly);
      register_ui_tool("action.route", ccad::TaskRisk::LowMutation);
      register_ui_tool("action.place", ccad::TaskRisk::LowMutation);
      register_ui_tool("ui.open_component_wizard", ccad::TaskRisk::ReadOnly);
  }
}

void AgentPanel::setContextProvider(ContextProvider provider) {
    context_provider_ = std::move(provider);
}

void AgentPanel::setProviderSecret(const QString& provider_id, const QString& secret) {
  const QString provider = provider_id.trimmed().isEmpty() ? QStringLiteral("openai") : provider_id.trimmed();
  if (secret.isEmpty()) {
    provider_secrets_.remove(provider);
  } else {
    provider_secrets_.insert(provider, secret);
  }
  provider_env_present_ = !secret.isEmpty() || providerEnvironmentPresent(providerSpecForId(provider));
  provider_status_ = provider_env_present_ ? QStringLiteral("configured_memory_only")
                                            : QStringLiteral("env_missing");
  refreshProviderStatus();
  // Secret crosses only the private child-process pipe. It is never persisted,
  // echoed, or included in context/tool/audit payloads.
  if (python_process_ && python_process_->state() == QProcess::Running) {
    QJsonObject params;
    params.insert("provider", provider);
    params.insert("secret", secret);
    sendJsonRpc("agent.set_provider_secret", params);
  }
  addActivityEvent("provider", "Provider credential updated",
                   provider + " | secret retained in process memory only",
                   "agent.provider_secret");
}

void AgentPanel::setConfigStateCallback(ConfigStateCallback cb) {
    config_state_cb_ = std::move(cb);
}

void AgentPanel::setMarketplaceCatalogCallback(MarketplaceCatalogCallback cb) {
  marketplace_catalog_cb_ = std::move(cb);
}

void AgentPanel::setComponentWizardCallback(ComponentWizardCallback cb) {
  component_wizard_cb_ = std::move(cb);
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
  const bool memory_present = provider_secrets_.contains(spec.id);
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
  provider.insert("provider_configured", env_present || memory_present);
  provider.insert("provider_secret_present", memory_present);
  provider.insert("provider_status", memory_present ? "configured_memory_only"
                                                     : env_present ? "env_present" : "env_missing");
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
  const bool memory_present = provider_secrets_.contains(spec.id);
  provider_env_present_ = providerEnvironmentPresent(spec) || memory_present;
  provider_status_ = memory_present ? "configured_memory_only"
                                    : provider_env_present_ ? "env_present" : "env_missing";

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
  if (python_process_ && python_process_->state() == QProcess::Running) {
    sendJsonRpc("agent.set_thread_id", QJsonObject{{"thread_id", durable_thread_id_}});
  }
  latest_checkpoint_id_ = metadata.latest_checkpoint_id;
  session_checkpoint_count_ = metadata.checkpoint_count;
  session_replayable_ = metadata.replayable;
  if (!metadata.queue_state.isEmpty()) {
    run_queue_id_ = metadata.queue_state.value("run_queue_id").toString(run_queue_id_);
    run_queue_status_ = metadata.queue_state.value("run_queue_status").toString(run_queue_status_);
    run_queue_current_step_ = metadata.queue_state.value("run_step_current").toString(run_queue_current_step_);
    run_queue_depth_ = metadata.queue_state.value("run_queue_depth").toInt(run_queue_depth_);
    run_queue_completed_count_ = metadata.queue_state.value("run_queue_completed_count").toInt(run_queue_completed_count_);
    run_queue_failed_count_ = metadata.queue_state.value("run_queue_failed_count").toInt(run_queue_failed_count_);
    run_steps_total_ = metadata.queue_state.value("run_steps_total").toInt(run_steps_total_);
    run_queue_cancelable_ = metadata.queue_state.value("run_queue_cancelable").toBool(run_queue_cancelable_);
    updateRunQueueLabels();
  }
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
  object.insert("run_queue_state", runQueueStateObject());
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
  if (python_process_ && python_process_->state() == QProcess::Running) {
    sendJsonRpc("agent.resume_thread", QJsonObject());
  }
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
  renderChatChecklist();
}

void AgentPanel::renderEvidenceCards() {
  // Evidences now handled via chat flow if needed.
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
  if (!pending_tool_name_.isEmpty() && orchestrator_) {
    ccad::OrchestratorConfig cfg;
    cfg.approved_tool_name = pending_tool_name_.toStdString();
    const std::string approved = orchestrator_->execute_tool(
        pending_tool_name_.toStdString(), pending_tool_args_.toStdString(), cfg);
    QJsonObject result{{"jsonrpc", "2.0"}, {"method", "tool_result"},
                       {"id", pending_tool_call_id_}};
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(
        QString::fromStdString(approved).toUtf8(), &error);
    if (error.error == QJsonParseError::NoError && document.isObject()) {
      result.insert("result", document.object());
    } else {
      result.insert("result", QString::fromStdString(approved));
    }
    if (python_process_) {
      python_process_->write(QJsonDocument(result).toJson(QJsonDocument::Compact) + "\n");
    }
    pending_tool_name_.clear();
    pending_tool_args_.clear();
    pending_tool_call_id_.clear();
  }
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
  if (!pending_tool_call_id_.isEmpty() && python_process_) {
    const QJsonObject result{
        {"jsonrpc", "2.0"},
        {"method", "tool_result"},
        {"id", pending_tool_call_id_},
        {"error", QJsonObject{{"code", -32001}, {"message", "approval_denied"}}}};
    python_process_->write(QJsonDocument(result).toJson(QJsonDocument::Compact) + "\n");
  }
  pending_tool_name_.clear();
  pending_tool_args_.clear();
  pending_tool_call_id_.clear();
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
  if (!pending_tool_call_id_.isEmpty() && python_process_) {
    const QJsonObject result{
        {"jsonrpc", "2.0"},
        {"method", "tool_result"},
        {"id", pending_tool_call_id_},
        {"error", QJsonObject{{"code", -32800}, {"message", "approval_canceled"}}}};
    python_process_->write(QJsonDocument(result).toJson(QJsonDocument::Compact) + "\n");
  }
  pending_tool_name_.clear();
  pending_tool_args_.clear();
  pending_tool_call_id_.clear();
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
  response.insert("visual_style", "agent_reference_panel_v5");
  response.insert("workspace_layout_version", 5);
  response.insert("reference_layout_density", "compact_sidebar");
  response.insert("reference_inspiration", "provided_agent_sidebar_samples");
  response.insert("active_agent_tab", "command");
  response.insert("visible_sections",
                  QJsonArray{"header_action_bar",
                             "command_composer"});
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
