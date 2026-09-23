#include <QApplication>
#include <QLineEdit>
#include <QTemporaryDir>
#include <QTimer>

#include <fstream>

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
  const QString secret_map = window.uiMapJson();
  settings.close();
  if (secret_map.contains("ui-map-must-not-leak") ||
      !secret_map.contains("[REDACTED]")) {
    return 12;
  }

  window.show();
  QTimer::singleShot(7000, &app, &QCoreApplication::quit);
  return QApplication::exec();
}
