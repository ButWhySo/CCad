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

  if (argc == 4 && (std::string(argv[1]) == "--dump-ui-map" ||
                    std::string(argv[1]) == "--validate-ui-map-targets")) {
    const bool validate_targets = std::string(argv[1]) == "--validate-ui-map-targets";
    const std::filesystem::path project_path(argv[2]);
    const std::filesystem::path output_path(argv[3]);
    ReviewWindow window;
    window.loadProjectPath(project_path);
    window.show();

    QTimer::singleShot(500, &window, [&window, output_path, validate_targets]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open UI map output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QString map = validate_targets ? window.validateUiMapTargetsJson(true)
                                           : window.uiMapJson();
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
    ReviewWindow window;
    window.loadProjectPath(project_path);
    window.show();

    QTimer::singleShot(500, &window, [&window, target_id, output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open UI target output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window.uiTargetJsonById(target_id).toUtf8();
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
    ReviewWindow window;
    window.loadProjectPath(project_path);
    window.show();

    QTimer::singleShot(500, &window, [&window, action_id, output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open UI action output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window.triggerSafeUiActionJson(action_id).toUtf8();
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
    ReviewWindow window;
    window.loadProjectPath(project_path);
    window.show();

    QTimer::singleShot(500, &window, [&window, output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open active layer output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window.activePcbLayerJson().toUtf8();
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
    ReviewWindow window;
    window.loadProjectPath(project_path);
    window.show();

    QTimer::singleShot(500, &window, [&window, layer_id, output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open active layer output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window.setActivePcbLayerForAutomation(layer_id).toUtf8();
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
    ReviewWindow window;
    window.loadProjectPath(project_path);
    window.show();

    QTimer::singleShot(500, &window, [&window, output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open active net output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window.activePcbNetJson().toUtf8();
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
    ReviewWindow window;
    window.loadProjectPath(project_path);
    window.show();

    QTimer::singleShot(500, &window, [&window, net_id, output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open active net output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window.setActivePcbNetForAutomation(net_id).toUtf8();
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
    ReviewWindow window;
    window.loadProjectPath(project_path);
    window.show();

    QTimer::singleShot(500, &window, [&window, x_mm, y_mm, output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open UI target output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window.uiTargetJsonForBoardPoint(x_mm, y_mm).toUtf8();
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
    ReviewWindow window;
    window.loadProjectPath(project_path);
    window.show();

    QTimer::singleShot(initial_wait_ms, &window,
                       [&window, output_dir, name, initial_wait_ms, per_target_wait_ms]() {
      QStringList entries;
      const QStringList target_ids = {"action:cursor", "action:measurement", "action:save",
                                      "menu:file", "panel:properties", "action:grid",
                                      "action:polar_coord", "action:unit_inch",
                                      "action:cursor_shape", "action:show_ratsnest",
                                      "action:net_highlight", "action:contrast_mode",
                                      "tab:agent", "panel:agent_session_strip",
                                      "panel:agent_header_action_bar",
                                      "panel:agent_mode_strip", "panel:agent_run_controls",
                                      "label:agent_run_state_chip",
                                      "panel:agent_run_queue",
                                      "label:agent_run_queue_status",
                                      "label:agent_run_queue_counts",
                                      "label:agent_run_queue_current_step",
                                      "action:agent_cancel_run_queue",
                                      "action:agent_clear_run_queue",
                                      "label:agent_trace_chip",
                                      "label:agent_session_chip", "panel:agent_trace_strip",
                                      "panel:agent_trace_links", "label:agent_trace_id",
                                      "label:agent_span_id", "label:agent_trace_status",
                                      "label:agent_trace_export_status",
                                      "action:agent_new_trace_context",
                                      "panel:agent_provider_controls",
                                      "control:agent_provider_family",
                                      "control:agent_provider_model",
                                      "action:agent_provider_refresh_status",
                                      "label:agent_provider_status",
                                      "label:agent_provider_env",
                                      "label:agent_provider_execution_status",
                                      "panel:agent_session_binding",
                                      "control:agent_session_path",
                                      "action:agent_load_session",
                                      "action:agent_checkpoint_session",
                                      "label:agent_session_status",
                                      "panel:agent_policy_surface",
                                      "label:agent_policy_decision",
                                      "label:agent_policy_risk",
                                      "control:agent_policy_dry_run",
                                      "action:agent_policy_preview",
                                      "action:agent_pause_run",
                                      "action:agent_resume_run", "action:agent_stop_run",
                                      "panel:agent_active_plan",
                                      "panel:agent_plan_row_1", "tab:agent_command",
                                      "tab:agent_evidence", "tab:agent_approvals",
                                      "panel:agent_activity_stream",
                                      "action:agent_header_request_context",
                                      "action:agent_header_trigger_drc",
                                      "action:agent_header_clear_output",
                                      "control:agent_command_input",
                                      "action:agent_submit_command",
                                      "panel:agent_footer_quick_actions",
                                      "action:agent_quick_request_context",
                                      "action:agent_quick_trigger_drc",
                                      "action:agent_footer_request_context",
                                      "action:agent_footer_trigger_drc",
                                      "control:agent_live_method",
                                      "control:agent_live_payload",
                                      "action:agent_live_query", "control:agent_goal",
                                      "action:agent_stage_goal", "action:agent_pin_evidence",
                                      "action:agent_clear_evidence",
                                      "panel:agent_evidence_thumbnail_strip",
                                      "card:agent_evidence_thumbnail_datasheet",
                                      "card:agent_evidence_thumbnail_drc",
                                      "card:agent_evidence_thumbnail_schematic",
                                      "card:agent_evidence_thumbnail_revision",
                                      "control:agent_approval_request",
                                      "action:agent_request_approval",
                                      "panel:agent_approval_preview",
                                      "panel:agent_approval_preview_artifact",
                                      "label:agent_approval_preview_summary",
                                      "label:agent_approval_preview_delta",
                                      "action:agent_approve_next",
                                      "action:agent_decline_next",
                                      "action:agent_cancel_approval",
                                      "action:agent_clear_approvals"};
      const QStringList trigger_before_capture_ids = {
          "action:grid",          "action:polar_coord",   "action:unit_inch",
          "action:cursor_shape",  "action:show_ratsnest", "action:net_highlight",
          "action:contrast_mode", "tab:agent"};
      const QStringList click_before_capture_ids = {"action:agent_footer_trigger_drc",
                                                    "action:agent_pin_evidence"};
      const auto runPass = [&window, &entries, &output_dir, &name, &target_ids,
                            &trigger_before_capture_ids,
                            &click_before_capture_ids,
                            per_target_wait_ms](
                               const QString& pass_name) {
        for (const QString& id : target_ids) {
          if (trigger_before_capture_ids.contains(id)) {
            window.triggerSafeUiActionJson(id);
            QApplication::processEvents();
          }
          if (click_before_capture_ids.contains(id)) {
            const QString payload = QString("{\"id\":%1}").arg(jsonStringLocal(id));
            window.runAgentUiQueryJson("ui.click", payload);
            QApplication::processEvents();
          }
          const QString target_json = window.uiTargetJsonById(id);
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
            QPixmap screenshot = window.grab();
            QPainter painter(&screenshot);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setPen(QPen(QColor("#ff00cc"), 3));
            const QPoint local_target = window.mapFromGlobal(QPoint(*x, *y));
            painter.drawEllipse(local_target, 10, 10);
            painter.drawLine(local_target.x() - 16, local_target.y(), local_target.x() + 16,
                             local_target.y());
            painter.drawLine(local_target.x(), local_target.y() - 16, local_target.x(),
                             local_target.y() + 16);
            painter.end();
            screenshot.save(screenshot_path);
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
      window.resize(1120, 720);
      QApplication::processEvents();
      QThread::msleep(static_cast<unsigned long>(per_target_wait_ms));
      runPass("resized");

      const std::filesystem::path output_path =
          output_dir / (name + "-target-sequence.json").toStdString();
      std::ofstream output(output_path, std::ios::binary);
      const QString report =
          QString("{\"schema_version\":1,\"name\":%1,\"initial_wait_ms\":%2,"
                  "\"per_target_wait_ms\":%3,\"entries\":[%4]}\n")
              .arg(jsonStringLocal(name))
              .arg(initial_wait_ms)
              .arg(per_target_wait_ms)
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
    ReviewWindow window;
    window.loadProjectPath(project_path);
    window.show();

    QTimer::singleShot(500, &window, [&window, footprint_path, x_mm, y_mm, output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open placement output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes =
          window.commitFootprintPlacementForAutomation(footprint_path, x_mm, y_mm).toUtf8();
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
    ReviewWindow window;
    window.loadProjectPath(project_path);
    window.show();

    QTimer::singleShot(500, &window, [&window, x_mm, y_mm, output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open via placement output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window.commitViaPlacementForAutomation(x_mm, y_mm).toUtf8();
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
    ReviewWindow window;
    window.loadProjectPath(project_path);
    window.show();

    QTimer::singleShot(500, &window, [&window, start_x_mm, start_y_mm, end_x_mm, end_y_mm,
                                      output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open track placement output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window
                                   .commitTrackPlacementForAutomation(start_x_mm, start_y_mm,
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
    ReviewWindow window;
    window.loadProjectPath(project_path);
    window.show();

    QTimer::singleShot(500, &window, [&window, start_x_mm, start_y_mm, end_x_mm, end_y_mm,
                                      output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open zone placement output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window
                                   .commitZonePlacementForAutomation(start_x_mm, start_y_mm,
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
    ReviewWindow window;
    window.loadProjectPath(project_path);
    window.show();

    QTimer::singleShot(500, &window, [&window, start_x_mm, start_y_mm, end_x_mm, end_y_mm,
                                      output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open keepout placement output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window
                                   .commitKeepoutPlacementForAutomation(start_x_mm, start_y_mm,
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
    ReviewWindow window;
    window.loadProjectPath(project_path);
    window.show();

    QTimer::singleShot(500, &window, [&window, object_id, output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open delete output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window.deleteBoardObjectForAutomation(object_id).toUtf8();
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
    ReviewWindow window;
    window.loadProjectPath(project_path);
    window.show();

    QTimer::singleShot(kSingleScreenshotWaitMs, &window,
                       [&window, screenshot_path, screenshot_arg]() {
      const QPixmap screenshot = window.grab();
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
  } else if (argc == 4 && (std::string(argv[1]) == "--screenshot-footprint" || std::string(argv[1]) == "--screenshot-symbol")) {
    const std::string mode = argv[1];
    const std::filesystem::path target_path(argv[2]);
    const char* screenshot_arg = argv[3];
    const QString screenshot_path = QString::fromLocal8Bit(screenshot_arg);
    
    ReviewWindow window;
    if (mode == "--screenshot-footprint") {
      window.loadFootprintPreview(target_path);
    } else {
      window.loadSymbolPreview(target_path);
    }
    window.show();
    
    QTimer::singleShot(kSingleScreenshotWaitMs, &window,
                       [&window, screenshot_path, screenshot_arg]() {
      const QPixmap screenshot = window.grab();
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
    ReviewWindow window;
    window.loadProjectPath(project_path);
    window.show();

    QTimer::singleShot(kSingleScreenshotWaitMs, &window,
                       [&window, screenshot_path, screenshot_arg]() {
      const QPixmap screenshot = window.grab();
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
    ReviewWindow window;
    window.loadProjectPath(project_path);
    window.show();

    QTimer::singleShot(500, &window, [&window]() {
      // Simulate switching to measure tool
      if (auto* tabs = window.findChild<QTabWidget*>("editorTabs")) {
        if (auto* view = dynamic_cast<BoardCanvasView*>(tabs->currentWidget())) {
          view->setToolMode(ToolMode::Measure);
          // Simulate drag from (10,10) to (50,50) in viewport
          QMouseEvent press(QEvent::MouseButtonPress, QPointF(100, 100), QPointF(100, 100), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
          QApplication::sendEvent(view->viewport(), &press);
          QMouseEvent move(QEvent::MouseMove, QPointF(400, 300), QPointF(400, 300), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
          QApplication::sendEvent(view->viewport(), &move);
        }
      }
    });

    QTimer::singleShot(kSingleScreenshotWaitMs, &window,
                       [&window, screenshot_path, screenshot_arg]() {
      const QPixmap screenshot = window.grab();
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
    ReviewWindow window;
    window.show();
    if (argc > 1) {
      const std::filesystem::path project_path(argv[1]);
      QTimer::singleShot(0, &window, [&window, project_path]() { window.loadProjectPath(project_path); });
    }
    const int exit_code = QApplication::exec();
    if (std::cout.good()) {
      std::cout.flush();
    }
    return exit_code;
  }
}
