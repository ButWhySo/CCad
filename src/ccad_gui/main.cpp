#include "review_window.hpp"
#include "board_canvas_view.hpp"
#include "library_browser_dialog.hpp"
#include "ui_map_server.hpp"

#include <QApplication>
#include <QCursor>
#include <QElapsedTimer>
#include <QDir>
#include <QFile>
#include <QPainter>
#include <QPixmap>
#include <QListWidget>
#include <QProxyStyle>
#include <QStyleOption>
#include <QTextBrowser>
#include <QThread>
#include <QTimer>

#include <QEventLoop>
#include <filesystem>
#include <fstream>
#include <iostream>
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
                       [window, output_dir, name, initial_wait_ms, per_target_wait_ms]() {
      QStringList entries;
      const QString catalog_startup = window->runAgentUiQueryJson("agent.workspace_state", "{}");
      const bool catalog_startup_verified =
          catalog_startup.contains("\"backend_ready\":true") &&
          catalog_startup.contains("\"native_tool_catalog_installed\":true") &&
          !catalog_startup.contains("\"native_tool_catalog_method_count\":0");
      if (name.startsWith("sprint968-task") ||
          name.startsWith("sprint969-context") ||
          name.startsWith("sprint970-compaction")) {
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
          const QString screenshot_path = QString::fromStdString(
              (output_dir / (name + "-" + action_name + ".png").toStdString()).string());
          window->grab().save(screenshot_path);
          QString popup_screenshot;
          if (auto* popup = window->findChild<QListWidget*>("panel:agent_slash_commands");
              popup && popup->isVisible()) {
            popup_screenshot = QString::fromStdString(
                (output_dir / (name + "-" + action_name + "-slash-popup.png").toStdString()).string());
            popup->grab().save(popup_screenshot);
          }
          entries << QString("{\"target\":%1,\"interaction\":%2,\"result\":%3,\"screenshot\":%4,\"popup_screenshot\":%5}")
                         .arg(jsonStringLocal(target_id), jsonStringLocal(method),
                              result.trimmed(), jsonStringLocal(screenshot_path),
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
        const QString report =
            QString("{\"schema_version\":1,\"name\":%1,\"interaction_plan\":"
                    "\"Feature-specific mapped Agent interaction with per-action screenshots; no model request\","
                    "\"catalog_startup_verified\":%2,\"entries\":[%3]}\n")
                .arg(jsonStringLocal(name))
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
      const QStringList target_ids = name.startsWith("sprint967-memory")
          ? QStringList{"action:settingsBtn", "control:categoryList",
                        "control:stmCb", "control:ltmCb",
                        "control:episodicCb", "label:memoryState",
                        "action:agent_memory_manage", "control:memoryEntries",
                        "control:memoryTier", "control:memoryContent",
                        "action:closeMemoryManager", "action:cancelSettingsButton"}
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
      const QStringList trigger_before_capture_ids = name.startsWith("sprint967-memory")
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
                                                    "action:agent_memory_manage",
                                                    "control:memoryTier",
                                                    "control:memoryContent",
                                                    "action:closeMemoryManager",
                                                    "action:cancelSettingsButton"};
      const auto runPass = [window, &entries, &output_dir, &name, &target_ids,
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
              id == "action:agent_memory_manage") {
            const bool memory_control = id == "control:stmCb" || id == "control:ltmCb" ||
                id == "control:episodicCb" || id == "label:memoryState" ||
                id == "action:agent_memory_manage";
            const int category = memory_control ? 2 : (id == "control:mcpServersTable" ||
                                         id == "action:addMcpServerBtn" ||
                                         id == "action:removeMcpServerBtn"
                                     ? 3
                                     : (id == "control:apiKeyInput" ? 4 : 1));
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
            const QString payload = id == "control:categoryList"
                ? QString("{\"id\":%1,\"row\":2}").arg(jsonStringLocal(id))
                : QString("{\"id\":%1}").arg(jsonStringLocal(id));
            const QString click_result = window->runAgentUiQueryJson("ui.click", payload);
            entries << QString("{\"pass\":%1,\"id\":%2,\"interaction\":\"ui.click\",\"result\":%3}")
                           .arg(jsonStringLocal(pass_name), jsonStringLocal(id), click_result.trimmed());
            QApplication::processEvents();
            if (id == "action:agent_memory_manage") {
              QThread::msleep(static_cast<unsigned long>(per_target_wait_ms));
              QApplication::processEvents();
            }
          }
          const QString target_json = window->uiTargetJsonById(id);
          const bool found = target_json.contains("\"found\":true");
          const std::optional<int> x = extractJsonInt(target_json, "\"logical_x\":");
          const std::optional<int> y = extractJsonInt(target_json, "\"logical_y\":");
          QString screenshot_path;
          if (found && x.has_value() && y.has_value()) {
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
          } else if (click_before_capture_ids.contains(id)) {
            QWidget* active = QApplication::activeWindow();
            if (active && active->isVisible()) {
              const std::filesystem::path path = output_dir /
                  (name + "-" + pass_name + "-" + id.mid(id.indexOf(':') + 1) + "-after.png").toStdString();
              screenshot_path = QString::fromStdString(path.string());
              active->grab().save(screenshot_path);
            }
          }
          if (id == "control:memoryContent" && found) {
            const QString typing = QString("{\"id\":%1,\"text\":\"visual validation text; not saved\"}")
                .arg(jsonStringLocal(id));
            const QString typing_result = window->runAgentUiQueryJson("ui.type_text", typing);
            QApplication::processEvents();
            QThread::msleep(static_cast<unsigned long>(per_target_wait_ms));
            QApplication::processEvents();
            const std::filesystem::path typed_path = output_dir /
                (name + "-" + pass_name + "-memory-content-typed.png").toStdString();
            QWidget* active = QApplication::activeWindow();
            if (active && active->isVisible()) active->grab().save(QString::fromStdString(typed_path.string()));
            entries << QString("{\"pass\":%1,\"id\":%2,\"interaction\":\"ui.type_text\",\"result\":%3,\"screenshot\":%4}")
                           .arg(jsonStringLocal(pass_name), jsonStringLocal(id), typing_result.trimmed(),
                                jsonStringLocal(QString::fromStdString(typed_path.string())));
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
      runPass("resized");

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
      if (!output) {
        std::cerr << "failed to write target sequence report: " << output_path.string() << '\n';
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
