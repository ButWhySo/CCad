#include "review_window.hpp"
#include "board_canvas_view.hpp"
#include "library_browser_dialog.hpp"
#include "ui_map_server.hpp"

#include <QApplication>
#include <QAbstractButton>
#include <QCheckBox>
#include <QCursor>
#include <QElapsedTimer>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QPixmap>
#include <QListWidget>
#include <QLabel>
#include <QMessageBox>
#include <QProxyStyle>
#include <QRegularExpression>
#include <QScreen>
#include <QStyleOption>
#include <QTextBrowser>
#include <QThread>
#include <QTimer>

#include <QEventLoop>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace {

class ThemedFocusStyle final : public QProxyStyle {
 public:
  using QProxyStyle::QProxyStyle;

  void drawPrimitive(const PrimitiveElement element, const QStyleOption* option,
                     QPainter* painter, const QWidget* widget = nullptr) const override {
    if (element == PE_FrameFocusRect) return;
    QProxyStyle::drawPrimitive(element, option, painter, widget);
  }
};

constexpr int kSingleScreenshotWaitMs = 7000;
constexpr int kMultiTargetInitialWaitMs = 5000;
constexpr int kMultiTargetPerTargetWaitMs = 800;

int parsePositiveIntArg(char** argv, const int index, const int fallback) {
  try {
    const int value = std::stoi(argv[index]);
    return value > 0 ? value : fallback;
  } catch (...) {
    return fallback;
  }
}

QString jsonStringLocal(const QString& value) {
  QString output = "\"";
  for (const QChar ch : value) {
    if (ch == '\\') {
      output += "\\\\";
    } else if (ch == '"') {
      output += "\\\"";
    } else if (ch == '\n') {
      output += "\\n";
    } else if (ch == '\r') {
      output += "\\r";
    } else if (ch == '\t') {
      output += "\\t";
    } else {
      output += ch;
    }
  }
  output += "\"";
  return output;
}

std::optional<int> extractJsonInt(const QString& json, const QString& key) {
  const int key_index = json.indexOf(key);
  if (key_index < 0) {
    return std::nullopt;
  }
  int index = key_index + key.size();
  while (index < json.size() && json.at(index).isSpace()) {
    ++index;
  }
  int end = index;
  if (end < json.size() && json.at(end) == '-') {
    ++end;
  }
  while (end < json.size() && json.at(end).isDigit()) {
    ++end;
  }
  if (end == index) {
    return std::nullopt;
  }
  bool ok = false;
  const int value = json.mid(index, end - index).toInt(&ok);
  return ok ? std::optional<int>(value) : std::nullopt;
}

}  // namespace

