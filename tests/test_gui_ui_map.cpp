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
                              .visible = true},
                 ccad::Layer{.id = "B.Cu",
                              .name = "Back copper",
                              .kind = "copper",
                              .visible = true},
                 ccad::Layer{.id = "F.SilkS",
                              .name = "Front silkscreen",
                              .kind = "silkscreen",
                              .visible = true},
                 ccad::Layer{.id = "Dwgs.User",
                              .name = "User drawings",
                              .kind = "user",
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
      .graphics = {},
      .texts = {},
      .zones = {},
      .route_requests = {},
  };
  project.nets = {ccad::Net{.id = "N1",
                            .members = {ccad::NetMember{.component_id = "U1",
                                                        .pin_name = "1"}}},
                  ccad::Net{.id = "N2", .members = {}}};
  return project;
}

std::filesystem::path writeProjectFixture(ccad::Project project) {
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  const std::filesystem::path path =
      std::filesystem::temp_directory_path() /
      ("ccad-ui-map-test-" + std::to_string(stamp) + ".ccad.json");
  std::ofstream output(path, std::ios::binary);
  output << ccad::dumpProjectJson(project);
  output.close();
  require(bool(output), "project fixture writes");
  return path;
}

std::filesystem::path writeProjectFixture() {
  return writeProjectFixture(uiMapProject());
}

