#include "ccad_core/model.hpp"
#include "ccad_core/kicad_footprint_import.hpp"
#include "ccad_core/serialize.hpp"
#include "ccad_gui/review_window.hpp"
#include "ccad_gui/ui_map_server.hpp"
#include "test_support.hpp"

#include <QApplication>
#include <QElapsedTimer>
#include <QLocalSocket>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

ccad::Project uiMapProject() {
  ccad::Project project;
  project.id = "proj-ui-map";
  project.name = "UI map";
  project.board = ccad::Board{
      .outline = ccad::Rect{
          .origin = ccad::Point{.x = ccad::millimeters(0), .y = ccad::millimeters(0)},
          .size = ccad::Size{.width = ccad::millimeters(42),
                              .height = ccad::millimeters(30)},
      },
      .design_rules = ccad::DesignRules{},
      .layers = {ccad::Layer{.id = "F.Cu",
                              .name = "Front copper",
                              .kind = "copper",
                              .visible = true}},
      .placement_regions = {},
      .keepouts = {},
      .pads = {ccad::Pad{.id = "U1.1",
                         .component_id = "U1",
                         .pin_name = "1",
                         .net_id = "N1",
                         .layers = {"F.Cu"},
                         .type = "smd",
                         .shape = "roundrect",
                         .position = ccad::Point{.x = ccad::millimeters(8),
                                                  .y = ccad::millimeters(9)},
                         .size = ccad::Size{.width = ccad::millimeters(1.5),
                                            .height = ccad::millimeters(1.0)},
                         .drill = std::nullopt,
                         .roundrect_rratio = 0.25,
                         .chamfer_ratio = std::nullopt}},
      .vias = {},
      .tracks = {},
      .route_requests = {},
  };
  return project;
}

std::filesystem::path writeProjectFixture() {
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  const std::filesystem::path path =
      std::filesystem::temp_directory_path() /
      ("ccad-ui-map-test-" + std::to_string(stamp) + ".ccad.json");
  std::ofstream output(path, std::ios::binary);
  output << ccad::dumpProjectJson(uiMapProject());
  output.close();
  require(bool(output), "project fixture writes");
  return path;
}

std::filesystem::path writeFootprintFixture() {
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  ccad::Footprint footprint;
  footprint.name = "UiMapFootprint";
  footprint.pads.push_back(ccad::FootprintPad{.number = "1",
                                              .type = "smd",
                                              .shape = "rect",
                                              .position = ccad::Point{},
                                              .rotation_degrees = 0.0,
                                              .size = ccad::Size{.width = ccad::millimeters(1.0),
                                                                 .height = ccad::millimeters(1.0)},
                                              .drill = std::nullopt,
                                              .layers = {"F.Cu", "F.Paste", "F.Mask"},
                                              .roundrect_rratio = std::nullopt,
                                              .chamfer_ratio = std::nullopt});
  const std::filesystem::path path =
      std::filesystem::temp_directory_path() /
      ("ccad-ui-map-footprint-" + std::to_string(stamp) + ".json");
  std::ofstream output(path, std::ios::binary);
  output << ccad::dumpFootprintJson(footprint);
  output.close();
  require(bool(output), "footprint fixture writes");
  return path;
}

bool contains(const QString& haystack, const char* needle) {
  return haystack.contains(QString::fromUtf8(needle));
}

void processUntil(bool (*predicate)(QLocalSocket&), QLocalSocket& socket, const char* message) {
  QElapsedTimer timer;
  timer.start();
  while (!predicate(socket) && timer.elapsed() < 3000) {
    QApplication::processEvents();
  }
  require(predicate(socket), message);
}