int screenshotWindow(QWidget& window, const QString& screenshot_path, const char* screenshot_arg) {
  const QPixmap screenshot = window.grab();
  if (!screenshot.save(screenshot_path)) {
    std::cerr << "failed to save GUI screenshot: " << screenshot_arg << '\n';
    std::cerr.flush();
    return 2;
  }
  std::cout << "screenshot saved: " << screenshot_arg << '\n';
  std::cout.flush();
  return 0;
}

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  app.setStyle(new ThemedFocusStyle());

  if (argc == 4 && (std::string(argv[1]) == "--dump-ui-map" ||
                    std::string(argv[1]) == "--validate-ui-map-targets")) {
    const bool validate_targets = std::string(argv[1]) == "--validate-ui-map-targets";
    const std::filesystem::path project_path(argv[2]);
    const std::filesystem::path output_path(argv[3]);
    auto* window = new ReviewWindow();
    window->setAutomationMode(true);

    window->loadProjectPath(project_path);
    window->show();

    QTimer::singleShot(500, window, [window, output_path, validate_targets]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open UI map output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QString map = validate_targets ? window->validateUiMapTargetsJson(true)
                                           : window->uiMapJson();
      const QByteArray bytes = map.toUtf8();
      output.write(bytes.constData(), bytes.size());
      if (!output) {
        std::cerr << "failed to write UI map output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      std::cout << (validate_targets ? "ui map target validation saved: " : "ui map saved: ")
                << output_path.string() << '\n';
      std::cout.flush();
      QCoreApplication::exit(0);
    });

    return QApplication::exec();
  } else if (argc == 5 && std::string(argv[1]) == "--ui-target-id") {
    const std::filesystem::path project_path(argv[2]);
    const QString target_id = QString::fromLocal8Bit(argv[3]);
    const std::filesystem::path output_path(argv[4]);
    auto* window = new ReviewWindow();
    window->setAutomationMode(true);

    window->loadProjectPath(project_path);
    window->show();

    QTimer::singleShot(500, window, [window, target_id, output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open UI target output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window->uiTargetJsonById(target_id).toUtf8();
      output.write(bytes.constData(), bytes.size());
      if (!output) {
        std::cerr << "failed to write UI target output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      std::cout << "ui target saved: " << output_path.string() << '\n';
      std::cout.flush();
      QCoreApplication::exit(0);
    });

    return QApplication::exec();
  } else if (argc == 5 && std::string(argv[1]) == "--ui-trigger-safe") {
    const std::filesystem::path project_path(argv[2]);
    const QString action_id = QString::fromLocal8Bit(argv[3]);
    const std::filesystem::path output_path(argv[4]);
    auto* window = new ReviewWindow();
    window->setAutomationMode(true);

    window->loadProjectPath(project_path);
    window->show();

    QTimer::singleShot(500, window, [window, action_id, output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open UI action output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window->triggerSafeUiActionJson(action_id).toUtf8();
      output.write(bytes.constData(), bytes.size());
      if (!output) {
        std::cerr << "failed to write UI action output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      std::cout << "ui action result saved: " << output_path.string() << '\n';
      std::cout.flush();
      QCoreApplication::exit(0);
    });

    return QApplication::exec();
  } else if (argc == 4 && std::string(argv[1]) == "--ui-active-layer") {
    const std::filesystem::path project_path(argv[2]);
    const std::filesystem::path output_path(argv[3]);
    auto* window = new ReviewWindow();
    window->setAutomationMode(true);

    window->loadProjectPath(project_path);
    window->show();

    QTimer::singleShot(500, window, [window, output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open active layer output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window->activePcbLayerJson().toUtf8();
      output.write(bytes.constData(), bytes.size());
      if (!output) {
        std::cerr << "failed to write active layer output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      std::cout << "active layer saved: " << output_path.string() << '\n';
      std::cout.flush();
      QCoreApplication::exit(0);
    });

    return QApplication::exec();
  } else if (argc == 5 && std::string(argv[1]) == "--ui-set-active-layer") {
    const std::filesystem::path project_path(argv[2]);
    const QString layer_id = QString::fromLocal8Bit(argv[3]);
    const std::filesystem::path output_path(argv[4]);
    auto* window = new ReviewWindow();
    window->setAutomationMode(true);

    window->loadProjectPath(project_path);
    window->show();

    QTimer::singleShot(500, window, [window, layer_id, output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open active layer output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window->setActivePcbLayerForAutomation(layer_id).toUtf8();
      output.write(bytes.constData(), bytes.size());
      if (!output) {
        std::cerr << "failed to write active layer output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      std::cout << "active layer result saved: " << output_path.string() << '\n';
      std::cout.flush();
      QCoreApplication::exit(0);
    });

    return QApplication::exec();
  } else if (argc == 4 && std::string(argv[1]) == "--ui-active-net") {
    const std::filesystem::path project_path(argv[2]);
    const std::filesystem::path output_path(argv[3]);
    auto* window = new ReviewWindow();
    window->setAutomationMode(true);

    window->loadProjectPath(project_path);
    window->show();

    QTimer::singleShot(500, window, [window, output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open active net output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window->activePcbNetJson().toUtf8();
      output.write(bytes.constData(), bytes.size());
      if (!output) {
        std::cerr << "failed to write active net output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      std::cout << "active net saved: " << output_path.string() << '\n';
      std::cout.flush();
      QCoreApplication::exit(0);
    });

    return QApplication::exec();
  } else if (argc == 5 && std::string(argv[1]) == "--ui-set-active-net") {
    const std::filesystem::path project_path(argv[2]);
    const QString net_id = QString::fromLocal8Bit(argv[3]);
    const std::filesystem::path output_path(argv[4]);
    auto* window = new ReviewWindow();
    window->setAutomationMode(true);

    window->loadProjectPath(project_path);
    window->show();

    QTimer::singleShot(500, window, [window, net_id, output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open active net output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window->setActivePcbNetForAutomation(net_id).toUtf8();
      output.write(bytes.constData(), bytes.size());
      if (!output) {
        std::cerr << "failed to write active net output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      std::cout << "active net result saved: " << output_path.string() << '\n';
      std::cout.flush();
      QCoreApplication::exit(0);
    });

    return QApplication::exec();
  } else if (argc == 5 && std::string(argv[1]) == "--serve-ui-map") {
    const std::filesystem::path project_path(argv[2]);
    const QString server_name = QString::fromLocal8Bit(argv[3]);
    const QString ready_path = QString::fromLocal8Bit(argv[4]);
    auto* window = new ReviewWindow();
    window->loadProjectPath(project_path);
    window->show();
    auto* server = new UiMapServer(*window);
    if (!server->listen(server_name)) {
      std::cerr << "failed to listen on UI map server: " << server->errorString().toStdString()
                << '\n';
      std::cerr.flush();
      delete server;
      delete window;
      return 2;
    }
    QFile ready_file(ready_path);
    if (!ready_file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
      std::cerr << "failed to write UI map server ready file: " << ready_path.toStdString()
                << '\n';
      std::cerr.flush();
      delete server;
      delete window;
      return 2;
    }
    ready_file.write(("server=" + server_name + "\n").toUtf8());
    ready_file.close();
    QObject::connect(&app, &QCoreApplication::aboutToQuit, window, [server]() {
      server->close();
      delete server;
    });

    return QApplication::exec();
  } else if (argc == 6 && std::string(argv[1]) == "--ui-target-board-point") {
    const std::filesystem::path project_path(argv[2]);
    const double x_mm = std::stod(argv[3]);
    const double y_mm = std::stod(argv[4]);
    const std::filesystem::path output_path(argv[5]);
    auto* window = new ReviewWindow();
    window->setAutomationMode(true);

    window->loadProjectPath(project_path);
    window->show();

    QTimer::singleShot(500, window, [window, x_mm, y_mm, output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open UI target output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window->uiTargetJsonForBoardPoint(x_mm, y_mm).toUtf8();
      output.write(bytes.constData(), bytes.size());
      if (!output) {
        std::cerr << "failed to write UI target output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      std::cout << "ui target saved: " << output_path.string() << '\n';
      std::cout.flush();
      QCoreApplication::exit(0);
    });

    return QApplication::exec();
  } else if ((argc == 5 || argc == 7) &&
             std::string(argv[1]) == "--test-ui-map-target-sequence") {
    const std::filesystem::path project_path(argv[2]);
    const std::filesystem::path output_dir(argv[3]);
    const QString name = QString::fromLocal8Bit(argv[4]);
    const int initial_wait_ms =
        argc == 7 ? parsePositiveIntArg(argv, 5, kMultiTargetInitialWaitMs)
                  : kMultiTargetInitialWaitMs;
    const int per_target_wait_ms =
        argc == 7 ? parsePositiveIntArg(argv, 6, kMultiTargetPerTargetWaitMs)
                  : kMultiTargetPerTargetWaitMs;
    std::filesystem::create_directories(output_dir);
    QString placed_via_id;
    auto* window = new ReviewWindow();
    window->setAutomationMode(true);

    // Suppress blocking QMessageBox dialogs during automated test runs so
    // the event loop cannot hang waiting for user interaction.
    window->setAutomationMode(true);
    window->loadProjectPath(project_path);
    // NOTE: showFullScreen() after showMaximized() causes STATUS_HEAP_CORRUPTION
    // (0xC0000374) on Windows inside the Qt6 MinGW runtime.  showMaximized()
    // alone gives a full-resolution window sufficient for screenshot validation.
    window->showMaximized();

    QTimer::singleShot(initial_wait_ms, window,
                       [window, output_dir, project_path, name, initial_wait_ms,
                        per_target_wait_ms, &placed_via_id]() {
      QStringList entries;
      const QString catalog_startup = window->runAgentUiQueryJson("agent.workspace_state", "{}");
      const bool catalog_startup_verified =
          catalog_startup.contains("\"backend_ready\":true") &&
          catalog_startup.contains("\"native_tool_catalog_installed\":true") &&
          !catalog_startup.contains("\"native_tool_catalog_method_count\":0");
      if (name.startsWith("sprint968-task") ||
          name.startsWith("sprint969-context") ||
          name.startsWith("sprint970-compaction") ||
          name.startsWith("sprint976-conversation") ||
          name.startsWith("sprint977-context") ||
          name.startsWith("sprint980-project-retrieval") ||
          name.startsWith("sprint981-schematic-project-graph") ||
          name.startsWith("sprint982-multilayer-project-context") ||
          name.startsWith("sprint983-project-index-typed-geometry") ||
          name.startsWith("sprint984-board-net-retrieval") ||
          name.startsWith("sprint985-project-reference-graph") ||
          name.startsWith("sprint986-project-spatial-index") ||
          name.startsWith("sprint987-schematic-metadata") ||
          name.startsWith("sprint975-memory-ui") ||
          name.startsWith("sprint974-memory")) {
        const auto interact = [window, &entries, &output_dir, &name,
                               per_target_wait_ms](const QString& method,
                                                  const QString& payload,
                                                  const QString& target_id,
                                                  const QString& action_name) {
          const QString target = window->uiTargetJsonById(target_id);
          if (!target.contains("\"found\":true")) {
            entries << QString("{\"target\":%1,\"found\":false}")
                           .arg(jsonStringLocal(target_id));
            return false;
          }
          const QString result = window->runAgentUiQueryJson(method, payload);
          QApplication::processEvents();
          QThread::msleep(static_cast<unsigned long>(per_target_wait_ms));
          QApplication::processEvents();
          const QStringList memory_checkpoints = {
              "memory-ui-settings-open", "memory-ui-manager-open",
              "memory-ui-settings-closed"};
          QString screenshot_path;
          if ((!name.startsWith("sprint975-memory-ui") &&
               !name.startsWith("sprint976-conversation") &&
               !name.startsWith("sprint977-context") &&
               !name.startsWith("sprint980-project-retrieval") &&
               !name.startsWith("sprint981-schematic-project-graph") &&
               !name.startsWith("sprint982-multilayer-project-context") &&
                !name.startsWith("sprint983-project-index-typed-geometry") &&
               !name.startsWith("sprint984-board-net-retrieval") &&
                !name.startsWith("sprint986-project-spatial-index") &&
                !name.startsWith("sprint987-schematic-metadata")) ||
              memory_checkpoints.contains(action_name)) {
            screenshot_path = QString::fromStdString(
                (output_dir / (name + "-" + action_name + ".png").toStdString()).string());
            if (QScreen* screen = window->screen())
              screen->grabWindow(0).save(screenshot_path);
          }
          QString popup_screenshot;
          if (auto* popup = window->findChild<QListWidget*>("panel:agent_slash_commands");
               !name.startsWith("sprint983-project-index-typed-geometry") &&
               !name.startsWith("sprint984-board-net-retrieval") &&
               !name.startsWith("sprint986-project-spatial-index") &&
               !name.startsWith("sprint985-project-reference-graph") &&
               !name.startsWith("sprint987-schematic-metadata") &&
              popup && popup->isVisible()) {
            popup_screenshot = QString::fromStdString(
                (output_dir / (name + "-" + action_name + "-slash-popup.png").toStdString()).string());
            popup->grab().save(popup_screenshot);
          }
          entries << QString("{\"target\":%1,\"interaction\":%2,\"result\":%3,\"screenshot\":%4,\"popup_screenshot\":%5}")
                         .arg(jsonStringLocal(target_id), jsonStringLocal(method),
                              result.trimmed(), screenshot_path.isEmpty()
                                  ? "null" : jsonStringLocal(screenshot_path),
                              popup_screenshot.isEmpty() ? "null" : jsonStringLocal(popup_screenshot));
          const QJsonDocument parsed = QJsonDocument::fromJson(result.toUtf8());
          if (!parsed.isObject() || !parsed.object().value("ok").toBool()) return false;
          const QJsonObject action_result = parsed.object().value("result").toObject();
          return action_result.value("performed").toBool(true);
        };
        bool ok = true;
        if (name.startsWith("sprint969-context")) {
          ok = interact("ui.click", "{\"id\":\"tab:pcb\"}",
                        "tab:pcb", "pcb-tab-checked") && ok;
          ok = interact("ui.click", "{\"id\":\"tab:schematic\"}",
                        "tab:schematic", "schematic-tab-checked") && ok;
          ok = interact("ui.click", "{\"id\":\"tab:agent\"}",
                        "tab:agent", "agent-tab-opened") && ok;
          ok = interact("ui.type_text",
                        "{\"id\":\"control:agent_chat_input\",\"text\":\"/context\"}",
                        "control:agent_chat_input", "context-palette-opened") && ok;
          ok = interact("ui.key", "{\"key\":\"Enter\"}",
                        "panel:agent_slash_commands", "context-command-selected") && ok;
          ok = interact("ui.type_text",
                        "{\"id\":\"control:agent_chat_input\",\"text\":\"/context preview Preserve ground clearance around U3\"}",
                        "control:agent_chat_input", "context-preview-draft-entered") && ok;
          ok = interact("ui.click", "{\"id\":\"action:agent_submit_chat\"}",
                        "action:agent_submit_chat", "context-preview-rendered") && ok;
          auto* chat = window->findChild<QTextBrowser*>("control:agent_chat_stream");
          const bool preview_visible = chat != nullptr &&
              chat->toPlainText().contains("Local context preview (large)") &&
              chat->toPlainText().contains("no provider request was sent") &&
              !chat->toPlainText().contains(
                  "Agent backend warning: [ccad-context-preview]");
          entries << QString("{\"context_preview_visible\":%1,\"provider_request_sent\":false}")
                         .arg(preview_visible ? "true" : "false");
          ok = preview_visible && ok;
        } else if (name.startsWith("sprint970-compaction")) {
          ok = interact("ui.click", "{\"id\":\"tab:pcb\"}",
                        "tab:pcb", "pcb-tab-checked") && ok;
          ok = interact("ui.click", "{\"id\":\"tab:schematic\"}",
                        "tab:schematic", "schematic-tab-checked") && ok;
          ok = interact("ui.click", "{\"id\":\"tab:agent\"}",
                        "tab:agent", "agent-tab-opened") && ok;
          ok = interact("ui.type_text",
                        "{\"id\":\"control:agent_chat_input\",\"text\":\"/cc\"}",
                        "control:agent_chat_input", "compact-command-suggested") && ok;
          ok = interact("ui.key", "{\"key\":\"Enter\"}",
                        "panel:agent_slash_commands", "compact-command-selected") && ok;
          ok = interact("ui.type_text",
                        "{\"id\":\"control:agent_chat_input\",\"text\":\"/cc\"}",
                        "control:agent_chat_input", "compact-command-ready") && ok;
          ok = interact("ui.click", "{\"id\":\"action:agent_submit_chat\"}",
                        "action:agent_submit_chat", "compact-command-result") && ok;
          auto* chat = window->findChild<QTextBrowser*>("control:agent_chat_stream");
          const bool safe_noop_visible = chat != nullptr &&
              chat->toPlainText().contains(
                  "No older conversation history needs compaction; no provider request was sent.");
          entries << QString("{\"safe_noop_visible\":%1,\"provider_request_sent\":false}")
                         .arg(safe_noop_visible ? "true" : "false");
          ok = safe_noop_visible && ok;
        } else if (name.startsWith("sprint976-conversation") ||
                   name.startsWith("sprint977-context") ||
                   name.startsWith("sprint980-project-retrieval") ||
                   name.startsWith("sprint981-schematic-project-graph") ||
                   name.startsWith("sprint982-multilayer-project-context") ||
                   name.startsWith("sprint983-project-index-typed-geometry") ||
                   name.startsWith("sprint984-board-net-retrieval") ||
                   name.startsWith("sprint985-project-reference-graph") ||
                   name.startsWith("sprint986-project-spatial-index") ||
                   name.startsWith("sprint987-schematic-metadata")) {
          const auto capture = [window, &output_dir, &name, &entries](const QString& state) {
            const QString path = QString::fromStdString(
                (output_dir / (name + "-" + state + ".png").toStdString()).string());
            window->raise();
            window->activateWindow();
            QApplication::processEvents();
            if (!window->grab().save(path)) return false;
            entries << QString("{\"conversation_screenshot\":%1,\"state\":%2}")
                           .arg(jsonStringLocal(path), jsonStringLocal(state));
            return true;
          };
          ok = capture("before") && ok;
          ok = interact("ui.click", "{\"id\":\"tab:pcb\"}",
                        "tab:pcb", "conversation-pcb-tab") && ok;
          ok = interact("ui.click", "{\"id\":\"tab:schematic\"}",
                        "tab:schematic", "conversation-schematic-tab") && ok;
          ok = interact("ui.click", "{\"id\":\"tab:agent\"}",
                        "tab:agent", "conversation-agent-tab") && ok;
          const bool context_memory_validation = name.startsWith("sprint977-context");
          const bool schematic_graph_validation =
              name.startsWith("sprint981-schematic-project-graph");
          const bool schematic_metadata_validation =
              name.startsWith("sprint987-schematic-metadata");
          const bool multilayer_project_validation =
              name.startsWith("sprint982-multilayer-project-context");
          const bool serialized_pad_layer_validation =
              name.startsWith("sprint983-project-index-typed-geometry");
          const bool board_net_validation =
              name.startsWith("sprint984-board-net-retrieval");
          const bool diagnostic_graph_validation =
              name.startsWith("sprint985-project-reference-graph");
          const bool spatial_diagnostic_validation =
              name.startsWith("sprint986-project-spatial-index");
          const bool diagnostic_validation = diagnostic_graph_validation ||
              spatial_diagnostic_validation;
          const bool project_retrieval_validation = schematic_graph_validation ||
              multilayer_project_validation ||
              serialized_pad_layer_validation ||
              board_net_validation ||
              diagnostic_validation ||
              schematic_metadata_validation ||
              name.startsWith("sprint980-project-retrieval");
          if (diagnostic_validation) {
            ok = interact("ui.click", "{\"id\":\"action:agent_quick_run_drc\"}",
                          "action:agent_quick_run_drc", "diagnostic-drc-command-entered") && ok;
            ok = interact("ui.click", "{\"id\":\"action:agent_submit_chat\"}",
                          "action:agent_submit_chat", "diagnostic-drc-command-run") && ok;
          }
          auto* chat = window->findChild<QTextBrowser*>("control:agent_chat_stream");
          bool project_diagnostic_visible = !diagnostic_validation;
          if (diagnostic_validation) {
            const QJsonObject live_context = QJsonDocument::fromJson(
                window->runAgentUiQueryJson("project.context", "{}").toUtf8())
                .object().value("result").toObject();
            bool exact_live_diagnostic = false;
            for (const QJsonValue& value :
                 live_context.value("project_diagnostics").toArray()) {
              const QJsonObject diagnostic = value.toObject();
              exact_live_diagnostic = exact_live_diagnostic ||
                  (diagnostic.value("engine").toString() == "drc" &&
                   diagnostic.value("code").toString() == "ZERO_LENGTH_TRACK" &&
                   diagnostic.value("object_id").toString() ==
                       (spatial_diagnostic_validation ? "T_SPRINT986_ZERO" :
                                                        "T_SPRINT985_ZERO"));
            }
            entries << QString("{\"live_diagnostic_object_link_verified\":%1}")
                           .arg(exact_live_diagnostic ? "true" : "false");
            project_diagnostic_visible = exact_live_diagnostic;
            ok = exact_live_diagnostic && ok;
            if (spatial_diagnostic_validation)
              ok = capture("diagnostics-ready") && ok;
          }
          if (multilayer_project_validation) {
            const QString before_state_json = window->runAgentUiQueryJson("project.state", "{}");
            const QJsonObject before_state = QJsonDocument::fromJson(
                before_state_json.toUtf8()).object().value("result").toObject()
                .value("project").toObject().value("board").toObject();
            const int before_via_count = before_state.value("vias").toArray().size();
            ok = interact("ui.trigger_safe", "{\"id\":\"action:add_via\"}",
                          "action:add_via", "multilayer-via-tool-activated") && ok;
            ok = interact("ui.canvas_click",
                          "{\"x_mm\":44,\"y_mm\":30,\"canvas\":\"canvas:pcb\"}",
                          "canvas:pcb", "multilayer-via-placed") && ok;
            const QString placed_state_json = window->runAgentUiQueryJson("project.state", "{}");
            const QJsonObject placed_state = QJsonDocument::fromJson(
                placed_state_json.toUtf8()).object().value("result").toObject()
                .value("project").toObject().value("board").toObject();
            const QJsonArray vias = placed_state.value("vias").toArray();
            for (const QJsonValue& value : vias) {
              const QJsonObject via = value.toObject();
              if (via.value("start_layer_id").toString() == "F.Cu" &&
                  via.value("end_layer_id").toString() == "B.Cu" &&
                  !before_state.value("vias").toArray().contains(value)) {
                placed_via_id = via.value("id").toString();
                break;
              }
            }
            const bool via_span_visible = vias.size() == before_via_count + 1 &&
                                          !placed_via_id.isEmpty();
            entries << QString("{\"multilayer_via_created\":%1,\"via_id\":%2,\"start_layer_id\":\"F.Cu\",\"end_layer_id\":\"B.Cu\"}")
                           .arg(via_span_visible ? "true" : "false",
                                jsonStringLocal(placed_via_id));
            ok = via_span_visible && capture("via-placed") && ok;
            if (via_span_visible)
              entries << QString("{\"via_persist_path\":%1}")
                             .arg(jsonStringLocal(QString::fromStdString(project_path.string())));
          }
          QString user_prompt = project_retrieval_validation
              ? (spatial_diagnostic_validation
                     ? QStringLiteral("Find DRC markers in bounding box from 10,10 to 14,14 mm.")
                     : diagnostic_graph_validation
                     ? QStringLiteral("Find DRC ZERO_LENGTH_TRACK on T_SPRINT985_ZERO and explain the affected object.")
                     : multilayer_project_validation
                     ? QStringLiteral("What copper layers does via %1 span?")
                           .arg(placed_via_id)
                      : schematic_graph_validation
                      ? QStringLiteral("Inspect schematic net AC1 and list its member pins.")
                      : serialized_pad_layer_validation
                      ? QString()
                      : board_net_validation
                      ? QStringLiteral("Inspect the PCB net AC1 and identify its exact member pads.")
                      : schematic_metadata_validation
                      ? QStringLiteral("Find the Manufacturer field ACME-42 on U3 and the Power Stage schematic sheet path sheets/power_stage.kicad_sch.")
                      : QStringLiteral("Describe component U_DEMO in the loaded project."))
              : context_memory_validation
              ? QStringLiteral("What memory applies to GND near U3 on F.Cu?")
              : QStringLiteral("Record this thread-local verification turn.");
          if (serialized_pad_layer_validation) {
            const QString state_json = window->runAgentUiQueryJson("project.state", "{}");
            const QJsonObject board = QJsonDocument::fromJson(state_json.toUtf8())
                .object().value("result").toObject().value("project").toObject()
                .value("board").toObject();
            QString pad_id;
            for (const QJsonValue& value : board.value("pads").toArray()) {
              const QJsonObject pad = value.toObject();
              const QJsonArray layers = pad.value("padstack").toObject()
                  .value("layer_set").toArray();
              if (layers.contains("F.Cu") && layers.contains("B.Cu")) {
                pad_id = pad.value("id").toString();
                break;
              }
            }
            user_prompt = pad_id.isEmpty()
                ? QStringLiteral("__SPRINT983_PAD_LAYER_FIXTURE_MISSING__")
                : QStringLiteral("What layers does pad %1 use? Include B.Cu.").arg(pad_id);
            entries << QString("{\"serialized_pad_fixture_found\":%1,\"pad_id\":%2}")
                           .arg(pad_id.isEmpty() ? "false" : "true",
                                jsonStringLocal(pad_id));
            ok = !pad_id.isEmpty() && ok;
          }
          bool schematic_metadata_serialized = !schematic_metadata_validation;
          bool schematic_metadata_retrieved = !schematic_metadata_validation;
          if (schematic_metadata_validation) {
            const QJsonObject project = QJsonDocument::fromJson(
                window->runAgentUiQueryJson("project.state", "{}").toUtf8())
                .object().value("result").toObject().value("project").toObject();
            bool found_sheet = false;
            for (const QJsonValue& value : project.value("sheets").toArray()) {
              const QJsonObject sheet = value.toObject();
              found_sheet = found_sheet ||
                  (sheet.value("id").toString() == "SHEET_SPRINT987" &&
                   sheet.value("name").toString() == "Power Stage" &&
                   sheet.value("file_path").toString() ==
                       "sheets/power_stage.kicad_sch");
            }
            bool found_property = false;
            for (const QJsonValue& value : project.value("components").toArray()) {
              const QJsonObject component = value.toObject();
              if (component.value("reference").toString() != "U3") continue;
              for (const QJsonValue& field_value : component.value("fields").toArray()) {
                const QJsonObject field = field_value.toObject();
                found_property = found_property ||
                    (field.value("name").toString() == "Manufacturer" &&
                     field.value("text").toString() == "ACME-42");
              }
            }
            schematic_metadata_serialized = found_sheet && found_property;
            entries << QString("{\"schematic_metadata_serialized\":%1,\"sheet_path\":%2,\"property_value\":%3}")
                           .arg(schematic_metadata_serialized ? "true" : "false",
                                jsonStringLocal(found_sheet ? "sheets/power_stage.kicad_sch" : ""),
                                jsonStringLocal(found_property ? "ACME-42" : ""));
            ok = schematic_metadata_serialized && ok;
          }
          ok = interact("ui.type_text",
                        QString("{\"id\":\"control:agent_chat_input\",\"text\":%1}")
                            .arg(jsonStringLocal(user_prompt)),
                        "control:agent_chat_input", "conversation-prompt-entered") && ok;
          ok = interact("ui.click", "{\"id\":\"action:agent_submit_chat\"}",
                        "action:agent_submit_chat", "conversation-turn-submitted") && ok;
          for (int attempt = 0; attempt < 60; ++attempt) {
            QApplication::processEvents();
            if (chat && (diagnostic_validation
                    ? chat->toPlainText().contains("is not configured")
                    : chat->toPlainText().contains(
                          "Provider execution is unavailable; configure a provider"))) break;
            QThread::msleep(100);
          }
          const bool memory_visible = !context_memory_validation ||
              (chat && chat->toPlainText().contains("Context package prepared") &&
               chat->toPlainText().contains("1 memories"));
          bool project_matches_visible = diagnostic_validation
              ? project_diagnostic_visible
              : !project_retrieval_validation ||
                    (chat && chat->toPlainText().contains("project matches") &&
                     !chat->toPlainText().contains("| 0 project matches"));
          if (schematic_metadata_validation && chat) {
            const QString transcript = chat->toPlainText();
            schematic_metadata_retrieved = transcript.contains("ACME-42") &&
                transcript.contains("sheets/power_stage.kicad_sch");
            project_matches_visible = transcript.contains("project matches") &&
                !transcript.contains("| 0 project matches");
            entries << QString("{\"schematic_metadata_retrieved\":%1}")
                           .arg(schematic_metadata_retrieved ? "true" : "false");
          }
          const bool schematic_pin_visible = !schematic_graph_validation ||
              (chat && chat->toPlainText().contains("schematic pins"));
          const bool schematic_symbol_visible = !schematic_graph_validation ||
              (chat && chat->toPlainText().contains("schematic symbols"));
          const bool pcb_layers_visible = !multilayer_project_validation ||
              (chat && chat->toPlainText().contains(" PCB layers"));
          bool board_net_visible = !board_net_validation;
          if (board_net_validation && chat) {
            for (int net_count = 1; net_count <= 32; ++net_count) {
              if (chat->toPlainText().contains(
                      QString("PCB nets: %1").arg(net_count))) {
                board_net_visible = true;
                break;
              }
            }
          }
          bool multiple_project_layers_visible = !serialized_pad_layer_validation;
          if (serialized_pad_layer_validation && chat) {
            for (int layer_count = 2; layer_count <= 32; ++layer_count) {
              if (chat->toPlainText().contains(
                      QString("%1 PCB layers").arg(layer_count))) {
                multiple_project_layers_visible = true;
                break;
              }
            }
          }
          bool project_diagnostic_count_visible = !diagnostic_validation;
          if (diagnostic_validation && chat) {
            for (int diagnostic_count = 1; diagnostic_count <= 256; ++diagnostic_count) {
              if (chat->toPlainText().contains(
                      QString("%1 DRC/ERC diagnostics").arg(diagnostic_count))) {
                project_diagnostic_count_visible = true;
                break;
              }
            }
          }
          const bool turn_visible = chat && memory_visible && project_matches_visible &&
              schematic_metadata_serialized &&
              schematic_metadata_retrieved &&
              schematic_pin_visible && schematic_symbol_visible && pcb_layers_visible &&
              multiple_project_layers_visible && board_net_visible && project_diagnostic_visible &&
              project_diagnostic_count_visible &&
              chat->toPlainText().contains(user_prompt) &&
              (diagnostic_validation
                   ? chat->toPlainText().contains("is not configured")
                   : chat->toPlainText().contains(
                         "Provider execution is unavailable; configure a provider"));
          entries << QString("{\"conversation_turn_visible\":%1,\"context_memory_attached\":%2,\"project_retrieval_visible\":%3,\"schematic_pin_retrieval_visible\":%4,\"schematic_symbol_retrieval_visible\":%5,\"project_layers_visible\":%6,\"multiple_project_layers_visible\":%7,\"board_net_count_visible\":%8,\"project_diagnostic_visible\":%9,\"project_diagnostic_count_visible\":%10,\"schematic_metadata_retrieved\":%11,\"provider_request_sent\":false}")
                         .arg(turn_visible ? "true" : "false",
                              memory_visible ? "true" : "false",
                              project_matches_visible ? "true" : "false",
                              schematic_pin_visible ? "true" : "false",
                              schematic_symbol_visible ? "true" : "false",
                              pcb_layers_visible ? "true" : "false",
                              multiple_project_layers_visible ? "true" : "false",
                              board_net_visible ? "true" : "false",
                              project_diagnostic_visible ? "true" : "false",
                              project_diagnostic_count_visible ? "true" : "false",
                              schematic_metadata_retrieved ? "true" : "false");
          ok = turn_visible && capture("turn-persisted") && ok;
          if (!diagnostic_validation) {
            ok = interact("ui.type_text",
                          "{\"id\":\"control:agent_chat_input\",\"text\":\"/clear\"}",
                          "control:agent_chat_input", "conversation-clear-entered") && ok;
            ok = interact("ui.click", "{\"id\":\"action:agent_submit_chat\"}",
                          "action:agent_submit_chat", "conversation-clear-submitted") && ok;
            QApplication::processEvents();
            const bool transcript_retained = chat && chat->toPlainText().contains(user_prompt);
            entries << QString("{\"canonical_transcript_retained_after_clear\":%1}")
                           .arg(transcript_retained ? "true" : "false");
            ok = transcript_retained && capture("projection-cleared") && ok;
          }
        } else if (name.startsWith("sprint975-memory-ui")) {
          const auto captureMemoryResult = [&]() {
            const QString path = QString::fromStdString(
                (output_dir / (name + "-memory-ui-" +
                    QString::number(entries.size()) + "-result.png").toStdString()).string());
            QScreen* screen = window->screen();
            if (!screen || !screen->grabWindow(0).save(path)) return false;
            entries << QString("{\"memory_result_screenshot\":%1}")
                           .arg(jsonStringLocal(path));
            return true;
          };
          ok = interact("ui.click", "{\"id\":\"tab:pcb\"}",
                        "tab:pcb", "memory-ui-pcb-tab") && ok;
          ok = interact("ui.click", "{\"id\":\"tab:schematic\"}",
                        "tab:schematic", "memory-ui-schematic-tab") && ok;
          ok = interact("ui.click", "{\"id\":\"tab:agent\"}",
                        "tab:agent", "memory-ui-agent-tab") && ok;
          ok = interact("ui.click", "{\"id\":\"action:settingsBtn\"}",
                        "action:settingsBtn", "memory-ui-settings-open") && ok;
          ok = interact("ui.click", "{\"id\":\"control:categoryList\",\"row\":2}",
                        "control:categoryList", "memory-ui-personalisation") && ok;
          ok = interact("ui.click", "{\"id\":\"action:agent_memory_manage\"}",
                        "action:agent_memory_manage", "memory-ui-manager-open") && ok;
          ok = interact("ui.click", "{\"id\":\"action:addMemory\"}",
                        "action:addMemory", "memory-ui-new-record") && ok;
          ok = interact("ui.click", "{\"id\":\"control:memoryTier\",\"value\":\"ltm\"}",
                        "control:memoryTier", "memory-ui-select-ltm") && ok;
          ok = interact("ui.type_text",
                        "{\"id\":\"control:memoryTitle\",\"text\":\"UI lifecycle record\"}",
                        "control:memoryTitle", "memory-ui-title") && ok;
          ok = interact("ui.type_text",
                        "{\"id\":\"control:memoryScope\",\"text\":\"conversation\"}",
                        "control:memoryScope", "memory-ui-scope") && ok;
          ok = interact("ui.click", "{\"id\":\"action:saveMemory\"}",
                        "action:saveMemory", "memory-ui-empty-save-rejected") && ok;
          const auto memoryStatus = []() -> QLabel* {
            for (QWidget* widget : QApplication::allWidgets()) {
              auto* label = qobject_cast<QLabel*>(widget);
              if (label && label->objectName() == "label:memoryManagerStatus" &&
                  label->isVisible()) return label;
            }
            return nullptr;
          };
          for (int attempt = 0; attempt < 40; ++attempt) {
            QThread::msleep(100);
            QApplication::processEvents();
            if (memoryStatus() && memoryStatus()->text().startsWith("Memory add failed:"))
              break;
          }
          const bool empty_rejected = memoryStatus() &&
              memoryStatus()->text().startsWith("Memory add failed:");
          entries << QString("{\"memory_empty_write_rejected\":%1}")
                         .arg(empty_rejected ? "true" : "false");
          ok = empty_rejected && ok;
          ok = captureMemoryResult() && ok;
          ok = interact("ui.type_text",
                        "{\"id\":\"control:memoryContent\",\"text\":\"Preserve the current ground return path around U3.\"}",
                        "control:memoryContent", "memory-ui-content") && ok;
          ok = interact("ui.click", "{\"id\":\"action:saveMemory\"}",
                        "action:saveMemory", "memory-ui-add-request") && ok;
          for (int attempt = 0; attempt < 40; ++attempt) {
            QThread::msleep(100);
            QApplication::processEvents();
            if (memoryStatus() && memoryStatus()->text() == "Memory added.") break;
          }
          const bool added = memoryStatus() && memoryStatus()->text() == "Memory added.";
          entries << QString("{\"memory_added\":%1}").arg(added ? "true" : "false");
          ok = added && ok;
          ok = captureMemoryResult() && ok;
          ok = interact("ui.click", "{\"id\":\"control:memoryEntries\",\"row\":0}",
                        "control:memoryEntries", "memory-ui-select-record") && ok;
          ok = interact("ui.type_text",
                        "{\"id\":\"control:memoryTitle\",\"text\":\"Updated UI lifecycle record\"}",
                        "control:memoryTitle", "memory-ui-update-title") && ok;
          ok = interact("ui.type_text",
                        "{\"id\":\"control:memoryContent\",\"text\":\"Preserve U3 ground return and maintain 0.25 mm clearance.\"}",
                        "control:memoryContent", "memory-ui-update-content") && ok;
          ok = interact("ui.click", "{\"id\":\"action:saveMemory\"}",
                        "action:saveMemory", "memory-ui-update-request") && ok;
          for (int attempt = 0; attempt < 40; ++attempt) {
            QThread::msleep(100);
            QApplication::processEvents();
            if (memoryStatus() && memoryStatus()->text() == "Memory updated.") break;
          }
          const bool updated = memoryStatus() && memoryStatus()->text() == "Memory updated.";
          entries << QString("{\"memory_updated\":%1}").arg(updated ? "true" : "false");
          ok = updated && ok;
          ok = captureMemoryResult() && ok;
          const auto* selected_memory = window->findChild<QListWidget*>("control:memoryEntries");
          const QString selected_memory_id = selected_memory && selected_memory->currentItem()
              ? selected_memory->currentItem()->data(Qt::UserRole).toString() : QString();
          const auto delete_confirmation = output_dir /
              (name + "-memory-ui-delete-confirmation.png").toStdString();
          auto confirmation_result = std::make_shared<int>(0);
          QTimer::singleShot(300, qApp, [delete_confirmation, confirmation_result]() {
            for (QWidget* top_level : QApplication::topLevelWidgets()) {
              auto* message = qobject_cast<QMessageBox*>(top_level);
              if (!message || !message->isVisible()) continue;
              message->grab().save(QString::fromStdString(delete_confirmation.string()));
              if (QAbstractButton* yes = message->button(QMessageBox::Yes)) {
                yes->click();
                if (message->isVisible()) message->done(QMessageBox::Yes);
              }
              *confirmation_result = message->result();
              return;
            }
          });
          ok = interact("ui.click", "{\"id\":\"action:deleteMemory\"}",
                        "action:deleteMemory", "memory-ui-delete-confirmed") && ok;
          for (int attempt = 0; attempt < 40; ++attempt) {
            QThread::msleep(100);
            QApplication::processEvents();
            if (memoryStatus() && memoryStatus()->text() == "Memory deleted.") break;
          }
          const bool deleted = memoryStatus() && memoryStatus()->text() == "Memory deleted.";
          ok = captureMemoryResult() && ok;
          entries << QString("{\"memory_deleted\":%1,\"memory_status\":%2,\"selected_memory_id\":%3,\"confirmation_result\":%4,\"confirmation_screenshot\":%5}")
                         .arg(deleted ? "true" : "false",
                              jsonStringLocal(memoryStatus() ? memoryStatus()->text() : QString("missing")),
                              jsonStringLocal(selected_memory_id),
                              QString::number(*confirmation_result),
                              jsonStringLocal(QString::fromStdString(delete_confirmation.string())));
          ok = deleted && ok;

          ok = interact("ui.click", "{\"id\":\"action:addMemory\"}",
                        "action:addMemory", "memory-ui-reset-fixture-new") && ok;
          ok = interact("ui.click", "{\"id\":\"control:memoryTier\",\"value\":\"ltm\"}",
                        "control:memoryTier", "memory-ui-reset-fixture-tier") && ok;
          ok = interact("ui.type_text",
                        "{\"id\":\"control:memoryTitle\",\"text\":\"Reset proof record\"}",
                        "control:memoryTitle", "memory-ui-reset-fixture-title") && ok;
          ok = interact("ui.type_text",
                        "{\"id\":\"control:memoryScope\",\"text\":\"conversation\"}",
                        "control:memoryScope", "memory-ui-reset-fixture-scope") && ok;
          ok = interact("ui.type_text",
                        "{\"id\":\"control:memoryContent\",\"text\":\"This record exists only to verify the confirmed reset flow.\"}",
                        "control:memoryContent", "memory-ui-reset-fixture-content") && ok;
          ok = interact("ui.click", "{\"id\":\"action:saveMemory\"}",
                        "action:saveMemory", "memory-ui-reset-fixture-save") && ok;
          for (int attempt = 0; attempt < 40; ++attempt) {
            QThread::msleep(100);
            QApplication::processEvents();
            if (memoryStatus() && memoryStatus()->text() == "Memory added.") break;
          }
          const bool reset_fixture_added =
              memoryStatus() && memoryStatus()->text() == "Memory added.";
          entries << QString("{\"memory_reset_fixture_added\":%1}")
                         .arg(reset_fixture_added ? "true" : "false");
          ok = reset_fixture_added && ok;
          ok = interact("ui.click", "{\"id\":\"action:closeMemoryManager\"}",
                        "action:closeMemoryManager", "memory-ui-manager-closed") && ok;
          const auto reset_confirmation_path = output_dir /
              (name + "-memory-ui-reset-confirmation.png").toStdString();
          auto reset_confirmation_result = std::make_shared<int>(0);
          QTimer::singleShot(300, qApp,
              [reset_confirmation_path, reset_confirmation_result]() {
            for (QWidget* top_level : QApplication::topLevelWidgets()) {
              auto* message = qobject_cast<QMessageBox*>(top_level);
              if (!message || !message->isVisible()) continue;
              message->grab().save(QString::fromStdString(
                  reset_confirmation_path.string()));
              if (QAbstractButton* yes = message->button(QMessageBox::Yes)) {
                yes->click();
                if (message->isVisible()) message->done(QMessageBox::Yes);
              }
              *reset_confirmation_result = message->result();
              return;
            }
          });
          ok = interact("ui.click", "{\"id\":\"action:agent_memory_reset\"}",
                        "action:agent_memory_reset", "memory-ui-reset-confirmed") && ok;
          auto* reset_status = window->findChild<QLabel*>(
              "label:memoryOperationStatus");
          for (int attempt = 0; attempt < 40; ++attempt) {
            QThread::msleep(100);
            QApplication::processEvents();
            if (reset_status && reset_status->text() ==
                "Memory reset complete: 1 record removed.") break;
          }
          const bool reset = reset_status && reset_status->text() ==
              "Memory reset complete: 1 record removed.";
          entries << QString("{\"memory_reset_complete\":%1,\"memory_reset_status\":%2,\"reset_confirmation_result\":%3,\"reset_confirmation_screenshot\":%4}")
                         .arg(reset ? "true" : "false",
                              jsonStringLocal(reset_status ? reset_status->text()
                                                          : QString("missing")),
                              QString::number(*reset_confirmation_result),
                              jsonStringLocal(QString::fromStdString(
                                  reset_confirmation_path.string())));
          ok = reset && ok;
          ok = captureMemoryResult() && ok;
          ok = interact("ui.click", "{\"id\":\"action:cancelSettingsButton\"}",
                        "action:cancelSettingsButton", "memory-ui-settings-closed") && ok;
        } else if (name.startsWith("sprint974-memory")) {
          ok = interact("ui.click", "{\"id\":\"tab:pcb\"}",
                        "tab:pcb", "pcb-tab-checked") && ok;
          ok = interact("ui.click", "{\"id\":\"tab:schematic\"}",
                        "tab:schematic", "schematic-tab-checked") && ok;
          ok = interact("ui.click", "{\"id\":\"tab:agent\"}",
                        "tab:agent", "agent-tab-opened") && ok;
          ok = interact("ui.type_text",
                        "{\"id\":\"control:agent_chat_input\",\"text\":\"/memory compact plan tier:ltm scope:conversation\"}",
                        "control:agent_chat_input", "compaction-plan-entered") && ok;
          ok = interact("ui.click", "{\"id\":\"action:agent_submit_chat\"}",
                        "action:agent_submit_chat", "compaction-plan-result") && ok;
          auto* chat = window->findChild<QTextBrowser*>("control:agent_chat_stream");
          const QString chat_text = chat ? chat->toPlainText() : QString{};
          const QRegularExpression plan_match("send:([0-9a-f]{32})");
          const auto match = plan_match.match(chat_text);
          const bool planned = chat_text.contains("Prepared compaction for") &&
                               match.hasMatch();
          entries << QString("{\"plan_created\":%1,\"provider_request_sent\":false}")
                         .arg(planned ? "true" : "false");
          ok = planned && ok;
          if (planned) {
            const QString cancel_command = "/memory compact cancel:" + match.captured(1);
            ok = interact("ui.type_text",
                          QString("{\"id\":\"control:agent_chat_input\",\"text\":%1}")
                              .arg(jsonStringLocal(cancel_command)),
                          "control:agent_chat_input", "compaction-cancel-entered") && ok;
            ok = interact("ui.click", "{\"id\":\"action:agent_submit_chat\"}",
                          "action:agent_submit_chat", "compaction-cancelled") && ok;
            const bool cancelled = chat && chat->toPlainText().contains(
                "Memory compaction cancelled; stored records are unchanged.");
            entries << QString("{\"plan_cancelled\":%1,\"persistent_change\":false}")
                           .arg(cancelled ? "true" : "false");
            ok = cancelled && ok;
          }
        } else {
          ok = interact("ui.click", "{\"id\":\"tab:agent\"}",
                        "tab:agent", "agent-tab-clicked") && ok;
          ok = interact("ui.type_text",
                        "{\"id\":\"control:agent_chat_input\",\"text\":\"/task\"}",
                        "control:agent_chat_input", "task-palette-opened") && ok;
          ok = interact("ui.key", "{\"key\":\"Enter\"}",
                        "panel:agent_slash_commands", "task-start-selected") && ok;
          ok = interact("ui.click", "{\"id\":\"action:agent_submit_chat\"}",
                        "action:agent_submit_chat", "task-started") && ok;
          ok = interact("ui.type_text",
                        "{\"id\":\"control:agent_chat_input\",\"text\":\"/task status\"}",
                        "control:agent_chat_input", "task-status-entered") && ok;
          ok = interact("ui.click", "{\"id\":\"action:agent_submit_chat\"}",
                        "action:agent_submit_chat", "task-status-checked") && ok;
          ok = interact("ui.type_text",
                        "{\"id\":\"control:agent_chat_input\",\"text\":\"/task end\"}",
                        "control:agent_chat_input", "task-end-entered") && ok;
          ok = interact("ui.click", "{\"id\":\"action:agent_submit_chat\"}",
                        "action:agent_submit_chat", "task-ended") && ok;
        }
        const std::filesystem::path output_path =
            output_dir / (name + "-target-sequence.json").toStdString();
        std::ofstream output(output_path, std::ios::binary);
        const QString interaction_plan = name.startsWith("sprint986-project-spatial-index")
            ? QStringLiteral("Run authoritative DRC on an isolated zero-length track, then ask the real Agent context builder for DRC markers inside an explicit PCB bounding box; verify the affected-object diagnostic count and provider-disabled result through mapped controls")
            : name.startsWith("sprint985-project-reference-graph")
            ? QStringLiteral("Verify exact live DRC/ERC diagnostics and affected-object identity through project.context; test typed graph retrieval separately with no-network context-broker contracts, then verify the GUI truthfully reports provider configuration failure across seven mapped actions")
            : name.startsWith("sprint984-board-net-retrieval")
            ? QStringLiteral("Ask the real Agent UI to inspect native PCB net AC1 and expose its board-net retrieval count in safe per-turn context metadata; complete seven mapped chat/editor actions with provider disabled")
            : name.startsWith("sprint983-project-index-typed-geometry")
            ? QStringLiteral("Retrieve a production-shaped nested padstack.layer_set through exact B.Cu context; inspect typed board state, complete seven mapped chat interactions, and prove multiple PCB layer IDs reached bounded context with provider disabled")
            : name.startsWith("sprint982-multilayer-project-context")
            ? QStringLiteral("Place one via on the disposable board through mapped toolbar and board-point interactions, then prove its F.Cu/B.Cu span survives exact project retrieval and bounded Agent context; provider disabled")
            : name.startsWith("sprint981-schematic-project-graph")
            ? QStringLiteral("Verify exact schematic net member pins and related symbols are counted in real turn context through seven mapped actions; provider disabled")
            : name.startsWith("sprint987-schematic-metadata")
            ? QStringLiteral("Load authoritative schematic fields and a relative sheet path, inspect them through project.state, then query both identifiers through real Agent context; provider disabled")
            : name.startsWith("sprint980-project-retrieval")
            ? QStringLiteral("Verify an actual typed-project retrieval match appears in per-turn Agent context and chat metadata via seven mapped actions; provider disabled")
            : name.startsWith("sprint977-context")
            ? QStringLiteral("Verify a real bounded thread-memory context is assembled and shown in chat via seven mapped actions; provider disabled")
            : name.startsWith("sprint976-conversation")
            ? QStringLiteral("Verify thread conversation persistence and /clear transcript preservation via seven mapped actions; provider disabled")
            : QStringLiteral("Mapped memory lifecycle with all actions logged and distinct visual checkpoints; no model request");
        const QString report =
            QString("{\"schema_version\":1,\"name\":%1,\"interaction_plan\":%2,"
                    "\"catalog_startup_verified\":%3,\"entries\":[%4]}\n")
                .arg(jsonStringLocal(name))
                .arg(jsonStringLocal(interaction_plan))
                .arg(catalog_startup_verified ? "true" : "false")
                .arg(entries.join(','));
        const QByteArray bytes = report.toUtf8();
        output.write(bytes.constData(), bytes.size());
        output.close();
        if (!output || !ok) {
          std::cerr << "scoped GUI-map interaction failed: "
                    << output_path.string() << '\n';
          std::cerr.flush();
          QCoreApplication::exit(2);
          return;
        }
        std::cout << "scoped GUI-map sequence saved: " << output_path.string() << '\n';
        std::cout.flush();
        QCoreApplication::exit(0);
        return;
      }
      const bool provider_target_sequence = name.startsWith("sprint972-provider");
      const bool memory_target_sequence = name.startsWith("sprint967-memory") ||
                                          name.startsWith("sprint971-memory");
      const bool semantic_memory_target_sequence = name.startsWith("sprint991-semantic-memory");
      const QStringList target_ids = provider_target_sequence
          ? QStringList{"action:settingsBtn", "control:categoryList",
                        "control:providerCombo", "control:modelCombo",
                        "control:categoryList", "control:apiKeyInput",
                        "label:providerTestTarget", "action:testProviderBtn",
                        "label:providerTestStatus", "action:cancelSettingsButton"}
          : memory_target_sequence
          ? QStringList{"action:settingsBtn", "control:categoryList",
                        "control:stmCb", "control:ltmCb",
                        "control:episodicCb", "label:memoryState",
                        "action:agent_memory_reset",
                        "action:agent_memory_manage", "control:memoryEntries",
                        "action:addMemory", "control:memoryTier",
                        "control:memoryTitle", "control:memoryScope",
                        "control:memoryContent", "action:saveMemory",
                        "action:closeMemoryManager", "action:cancelSettingsButton"}
          : semantic_memory_target_sequence
          ? QStringList{"action:settingsBtn", "control:categoryList",
                        "control:semanticMemoryEnabled", "control:semanticMemoryEndpoint",
                        "control:semanticMemoryModel", "action:primaryButton"}
          : QStringList{"action:cursor", "action:measurement", "action:save",
                                      "menu:file", "panel:properties", "action:grid",
                                      "action:polar_coord", "action:unit_inch",
                                      "action:cursor_shape", "action:show_ratsnest",
                                      "action:net_highlight", "action:contrast_mode",
                                      "tab:agent", "control:agent_chat_input",
                                      "action:agent_submit_chat", "action:settingsBtn",
                                      "control:stmCb", "control:ltmCb",
                                      "control:episodicCb", "label:memoryState",
                                      "action:agent_memory_manage", "control:memoryEntries",
                                      "control:memoryTier", "control:memoryContent",
                                      "action:closeMemoryManager",
                                      "control:providerCombo", "control:modelCombo",
                                      "control:apiKeyInput", "control:mcpServersTable",
                                      "action:addMcpServerBtn", "action:removeMcpServerBtn"};
      const QStringList trigger_before_capture_ids = memory_target_sequence ||
          semantic_memory_target_sequence || provider_target_sequence
          ? QStringList{"action:settingsBtn"}
          : QStringList{
          "action:grid",          "action:polar_coord",   "action:unit_inch",
          "action:cursor_shape",  "action:show_ratsnest", "action:net_highlight",
          "action:contrast_mode", "tab:agent", "action:settingsBtn"};
      const QStringList click_before_capture_ids = {"action:agent_footer_trigger_drc",
                                                    "action:agent_pin_evidence",
                                                    "control:categoryList",
                                                    "control:stmCb", "control:ltmCb",
                                                    "control:episodicCb",
                                                    "control:semanticMemoryEnabled",
                                                    "control:semanticMemoryEndpoint",
                                                    "control:semanticMemoryModel",
                                                    "action:agent_memory_reset",
                                                    "action:agent_memory_manage",
                                                    "action:addMemory", "action:saveMemory",
                                                    "control:memoryTier",
                                                    "control:memoryTitle", "control:memoryScope",
                                                    "control:memoryContent",
                                                    "action:closeMemoryManager",
                                                    "action:cancelSettingsButton",
                                                    "action:primaryButton",
                                                    "action:testProviderBtn"};
      bool memory_target_actions_ok = true;
      QJsonObject initial_memory_toggle_state;
      const auto visibleMemoryCheckbox = [](const QString& id) -> QCheckBox* {
        for (QWidget* widget : QApplication::allWidgets()) {
          auto* checkbox = qobject_cast<QCheckBox*>(widget);
          if (checkbox && checkbox->objectName() == id && checkbox->isVisible())
            return checkbox;
        }
        return nullptr;
      };
      const auto runPass = [window, &entries, &output_dir, &name, &target_ids,
                            memory_target_sequence,
                            semantic_memory_target_sequence,
                            provider_target_sequence,
                            &memory_target_actions_ok, &initial_memory_toggle_state,
                            &visibleMemoryCheckbox,
                            &trigger_before_capture_ids,
                            &click_before_capture_ids,
                            per_target_wait_ms](
                               const QString& pass_name) {
        for (const QString& id : target_ids) {
          if (trigger_before_capture_ids.contains(id)) {
            window->triggerSafeUiActionJson(id);
            QApplication::processEvents();
            // Settings opens through a queued callback so the modeless dialog
            // can finish constructing before its child controls are queried.
            if (id == "action:settingsBtn") {
              QThread::msleep(static_cast<unsigned long>(per_target_wait_ms));
              QApplication::processEvents();
            }
          }
          if (id == "control:providerCombo" || id == "control:modelCombo" ||
              id == "control:apiKeyInput" || id == "control:mcpServersTable" ||
              id == "action:addMcpServerBtn" || id == "action:removeMcpServerBtn" ||
              id == "control:stmCb" || id == "control:ltmCb" ||
              id == "control:episodicCb" || id == "label:memoryState" ||
              id == "control:semanticMemoryEnabled" ||
              id == "control:semanticMemoryEndpoint" ||
              id == "control:semanticMemoryModel" ||
              id == "label:semanticMemoryState" ||
              id == "action:agent_memory_manage" ||
              id == "action:agent_memory_reset" ||
              (provider_target_sequence && id == "action:testProviderBtn")) {
            const bool memory_control = id == "control:stmCb" || id == "control:ltmCb" ||
                id == "control:episodicCb" || id == "label:memoryState" ||
                id == "action:agent_memory_manage" ||
                id == "action:agent_memory_reset";
            const int category = provider_target_sequence &&
                                         (id == "control:providerCombo" || id == "control:modelCombo")
                                     ? 1
                                     : (provider_target_sequence &&
                                                (id == "control:apiKeyInput" ||
                                                 id == "action:testProviderBtn")
                                            ? 4
                                            : ((memory_control || semantic_memory_target_sequence) ? 2 : (id == "control:mcpServersTable" ||
                                         id == "action:addMcpServerBtn" ||
                                         id == "action:removeMcpServerBtn"
                                     ? 3
                                     : (id == "control:apiKeyInput" ? 4 : 1))));
            for (QWidget* top_level : QApplication::topLevelWidgets()) {
              auto* categories = top_level->findChild<QListWidget*>("control:categoryList");
              if (categories == nullptr || !top_level->isVisible()) {
                continue;
              }
              categories->setCurrentRow(category);
              QApplication::processEvents();
              QThread::msleep(static_cast<unsigned long>(per_target_wait_ms));
              QApplication::processEvents();
              break;
            }
          }
          if (click_before_capture_ids.contains(id)) {
            if (memory_target_sequence && pass_name == "resized" &&
                (id == "action:addMemory" || id == "action:saveMemory")) {
              // The first pass created a real LTM record in the isolated test profile;
              // the second pass only observes state after toggling the tier off.
            } else {
            if (memory_target_sequence && pass_name == "initial" &&
                (id == "control:stmCb" || id == "control:ltmCb" ||
                 id == "control:episodicCb")) {
              if (QCheckBox* checkbox = visibleMemoryCheckbox(id))
                initial_memory_toggle_state.insert(id, checkbox->isChecked());
            }
            if (id == "action:agent_memory_reset") {
              const auto confirm_path = output_dir /
                  (name + "-" + pass_name + "-reset-confirmation.png").toStdString();
              QTimer::singleShot(300, qApp, [confirm_path]() {
                for (QWidget* top_level : QApplication::topLevelWidgets()) {
                  auto* message = qobject_cast<QMessageBox*>(top_level);
                  if (message == nullptr || !message->isVisible()) continue;
                  message->grab().save(QString::fromStdString(confirm_path.string()));
                  if (QAbstractButton* no = message->button(QMessageBox::No)) no->click();
                  break;
                }
              });
            }
            static int provider_category_click = 0;
            const int category_row = provider_target_sequence
                ? ((provider_category_click++ % 2) == 0 ? 1 : 4) : 2;
            const QString payload = id == "control:categoryList"
                ? QString("{\"id\":%1,\"row\":%2}").arg(jsonStringLocal(id)).arg(category_row)
                : (id == "control:memoryTier"
                    ? QString("{\"id\":%1,\"value\":\"ltm\"}").arg(jsonStringLocal(id))
                    : QString("{\"id\":%1}").arg(jsonStringLocal(id)));
            const QString click_result = window->runAgentUiQueryJson("ui.click", payload);
            if (memory_target_sequence || provider_target_sequence) {
              const QJsonDocument click_doc = QJsonDocument::fromJson(click_result.toUtf8());
              const bool performed = click_doc.isObject() &&
                  click_doc.object().value("ok").toBool() &&
                  click_doc.object().value("result").toObject()
                      .value("performed").toBool();
              memory_target_actions_ok = memory_target_actions_ok && performed;
            }
            entries << QString("{\"pass\":%1,\"id\":%2,\"interaction\":\"ui.click\",\"result\":%3}")
                           .arg(jsonStringLocal(pass_name), jsonStringLocal(id), click_result.trimmed());
            QApplication::processEvents();
            if (provider_target_sequence && id == "action:testProviderBtn") {
              QString status;
              for (int attempt = 0; attempt < 50; ++attempt) {
                QThread::msleep(100);
                QApplication::processEvents();
                status = window->uiTargetJsonById("label:providerTestStatus");
                if (status.contains("Provider validation: ready (network not probed)"))
                  break;
              }
              const bool ready = status.contains("Provider validation: ready (network not probed)");
              memory_target_actions_ok = memory_target_actions_ok && ready;
              entries << QString("{\"provider_local_validation_ready\":%1,\"status\":%2}")
                             .arg(ready ? "true" : "false", status);
            }
            if (id == "action:agent_memory_manage") {
              QThread::msleep(static_cast<unsigned long>(per_target_wait_ms));
              QApplication::processEvents();
            }
            }
          }
          const QString target_json = window->uiTargetJsonById(id);
          const bool found = target_json.contains("\"found\":true");
          if (provider_target_sequence && id != "action:cancelSettingsButton" && !found)
            memory_target_actions_ok = false;
          const std::optional<int> x = extractJsonInt(target_json, "\"logical_x\":");
          const std::optional<int> y = extractJsonInt(target_json, "\"logical_y\":");
          QString screenshot_path;
          if (found && x.has_value() && y.has_value() &&
              (!semantic_memory_target_sequence || id == "action:settingsBtn" ||
               id == "action:primaryButton" || id == "action:cancelSettingsButton")) {
            QCursor::setPos(*x, *y);
            QApplication::processEvents();
            QThread::msleep(static_cast<unsigned long>(per_target_wait_ms));
            QApplication::processEvents();
            const QString safe_id = id;
            QString slug = safe_id;
            slug.replace(':', '_');
            const std::filesystem::path path =
                output_dir / (name + "-" + pass_name + "-" + slug + ".png").toStdString();
            screenshot_path = QString::fromStdString(path.string());
            QWidget* capture_window = window;
            for (QWidget* top_level : QApplication::topLevelWidgets()) {
              if (top_level == window || !top_level->isVisible()) {
                continue;
              }
              if (top_level->frameGeometry().contains(QPoint(*x, *y))) {
                capture_window = top_level;
                break;
              }
            }
            if (semantic_memory_target_sequence && id == "action:settingsBtn") {
              QWidget* active = QApplication::activeWindow();
              if (active && active->isVisible()) capture_window = active;
            }
            QPixmap screenshot = capture_window->grab();
            QPainter painter(&screenshot);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setPen(QPen(QColor("#ff00cc"), 3));
            const QPoint local_target = capture_window->mapFromGlobal(QPoint(*x, *y));
            painter.drawEllipse(local_target, 10, 10);
            painter.drawLine(local_target.x() - 16, local_target.y(), local_target.x() + 16,
                             local_target.y());
            painter.drawLine(local_target.x(), local_target.y() - 16, local_target.x(),
                             local_target.y() + 16);
            painter.end();
            screenshot.save(screenshot_path);
          } else if ((click_before_capture_ids.contains(id) &&
                      !(semantic_memory_target_sequence &&
                        (id == "control:categoryList" ||
                         id == "action:primaryButton" ||
                         id == "control:semanticMemoryEndpoint" ||
                         id == "control:semanticMemoryModel"))) ||
                     (semantic_memory_target_sequence && id == "label:semanticMemoryState")) {
            QWidget* active = QApplication::activeWindow();
            if (active && active->isVisible()) {
              const std::filesystem::path path = output_dir /
                  (name + "-" + pass_name + "-" + id.mid(id.indexOf(':') + 1) + "-after.png").toStdString();
              screenshot_path = QString::fromStdString(path.string());
              active->grab().save(screenshot_path);
            }
          }
          if ((id == "control:memoryContent" || id == "control:memoryTitle" ||
               id == "control:memoryScope") && found &&
              (!memory_target_sequence || pass_name == "initial")) {
            const QString value = id == "control:memoryContent"
                ? "Sprint 971 UI-map proof record; safe to delete"
                : (id == "control:memoryTitle" ? "UI-map verification" : "conversation");
            const QString typing = QString("{\"id\":%1,\"text\":%2}")
                .arg(jsonStringLocal(id), jsonStringLocal(value));
            const QString typing_result = window->runAgentUiQueryJson("ui.type_text", typing);
            if (memory_target_sequence) {
              const QJsonDocument typing_doc = QJsonDocument::fromJson(typing_result.toUtf8());
              const bool performed = typing_doc.isObject() &&
                  typing_doc.object().value("ok").toBool() &&
                  typing_doc.object().value("result").toObject()
                      .value("performed").toBool();
              memory_target_actions_ok = memory_target_actions_ok && performed;
            }
            QApplication::processEvents();
            QThread::msleep(static_cast<unsigned long>(per_target_wait_ms));
            QApplication::processEvents();
            QString typed_slug = id.mid(id.indexOf(':') + 1);
            const std::filesystem::path typed_path = output_dir /
                (name + "-" + pass_name + "-" + typed_slug + "-typed.png").toStdString();
            QWidget* active = QApplication::activeWindow();
            if (active && active->isVisible()) active->grab().save(QString::fromStdString(typed_path.string()));
            entries << QString("{\"pass\":%1,\"id\":%2,\"interaction\":\"ui.type_text\",\"result\":%3,\"screenshot\":%4}")
                           .arg(jsonStringLocal(pass_name), jsonStringLocal(id), typing_result.trimmed(),
                                jsonStringLocal(QString::fromStdString(typed_path.string())));
          }
          if ((id == "control:semanticMemoryEndpoint" ||
               id == "control:semanticMemoryModel") && found &&
              semantic_memory_target_sequence && pass_name == "initial") {
            const QString value = id == "control:semanticMemoryEndpoint"
                ? "http://127.0.0.1:11434" : "embeddinggemma";
            const QString typing = QString("{\"id\":%1,\"text\":%2}")
                .arg(jsonStringLocal(id), jsonStringLocal(value));
            const QString typing_result = window->runAgentUiQueryJson("ui.type_text", typing);
            const QJsonDocument typing_doc = QJsonDocument::fromJson(typing_result.toUtf8());
            const bool performed = typing_doc.isObject() &&
                typing_doc.object().value("ok").toBool() &&
                typing_doc.object().value("result").toObject().value("performed").toBool();
            memory_target_actions_ok = memory_target_actions_ok && performed;
            QApplication::processEvents();
            entries << QString("{\"pass\":%1,\"id\":%2,\"interaction\":\"ui.type_text\",\"result\":%3}")
                .arg(jsonStringLocal(pass_name), jsonStringLocal(id), typing_result.trimmed());
          }
          entries << QString("{\"pass\":%1,\"id\":%2,\"found\":%3,\"target\":%4,"
                             "\"screenshot\":%5}")
                         .arg(jsonStringLocal(pass_name))
                         .arg(jsonStringLocal(id))
                         .arg(found ? "true" : "false")
                         .arg(found ? target_json.mid(target_json.indexOf("\"target\":") + 9)
                                           .section('}', 0, 0) + "}"
                                    : "null")
                         .arg(jsonStringLocal(screenshot_path));
        }
      };

      runPass("initial");
      // window->resize(1120, 720); // Removed because fullscreen resize crashes Qt on Windows
      QApplication::processEvents();
      QThread::msleep(static_cast<unsigned long>(per_target_wait_ms));
      if (!semantic_memory_target_sequence) runPass("resized");
      if (memory_target_sequence) {
        const QString reopen_result = window->runAgentUiQueryJson(
            "ui.click", "{\"id\":\"action:settingsBtn\"}");
        QApplication::processEvents();
        QThread::msleep(static_cast<unsigned long>(per_target_wait_ms));
        QApplication::processEvents();
        const auto reloaded_settings_path = output_dir /
            (name + "-preferences-reloaded.png").toStdString();
        for (QWidget* top_level : QApplication::topLevelWidgets()) {
          if (top_level != window && top_level->isVisible()) {
            top_level->grab().save(
                QString::fromStdString(reloaded_settings_path.string()));
            break;
          }
        }
        const QJsonDocument reopen_doc = QJsonDocument::fromJson(reopen_result.toUtf8());
        memory_target_actions_ok = memory_target_actions_ok && reopen_doc.isObject() &&
            reopen_doc.object().value("ok").toBool() &&
            reopen_doc.object().value("result").toObject().value("performed").toBool();
        entries << QString("{\"preference_reopen\":%1,\"screenshot\":%2}")
                       .arg(memory_target_actions_ok ? "true" : "false",
                            jsonStringLocal(QString::fromStdString(
                                reloaded_settings_path.string())));
        const QString memory_page_result = window->runAgentUiQueryJson(
            "ui.click", "{\"id\":\"control:categoryList\",\"row\":2}");
        QApplication::processEvents();
        QThread::msleep(static_cast<unsigned long>(per_target_wait_ms));
        QApplication::processEvents();
        const QJsonDocument memory_page_doc =
            QJsonDocument::fromJson(memory_page_result.toUtf8());
        const bool memory_page_selected = memory_page_doc.isObject() &&
            memory_page_doc.object().value("ok").toBool() &&
            memory_page_doc.object().value("result").toObject()
                .value("performed").toBool();
        memory_target_actions_ok = memory_target_actions_ok && memory_page_selected;
        const auto memory_page_path = output_dir /
            (name + "-preferences-memory-page.png").toStdString();
        for (QWidget* top_level : QApplication::topLevelWidgets()) {
          if (top_level != window && top_level->isVisible()) {
            top_level->grab().save(QString::fromStdString(memory_page_path.string()));
            break;
          }
        }
        entries << QString("{\"memory_page_selected\":%1,\"screenshot\":%2}")
                       .arg(memory_page_selected ? "true" : "false",
                            jsonStringLocal(QString::fromStdString(
                                memory_page_path.string())));
        for (auto it = initial_memory_toggle_state.constBegin();
             it != initial_memory_toggle_state.constEnd(); ++it) {
          const QCheckBox* checkbox = visibleMemoryCheckbox(it.key());
          const bool matches = checkbox != nullptr &&
                               checkbox->isChecked() == it.value().toBool();
          memory_target_actions_ok = memory_target_actions_ok && matches;
          entries << QString("{\"toggle\":%1,\"initial\":%2,\"final\":%3,\"matches\":%4}")
                         .arg(jsonStringLocal(it.key()))
                         .arg(it.value().toBool() ? "true" : "false")
                         .arg(checkbox && checkbox->isChecked() ? "true" : "false")
                         .arg(matches ? "true" : "false");
        }
        const QString close_result = window->runAgentUiQueryJson(
            "ui.click", "{\"id\":\"action:cancelSettingsButton\"}");
        QApplication::processEvents();
        const auto closed_settings_path = output_dir /
            (name + "-preferences-reloaded-closed.png").toStdString();
        window->grab().save(QString::fromStdString(closed_settings_path.string()));
        const QJsonDocument close_doc = QJsonDocument::fromJson(close_result.toUtf8());
        memory_target_actions_ok = memory_target_actions_ok && close_doc.isObject() &&
            close_doc.object().value("ok").toBool() &&
            close_doc.object().value("result").toObject().value("performed").toBool();
        entries << QString("{\"preferences_dialog_closed\":%1,\"screenshot\":%2}")
                       .arg(memory_target_actions_ok ? "true" : "false",
                            jsonStringLocal(QString::fromStdString(
                                closed_settings_path.string())));
        entries << QString("{\"memory_actions_ok\":%1}")
                       .arg(memory_target_actions_ok ? "true" : "false");
      }
      if (semantic_memory_target_sequence) {
        const QString reopen_result = window->runAgentUiQueryJson(
            "ui.click", "{\"id\":\"action:settingsBtn\"}");
        QApplication::processEvents();
        QThread::msleep(static_cast<unsigned long>(per_target_wait_ms));
        QApplication::processEvents();
        auto recordPerformed = [&entries, &memory_target_actions_ok](
                                   const QString& name, const QString& result) {
          const QJsonDocument document = QJsonDocument::fromJson(result.toUtf8());
          const bool performed = document.isObject() &&
              document.object().value("ok").toBool() &&
              document.object().value("result").toObject().value("performed").toBool();
          memory_target_actions_ok = memory_target_actions_ok && performed;
          entries << QString("{\"interaction\":%1,\"result\":%2}")
              .arg(jsonStringLocal(name), result.trimmed());
          return performed;
        };
        recordPerformed("ui.click:action:settingsBtn", reopen_result);
        const QString category_result = window->runAgentUiQueryJson(
            "ui.click", "{\"id\":\"control:categoryList\",\"row\":2}");
        QApplication::processEvents();
        QThread::msleep(static_cast<unsigned long>(per_target_wait_ms));
        QApplication::processEvents();
        recordPerformed("ui.click:control:categoryList:row2", category_result);
        QString status_target;
        for (int attempt = 0; attempt < 30; ++attempt) {
          status_target = window->uiTargetJsonById("label:semanticMemoryState");
          if (status_target.contains("service_unavailable") ||
              status_target.contains("model_not_installed") ||
              status_target.contains("\"label\":\"Semantic retrieval: ready"))
            break;
          QThread::msleep(100);
          QApplication::processEvents();
        }
        const bool status_found = status_target.contains("\"found\":true") &&
            (status_target.contains("service_unavailable") ||
             status_target.contains("model_not_installed") ||
             status_target.contains("\"label\":\"Semantic retrieval: ready"));
        memory_target_actions_ok = memory_target_actions_ok && status_found;
        QString status_screenshot;
        for (QWidget* top_level : QApplication::topLevelWidgets()) {
          if (top_level != window && top_level->isVisible()) {
            const auto status_path = output_dir /
                (name + "-runtime-status.png").toStdString();
            status_screenshot = QString::fromStdString(status_path.string());
            top_level->grab().save(status_screenshot);
            break;
          }
        }
        entries << QString("{\"semantic_runtime_status_found\":%1,\"target\":%2,\"screenshot\":%3}")
            .arg(status_found ? "true" : "false", status_target,
                 jsonStringLocal(status_screenshot));
        const QString close_result = window->runAgentUiQueryJson(
            "ui.click", "{\"id\":\"action:cancelSettingsButton\"}");
        QApplication::processEvents();
        QThread::msleep(static_cast<unsigned long>(per_target_wait_ms));
        QApplication::processEvents();
        recordPerformed("ui.click:action:cancelSettingsButton", close_result);
        const auto final_path = output_dir /
            (name + "-final.png").toStdString();
        window->grab().save(QString::fromStdString(final_path.string()));
        entries << QString("{\"final_screenshot\":%1}")
            .arg(jsonStringLocal(QString::fromStdString(final_path.string())));
      }

      const std::filesystem::path output_path =
          output_dir / (name + "-target-sequence.json").toStdString();
      std::ofstream output(output_path, std::ios::binary);
      const QString report =
          QString("{\"schema_version\":1,\"name\":%1,\"initial_wait_ms\":%2,"
                 "\"per_target_wait_ms\":%3,\"catalog_startup_verified\":%4,"
                 "\"catalog_startup\":%5,\"entries\":[%6]}\n")
              .arg(jsonStringLocal(name))
              .arg(initial_wait_ms)
              .arg(per_target_wait_ms)
              .arg(catalog_startup_verified ? "true" : "false")
              .arg(jsonStringLocal(catalog_startup))
              .arg(entries.join(','));
      const QByteArray bytes = report.toUtf8();
      output.write(bytes.constData(), bytes.size());
      if (!output || ((memory_target_sequence || provider_target_sequence ||
                       semantic_memory_target_sequence) &&
                      !memory_target_actions_ok)) {
        std::cerr << "GUI-map target sequence failed; report: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      std::cout << "ui map target sequence saved: " << output_path.string() << '\n';
      std::cout.flush();
      QCoreApplication::exit(0);
    });

    return QApplication::exec();
  } else if (argc == 7 && std::string(argv[1]) == "--test-place-footprint-click") {
    const std::filesystem::path project_path(argv[2]);
    const std::filesystem::path footprint_path(argv[3]);
    const double x_mm = std::stod(argv[4]);
    const double y_mm = std::stod(argv[5]);
    const std::filesystem::path output_path(argv[6]);
    auto* window = new ReviewWindow();
    window->setAutomationMode(true);

    window->loadProjectPath(project_path);
    window->show();

    QTimer::singleShot(500, window, [window, footprint_path, x_mm, y_mm, output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open placement output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes =
          window->commitFootprintPlacementForAutomation(footprint_path, x_mm, y_mm).toUtf8();
      output.write(bytes.constData(), bytes.size());
      if (!output) {
        std::cerr << "failed to write placement output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      std::cout << "placement result saved: " << output_path.string() << '\n';
      std::cout.flush();
      QCoreApplication::exit(0);
    });

    return QApplication::exec();
  } else if (argc == 6 && std::string(argv[1]) == "--test-place-via-click") {
    const std::filesystem::path project_path(argv[2]);
    const double x_mm = std::stod(argv[3]);
    const double y_mm = std::stod(argv[4]);
    const std::filesystem::path output_path(argv[5]);
    auto* window = new ReviewWindow();
    window->setAutomationMode(true);

    window->loadProjectPath(project_path);
    window->show();

    QTimer::singleShot(500, window, [window, x_mm, y_mm, output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open via placement output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window->commitViaPlacementForAutomation(x_mm, y_mm).toUtf8();
      output.write(bytes.constData(), bytes.size());
      if (!output) {
        std::cerr << "failed to write via placement output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      std::cout << "via placement result saved: " << output_path.string() << '\n';
      std::cout.flush();
      QCoreApplication::exit(0);
    });

    return QApplication::exec();
  } else if (argc == 8 && std::string(argv[1]) == "--test-route-track-click") {
    const std::filesystem::path project_path(argv[2]);
    const double start_x_mm = std::stod(argv[3]);
    const double start_y_mm = std::stod(argv[4]);
    const double end_x_mm = std::stod(argv[5]);
    const double end_y_mm = std::stod(argv[6]);
    const std::filesystem::path output_path(argv[7]);
    auto* window = new ReviewWindow();
    window->setAutomationMode(true);

    window->loadProjectPath(project_path);
    window->show();

    QTimer::singleShot(500, window, [window, start_x_mm, start_y_mm, end_x_mm, end_y_mm,
                                      output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open track placement output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window->commitTrackPlacementForAutomation(start_x_mm, start_y_mm,
                                                                      end_x_mm, end_y_mm)
                                   .toUtf8();
      output.write(bytes.constData(), bytes.size());
      if (!output) {
        std::cerr << "failed to write track placement output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      std::cout << "track placement result saved: " << output_path.string() << '\n';
      std::cout.flush();
      QCoreApplication::exit(0);
    });

    return QApplication::exec();
  } else if (argc == 8 && std::string(argv[1]) == "--test-place-zone-click") {
    const std::filesystem::path project_path(argv[2]);
    const double start_x_mm = std::stod(argv[3]);
    const double start_y_mm = std::stod(argv[4]);
    const double end_x_mm = std::stod(argv[5]);
    const double end_y_mm = std::stod(argv[6]);
    const std::filesystem::path output_path(argv[7]);
    auto* window = new ReviewWindow();
    window->setAutomationMode(true);

    window->loadProjectPath(project_path);
    window->show();

    QTimer::singleShot(500, window, [window, start_x_mm, start_y_mm, end_x_mm, end_y_mm,
                                      output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open zone placement output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window->commitZonePlacementForAutomation(start_x_mm, start_y_mm,
                                                                     end_x_mm, end_y_mm)
                                   .toUtf8();
      output.write(bytes.constData(), bytes.size());
      if (!output) {
        std::cerr << "failed to write zone placement output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      std::cout << "zone placement result saved: " << output_path.string() << '\n';
      std::cout.flush();
      QCoreApplication::exit(0);
    });

    return QApplication::exec();
  } else if (argc == 8 && std::string(argv[1]) == "--test-place-keepout-click") {
    const std::filesystem::path project_path(argv[2]);
    const double start_x_mm = std::stod(argv[3]);
    const double start_y_mm = std::stod(argv[4]);
    const double end_x_mm = std::stod(argv[5]);
    const double end_y_mm = std::stod(argv[6]);
    const std::filesystem::path output_path(argv[7]);
    auto* window = new ReviewWindow();
    window->setAutomationMode(true);

    window->loadProjectPath(project_path);
    window->show();

    QTimer::singleShot(500, window, [window, start_x_mm, start_y_mm, end_x_mm, end_y_mm,
                                      output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open keepout placement output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window->commitKeepoutPlacementForAutomation(start_x_mm, start_y_mm,
                                                                        end_x_mm, end_y_mm)
                                   .toUtf8();
      output.write(bytes.constData(), bytes.size());
      if (!output) {
        std::cerr << "failed to write keepout placement output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      std::cout << "keepout placement result saved: " << output_path.string() << '\n';
      std::cout.flush();
      QCoreApplication::exit(0);
    });

    return QApplication::exec();
  } else if (argc == 5 && std::string(argv[1]) == "--test-delete-board-object") {
    const std::filesystem::path project_path(argv[2]);
    const QString object_id = QString::fromLocal8Bit(argv[3]);
    const std::filesystem::path output_path(argv[4]);
    auto* window = new ReviewWindow();
    window->setAutomationMode(true);

    window->loadProjectPath(project_path);
    window->show();

    QTimer::singleShot(500, window, [window, object_id, output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open delete output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window->deleteBoardObjectForAutomation(object_id).toUtf8();
      output.write(bytes.constData(), bytes.size());
      if (!output) {
        std::cerr << "failed to write delete output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      std::cout << "delete result saved: " << output_path.string() << '\n';
      std::cout.flush();
      QCoreApplication::exit(0);
    });

    return QApplication::exec();
  } else if (argc == 4 && std::string(argv[1]) == "--screenshot") {
    const std::filesystem::path project_path(argv[2]);
    const char* screenshot_arg = argv[3];
    const QString screenshot_path = QString::fromLocal8Bit(screenshot_arg);
    auto* window = new ReviewWindow();
    window->setAutomationMode(true);

    window->loadProjectPath(project_path);
    window->show();

    QTimer::singleShot(kSingleScreenshotWaitMs, window,
                       [window, screenshot_path, screenshot_arg]() {
      const QPixmap screenshot = window->grab();
      if (!screenshot.save(screenshot_path)) {
        std::cerr << "failed to save GUI screenshot: " << screenshot_arg << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
      } else {
        std::cout << "screenshot saved: " << screenshot_arg << '\n';
        std::cout.flush();
        QCoreApplication::exit(0);
      }
    });

    return QApplication::exec();
  } else if (argc == 4 && std::string(argv[1]) == "--screenshot-schematic") {
    const std::filesystem::path project_path(argv[2]);
    const char* screenshot_arg = argv[3];
    const QString screenshot_path = QString::fromLocal8Bit(screenshot_arg);
    auto* window = new ReviewWindow();
    window->setAutomationMode(true);

    window->loadProjectPath(project_path);
    window->show();

    QTimer::singleShot(500, window, [window, screenshot_path, screenshot_arg]() {
      if (auto* tabs = window->findChild<QTabWidget*>("editorTabs")) {
        tabs->setCurrentIndex(1); // switch to schematic tab
      }
      QTimer::singleShot(kSingleScreenshotWaitMs - 500, window, [window, screenshot_path, screenshot_arg]() {
        const QPixmap screenshot = window->grab();
        if (!screenshot.save(screenshot_path)) {
          std::cerr << "failed to save GUI screenshot: " << screenshot_arg << '\n';
          std::cerr.flush();
          QCoreApplication::exit(2);
        } else {
          std::cout << "screenshot saved: " << screenshot_arg << '\n';
          std::cout.flush();
          QCoreApplication::exit(0);
        }
      });
    });

    return QApplication::exec();
  } else if (argc == 4 && (std::string(argv[1]) == "--screenshot-footprint" || std::string(argv[1]) == "--screenshot-symbol")) {
    const std::string mode = argv[1];
    const std::filesystem::path target_path(argv[2]);
    const char* screenshot_arg = argv[3];
    const QString screenshot_path = QString::fromLocal8Bit(screenshot_arg);
    
    auto* window = new ReviewWindow();
    window->setAutomationMode(true);

    if (mode == "--screenshot-footprint") {
      window->loadFootprintPreview(target_path);
    } else {
      window->loadSymbolPreview(target_path);
    }
    window->show();
    
    QTimer::singleShot(kSingleScreenshotWaitMs, window,
                       [window, screenshot_path, screenshot_arg]() {
      const QPixmap screenshot = window->grab();
      if (!screenshot.save(screenshot_path)) {
        std::cerr << "failed to save GUI screenshot: " << screenshot_arg << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
      } else {
        std::cout << "screenshot saved: " << screenshot_arg << '\n';
        std::cout.flush();
        QCoreApplication::exit(0);
      }
    });

    return QApplication::exec();
  } else if (argc == 4 && (std::string(argv[1]) == "--screenshot-chooser-footprint" ||
                           std::string(argv[1]) == "--screenshot-chooser-symbol")) {
    const LibraryType type = std::string(argv[1]) == "--screenshot-chooser-footprint"
                                 ? LibraryType::Footprint
                                 : LibraryType::Symbol;
    const QString cache_root = QString::fromLocal8Bit(argv[2]);
    const char* screenshot_arg = argv[3];
    const QString screenshot_path = QString::fromLocal8Bit(screenshot_arg);
    auto* dialog = new LibraryBrowserDialog(type, cache_root);
    dialog->show();
    QTimer::singleShot(250, dialog, [dialog]() {
      if (!dialog->selectFirstVisibleItemForTest()) {
        std::cerr << "chooser had no visible items to select\n";
        std::cerr.flush();
        QCoreApplication::exit(3);
      }
    });
    QTimer::singleShot(kSingleScreenshotWaitMs, dialog, [dialog, screenshot_path, screenshot_arg]() {
      QCoreApplication::exit(screenshotWindow(*dialog, screenshot_path, screenshot_arg));
    });

    return QApplication::exec();
  } else if (argc == 4 && std::string(argv[1]) == "--screenshot-project") {
    const std::filesystem::path project_path(argv[2]);
    const char* screenshot_arg = argv[3];
    const QString screenshot_path = QString::fromLocal8Bit(screenshot_arg);
    auto* window = new ReviewWindow();
    window->setAutomationMode(true);

    window->loadProjectPath(project_path);
    window->show();

    QTimer::singleShot(kSingleScreenshotWaitMs, window,
                       [window, screenshot_path, screenshot_arg]() {
      const QPixmap screenshot = window->grab();
      if (!screenshot.save(screenshot_path)) {
        std::cerr << "failed to save GUI screenshot: " << screenshot_arg << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
      } else {
        std::cout << "screenshot saved: " << screenshot_arg << '\n';
        std::cout.flush();
        QCoreApplication::exit(0);
      }
    });

    return QApplication::exec();
  } else if (argc == 4 && std::string(argv[1]) == "--screenshot-measure") {
    const std::filesystem::path project_path(argv[2]);
    const char* screenshot_arg = argv[3];
    const QString screenshot_path = QString::fromLocal8Bit(screenshot_arg);
    auto* window = new ReviewWindow();
    window->setAutomationMode(true);

    window->loadProjectPath(project_path);
    window->show();

    QTimer::singleShot(500, window, [window]() {
      // Simulate switching to measure tool
      if (auto* tabs = window->findChild<QTabWidget*>("editorTabs")) {
        if (auto* view = dynamic_cast<BoardCanvasView*>(tabs->currentWidget())) {
          view->setToolMode(ToolMode::Measure);
          // Simulate drag from (100,100) to (400,300) in viewport
          QMouseEvent press(QEvent::MouseButtonPress, QPointF(100, 100), QPointF(100, 100), Qt::LeftButton, 
Qt::LeftButton, Qt::NoModifier);
          QApplication::sendEvent(view->viewport(), &press);
          QMouseEvent move(QEvent::MouseMove, QPointF(400, 300), QPointF(400, 300), Qt::NoButton, Qt::LeftButton, 
Qt::NoModifier);
          QApplication::sendEvent(view->viewport(), &move);

          // Also simulate a double click to open the SelectionInspector properties on the track arc at (24, 12) mm
          QTimer::singleShot(200, window, [view]() {
              view->setToolMode(ToolMode::Select);
              QPoint viewport_pt = view->mapFromScene(QPointF(24.0 * 1e6, 12.0 * 1e6));
              QMouseEvent click(QEvent::MouseButtonPress, QPointF(viewport_pt), QPointF(viewport_pt), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
              QApplication::sendEvent(view->viewport(), &click);
              QMouseEvent release(QEvent::MouseButtonRelease, QPointF(viewport_pt), QPointF(viewport_pt), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
              QApplication::sendEvent(view->viewport(), &release);
              QMouseEvent dclick(QEvent::MouseButtonDblClick, QPointF(viewport_pt), QPointF(viewport_pt), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
              QApplication::sendEvent(view->viewport(), &dclick);
          });
        }
      }
    });

    QTimer::singleShot(kSingleScreenshotWaitMs, window,
                       [window, screenshot_path, screenshot_arg]() {
      const QPixmap screenshot = window->grab();
      if (!screenshot.save(screenshot_path)) {
        std::cerr << "failed to save GUI screenshot: " << screenshot_arg << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
      } else {
        std::cout << "screenshot saved: " << screenshot_arg << '\n';
        std::cout.flush();
        QCoreApplication::exit(0);
      }
    });

    return QApplication::exec();
  } else {
    auto* window = new ReviewWindow();
    window->showMaximized();
    window->raise();
    window->activateWindow();
    if (argc > 1) {
      const std::filesystem::path project_path(argv[1]);
      QTimer::singleShot(0, window, [window, project_path]() { window->loadProjectPath(project_path); });
    }
    const int exit_code = QApplication::exec();
    if (std::cout.good()) {
      std::cout.flush();
    }
    return exit_code;
  }
}
