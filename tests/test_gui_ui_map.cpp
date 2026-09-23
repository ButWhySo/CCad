#include <QApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QTemporaryDir>
#include <QTimer>

#include <fstream>

#include "ccad_cli/app.hpp"
#include "ccad_core/geometry.hpp"
#include "ccad_core/model.hpp"
#include "ccad_core/serialize.hpp"
#include "ccad_gui/agent_settings_dialog.hpp"
#include "ccad_gui/review_window.hpp"

namespace {

bool writeProject(const QString& path, const ccad::Project& project) {
  std::ofstream out(path.toStdString(), std::ios::binary);
  out << ccad::dumpProjectJson(project);
  return static_cast<bool>(out);
}

ccad::Project emptyProject() {
  ccad::Project project;
  project.id = "proj-gui-empty";
  project.name = "gui-empty";
  return project;
}

ccad::Project boardOnlyProject() {
  ccad::Project project;
  project.id = "proj-gui-board-only";
  project.name = "gui-board-only";
  ccad::Board board;
  board.outline.origin = ccad::Point{ccad::millimeters(0.0), ccad::millimeters(0.0)};
  board.outline.size = ccad::Size{ccad::millimeters(20.0), ccad::millimeters(10.0)};
  board.layers.push_back(ccad::Layer{.id = "F.Cu",
                                     .name = "Front copper",
                                     .kind = "copper",
                                     .visible = true});
  board.layers.push_back(ccad::Layer{.id = "B.Cu",
                                     .name = "Back copper",
                                     .kind = "copper",
                                     .visible = true});
  project.boards.push_back(board);
  ccad::Schematic schematic;
  schematic.id = "S1";
  schematic.name = "main";
  schematic.symbols.push_back(ccad::SchSymbol{.id = "U1", .lib_id = "MCU", .reference = "U1",
                                               .position = {ccad::millimeters(4.0), ccad::millimeters(5.0)},
                                               .fields = {}, .pins = {}});
  schematic.nets.push_back(ccad::Net{.id = "GND", .members = {}});
  project.schematics.push_back(std::move(schematic));
  return project;
}

}  // namespace

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  QTemporaryDir temp_dir;
  if (!temp_dir.isValid()) {
    return 2;
  }

  const QString empty_path = temp_dir.filePath("empty.ccad.json");
  const QString board_path = temp_dir.filePath("board-only.ccad.json");
  if (!writeProject(empty_path, emptyProject()) ||
      !writeProject(board_path, boardOnlyProject())) {
    return 3;
  }

  ReviewWindow window;
  window.loadProjectPath(empty_path.toStdString());
  const QString empty_map = window.uiMapJson();
  if (!empty_map.contains("\"schema_version\"")) {
    return 4;
  }

  window.loadProjectPath(board_path.toStdString());
  const QString board_map = window.uiMapJson();
  if (!board_map.contains("\"schema_version\"")) {
    return 5;
  }
  if (!board_map.contains("action:agent_request_approval") ||
      !board_map.contains("action:agent_approve_next") ||
      !board_map.contains("action:agent_decline_next") ||
      !board_map.contains("action:agent_cancel_approval") ||
      !board_map.contains("control:agent_approval_request")) {
    return 6;
  }

  const QString methods = window.runAgentUiQueryJson("agent.methods", "{}");
  const QString context = window.runAgentUiQueryJson("project.context", "{}");
  const QString state = window.runAgentUiQueryJson("project.state", "{}");
  if (!methods.contains("project.state") || !context.contains("typed_state") ||
      !context.contains("Front copper") || !context.contains("U1") ||
      !state.contains("binary_payloads_excluded") || !state.contains("GND")) {
    return 13;
  }

  const QJsonObject methods_envelope =
      QJsonDocument::fromJson(methods.toUtf8()).object();
  const QJsonObject registry = methods_envelope.value("result").toObject();
  const QJsonArray registry_methods = registry.value("methods").toArray();
  const QJsonObject cli_help = QJsonDocument::fromJson(
      QByteArray::fromStdString(ccad_cli::commandCatalogJson())).object();
  int cli_descriptor_count = 0;
  int callable_descriptor_count = 0;
  for (const QJsonValue& value : registry_methods) {
    const QJsonObject entry = value.toObject();
    if (entry.value("surface").toString() == "ccad_cli") {
      ++cli_descriptor_count;
      if (entry.value("callable").toBool(true)) return 17;
    }
    if (entry.value("callable").toBool()) ++callable_descriptor_count;
  }
  const QJsonObject native_drc = [&]() {
    for (const QJsonValue& value : registry_methods) {
      const QJsonObject entry = value.toObject();
      if (entry.value("method").toString() == "project.drc") return entry;
    }
    return QJsonObject{};
  }();
  const QJsonObject cli_track = [&]() {
    for (const QJsonValue& value : registry_methods) {
      const QJsonObject entry = value.toObject();
      if (entry.value("method").toString() == "cli.pcb.add-track") return entry;
    }
    return QJsonObject{};
  }();
  if (registry.value("catalog_kind").toString() != "ccad_unified_agent_registry" ||
      registry.value("method_count").toInt() != registry_methods.size() ||
      registry.value("callable_method_count").toInt() != callable_descriptor_count ||
      cli_descriptor_count != cli_help.value("commands").toArray().size() ||
      cli_descriptor_count == 0 ||
      native_drc.value("callable").toBool() != true ||
      native_drc.value("surface").toString() != "native_gui_broker" ||
      cli_track.value("callable").toBool(true) ||
      cli_track.value("side_effect").toString() != "project_mutation" ||
      cli_track.value("inputSchema").toObject().value("properties").toObject()
              .value("argv").toObject().value("type").toString() != "array" ||
      cli_track.value("examples").toArray().isEmpty()) {
    return 17;
  }
  const QString cli_track_schema = window.runAgentUiQueryJson(
      "agent.method_schema", R"({"method_name":"cli.pcb.add-track"})");
  if (!cli_track_schema.contains("\"found\":true") ||
      !cli_track_schema.contains("\"callable\":false")) {
    return 18;
  }

  const QJsonObject python_control{{"schema_version", 1},
                                   {"secret_value_visible", false},
                                   {"methods", QJsonArray{
                                       QJsonObject{{"name", "agent.memory_set_enabled"},
                                                   {"transport", "python_json_rpc"},
                                                   {"dispatchable", true},
                                                   {"agent_tool_callable", false},
                                                   {"read_only", false},
                                                   {"secrets", false},
                                                   {"side_effect", "persist_memory_preference"},
                                                   {"params", QJsonObject{
                                                       {"tier", QJsonObject{{"type", "string"},
                                                                             {"enum", QJsonArray{"stm", "ltm"}}}},
                                                       {"enabled", QJsonObject{{"type", "boolean"}}}}},
                                                   {"response", QJsonObject{
                                                       {"method", "memory_state"},
                                                       {"fields", QJsonArray{"tier", "enabled"}}}}}}}};
  if (!window.setPythonControlMethodCatalog(python_control)) return 19;
  const QJsonObject integrated_methods = QJsonDocument::fromJson(
      window.runAgentUiQueryJson("agent.methods", "{}").toUtf8()).object()
      .value("result").toObject();
  const QJsonObject integrated_entry = [&]() {
    for (const QJsonValue& value : integrated_methods.value("methods").toArray()) {
      const QJsonObject entry = value.toObject();
      if (entry.value("method").toString() == "agent.memory_set_enabled") return entry;
    }
    return QJsonObject{};
  }();
  const QJsonObject integrated_schema = QJsonDocument::fromJson(
      window.runAgentUiQueryJson("agent.method_schema",
          R"({"method_name":"agent.memory_set_enabled"})").toUtf8())
      .object().value("result").toObject().value("entry").toObject()
      .value("inputSchema").toObject();
  if (integrated_methods.value("registry_sources").toArray().contains(
          "python_json_rpc_control") == false ||
      integrated_entry.value("surface").toString() != "python_json_rpc_control" ||
      integrated_entry.value("callable").toBool(true) ||
      integrated_entry.value("agent_tool_callable").toBool(true) ||
      integrated_entry.value("control_dispatchable").toBool() != true ||
      integrated_entry.value("result_shape").toObject().value("method").toString() != "memory_state" ||
      integrated_schema.value("properties").toObject().value("tier").toObject()
          .value("enum").toArray().size() != 2 ||
      integrated_schema.value("required").toArray().size() != 2 ||
      integrated_methods.value("callable_method_count").toInt() != callable_descriptor_count ||
      window.setPythonControlMethodCatalog(QJsonObject{{"schema_version", 1},
          {"secret_value_visible", true}, {"methods", QJsonArray{}}}) ||
      !window.runAgentUiQueryJson("agent.method_schema",
          R"({"method_name":"agent.memory_set_enabled"})").contains("\"found\":true")) {
    return 19;
  }

  const QString add_wire = window.runAgentUiQueryJson(
      "ui.add_wire", "{\"x1\":1,\"y1\":1,\"x2\":5,\"y2\":1}");
  const QString add_label = window.runAgentUiQueryJson(
      "ui.add_label", "{\"x\":5,\"y\":1,\"text\":\"GND\"}");
  if (!add_wire.contains("\"performed\":true") || !add_wire.contains("\"wire_count\":1") ||
      !add_label.contains("\"performed\":true") || !add_label.contains("\"label_count\":1")) {
    return 14;
  }
  const QString add_via = window.runAgentUiQueryJson(
      "ui.place_via", "{\"x_mm\":3,\"y_mm\":3}");
  if (!add_via.contains("\"performed\":true") || !add_via.contains("\"via_count\":1")) {
    return 16;
  }

  const QString add_zone = window.runAgentUiQueryJson(
      "ui.add_zone",
      "{\"start_x_mm\":2,\"start_y_mm\":2,\"end_x_mm\":6,\"end_y_mm\":5}");
  if (!add_zone.contains("\"performed\":true") ||
      !add_zone.contains("\"zone_count\":1")) {
    return 7;
  }
  const QString add_keepout = window.runAgentUiQueryJson(
      "ui.add_keepout",
      "{\"start_x_mm\":8,\"start_y_mm\":2,\"end_x_mm\":12,\"end_y_mm\":5}");
  if (!add_keepout.contains("\"performed\":true") ||
      !add_keepout.contains("\"keepout_count\":1")) {
    return 8;
  }
  const QString add_graphic = window.runAgentUiQueryJson(
      "ui.draw_graphic",
      "{\"start_x_mm\":2,\"start_y_mm\":7,\"end_x_mm\":6,\"end_y_mm\":7}");
  if (!add_graphic.contains("\"performed\":true") ||
      !add_graphic.contains("\"graphic_count\":1")) {
    return 9;
  }
  const QString add_text = window.runAgentUiQueryJson(
      "ui.place_text", "{\"x_mm\":2,\"y_mm\":8,\"text\":\"AGENT\"}");
  if (!add_text.contains("\"performed\":true") ||
      !add_text.contains("\"text_count\":1")) {
    return 15;
  }
  const QString outside_graphic = window.runAgentUiQueryJson(
      "ui.draw_graphic",
      "{\"start_x_mm\":2,\"start_y_mm\":2,\"end_x_mm\":25,\"end_y_mm\":2}");
  if (!outside_graphic.contains("\"performed\":false") ||
      !outside_graphic.contains("outside_board_outline")) {
    return 10;
  }

  AgentSettingsDialog settings(nullptr, &window);
  settings.show();
  QApplication::processEvents();
  auto* public_key = settings.findChild<QLineEdit*>("control:langfusePublicKeyInput");
  auto* secret_key = settings.findChild<QLineEdit*>("control:langfuseSecretKeyInput");
  if (public_key == nullptr || secret_key == nullptr) {
    return 11;
  }
  public_key->setText("public-ui-value-must-not-leak");
  secret_key->setText("secret-ui-value-must-not-leak");
  QLineEdit memory_title(&window);
  memory_title.setObjectName("control:memoryTitle");
  memory_title.setText("private-memory-title-must-not-leak");
  const QString secret_map = window.uiMapJson();
  settings.close();
  if (secret_map.contains("ui-map-must-not-leak") ||
      secret_map.contains("private-memory-title-must-not-leak") ||
      !secret_map.contains("[REDACTED]")) {
    return 12;
  }

  window.show();
  QTimer::singleShot(7000, &app, &QCoreApplication::quit);
  return QApplication::exec();
}