std::string readFile(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  require(bool(input), "fixture file opens for reading");
  return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
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

int extractInt(const QString& json, const char* key_text) {
  const QString key = QString::fromUtf8(key_text);
  int index = json.indexOf(key);
  require(index >= 0, "integer key exists in json");
  index += key.size();
  while (index < json.size() && json.at(index).isSpace()) {
    ++index;
  }
  int end = index;
  while (end < json.size() && json.at(end).isDigit()) {
    ++end;
  }
  bool ok = false;
  const int value = json.mid(index, end - index).toInt(&ok);
  require(ok, "integer value parses from json");
  return value;
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
  require(contains(map, "\"id\":\"control:agent_live_method\""),
          "UI map exposes agent live-query method input");
  require(contains(map, "\"id\":\"control:agent_live_payload\""),
          "UI map exposes agent live-query payload input");
  require(contains(map, "\"id\":\"action:agent_live_query\""),
          "UI map exposes agent live-query button");
  require(contains(map, "\"id\":\"tab:pcb\""), "UI map exposes PCB tab");
  require(contains(map, "\"id\":\"tab:schematic\""), "UI map exposes schematic tab");
  require(contains(map, "\"id\":\"tab:agent\""), "UI map exposes agent bottom tab");
  require(contains(map, "\"id\":\"canvas:pcb\""), "UI map exposes PCB canvas");
  require(contains(map, "\"id\":\"canvas_object:U1.1\""),
          "UI map exposes typed canvas object");
  require(contains(map, "\"object_id\":\"U1.1\""), "UI map keeps original object id");
  require(contains(map, "\"layer_id\":\"F.Cu\""), "UI map carries layer metadata");
  require(contains(map, "\"active_pcb_layer_id\":\"F.Cu\""),
          "UI map reports the default active PCB layer");
  require(contains(map, "\"id\":\"control:active_pcb_layer\""),
          "UI map exposes the active PCB layer selector");
  require(contains(map, "\"active_pcb_net_id\":\"N1\""),
          "UI map reports the default active PCB net");
  require(contains(map, "\"id\":\"control:active_pcb_net\""),
          "UI map exposes the active PCB net selector");
  require(contains(map, "\"target_x\":"), "UI map carries click target x coordinate");
  require(contains(map, "\"target_y\":"), "UI map carries click target y coordinate");

  const QString active_layer = window.activePcbLayerJson();
  require(contains(active_layer, "\"active_layer_id\":\"F.Cu\""),
          "agent active-layer query defaults to F.Cu");

  const QString active_net = window.activePcbNetJson();
  require(contains(active_net, "\"active_net_id\":\"N1\""),
          "agent active-net query defaults to the first board net");
  require(contains(active_net, "\"net_count\":2"), "agent active-net query reports net count");

  const int initial_epoch = extractInt(map, "\"ui_epoch\":");
  const QString current_delta = window.uiMapDeltaJson(initial_epoch);
  require(contains(current_delta, "\"changed\":false"),
          "UI map delta is empty for the current epoch");
  require(contains(current_delta, "\"nodes\":[]"),
          "UI map delta returns no changed nodes for the current epoch");
  const QString stale_delta = window.uiMapDeltaJson(initial_epoch - 1);
  require(contains(stale_delta, "\"changed\":true"),
          "UI map delta returns changed data for a stale epoch");
  require(contains(stale_delta, "\"nodes\":["), "stale UI map delta returns compact nodes");
  require(contains(stale_delta, "\"id\":\"action:add_footprint\""),
          "stale UI map delta includes current compact nodes");
  require(!contains(stale_delta, "\"map\":"),
          "stale UI map delta does not embed a full nested map");

  const QString compact_actions = window.uiMapCompactJson("action", 80);
  require(contains(compact_actions, "\"role\":\"action\""),
          "compact UI map includes action nodes");
  require(contains(compact_actions, "\"id\":\"action:add_footprint\""),
          "compact UI map can filter to add-footprint action");
  require(contains(compact_actions, "\"limit\":80"),
          "compact UI map reports normalized limit");
  require(!contains(compact_actions, "\"global_rect\":"),
          "compact UI map omits heavy target rectangles");

  const QString compact_canvas_objects = window.uiMapCompactJson("canvas_object", 20);
  require(contains(compact_canvas_objects, "\"id\":\"canvas_object:U1.1\""),
          "compact UI map returns canvas objects");
  require(contains(compact_canvas_objects, "\"object_id\":\"U1.1\""),
          "compact UI map preserves CAD object ids");
  require(contains(compact_canvas_objects, "\"layer_id\":\"F.Cu\""),
          "compact UI map preserves layer metadata");

  const QString role_summary = window.uiRoleSummaryJson();
  require(contains(role_summary, "\"role\":\"action\""),
          "UI role summary includes action role");
  require(contains(role_summary, "\"role\":\"canvas_object\""),
          "UI role summary includes canvas-object role");
  require(contains(role_summary, "\"total_node_count\":"),
          "UI role summary reports total node count");

  const QString find_actions = window.uiFindJson("add", "action", 6);
  require(contains(find_actions, "\"match_count\":"), "UI map find reports match count");
  require(contains(find_actions, "\"id\":\"action:add_footprint\""),
          "UI map find returns matching action IDs");
  require(!contains(find_actions, "\"id\":\"panel:agent\""),
          "UI map find respects the requested role");

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
  const int file_menu_x = extractInt(menu_target, "\"logical_x\":");
  const int file_menu_y = extractInt(menu_target, "\"logical_y\":");
  const QString file_hit = window.uiHitTestJson(file_menu_x, file_menu_y);
  require(contains(file_hit, "\"found\":true"), "UI hit-test finds a node under target");
  require(contains(file_hit, "\"id\":\"menu:file\""), "UI hit-test resolves File menu");

  const QString active_layer_target = window.uiTargetJsonById("control:active_pcb_layer");
  require(contains(active_layer_target, "\"found\":true"),
          "active layer selector is directly targetable");
  require(contains(active_layer_target, "\"role\":\"control\""),
          "active layer selector target reports control role");

  const QString active_net_target = window.uiTargetJsonById("control:active_pcb_net");
  require(contains(active_net_target, "\"found\":true"),
          "active net selector is directly targetable");
  require(contains(active_net_target, "\"role\":\"control\""),
          "active net selector target reports control role");

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
  window.triggerSafeUiActionJson("tab:agent");
  const QString agent_live_method_target =
      window.uiTargetJsonById("control:agent_live_method");
  require(contains(agent_live_method_target, "\"found\":true"),
          "target query finds agent live method control");
  require(contains(agent_live_method_target, "\"role\":\"control\""),
          "agent live method target reports control role");
  const QString agent_live_payload_target =
      window.uiTargetJsonById("control:agent_live_payload");
  require(contains(agent_live_payload_target, "\"found\":true"),
          "target query finds agent live payload control");
  const QString agent_live_query_target =
      window.uiTargetJsonById("action:agent_live_query");
  require(contains(agent_live_query_target, "\"found\":true"),
          "target query finds agent live query action");
  require(contains(agent_live_query_target, "\"role\":\"action\""),
          "agent live query target reports action role");
  window.triggerSafeUiActionJson("tab:pcb");

  const QString pad_target = window.uiTargetJsonById("canvas_object:U1.1");
  require(contains(pad_target, "\"found\":true"), "canvas object target query finds pad");
  require(contains(pad_target, "\"role\":\"canvas_object\""),
          "canvas object target query reports role");

  const QString board_point = window.uiTargetJsonForBoardPoint(8.0, 9.0);
  require(contains(board_point, "\"found\":true"), "board point target query finds point");
  require(contains(board_point, "\"space\":\"board\""), "board point target reports space");
  require(contains(board_point, "\"scene_x\":"), "board point target reports scene mapping");

  const QString nearest_pad = window.uiNearestCanvasObjectJson(8.0, 9.0, "canvas:pcb", 4);
  require(contains(nearest_pad, "\"found\":true"),
          "nearest canvas-object query finds a board object");
  require(contains(nearest_pad, "\"id\":\"canvas_object:U1.1\""),
          "nearest canvas-object query resolves the pad at the board point");
  require(contains(nearest_pad, "\"scene_distance\":"),
          "nearest canvas-object query reports scene distance");

  const QString dry_click =
      window.runAgentUiQueryJson("ui.click", "{\"id\":\"action:zoom_in\",\"dry_run\":true}");
  require(contains(dry_click, "\"ok\":true"), "agent click dry run routes successfully");
  require(contains(dry_click, "\"dry_run\":true"), "agent click dry run reports dry-run mode");
  require(contains(dry_click, "\"id\":\"action:zoom_in\""),
          "agent click dry run resolves target id");

  const QString zoom_click =
      window.runAgentUiQueryJson("ui.click", "{\"id\":\"action:zoom_in\"}");
  require(contains(zoom_click, "\"ok\":true"), "agent click routes to safe actions");
  require(contains(zoom_click, "\"performed\":true"), "agent click performs safe action");
  require(contains(zoom_click, "\"reason\":\"triggered\""),
          "agent click preserves safe-trigger reason");

  const QString unsafe_click = window.runAgentUiQueryJson("ui.click", "{\"id\":\"action:save\"}");
  require(contains(unsafe_click, "\"ok\":true"), "agent click reports unsafe actions structurally");
  require(contains(unsafe_click, "\"performed\":false"),
          "agent click refuses unsafe actions");
  require(contains(unsafe_click, "\"reason\":\"unsafe_action_requires_human_or_kernel_tool\""),
          "agent click keeps safe-trigger refusal reason");

  const QString double_click =
      window.runAgentUiQueryJson("ui.double_click", "{\"id\":\"menu:file\",\"dry_run\":true}");
  require(contains(double_click, "\"ok\":true"), "agent double-click dry run routes");
  require(contains(double_click, "\"dry_run\":true"), "agent double-click supports dry run");
  require(contains(double_click, "\"id\":\"menu:file\""),
          "agent double-click resolves target id");

  const QString agent_tab_click = window.runAgentUiQueryJson("ui.click", "{\"id\":\"tab:agent\"}");
  require(contains(agent_tab_click, "\"performed\":true"),
          "agent click can select the Agent tab");
  require(contains(agent_tab_click, "\"reason\":\"tab_selected\""),
          "agent click uses tab safe-trigger behavior");
  const QString type_method = window.runAgentUiQueryJson(
      "ui.type_text", "{\"id\":\"control:agent_live_method\",\"text\":\"ui.role_summary\"}");
  require(contains(type_method, "\"performed\":true"),
          "agent type_text writes into the live method control");
  require(contains(type_method, "\"value\":\"ui.role_summary\""),
          "agent type_text reports live method value");
  const QString type_payload = window.runAgentUiQueryJson(
      "ui.type_text", "{\"id\":\"control:agent_live_payload\",\"text\":\"{}\"}");
  require(contains(type_payload, "\"performed\":true"),
          "agent type_text writes into the live payload control");
  const QString live_query_click =
      window.runAgentUiQueryJson("ui.click", "{\"id\":\"action:agent_live_query\"}");
  require(contains(live_query_click, "\"performed\":true"),
          "agent click can trigger the Agent panel live-query button");
  require(contains(live_query_click, "\"reason\":\"button_clicked\""),
          "agent click reports direct button click");
  window.triggerSafeUiActionJson("tab:pcb");

  window.triggerSafeUiActionJson("action:add_tracks");
  const QString escape_key = window.runAgentUiQueryJson("ui.key", "{\"key\":\"Escape\"}");
  require(contains(escape_key, "\"performed\":true"), "agent key sends Escape");
  require(contains(escape_key, "\"reason\":\"escape_cancelled\""),
          "agent key reports Escape cancellation");
  require(contains(escape_key, "\"mode\":\"default\""), "agent key returns to default mode");

  const QString select_pad = window.runAgentUiQueryJson(
      "ui.select_canvas_object", "{\"id\":\"U1.1\",\"canvas\":\"canvas:pcb\"}");
  require(contains(select_pad, "\"performed\":true"),
          "agent can select a canvas object by stable id");
  require(contains(select_pad, "\"id\":\"canvas_object:U1.1\""),
          "agent canvas selection reports selected node id");
  const QString selection = window.runAgentUiQueryJson("ui.get_selection", "{}");
  require(contains(selection, "\"count\":1"), "agent selection query reports selected count");
  require(contains(selection, "\"object_id\":\"U1.1\""),
          "agent selection query reports selected object id");

  const QString epoch_wait = window.runAgentUiQueryJson(
      "ui.wait_for_epoch",
      QString("{\"minimum_epoch\":%1,\"timeout_ms\":20}").arg(initial_epoch));
  require(contains(epoch_wait, "\"ok\":true"), "agent wait-for-epoch routes");
  require(contains(epoch_wait, "\"reached\":true"), "agent wait-for-epoch reaches current epoch");

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

  const QString graphic_tool = window.triggerSafeUiActionJson("action:add_graphical_segments");
  require(contains(graphic_tool, "\"performed\":true"), "draw-graphic tool enters edit mode");
  require(contains(graphic_tool, "\"reason\":\"editor_tool_selected\""),
          "draw-graphic tool reports real editor-tool selection");
  require(contains(graphic_tool, "\"mode\":\"draw_graphic\""),
          "draw-graphic tool reports selected mode");

  const QString text_tool = window.triggerSafeUiActionJson("action:text");
  require(contains(text_tool, "\"performed\":true"), "place-text tool enters edit mode");
  require(contains(text_tool, "\"reason\":\"editor_tool_selected\""),
          "place-text tool reports real editor-tool selection");
  require(contains(text_tool, "\"mode\":\"place_text\""),
          "place-text tool reports selected mode");

  const QString zone_tool = window.triggerSafeUiActionJson("action:add_zone");
  require(contains(zone_tool, "\"performed\":true"), "add-zone tool enters edit mode");
  require(contains(zone_tool, "\"reason\":\"editor_tool_selected\""),
          "add-zone tool reports real editor-tool selection");
  require(contains(zone_tool, "\"mode\":\"add_zone\""),
          "add-zone tool reports selected mode");

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
  const QString delta_response =
      requestLine(socket, "{\"method\":\"ui.map_delta\",\"since_epoch\":0}");
  require(contains(delta_response, "\"ok\":true"),
          "live server returns successful ui.map_delta");
  require(contains(delta_response, "\"changed\":true"),
          "live server UI map delta reports stale clients");
  require(!contains(delta_response, "\"map\":"),
          "live server UI map delta avoids full nested maps");
  const QString compact_response =
      requestLine(socket, "{\"method\":\"ui.map_compact\",\"role\":\"action\",\"limit\":80}");
  require(contains(compact_response, "\"ok\":true"),
          "live server returns successful ui.map_compact");
  require(contains(compact_response, "\"id\":\"action:add_footprint\""),
          "live server compact map returns action IDs");
  require(!contains(compact_response, "\"global_rect\":"),
          "live server compact map omits heavy rectangles");
  const QString role_summary_response = requestLine(socket, "{\"method\":\"ui.role_summary\"}");
  require(contains(role_summary_response, "\"ok\":true"),
          "live server returns successful ui.role_summary");
  require(contains(role_summary_response, "\"role\":\"action\""),
          "live server role summary includes action role");
  const QString hit_response = requestLine(
      socket,
      QString("{\"method\":\"ui.hit_test\",\"x\":%1,\"y\":%2}").arg(file_menu_x).arg(file_menu_y));
  require(contains(hit_response, "\"ok\":true"), "live server returns successful ui.hit_test");
  require(contains(hit_response, "\"id\":\"menu:file\""),
          "live server hit-test resolves File menu");
  const QString nearest_response = requestLine(
      socket,
      "{\"method\":\"ui.nearest_canvas_object\",\"canvas\":\"canvas:pcb\",\"x_mm\":8,\"y_mm\":9,\"limit\":4}");
  require(contains(nearest_response, "\"ok\":true"),
          "live server returns successful ui.nearest_canvas_object");
  require(contains(nearest_response, "\"id\":\"canvas_object:U1.1\""),
          "live server nearest canvas-object resolves the first pad");
  const QString click_response =
      requestLine(socket, "{\"method\":\"ui.click\",\"id\":\"action:zoom_out\",\"dry_run\":true}");
  require(contains(click_response, "\"ok\":true"),
          "live server returns successful ui.click");
  require(contains(click_response, "\"dry_run\":true"),
          "live server ui.click supports dry run");
  const QString select_response =
      requestLine(socket, "{\"method\":\"ui.select_canvas_object\",\"id\":\"U1.1\"}");
  require(contains(select_response, "\"ok\":true"),
          "live server returns successful ui.select_canvas_object");
  require(contains(select_response, "\"performed\":true"),
          "live server selects a canvas object");
  const QString selection_response = requestLine(socket, "{\"method\":\"ui.get_selection\"}");
  require(contains(selection_response, "\"ok\":true"),
          "live server returns successful ui.get_selection");
  require(contains(selection_response, "\"object_id\":\"U1.1\""),
          "live server selection query reports selected object id");
  const QString epoch_wait_response =
      requestLine(socket, "{\"method\":\"ui.wait_for_epoch\",\"minimum_epoch\":1,\"timeout_ms\":20}");
  require(contains(epoch_wait_response, "\"ok\":true"),
          "live server returns successful ui.wait_for_epoch");
  require(contains(epoch_wait_response, "\"reached\":true"),
          "live server wait-for-epoch reaches current epoch");
  const QString find_response =
      requestLine(socket, "{\"method\":\"ui.find\",\"query\":\"add\",\"role\":\"action\",\"limit\":5}");
  require(contains(find_response, "\"ok\":true"), "live server returns successful ui.find");
  require(contains(find_response, "\"id\":\"action:add_footprint\""),
          "live server ui.find returns compact action matches");
  const QString target_response =
      requestLine(socket, "{\"method\":\"ui.target\",\"id\":\"menu:file\"}");
  require(contains(target_response, "\"ok\":true"), "live server returns successful ui.target");
  require(contains(target_response, "\"found\":true"), "live server target finds File menu");
  const QString board_point_response =
      requestLine(socket, "{\"method\":\"ui.target_board_point\",\"x_mm\":8,\"y_mm\":9}");
  require(contains(board_point_response, "\"ok\":true"),
          "live server returns successful ui.target_board_point");
  require(contains(board_point_response, "\"found\":true"),
          "live server board-point targeting finds an in-board point");
  const QString safe_trigger_response =
      requestLine(socket, "{\"method\":\"ui.trigger_safe\",\"id\":\"tab:agent\"}");
  require(contains(safe_trigger_response, "\"ok\":true"),
          "live server returns successful ui.trigger_safe");
  require(contains(safe_trigger_response, "\"performed\":true"),
          "live server safe trigger forwards allowlisted actions");
  const QString epoch_response = requestLine(socket, "{\"method\":\"ui.epoch\"}");
  require(contains(epoch_response, "\"ui_epoch\":"), "live server returns current UI epoch");
  const QString active_layer_response = requestLine(socket, "{\"method\":\"ui.active_layer\"}");
  require(contains(active_layer_response, "\"ok\":true"),
          "live server returns successful ui.active_layer");
  require(contains(active_layer_response, "\"active_layer_id\":\"F.Cu\""),
          "live server reports default active layer");
  const QString set_active_layer_response =
      requestLine(socket, "{\"method\":\"ui.set_active_layer\",\"layer_id\":\"B.Cu\"}");
  require(contains(set_active_layer_response, "\"ok\":true"),
          "live server returns successful ui.set_active_layer");
  require(contains(set_active_layer_response, "\"active_layer_id\":\"B.Cu\""),
          "live server sets active layer");
  const QString active_layer_after_set =
      requestLine(socket, "{\"method\":\"ui.active_layer\"}");
  require(contains(active_layer_after_set, "\"active_layer_id\":\"B.Cu\""),
          "live server active-layer query reflects server-side set");
  const QString active_net_response = requestLine(socket, "{\"method\":\"ui.active_net\"}");
  require(contains(active_net_response, "\"ok\":true"),
          "live server returns successful ui.active_net");
  require(contains(active_net_response, "\"active_net_id\":\"N1\""),
          "live server reports default active net");
  const QString set_active_net_response =
      requestLine(socket, "{\"method\":\"ui.set_active_net\",\"net_id\":\"N2\"}");
  require(contains(set_active_net_response, "\"ok\":true"),
          "live server returns successful ui.set_active_net");
  require(contains(set_active_net_response, "\"active_net_id\":\"N2\""),
          "live server sets active net");
  const QString active_net_after_set = requestLine(socket, "{\"method\":\"ui.active_net\"}");
  require(contains(active_net_after_set, "\"active_net_id\":\"N2\""),
          "live server active-net query reflects server-side set");
  const QString bad_response = requestLine(socket, "{\"method\":\"ui.unknown\"}");
  require(contains(bad_response, "\"ok\":false"), "live server refuses unsupported methods");
  socket.disconnectFromServer();
  server.close();

  const QString delete_selected = window.triggerSafeUiActionJson("action:delete_cursor");
  require(contains(delete_selected, "\"performed\":true"),
          "delete action removes the selected board object");
  require(contains(delete_selected, "\"reason\":\"deleted\""),
          "delete action reports deletion");
  require(contains(delete_selected, "\"deleted_type\":\"pad\""),
          "delete action reports deleted object type");
  require(!contains(window.uiMapJson(), "\"id\":\"canvas_object:U1.1\""),
          "UI map drops the deleted pad");

  const QString missing_layer = window.setActivePcbLayerForAutomation("NOPE");
  require(contains(missing_layer, "\"performed\":false"),
          "active layer setter rejects missing layers");
  require(contains(missing_layer, "\"reason\":\"layer_not_found\""),
          "active layer setter reports missing layer");
  const QString invalid_layer = window.setActivePcbLayerForAutomation("F.SilkS");
  require(contains(invalid_layer, "\"performed\":false"),
          "active layer setter rejects non-copper board layers");
  require(contains(invalid_layer, "\"reason\":\"non_copper_layer\""),
          "active layer setter reports non-copper layer");

  const QString set_back_layer = window.setActivePcbLayerForAutomation("B.Cu");
  require(contains(set_back_layer, "\"performed\":true"),
          "active layer setter accepts B.Cu");
  require(contains(set_back_layer, "\"active_layer_id\":\"B.Cu\""),
          "active layer setter reports selected layer");
  require(contains(window.activePcbLayerJson(), "\"active_layer_id\":\"B.Cu\""),
          "agent active-layer query reflects B.Cu");
  require(contains(window.uiMapJson(), "\"active_pcb_layer_id\":\"B.Cu\""),
          "UI map reflects the selected active layer");

  const QString invalid_net = window.setActivePcbNetForAutomation("NOPE");
  require(contains(invalid_net, "\"performed\":false"), "active net setter rejects unknown nets");
  require(contains(invalid_net, "\"reason\":\"net_not_found\""),
          "active net setter reports missing net");

  const QString set_net = window.setActivePcbNetForAutomation("N2");
  require(contains(set_net, "\"performed\":true"), "active net setter accepts N2");
  require(contains(set_net, "\"active_net_id\":\"N2\""),
          "active net setter reports selected net");
  require(contains(window.activePcbNetJson(), "\"active_net_id\":\"N2\""),
          "agent active-net query reflects N2");
  require(contains(window.uiMapJson(), "\"active_pcb_net_id\":\"N2\""),
          "UI map reflects the selected active net");

  const QString placement =
      window.commitFootprintPlacementForAutomation(writeFootprintFixture(), 12.0, 10.0);
  require(contains(placement, "\"performed\":true"), "automation footprint placement succeeds");
  require(contains(placement, "\"reason\":\"placed\""), "automation placement reports placed");
  require(contains(placement, "\"pad_count\":1"), "automation placement adds one pad");
  const ccad::Project after_placement = ccad::loadProjectJson(readFile(project_path));
  require(after_placement.board.has_value(), "placed fixture still has a board");
  require(!after_placement.board->pads.empty(), "placed footprint adds a pad to the file");
  require(after_placement.board->pads.back().layers.front() == "B.Cu",
          "footprint placement writes the selected B.Cu active layer");

  const QString via = window.commitViaPlacementForAutomation(15.0, 11.0);
  require(contains(via, "\"performed\":true"), "automation via placement succeeds");
  require(contains(via, "\"reason\":\"placed\""), "automation via placement reports placed");
  require(contains(via, "\"via_count\":1"), "automation via placement adds one via");
  require(contains(window.uiMapJson(), "\"id\":\"canvas_object:V1\""),
          "UI map exposes placed via");
  const ccad::Project after_via = ccad::loadProjectJson(readFile(project_path));
  require(after_via.board.has_value(), "via fixture still has a board");
  require(!after_via.board->vias.empty(), "via placement writes a via to the file");
  require(after_via.board->vias.back().net_id == "N2",
          "via placement writes the selected active net");

  const QString track = window.commitTrackPlacementForAutomation(8.0, 9.0, 15.0, 11.0);
  require(contains(track, "\"performed\":true"), "automation track routing succeeds");
  require(contains(track, "\"reason\":\"placed\""), "automation track routing reports placed");
  require(contains(track, "\"track_count\":1"), "automation track routing adds one track");
  require(contains(window.uiMapJson(), "\"id\":\"canvas_object:T1\""),
          "UI map exposes placed track");
  const ccad::Project after_track = ccad::loadProjectJson(readFile(project_path));
  require(after_track.board.has_value(), "track fixture still has a board");
  require(!after_track.board->tracks.empty(), "track routing writes a track to the file");
  require(after_track.board->tracks.back().layer_id == "B.Cu",
          "route track writes the selected B.Cu active layer");
  require(after_track.board->tracks.back().net_id == "N2",
          "route track writes the selected active net");

  const QString keepout = window.commitKeepoutPlacementForAutomation(20.0, 10.0, 25.0, 14.0);
  require(contains(keepout, "\"performed\":true"), "automation keepout placement succeeds");
  require(contains(keepout, "\"reason\":\"placed\""),
          "automation keepout placement reports placed");
  require(contains(keepout, "\"keepout_count\":1"),
          "automation keepout placement adds one keepout");
  require(contains(window.uiMapJson(), "\"id\":\"canvas_object:K1\""),
          "UI map exposes placed keepout");

  const QString zone = window.commitZonePlacementForAutomation(2.0, 2.0, 20.0, 12.0);
  require(contains(zone, "\"performed\":true"), "automation zone placement succeeds");
  require(contains(zone, "\"reason\":\"placed\""),
          "automation zone placement reports placed");
  require(contains(zone, "\"zone_count\":1"), "automation zone placement adds one zone");
  require(contains(window.uiMapJson(), "\"id\":\"canvas_object:Z1\""),
          "UI map exposes placed zone");
  const ccad::Project after_zone = ccad::loadProjectJson(readFile(project_path));
  require(after_zone.board.has_value(), "zone fixture still has a board");
  require(!after_zone.board->zones.empty(), "zone placement writes a zone to the file");
  require(after_zone.board->zones.back().layer_ids.size() == 1,
          "zone placement writes the active copper layer set");
  require(after_zone.board->zones.back().net_id == "N2",
          "zone placement writes the selected active net");

  const QString graphic =
      window.commitGraphicLinePlacementForAutomation(3.0, 4.0, 16.0, 4.0);
  require(contains(graphic, "\"performed\":true"), "automation graphic line placement succeeds");
  require(contains(graphic, "\"reason\":\"placed\""),
          "automation graphic line placement reports placed");
  require(contains(graphic, "\"graphic_count\":1"),
          "automation graphic line placement adds one graphic");
  require(contains(window.uiMapJson(), "\"id\":\"canvas_object:G1\""),
          "UI map exposes placed board graphic");
  const ccad::Project after_graphic = ccad::loadProjectJson(readFile(project_path));
  require(after_graphic.board.has_value(), "graphic fixture still has a board");
  require(!after_graphic.board->graphics.empty(), "graphic placement writes a graphic to the file");
  require(after_graphic.board->graphics.back().layer_id == "Dwgs.User",
          "graphic placement writes the default drawing layer");

  const QString text = window.commitBoardTextPlacementForAutomation("GUI TEXT", 8.0, 22.0);
  require(contains(text, "\"performed\":true"), "automation board text placement succeeds");
  require(contains(text, "\"reason\":\"placed\""),
          "automation board text placement reports placed");
  require(contains(text, "\"text_count\":1"),
          "automation board text placement adds one text object");
  require(contains(window.uiMapJson(), "\"id\":\"canvas_object:BT1\""),
          "UI map exposes placed board text");
  const ccad::Project after_text = ccad::loadProjectJson(readFile(project_path));
  require(after_text.board.has_value(), "text fixture still has a board");
  require(!after_text.board->texts.empty(), "text placement writes text to the file");
  require(after_text.board->texts.back().layer_id == "F.SilkS",
          "text placement writes the default silkscreen layer");
  require(after_text.board->texts.back().text == "GUI TEXT",
          "text placement writes the requested label");

  const QString delete_graphic = window.deleteBoardObjectForAutomation("G1");
  require(contains(delete_graphic, "\"performed\":true"), "automation delete removes graphic");
  require(contains(delete_graphic, "\"deleted_type\":\"graphic\""),
          "automation delete reports graphic type");
  require(!contains(window.uiMapJson(), "\"id\":\"canvas_object:G1\""),
          "UI map no longer exposes deleted graphic");

  const QString delete_text = window.deleteBoardObjectForAutomation("BT1");
  require(contains(delete_text, "\"performed\":true"), "automation delete removes board text");
  require(contains(delete_text, "\"deleted_type\":\"text\""),
          "automation delete reports text type");
  require(!contains(window.uiMapJson(), "\"id\":\"canvas_object:BT1\""),
          "UI map no longer exposes deleted board text");

  const QString delete_zone = window.deleteBoardObjectForAutomation("Z1");
  require(contains(delete_zone, "\"performed\":true"), "automation delete removes zone");
  require(contains(delete_zone, "\"deleted_type\":\"zone\""),
          "automation delete reports zone type");
  require(!contains(window.uiMapJson(), "\"id\":\"canvas_object:Z1\""),
          "UI map no longer exposes deleted zone");

  ccad::Project hidden_user_layer_project = uiMapProject();
  for (ccad::Layer& layer : hidden_user_layer_project.board->layers) {
    if (layer.id == "Dwgs.User") {
      layer.visible = false;
    }
  }
  const std::filesystem::path hidden_user_layer_path =
      writeProjectFixture(hidden_user_layer_project);
  ReviewWindow hidden_user_layer_window;
  hidden_user_layer_window.loadProjectPath(hidden_user_layer_path);
  QApplication::processEvents();
  const QString hidden_user_layer_graphic =
      hidden_user_layer_window.commitGraphicLinePlacementForAutomation(3.0, 4.0, 16.0, 4.0);
  require(contains(hidden_user_layer_graphic, "\"performed\":true"),
          "automation graphic placement succeeds when Dwgs.User is hidden");
  const ccad::Project hidden_user_layer_after =
      ccad::loadProjectJson(readFile(hidden_user_layer_path));
  require(hidden_user_layer_after.board.has_value(), "hidden-layer fixture still has a board");
  require(!hidden_user_layer_after.board->graphics.empty(),
          "hidden-layer graphic placement writes a graphic");
  require(hidden_user_layer_after.board->graphics.back().layer_id == "F.SilkS",
          "graphic placement avoids hidden Dwgs.User by using visible silkscreen");

  const QString delete_via = window.deleteBoardObjectForAutomation("V1");
  require(contains(delete_via, "\"performed\":true"), "automation delete removes via");
  require(contains(delete_via, "\"reason\":\"deleted\""), "automation delete reports deletion");
  require(contains(delete_via, "\"deleted_type\":\"via\""),
          "automation delete reports deleted type");
  require(!contains(window.uiMapJson(), "\"id\":\"canvas_object:V1\""),
          "UI map no longer exposes deleted via");
}