QString requestLine(QLocalSocket& socket, const QString& line) {
  socket.write((line + "\n").toUtf8());
  socket.flush();
  processUntil([](QLocalSocket& s) { return s.canReadLine(); }, socket,
               "UI map server writes a response line");
  return QString::fromUtf8(socket.readLine()).trimmed();
}

}  // namespace

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  const std::filesystem::path project_path = writeProjectFixture();
  ReviewWindow window;
  window.loadProjectPath(project_path);
  window.show();
  QApplication::processEvents();

  const QString map = window.uiMapJson();
  require(contains(map, "\"schema_version\":1"), "UI map includes schema version");
  require(contains(map, "\"ui_epoch\":"), "UI map includes changing epoch");
  require(contains(map, "\"id\":\"action:add_footprint\""),
          "UI map exposes add footprint action");
  require(contains(map, "\"id\":\"menu:file\""), "UI map exposes File menu");
  require(contains(map, "\"id\":\"panel:layers_objects\""),
          "UI map exposes layers/object panel");
  require(contains(map, "\"id\":\"panel:diagnostics\""), "UI map exposes diagnostics panel");
  require(contains(map, "\"id\":\"panel:agent\""), "UI map exposes agent panel");
  require(contains(map, "\"id\":\"tab:pcb\""), "UI map exposes PCB tab");
  require(contains(map, "\"id\":\"tab:schematic\""), "UI map exposes schematic tab");
  require(contains(map, "\"id\":\"tab:agent\""), "UI map exposes agent bottom tab");
  require(contains(map, "\"id\":\"canvas:pcb\""), "UI map exposes PCB canvas");
  require(contains(map, "\"id\":\"canvas_object:U1.1\""),
          "UI map exposes typed canvas object");
  require(contains(map, "\"object_id\":\"U1.1\""), "UI map keeps original object id");
  require(contains(map, "\"layer_id\":\"F.Cu\""), "UI map carries layer metadata");
  require(contains(map, "\"target_x\":"), "UI map carries click target x coordinate");
  require(contains(map, "\"target_y\":"), "UI map carries click target y coordinate");

  const QString validation = window.validateUiMapTargetsJson(false);
  require(contains(validation, "\"summary\":"), "UI map validation includes summary");
  require(contains(validation, "\"failures\":0"), "UI map validation has no misses");
  require(contains(validation, "\"id\":\"action:add_footprint\""),
          "UI map validation covers add footprint action");
  require(contains(validation, "\"id\":\"canvas_object:U1.1\""),
          "UI map validation covers canvas object target");

  const QString action_target = window.uiTargetJsonById("action:add_footprint");
  require(contains(action_target, "\"found\":true"), "action target query finds add footprint");
  require(contains(action_target, "\"role\":\"action\""), "action target query reports role");
  require(contains(action_target, "\"physical_x\":"), "action target reports physical pixels");

  const QString menu_target = window.uiTargetJsonById("menu:file");
  require(contains(menu_target, "\"found\":true"), "menu target query finds File menu");
  require(contains(menu_target, "\"role\":\"menu\""), "menu target query reports role");

  const QString panel_target = window.uiTargetJsonById("panel:layers_objects");
  require(contains(panel_target, "\"found\":true"), "panel target query finds layers panel");
  require(contains(panel_target, "\"role\":\"panel\""), "panel target query reports role");

  const QString agent_panel_target = window.uiTargetJsonById("panel:agent");
  require(contains(agent_panel_target, "\"found\":true"), "panel target query finds agent panel");
  require(contains(agent_panel_target, "\"role\":\"panel\""),
          "agent panel target query reports role");

  const QString agent_tab_target = window.uiTargetJsonById("tab:agent");
  require(contains(agent_tab_target, "\"found\":true"), "tab target query finds agent tab");
  require(contains(agent_tab_target, "\"role\":\"tab\""), "agent tab target query reports role");

  const QString pad_target = window.uiTargetJsonById("canvas_object:U1.1");
  require(contains(pad_target, "\"found\":true"), "canvas object target query finds pad");
  require(contains(pad_target, "\"role\":\"canvas_object\""),
          "canvas object target query reports role");

  const QString board_point = window.uiTargetJsonForBoardPoint(8.0, 9.0);
  require(contains(board_point, "\"found\":true"), "board point target query finds point");
  require(contains(board_point, "\"space\":\"board\""), "board point target reports space");
  require(contains(board_point, "\"scene_x\":"), "board point target reports scene mapping");

  const QString unknown = window.uiTargetJsonById("action:not_real");
  require(contains(unknown, "\"found\":false"), "unknown target id fails explicitly");
  require(contains(unknown, "\"reason\":\"unknown_id\""), "unknown target id reports reason");

  const QString outside = window.uiTargetJsonForBoardPoint(99.0, 99.0);
  require(contains(outside, "\"found\":false"), "outside board point fails explicitly");
  require(contains(outside, "\"reason\":\"outside_board\""),
          "outside board point reports reason");

  const QString zoom_action = window.triggerSafeUiActionJson("action:zoom_in");
  require(contains(zoom_action, "\"performed\":true"), "safe zoom action triggers");
  require(contains(zoom_action, "\"reason\":\"triggered\""), "safe action reports trigger");

  const QString schematic_tab = window.triggerSafeUiActionJson("tab:schematic");
  require(contains(schematic_tab, "\"performed\":true"), "safe schematic tab trigger works");
  require(contains(window.uiTargetJsonById("tab:schematic"), "\"visible\":true"),
          "schematic tab remains targetable after trigger");

  const QString pcb_tab = window.triggerSafeUiActionJson("tab:pcb");
  require(contains(pcb_tab, "\"performed\":true"), "safe PCB tab trigger works");

  const QString agent_tab = window.triggerSafeUiActionJson("tab:agent");
  require(contains(agent_tab, "\"performed\":true"), "safe agent tab trigger works");
  require(contains(agent_tab, "\"reason\":\"tab_selected\""),
          "safe agent tab trigger reports tab selection");

  const QString unsafe = window.triggerSafeUiActionJson("action:save");
  require(contains(unsafe, "\"performed\":false"), "unsafe action is refused");
  require(contains(unsafe, "\"reason\":\"unsafe_action_requires_human_or_kernel_tool\""),
          "unsafe action reports reason");

  const QString route_tool = window.triggerSafeUiActionJson("action:add_tracks");
  require(contains(route_tool, "\"performed\":true"), "route-track tool enters edit mode");
  require(contains(route_tool, "\"reason\":\"editor_tool_selected\""),
          "route-track tool reports real editor-tool selection");
  require(contains(route_tool, "\"mode\":\"route_track\""),
          "route-track tool reports selected mode");

  const QString via_tool = window.triggerSafeUiActionJson("action:add_via");
  require(contains(via_tool, "\"performed\":true"), "add-via tool enters edit mode");
  require(contains(via_tool, "\"mode\":\"add_via\""), "add-via tool reports selected mode");

  const QString keepout_tool = window.triggerSafeUiActionJson("action:add_keepout_area");
  require(contains(keepout_tool, "\"performed\":true"), "add-keepout tool enters edit mode");
  require(contains(keepout_tool, "\"mode\":\"add_keepout\""),
          "add-keepout tool reports selected mode");

  const QString delete_selected = window.triggerSafeUiActionJson("action:delete_cursor");
  require(contains(delete_selected, "\"performed\":true"),
          "delete action removes the selected board object");
  require(contains(delete_selected, "\"reason\":\"deleted\""),
          "delete action reports deletion");
  require(contains(delete_selected, "\"deleted_type\":\"pad\""),
          "delete action reports deleted object type");
  require(!contains(window.uiMapJson(), "\"id\":\"canvas_object:U1.1\""),
          "UI map drops the deleted pad");

  const QString future_tool = window.triggerSafeUiActionJson("action:add_zone");
  require(contains(future_tool, "\"performed\":false"),
          "unsupported zone toolbar action is still not silent");
  require(contains(future_tool, "\"reason\":\"future_tool_not_implemented\""),
          "unsupported zone toolbar action reports planned-tool reason");
  require(contains(future_tool, "\"label\":\"Add Zone\""),
          "unsupported zone toolbar action reports user-facing label");

  const QString grid_toggle = window.triggerSafeUiActionJson("action:grid");
  require(contains(grid_toggle, "\"performed\":true"), "grid toolbar action performs");
  require(contains(grid_toggle, "\"reason\":\"display_state_toggled\""),
          "grid toolbar action reports display-state toggle");
  require(contains(grid_toggle, "\"state\":false"), "grid toolbar action reports disabled state");

  const QString polar_toggle = window.triggerSafeUiActionJson("action:polar_coord");
  require(contains(polar_toggle, "\"performed\":true"), "polar coordinates action performs");
  require(contains(polar_toggle, "\"state\":true"), "polar coordinates action reports enabled state");

  const QString units_toggle = window.triggerSafeUiActionJson("action:unit_inch");
  require(contains(units_toggle, "\"performed\":true"), "unit toggle action performs");
  require(contains(units_toggle, "\"units\":\"in\""), "unit toggle reports inch mode");

  const QString crosshair_toggle = window.triggerSafeUiActionJson("action:cursor_shape");
  require(contains(crosshair_toggle, "\"performed\":true"), "crosshair toolbar action performs");
  require(contains(crosshair_toggle, "\"state\":true"), "crosshair action reports enabled state");

  const QString ratsnest_toggle = window.triggerSafeUiActionJson("action:show_ratsnest");
  require(contains(ratsnest_toggle, "\"performed\":true"), "ratsnest toolbar action performs");
  require(contains(ratsnest_toggle, "\"state\":false"), "ratsnest action reports hidden state");

  const QString net_highlight_toggle = window.triggerSafeUiActionJson("action:net_highlight");
  require(contains(net_highlight_toggle, "\"performed\":true"), "net highlight action performs");
  require(contains(net_highlight_toggle, "\"state\":true"), "net highlight action reports enabled state");

  const QString contrast_toggle = window.triggerSafeUiActionJson("action:contrast_mode");
  require(contains(contrast_toggle, "\"performed\":true"), "display modes action performs");
  require(contains(contrast_toggle, "\"mode\":\"high_contrast\""),
          "display modes action reports high-contrast mode");

  const QString hide_layers = window.triggerSafeUiActionJson("action:layers_manager");
  require(contains(hide_layers, "\"performed\":true"), "show layers action toggles panel");
  require(contains(hide_layers, "\"reason\":\"panel_toggled\""),
          "show layers action reports panel toggle");
  require(contains(window.uiTargetJsonById("panel:layers_objects"), "\"visible\":false"),
          "show layers action hides layers/object panel");

  const QString show_layers = window.triggerSafeUiActionJson("action:layers_manager");
  require(contains(show_layers, "\"performed\":true"), "show layers action toggles panel back");
  require(contains(window.uiTargetJsonById("panel:layers_objects"), "\"visible\":true"),
          "show layers action shows layers/object panel");

  const QString hide_properties = window.triggerSafeUiActionJson("action:part_properties");
  require(contains(hide_properties, "\"performed\":true"), "show properties action toggles panel");
  require(contains(hide_properties, "\"reason\":\"panel_toggled\""),
          "show properties action reports panel toggle");
  require(contains(window.uiTargetJsonById("panel:properties"), "\"visible\":false"),
          "show properties action hides properties panel");

  const QString show_properties = window.triggerSafeUiActionJson("action:part_properties");
  require(contains(show_properties, "\"performed\":true"),
          "show properties action toggles panel back");
  require(contains(window.uiTargetJsonById("panel:properties"), "\"visible\":true"),
          "show properties action shows properties panel");

  const QString unknown_action = window.triggerSafeUiActionJson("action:nope");
  require(contains(unknown_action, "\"performed\":false"), "unknown action is refused");
  require(contains(unknown_action, "\"reason\":\"unknown_or_not_allowlisted\""),
          "unknown action reports reason");

  const QString server_name = "ccad-ui-map-test-" + QString::number(QCoreApplication::applicationPid());
  UiMapServer server(window);
  require(server.listen(server_name), "UI map server starts on a local socket");
  QLocalSocket socket;
  socket.connectToServer(server_name);
  processUntil([](QLocalSocket& s) { return s.state() == QLocalSocket::ConnectedState; }, socket,
               "UI map server accepts a local socket connection");
  const QString map_response = requestLine(socket, "{\"method\":\"ui.map\"}");
  require(contains(map_response, "\"ok\":true"), "live server returns successful ui.map");
  require(contains(map_response, "\"id\":\"action:add_footprint\""),
          "live server ui.map includes action IDs");
  const QString target_response =
      requestLine(socket, "{\"method\":\"ui.target\",\"id\":\"menu:file\"}");
  require(contains(target_response, "\"ok\":true"), "live server returns successful ui.target");
  require(contains(target_response, "\"found\":true"), "live server target finds File menu");
  const QString epoch_response = requestLine(socket, "{\"method\":\"ui.epoch\"}");
  require(contains(epoch_response, "\"ui_epoch\":"), "live server returns current UI epoch");
  const QString bad_response = requestLine(socket, "{\"method\":\"ui.unknown\"}");
  require(contains(bad_response, "\"ok\":false"), "live server refuses unsupported methods");
  socket.disconnectFromServer();
  server.close();

  const QString placement =
      window.commitFootprintPlacementForAutomation(writeFootprintFixture(), 12.0, 10.0);
  require(contains(placement, "\"performed\":true"), "automation footprint placement succeeds");
  require(contains(placement, "\"reason\":\"placed\""), "automation placement reports placed");
  require(contains(placement, "\"pad_count\":1"), "automation placement adds one pad");

  const QString via = window.commitViaPlacementForAutomation(15.0, 11.0);
  require(contains(via, "\"performed\":true"), "automation via placement succeeds");
  require(contains(via, "\"reason\":\"placed\""), "automation via placement reports placed");
  require(contains(via, "\"via_count\":1"), "automation via placement adds one via");
  require(contains(window.uiMapJson(), "\"id\":\"canvas_object:V1\""),
          "UI map exposes placed via");

  const QString track = window.commitTrackPlacementForAutomation(8.0, 9.0, 15.0, 11.0);
  require(contains(track, "\"performed\":true"), "automation track routing succeeds");
  require(contains(track, "\"reason\":\"placed\""), "automation track routing reports placed");
  require(contains(track, "\"track_count\":1"), "automation track routing adds one track");
  require(contains(window.uiMapJson(), "\"id\":\"canvas_object:T1\""),
          "UI map exposes placed track");

  const QString keepout = window.commitKeepoutPlacementForAutomation(20.0, 10.0, 25.0, 14.0);
  require(contains(keepout, "\"performed\":true"), "automation keepout placement succeeds");
  require(contains(keepout, "\"reason\":\"placed\""),
          "automation keepout placement reports placed");
  require(contains(keepout, "\"keepout_count\":1"),
          "automation keepout placement adds one keepout");
  require(contains(window.uiMapJson(), "\"id\":\"canvas_object:K1\""),
          "UI map exposes placed keepout");

  const QString delete_via = window.deleteBoardObjectForAutomation("V1");
  require(contains(delete_via, "\"performed\":true"), "automation delete removes via");
  require(contains(delete_via, "\"reason\":\"deleted\""), "automation delete reports deletion");
  require(contains(delete_via, "\"deleted_type\":\"via\""),
          "automation delete reports deleted type");
  require(!contains(window.uiMapJson(), "\"id\":\"canvas_object:V1\""),
          "UI map no longer exposes deleted via");
}
