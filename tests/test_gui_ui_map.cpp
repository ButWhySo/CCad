#include "ccad_core/model.hpp"
#include "ccad_core/kicad_footprint_import.hpp"
#include "ccad_core/serialize.hpp"
#include "ccad_gui/review_window.hpp"
#include "ccad_gui/ui_map_server.hpp"
#include "test_support.hpp"

#include <QApplication>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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

QString jsonStringLiteral(const QString& value) {
  QString escaped;
  escaped.reserve(value.size() + 2);
  escaped.append('"');
  for (const QChar ch : value) {
    if (ch == '\\') {
      escaped.append("\\\\");
    } else if (ch == '"') {
      escaped.append("\\\"");
    } else if (ch == '\n') {
      escaped.append("\\n");
    } else if (ch == '\r') {
      escaped.append("\\r");
    } else if (ch == '\t') {
      escaped.append("\\t");
    } else {
      escaped.append(ch);
    }
  }
  escaped.append('"');
  return escaped;
}

QString jsonStringLiteral(const std::filesystem::path& path) {
  return jsonStringLiteral(QString::fromStdString(path.generic_string()));
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

QJsonObject jsonObjectFromLine(const QString& json) {
  QJsonParseError error{};
  const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8(), &error);
  require(error.error == QJsonParseError::NoError, "json parses");
  require(document.isObject(), "json root is an object");
  return document.object();
}

QJsonObject nodeById(const QString& map_json, const QString& id) {
  const QJsonObject map = jsonObjectFromLine(map_json);
  const QJsonArray nodes = map.value("nodes").toArray();
  for (const QJsonValue& node_value : nodes) {
    const QJsonObject node = node_value.toObject();
    if (node.value("id").toString() == id) {
      return node;
    }
  }
  require(false, "ui map node exists");
  return {};
}

int nodeRectValue(const QJsonObject& node, const char* key) {
  const QJsonObject rect = node.value("global_rect").toObject();
  require(rect.contains(key), "ui map node has requested rect key");
  return rect.value(key).toInt();
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
  require(contains(map, "\"id\":\"panel:agent_session_strip\""),
          "UI map exposes the agent session strip");
  require(contains(map, "\"id\":\"panel:agent_mode_strip\""),
          "UI map exposes the agent mode strip");
  require(contains(map, "\"id\":\"panel:agent_run_controls\""),
          "UI map exposes the agent run-control strip");
  require(contains(map, "\"id\":\"label:agent_run_state_chip\""),
          "UI map exposes the agent run-state chip");
  require(contains(map, "\"id\":\"label:agent_trace_chip\""),
          "UI map exposes the agent trace chip");
  require(contains(map, "\"id\":\"label:agent_session_chip\""),
          "UI map exposes the agent session chip");
  require(contains(map, "\"id\":\"panel:agent_trace_strip\""),
          "UI map exposes the agent trace/session strip");
  require(contains(map, "\"id\":\"action:agent_pause_run\""),
          "UI map exposes the agent pause action");
  require(contains(map, "\"id\":\"action:agent_resume_run\""),
          "UI map exposes the agent resume action");
  require(contains(map, "\"id\":\"action:agent_stop_run\""),
          "UI map exposes the agent stop action");
  require(contains(map, "\"id\":\"panel:agent_active_plan\""),
          "UI map exposes the agent active-plan section");
  require(contains(map, "\"id\":\"panel:agent_plan_row_1\""),
          "UI map exposes the first agent plan row");
  require(contains(map, "\"id\":\"panel:agent_activity_stream\""),
          "UI map exposes the agent activity stream");
  require(contains(map, "\"id\":\"tab:agent_command\""),
          "UI map exposes the agent command tab selector");
  require(contains(map, "\"id\":\"tab:agent_evidence\""),
          "UI map exposes the agent evidence tab selector");
  require(contains(map, "\"id\":\"tab:agent_approvals\""),
          "UI map exposes the agent approvals tab selector");
  require(contains(map, "\"id\":\"label:agent_permission_chip\""),
          "UI map exposes the agent permission chip");
  require(contains(map, "\"id\":\"control:agent_command_input\""),
          "UI map exposes agent command input");
  require(contains(map, "\"id\":\"action:agent_submit_command\""),
          "UI map exposes agent command submit action");
  require(contains(map, "\"id\":\"action:agent_header_request_context\""),
          "UI map exposes agent header context action");
  require(contains(map, "\"id\":\"action:agent_header_trigger_drc\""),
          "UI map exposes agent header diagnostics action");
  require(contains(map, "\"id\":\"action:agent_header_clear_output\""),
          "UI map exposes agent header clear action");
  require(contains(map, "\"id\":\"action:agent_footer_request_context\""),
          "UI map exposes visible agent request-context footer action");
  require(contains(map, "\"id\":\"action:agent_footer_trigger_drc\""),
          "UI map exposes visible agent DRC footer action");
  require(contains(map, "\"id\":\"control:agent_live_method\""),
          "UI map exposes agent live-query method input");
  require(contains(map, "\"id\":\"control:agent_live_payload\""),
          "UI map exposes agent live-query payload input");
  require(contains(map, "\"id\":\"action:agent_live_query\""),
          "UI map exposes agent live-query button");
  require(contains(map, "\"id\":\"action:agent_preset_harness_context\""),
          "UI map exposes agent harness-context preset");
  require(contains(map, "\"id\":\"action:agent_preset_project_diagnostics\""),
          "UI map exposes agent diagnostics preset");
  require(contains(map, "\"id\":\"action:agent_preset_tool_guide\""),
          "UI map exposes agent tool-guide preset");
  require(contains(map, "\"id\":\"action:agent_clear_output\""),
          "UI map exposes agent clear-output action");
  require(contains(map, "\"id\":\"control:agent_goal\""),
          "UI map exposes agent goal input");
  require(contains(map, "\"id\":\"action:agent_stage_goal\""),
          "UI map exposes agent stage-goal action");
  require(contains(map, "\"id\":\"action:agent_pin_evidence\""),
          "UI map exposes agent pin-evidence action");
  require(contains(map, "\"id\":\"action:agent_clear_evidence\""),
          "UI map exposes agent clear-evidence action");
  require(contains(map, "\"id\":\"control:agent_approval_request\""),
          "UI map exposes agent approval request input");
  require(contains(map, "\"id\":\"action:agent_request_approval\""),
          "UI map exposes request-approval action");
  require(contains(map, "\"id\":\"action:agent_approve_next\""),
          "UI map exposes approve-next action");
  require(contains(map, "\"id\":\"action:agent_decline_next\""),
          "UI map exposes decline-next action");
  require(contains(map, "\"id\":\"action:agent_cancel_approval\""),
          "UI map exposes cancel-approval action");
  require(contains(map, "\"id\":\"action:agent_clear_approvals\""),
          "UI map exposes clear-approvals action");
  require(contains(map, "\"id\":\"tab:pcb\""), "UI map exposes PCB tab");
  require(contains(map, "\"id\":\"tab:schematic\""), "UI map exposes schematic tab");
  require(contains(map, "\"id\":\"tab:agent\""), "UI map exposes agent dock tab alias");
  require(contains(map, "\"dock_area\":\"right\""),
          "UI map reports the Agent pane as a right-side dock");
  const QJsonObject layers_node = nodeById(map, "panel:layers_objects");
  const QJsonObject agent_node = nodeById(map, "panel:agent");
  require(nodeRectValue(agent_node, "x") > nodeRectValue(layers_node, "x"),
          "Agent pane is a right-side column beside Layers/Objects, not stacked below it");
  require(nodeRectValue(agent_node, "height") >= nodeRectValue(layers_node, "height") - 24,
          "Agent pane is a full-height vertical workspace beside Layers/Objects");
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

  const int before_agent_tab_epoch = extractInt(window.uiMapJson(), "\"ui_epoch\":");
  const QString agent_tab_delta_trigger = window.triggerSafeUiActionJson("tab:agent");
  require(contains(agent_tab_delta_trigger, "\"performed\":true"),
          "tab trigger switches to Agent tab before delta check");
  const QString agent_tab_delta = window.uiMapDeltaJson(before_agent_tab_epoch);
  require(contains(agent_tab_delta, "\"changed\":true"),
          "dirty UI map delta reports Agent tab change");
  require(contains(agent_tab_delta, "\"dirty_node_count\":"),
          "dirty UI map delta reports dirty node count");
  require(contains(agent_tab_delta, "\"changed_roles\":["),
          "dirty UI map delta reports changed roles");
  require(contains(agent_tab_delta, "\"id\":\"tab:agent\""),
          "dirty UI map delta includes the changed Agent tab");
  require(contains(agent_tab_delta, "\"id\":\"panel:agent\""),
          "dirty UI map delta includes the changed Agent panel");
  require(!contains(agent_tab_delta, "\"id\":\"canvas_object:U1.1\""),
          "dirty UI map delta avoids unrelated canvas objects");

  const QString wait_delta_current = window.runAgentUiQueryJson(
      "ui.wait_for_delta",
      QString("{\"since_epoch\":%1,\"timeout_ms\":1}").arg(extractInt(window.uiMapJson(),
                                                                     "\"ui_epoch\":")));
  require(contains(wait_delta_current, "\"ok\":true"),
          "agent wait-for-delta query routes successfully");
  require(contains(wait_delta_current, "\"changed\":false"),
          "agent wait-for-delta returns unchanged for the current epoch");
  window.triggerSafeUiActionJson("tab:pcb");

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

  const QString index_stats = window.runAgentUiQueryJson("ui.index_stats", "{}");
  require(contains(index_stats, "\"ok\":true"), "agent UI index stats route successfully");
  require(contains(index_stats, "\"cache_state\":\"fresh\""),
          "agent UI index stats report a fresh cache");
  require(contains(index_stats, "\"id_index_count\":"),
          "agent UI index stats report ID index size");
  require(contains(index_stats, "\"role_index_count\":"),
          "agent UI index stats report role index size");
  require(contains(index_stats, "\"canvas_object_count\":"),
          "agent UI index stats report canvas-object count");

  const QString indexed_node =
      window.runAgentUiQueryJson("ui.get_node", "{\"id\":\"tab:agent\"}");
  require(contains(indexed_node, "\"ok\":true"), "agent indexed node query routes");
  require(contains(indexed_node, "\"found\":true"), "agent indexed node query finds a node");
  require(contains(indexed_node, "\"lookup_kind\":\"id_index\""),
          "agent indexed node query reports ID-index lookup");
  require(contains(indexed_node, "\"id\":\"tab:agent\""),
          "agent indexed node query returns the requested node");

  const QString role_nodes =
      window.runAgentUiQueryJson("ui.nodes_by_role", "{\"role\":\"action\",\"limit\":80}");
  require(contains(role_nodes, "\"ok\":true"), "agent role-index query routes");
  require(contains(role_nodes, "\"lookup_kind\":\"role_index\""),
          "agent role-index query reports role-index lookup");
  require(contains(role_nodes, "\"id\":\"action:add_footprint\""),
          "agent role-index query returns action nodes");

  const QString method_catalog = window.runAgentUiQueryJson("agent.methods", "{}");
  require(contains(method_catalog, "\"ok\":true"), "agent method catalog query routes");
  require(contains(method_catalog, "\"schema_version\":1"),
          "agent method catalog reports schema version");
  require(contains(method_catalog, "\"method\":\"ui.map\""),
          "agent method catalog includes UI map query");
  require(contains(method_catalog, "\"method\":\"ui.watch_delta\""),
          "agent method catalog includes watch-delta query");
  require(contains(method_catalog, "\"method\":\"agent.workspace_state\""),
          "agent method catalog includes workspace-state query");
  require(contains(method_catalog, "\"method\":\"project.drc\""),
          "agent method catalog includes DRC query");
  require(contains(method_catalog, "\"read_only\":true"),
          "agent method catalog marks read-only calls");
  require(contains(method_catalog, "\"supports_dry_run\":true"),
          "agent method catalog marks dry-run capable calls");

  const QString watch_schema =
      window.runAgentUiQueryJson("agent.method_schema", "{\"method\":\"ui.watch_delta\"}");
  require(contains(watch_schema, "\"ok\":true"), "agent method schema query routes");
  require(contains(watch_schema, "\"found\":true"),
          "agent method schema finds a known method");
  require(contains(watch_schema, "\"method\":\"ui.watch_delta\""),
          "agent method schema returns the requested method");
  require(contains(watch_schema, "\"since_epoch\""),
          "agent method schema documents watch-delta epoch input");
  require(contains(watch_schema, "\"timeout_ms\""),
          "agent method schema documents watch-delta timeout input");
  require(contains(watch_schema, "\"max_events\""),
          "agent method schema documents watch-delta event bound input");

  const QString missing_schema =
      window.runAgentUiQueryJson("agent.method_schema", "{\"method\":\"ui.nope\"}");
  require(contains(missing_schema, "\"ok\":true"), "agent method schema handles misses");
  require(contains(missing_schema, "\"found\":false"),
          "agent method schema reports unknown methods without failing the request");

  const QString quickstart = window.runAgentUiQueryJson("agent.quickstart", "{}");
  require(contains(quickstart, "\"ok\":true"), "agent quickstart query routes");
  require(contains(quickstart, "\"workflow\":\"ui_map_agent_loop\""),
          "agent quickstart reports the UI-map workflow");
  require(contains(quickstart, "\"ui.index_stats\""),
          "agent quickstart points agents to indexed lookup first");
  require(contains(quickstart, "\"ui.watch_delta\""),
          "agent quickstart points agents to live delta watching");
  require(contains(quickstart, "\"ui.screenshot\""),
          "agent quickstart explains screenshot use");

  const QString harness_context = window.runAgentUiQueryJson("agent.harness_context", "{}");
  require(contains(harness_context, "\"ok\":true"), "agent harness context query routes");
  require(contains(harness_context, "\"harness_kind\":\"ccad_native_qt_agent_surface\""),
          "agent harness context identifies the native Qt harness surface");
  require(contains(harness_context, "\"session_state\""),
          "agent harness context includes durable-session state shape");
  require(contains(harness_context, "\"pending_diagnostics\""),
          "agent harness context includes pending diagnostic counts");
  require(contains(harness_context, "\"last_verified_visual_artifact\""),
          "agent harness context reserves the last verified visual artifact field");

  const QString run_profile = window.runAgentUiQueryJson("agent.run_profile", "{}");
  require(contains(run_profile, "\"ok\":true"), "agent run profile query routes");
  require(contains(run_profile, "\"durability_reference\":\"langgraph\""),
          "agent run profile records LangGraph as the durability reference");
  require(contains(run_profile, "\"retry_policy\""),
          "agent run profile includes retry policy metadata");
  require(contains(run_profile, "\"circuit_breaker\""),
          "agent run profile includes circuit-breaker metadata");
  require(contains(run_profile, "\"stop_conditions\""),
          "agent run profile includes stop conditions");

  const QString safety_policy = window.runAgentUiQueryJson("agent.safety_policy", "{}");
  require(contains(safety_policy, "\"ok\":true"), "agent safety policy query routes");
  require(contains(safety_policy, "\"human_approval_required\""),
          "agent safety policy names human approval gates");
  require(contains(safety_policy, "\"env_or_os_credential_store_only\""),
          "agent safety policy keeps secrets out of project files");
  require(contains(safety_policy, "\"no_project_file_execution\""),
          "agent safety policy forbids executing design files");

  const QString provider_policy = window.runAgentUiQueryJson("agent.provider_policy", "{}");
  require(contains(provider_policy, "\"ok\":true"), "agent provider policy query routes");
  require(contains(provider_policy, "\"byok\""),
          "agent provider policy exposes BYOK as the intended model path");
  require(contains(provider_policy, "\"no_consumer_web_ui_automation\""),
          "agent provider policy rejects consumer web UI automation as a clean integration path");
  require(contains(provider_policy, "\"local_model_server\""),
          "agent provider policy includes local model servers");

  const QString workspace_state_initial =
      window.runAgentUiQueryJson("agent.workspace_state", "{}");
  require(contains(workspace_state_initial, "\"ok\":true"),
          "agent workspace state query routes");
  require(contains(workspace_state_initial, "\"evidence_count\":0"),
          "agent workspace state starts with no pinned evidence");
  require(contains(workspace_state_initial, "\"approval_pending_count\":0"),
          "agent workspace state starts with no pending approvals");

  const QString observability_config =
      window.runAgentUiQueryJson("agent.observability_config", "{}");
  require(contains(observability_config, "\"ok\":true"),
          "agent observability config query routes");
  require(contains(observability_config, "\"opentelemetry\""),
          "agent observability config names OpenTelemetry");
  require(contains(observability_config, "\"langfuse\""),
          "agent observability config names Langfuse as an OTEL backend option");
  require(contains(observability_config, "\"redaction_policy\""),
          "agent observability config includes redaction policy");

  const QString evidence_schema =
      window.runAgentUiQueryJson("agent.evidence_manifest_schema", "{}");
  require(contains(evidence_schema, "\"ok\":true"), "agent evidence schema query routes");
  require(contains(evidence_schema, "\"ccad_agent_evidence_manifest\""),
          "agent evidence schema identifies the manifest kind");
  require(contains(evidence_schema, "\"screenshots\""),
          "agent evidence schema tracks screenshot artifacts");
  require(contains(evidence_schema, "\"drc_reports\""),
          "agent evidence schema tracks DRC artifacts");
  require(contains(evidence_schema, "\"source_references\""),
          "agent evidence schema tracks external references");
  require(contains(evidence_schema, "\"evidence_cards\""),
          "agent evidence schema documents workspace evidence cards");
  require(contains(evidence_schema, "\"card_fields\""),
          "agent evidence schema documents card fields");
  require(contains(evidence_schema, "\"artifact_path\""),
          "agent evidence schema documents artifact path fields");
  require(contains(evidence_schema, "\"trace_id\""),
          "agent evidence schema includes trace-ready fields");

  const QString route_track_guide =
      window.runAgentUiQueryJson("agent.tool_guide", "{\"method_name\":\"ui.route_track\"}");
  require(contains(route_track_guide, "\"ok\":true"), "agent tool guide query routes");
  require(contains(route_track_guide, "\"found\":true"),
          "agent tool guide finds a known method");
  require(contains(route_track_guide, "\"method\":\"ui.route_track\""),
          "agent tool guide names the requested method");
  require(contains(route_track_guide, "\"preferred_surface\""),
          "agent tool guide includes preferred surface guidance");
  require(contains(route_track_guide, "\"verification\""),
          "agent tool guide includes verification guidance");
  require(contains(route_track_guide, "\"recovery_loop\""),
          "agent tool guide includes recovery-loop guidance");

  const QString missing_guide =
      window.runAgentUiQueryJson("agent.tool_guide", "{\"method_name\":\"agent.nope\"}");
  require(contains(missing_guide, "\"ok\":true"), "agent tool guide handles misses");
  require(contains(missing_guide, "\"found\":false"),
          "agent tool guide reports unknown methods without failing the request");

  window.triggerSafeUiActionJson("tab:transactions");
  const int before_watch_epoch = extractInt(window.uiMapJson(), "\"ui_epoch\":");
  window.triggerSafeUiActionJson("tab:diagnostics");
  window.triggerSafeUiActionJson("tab:agent");
  const QString watched_delta = window.runAgentUiQueryJson(
      "ui.watch_delta",
      QString("{\"since_epoch\":%1,\"timeout_ms\":1,\"max_events\":4}").arg(before_watch_epoch));
  require(contains(watched_delta, "\"ok\":true"), "agent watch-delta query routes");
  require(contains(watched_delta, "\"event_count\":2"),
          "agent watch-delta returns bounded dirty event history");
  require(contains(watched_delta, "\"from_epoch\":"),
          "agent watch-delta reports per-event source epochs");
  require(contains(watched_delta, "\"id\":\"tab:agent\""),
          "agent watch-delta includes Agent tab dirty nodes");
  const QString aggregate_delta = window.uiMapDeltaJson(before_watch_epoch);
  require(contains(aggregate_delta, "\"dirty_event_count\":2"),
          "UI map delta reports aggregated dirty event count");
  require(contains(aggregate_delta, "\"id\":\"tab:agent\""),
          "UI map delta aggregates dirty history since the requested epoch");

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
  require(contains(agent_panel_target, "\"dock_area\":\"right\""),
          "agent panel target reports right dock placement");

  const QString agent_tab_target = window.uiTargetJsonById("tab:agent");
  require(contains(agent_tab_target, "\"found\":true"), "tab target query finds agent tab");
  require(contains(agent_tab_target, "\"role\":\"tab\""), "agent tab target query reports role");
  window.triggerSafeUiActionJson("tab:agent");
  const QString agent_session_strip_target =
      window.uiTargetJsonById("panel:agent_session_strip");
  require(contains(agent_session_strip_target, "\"found\":true"),
          "target query finds agent session strip");
  require(contains(agent_session_strip_target, "\"role\":\"panel\""),
          "agent session strip target reports panel role");
  const QString agent_mode_strip_target = window.uiTargetJsonById("panel:agent_mode_strip");
  require(contains(agent_mode_strip_target, "\"found\":true"),
          "target query finds agent mode strip");
  const QString agent_run_controls_target =
      window.uiTargetJsonById("panel:agent_run_controls");
  require(contains(agent_run_controls_target, "\"found\":true"),
          "target query finds agent run-control strip");
  const QString agent_run_state_target =
      window.uiTargetJsonById("label:agent_run_state_chip");
  require(contains(agent_run_state_target, "\"found\":true"),
          "target query finds agent run-state chip");
  const QString agent_trace_chip_target = window.uiTargetJsonById("label:agent_trace_chip");
  require(contains(agent_trace_chip_target, "\"found\":true"),
          "target query finds agent trace chip");
  const QString agent_trace_strip_target = window.uiTargetJsonById("panel:agent_trace_strip");
  require(contains(agent_trace_strip_target, "\"found\":true"),
          "target query finds agent trace/session strip");
  const QString agent_pause_target = window.uiTargetJsonById("action:agent_pause_run");
  require(contains(agent_pause_target, "\"found\":true"),
          "target query finds agent pause action");
  const QString agent_resume_target = window.uiTargetJsonById("action:agent_resume_run");
  require(contains(agent_resume_target, "\"found\":true"),
          "target query finds agent resume action");
  const QString agent_stop_target = window.uiTargetJsonById("action:agent_stop_run");
  require(contains(agent_stop_target, "\"found\":true"),
          "target query finds agent stop action");
  const QString agent_plan_target = window.uiTargetJsonById("panel:agent_active_plan");
  require(contains(agent_plan_target, "\"found\":true"),
          "target query finds agent active-plan section");
  const QString agent_plan_row_target = window.uiTargetJsonById("panel:agent_plan_row_1");
  require(contains(agent_plan_row_target, "\"found\":true"),
          "target query finds agent plan row");
  const QString agent_activity_target =
      window.uiTargetJsonById("panel:agent_activity_stream");
  require(contains(agent_activity_target, "\"found\":true"),
          "target query finds agent activity stream");
  const QString agent_permission_chip_target =
      window.uiTargetJsonById("label:agent_permission_chip");
  require(contains(agent_permission_chip_target, "\"found\":true"),
          "target query finds agent permission chip");
  require(contains(agent_permission_chip_target, "\"role\":\"label\""),
          "agent permission chip target reports label role");
  const QString agent_command_tab_target = window.uiTargetJsonById("tab:agent_command");
  require(contains(agent_command_tab_target, "\"found\":true"),
          "target query finds agent command tab selector");
  const QString agent_header_context_target =
      window.uiTargetJsonById("action:agent_header_request_context");
  require(contains(agent_header_context_target, "\"found\":true"),
          "target query finds agent header context action");
  const QString agent_header_drc_target =
      window.uiTargetJsonById("action:agent_header_trigger_drc");
  require(contains(agent_header_drc_target, "\"found\":true"),
          "target query finds agent header diagnostics action");
  const QString agent_header_clear_target =
      window.uiTargetJsonById("action:agent_header_clear_output");
  require(contains(agent_header_clear_target, "\"found\":true"),
          "target query finds agent header clear action");
  const QString agent_command_target =
      window.uiTargetJsonById("control:agent_command_input");
  require(contains(agent_command_target, "\"found\":true"),
          "target query finds agent command input");
  require(contains(agent_command_target, "\"role\":\"control\""),
          "agent command input target reports control role");
  const QString agent_submit_target =
      window.uiTargetJsonById("action:agent_submit_command");
  require(contains(agent_submit_target, "\"found\":true"),
          "target query finds agent submit action");
  require(contains(agent_submit_target, "\"role\":\"action\""),
          "agent submit target reports action role");
  const QString footer_context_target =
      window.uiTargetJsonById("action:agent_footer_request_context");
  require(contains(footer_context_target, "\"found\":true"),
          "target query finds visible agent footer context action");
  const QString footer_drc_target = window.uiTargetJsonById("action:agent_footer_trigger_drc");
  require(contains(footer_drc_target, "\"found\":true"),
          "target query finds visible agent footer DRC action");
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
  const QString harness_preset_target =
      window.uiTargetJsonById("action:agent_preset_harness_context");
  require(contains(harness_preset_target, "\"found\":true"),
          "target query finds agent harness-context preset");
  const QString diagnostics_preset_target =
      window.uiTargetJsonById("action:agent_preset_project_diagnostics");
  require(contains(diagnostics_preset_target, "\"found\":true"),
          "target query finds agent diagnostics preset");
  const QString tool_guide_preset_target =
      window.uiTargetJsonById("action:agent_preset_tool_guide");
  require(contains(tool_guide_preset_target, "\"found\":true"),
          "target query finds agent tool-guide preset");
  const QString clear_output_target = window.uiTargetJsonById("action:agent_clear_output");
  require(contains(clear_output_target, "\"found\":true"),
          "target query finds agent clear-output action");
  const QString goal_input_target = window.uiTargetJsonById("control:agent_goal");
  require(contains(goal_input_target, "\"found\":true"),
          "target query finds agent goal input");
  const QString stage_goal_target = window.uiTargetJsonById("action:agent_stage_goal");
  require(contains(stage_goal_target, "\"found\":true"),
          "target query finds agent stage-goal action");
  const QString pin_evidence_target = window.uiTargetJsonById("action:agent_pin_evidence");
  require(contains(pin_evidence_target, "\"found\":true"),
          "target query finds agent pin-evidence action");
  const QString clear_evidence_target = window.uiTargetJsonById("action:agent_clear_evidence");
  require(contains(clear_evidence_target, "\"found\":true"),
          "target query finds agent clear-evidence action");
  const QString approval_request_target =
      window.uiTargetJsonById("control:agent_approval_request");
  require(contains(approval_request_target, "\"found\":true"),
          "target query finds agent approval request input");
  const QString request_approval_target =
      window.uiTargetJsonById("action:agent_request_approval");
  require(contains(request_approval_target, "\"found\":true"),
          "target query finds request-approval action");
  const QString approve_next_target = window.uiTargetJsonById("action:agent_approve_next");
  require(contains(approve_next_target, "\"found\":true"),
          "target query finds approve-next action");
  const QString decline_next_target = window.uiTargetJsonById("action:agent_decline_next");
  require(contains(decline_next_target, "\"found\":true"),
          "target query finds decline-next action");
  const QString cancel_approval_target =
      window.uiTargetJsonById("action:agent_cancel_approval");
  require(contains(cancel_approval_target, "\"found\":true"),
          "target query finds cancel-approval action");
  const QString clear_approvals_target =
      window.uiTargetJsonById("action:agent_clear_approvals");
  require(contains(clear_approvals_target, "\"found\":true"),
          "target query finds clear-approvals action");
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
  require(contains(nearest_pad, "\"index_kind\":\"uniform_grid\""),
          "nearest canvas-object query reports indexed lookup strategy");
  require(contains(nearest_pad, "\"indexed_object_count\":"),
          "nearest canvas-object query reports indexed object count");
  require(contains(nearest_pad, "\"scanned_candidate_count\":"),
          "nearest canvas-object query reports filtered candidate scan count");

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
  const QString type_command = window.runAgentUiQueryJson(
      "ui.type_text", "{\"id\":\"control:agent_command_input\",\"text\":\"Inspect DRC\"}");
  require(contains(type_command, "\"performed\":true"),
          "agent type_text writes into the command input");
  require(contains(type_command, "\"value\":\"Inspect DRC\""),
          "agent type_text reports command input value");
  const QString submit_command_click =
      window.runAgentUiQueryJson("ui.click", "{\"id\":\"action:agent_submit_command\"}");
  require(contains(submit_command_click, "\"performed\":true"),
          "agent click can submit the command input");
  const QString footer_context_click = window.runAgentUiQueryJson(
      "ui.click", "{\"id\":\"action:agent_footer_request_context\"}");
  require(contains(footer_context_click, "\"performed\":true"),
          "agent click can trigger the visible footer context action");
  const QString footer_drc_click =
      window.runAgentUiQueryJson("ui.click", "{\"id\":\"action:agent_footer_trigger_drc\"}");
  require(contains(footer_drc_click, "\"performed\":true"),
          "agent click can trigger the visible footer DRC action");
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
  const QString harness_preset_click = window.runAgentUiQueryJson(
      "ui.click", "{\"id\":\"action:agent_preset_harness_context\"}");
  require(contains(harness_preset_click, "\"performed\":true"),
          "agent click can trigger the harness-context preset");
  const QString diagnostics_preset_click = window.runAgentUiQueryJson(
      "ui.click", "{\"id\":\"action:agent_preset_project_diagnostics\"}");
  require(contains(diagnostics_preset_click, "\"performed\":true"),
          "agent click can trigger the diagnostics preset");
  const QString tool_guide_preset_click = window.runAgentUiQueryJson(
      "ui.click", "{\"id\":\"action:agent_preset_tool_guide\"}");
  require(contains(tool_guide_preset_click, "\"performed\":true"),
          "agent click can trigger the tool-guide preset");
  const QString clear_output_click =
      window.runAgentUiQueryJson("ui.click", "{\"id\":\"action:agent_clear_output\"}");
  require(contains(clear_output_click, "\"performed\":true"),
          "agent click can trigger the clear-output action");
  const QString type_goal = window.runAgentUiQueryJson(
      "ui.type_text",
      "{\"id\":\"control:agent_goal\",\"text\":\"Inspect bridge rectifier evidence\"}");
  require(contains(type_goal, "\"performed\":true"),
          "agent can type into the task goal control");
  require(contains(type_goal, "\"value\":\"Inspect bridge rectifier evidence\""),
          "agent goal typing reports the typed value");
  const QString stage_goal_click =
      window.runAgentUiQueryJson("ui.click", "{\"id\":\"action:agent_stage_goal\"}");
  require(contains(stage_goal_click, "\"performed\":true"),
          "agent click can stage the task goal");
  const QString pin_evidence_click =
      window.runAgentUiQueryJson("ui.click", "{\"id\":\"action:agent_pin_evidence\"}");
  require(contains(pin_evidence_click, "\"performed\":true"),
          "agent click can pin the current output as evidence");
  const QString workspace_state_after =
      window.runAgentUiQueryJson("agent.workspace_state", "{}");
  require(contains(workspace_state_after, "\"goal\":\"Inspect bridge rectifier evidence\""),
          "workspace state reports the staged goal");
  require(contains(workspace_state_after, "\"evidence_count\":1"),
          "workspace state reports pinned evidence");
  require(contains(workspace_state_after, "\"evidence_cards\":["),
          "workspace state reports structured evidence cards");
  require(contains(workspace_state_after, "\"kind\":\"tool_result\""),
          "workspace state classifies pinned tool-guide evidence");
  require(contains(workspace_state_after, "\"method\":\"agent.tool_guide\""),
          "workspace state keeps the evidence producer method");
  require(contains(workspace_state_after, "\"trace_id\":\"\""),
          "workspace state evidence cards include trace-ready metadata");
  const QString clear_evidence_click =
      window.runAgentUiQueryJson("ui.click", "{\"id\":\"action:agent_clear_evidence\"}");
  require(contains(clear_evidence_click, "\"performed\":true"),
          "agent click can clear pinned evidence");
  const QString type_approval = window.runAgentUiQueryJson(
      "ui.type_text",
      "{\"id\":\"control:agent_approval_request\",\"text\":\"Approve tool-run before DRC\"}");
  require(contains(type_approval, "\"performed\":true"),
          "agent can type into the approval request control");
  const QString request_approval_click =
      window.runAgentUiQueryJson("ui.click", "{\"id\":\"action:agent_request_approval\"}");
  require(contains(request_approval_click, "\"performed\":true"),
          "agent click can request approval");
  QString approval_workspace_state =
      window.runAgentUiQueryJson("agent.workspace_state", "{}");
  require(contains(approval_workspace_state, "\"approval_pending_count\":1"),
          "workspace state reports one pending approval");
  require(contains(approval_workspace_state,
                   "\"approval_request\":\"Approve tool-run before DRC\""),
          "workspace state reports the approval request text");
  const QString approve_next_click =
      window.runAgentUiQueryJson("ui.click", "{\"id\":\"action:agent_approve_next\"}");
  require(contains(approve_next_click, "\"performed\":true"),
          "agent click can accept approval");
  approval_workspace_state = window.runAgentUiQueryJson("agent.workspace_state", "{}");
  require(contains(approval_workspace_state, "\"approval_last_decision\":\"accept\""),
          "workspace state reports accepted approval decision");

  window.runAgentUiQueryJson(
      "ui.type_text",
      "{\"id\":\"control:agent_approval_request\",\"text\":\"Decline destructive action\"}");
  window.runAgentUiQueryJson("ui.click", "{\"id\":\"action:agent_request_approval\"}");
  const QString decline_next_click =
      window.runAgentUiQueryJson("ui.click", "{\"id\":\"action:agent_decline_next\"}");
  require(contains(decline_next_click, "\"performed\":true"),
          "agent click can decline approval");
  approval_workspace_state = window.runAgentUiQueryJson("agent.workspace_state", "{}");
  require(contains(approval_workspace_state, "\"approval_last_decision\":\"decline\""),
          "workspace state reports declined approval decision");

  window.runAgentUiQueryJson(
      "ui.type_text",
      "{\"id\":\"control:agent_approval_request\",\"text\":\"Cancel stale action\"}");
  window.runAgentUiQueryJson("ui.click", "{\"id\":\"action:agent_request_approval\"}");
  const QString cancel_approval_click =
      window.runAgentUiQueryJson("ui.click", "{\"id\":\"action:agent_cancel_approval\"}");
  require(contains(cancel_approval_click, "\"performed\":true"),
          "agent click can cancel approval");
  approval_workspace_state = window.runAgentUiQueryJson("agent.workspace_state", "{}");
  require(contains(approval_workspace_state, "\"approval_last_decision\":\"cancel\""),
          "workspace state reports canceled approval decision");

  window.runAgentUiQueryJson(
      "ui.type_text",
      "{\"id\":\"control:agent_approval_request\",\"text\":\"Clear approval lane\"}");
  window.runAgentUiQueryJson("ui.click", "{\"id\":\"action:agent_request_approval\"}");
  const QString clear_approvals_click =
      window.runAgentUiQueryJson("ui.click", "{\"id\":\"action:agent_clear_approvals\"}");
  require(contains(clear_approvals_click, "\"performed\":true"),
          "agent click can clear approval state");
  approval_workspace_state = window.runAgentUiQueryJson("agent.workspace_state", "{}");
  require(contains(approval_workspace_state, "\"approval_pending_count\":0"),
          "workspace state reports cleared approvals");
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

  const std::filesystem::path canvas_input_path = writeProjectFixture();
  ReviewWindow canvas_input_window;
  canvas_input_window.loadProjectPath(canvas_input_path);
  canvas_input_window.show();
  QApplication::processEvents();

  const QString canvas_click_dry_run = canvas_input_window.runAgentUiQueryJson(
      "ui.canvas_click", "{\"x_mm\":12,\"y_mm\":10,\"dry_run\":true}");
  require(contains(canvas_click_dry_run, "\"ok\":true"),
          "agent canvas_click dry run routes through dispatcher");
  require(contains(canvas_click_dry_run, "\"dry_run\":true"),
          "agent canvas_click reports dry-run mode");
  require(contains(canvas_click_dry_run, "\"board_x_mm\":"),
          "agent canvas_click reports board coordinate mapping");
  require(contains(canvas_click_dry_run, "\"mode_before\":\"default\""),
          "agent canvas_click reports interaction mode before event");

  canvas_input_window.runAgentUiQueryJson("ui.click", "{\"id\":\"action:add_via\"}");
  const QString canvas_via =
      canvas_input_window.runAgentUiQueryJson("ui.canvas_click", "{\"x_mm\":15,\"y_mm\":11}");
  require(contains(canvas_via, "\"ok\":true"), "agent canvas_click routes via placement");
  require(contains(canvas_via, "\"performed\":true"),
          "agent canvas_click sends a real viewport click");
  require(contains(canvas_via, "\"reason\":\"event_sent\""),
          "agent canvas_click reports injected event delivery");
  require(contains(canvas_via, "\"mode_before\":\"add_via\""),
          "agent canvas_click sees the active Add Via tool");
  require(contains(canvas_via, "\"mode_after\":\"default\""),
          "agent canvas_click returns Add Via to default mode");
  require(contains(canvas_via, "\"via_count\":1"),
          "agent canvas_click places a via through the GUI event path");
  require(contains(canvas_input_window.uiMapJson(), "\"id\":\"canvas_object:V1\""),
          "agent canvas_click exposes the placed via in the UI map");

  canvas_input_window.runAgentUiQueryJson("ui.click", "{\"id\":\"action:add_tracks\"}");
  const QString route_start =
      canvas_input_window.runAgentUiQueryJson("ui.canvas_click", "{\"x_mm\":8,\"y_mm\":9}");
  require(contains(route_start, "\"performed\":true"),
          "agent canvas_click sends the first route anchor click");
  require(contains(route_start, "\"mode_after\":\"route_track\""),
          "agent canvas_click keeps Route Track active after the first point");
  const QString route_end =
      canvas_input_window.runAgentUiQueryJson("ui.canvas_click", "{\"x_mm\":18,\"y_mm\":12}");
  require(contains(route_end, "\"performed\":true"),
          "agent canvas_click sends the second route click");
  require(contains(route_end, "\"track_count\":1"),
          "agent canvas_click places a track through the GUI event path");
  require(contains(canvas_input_window.uiMapJson(), "\"id\":\"canvas_object:T1\""),
          "agent canvas_click exposes the placed track in the UI map");

  canvas_input_window.runAgentUiQueryJson("ui.click", "{\"id\":\"action:add_zone\"}");
  const QString zone_drag = canvas_input_window.runAgentUiQueryJson(
      "ui.canvas_drag", "{\"start_x_mm\":2,\"start_y_mm\":2,\"end_x_mm\":20,\"end_y_mm\":12}");
  require(contains(zone_drag, "\"ok\":true"), "agent canvas_drag routes zone placement");
  require(contains(zone_drag, "\"performed\":true"),
          "agent canvas_drag sends real viewport events");
  require(contains(zone_drag, "\"zone_count\":1"),
          "agent canvas_drag places a zone through the GUI event path");

  canvas_input_window.runAgentUiQueryJson("ui.click", "{\"id\":\"action:add_keepout_area\"}");
  const QString keepout_drag = canvas_input_window.runAgentUiQueryJson(
      "ui.canvas_drag", "{\"start_x_mm\":22,\"start_y_mm\":8,\"end_x_mm\":28,\"end_y_mm\":14}");
  require(contains(keepout_drag, "\"performed\":true"),
          "agent canvas_drag routes keepout placement");
  require(contains(keepout_drag, "\"keepout_count\":1"),
          "agent canvas_drag places a keepout through the GUI event path");

  canvas_input_window.runAgentUiQueryJson("ui.click", "{\"id\":\"action:add_graphical_segments\"}");
  const QString graphic_drag = canvas_input_window.runAgentUiQueryJson(
      "ui.canvas_drag", "{\"start_x_mm\":4,\"start_y_mm\":24,\"end_x_mm\":18,\"end_y_mm\":24}");
  require(contains(graphic_drag, "\"performed\":true"),
          "agent canvas_drag routes graphic line placement");
  require(contains(graphic_drag, "\"graphic_count\":1"),
          "agent canvas_drag places a graphic line through the GUI event path");

  canvas_input_window.runAgentUiQueryJson("ui.click", "{\"id\":\"action:text\"}");
  const QString text_click = canvas_input_window.runAgentUiQueryJson(
      "ui.canvas_click", "{\"x_mm\":8,\"y_mm\":22,\"text\":\"AGENT TEXT\"}");
  require(contains(text_click, "\"performed\":true"),
          "agent canvas_click routes text placement");
  require(contains(text_click, "\"text_count\":1"),
          "agent canvas_click places board text through the GUI event path");
  const ccad::Project after_canvas_text = ccad::loadProjectJson(readFile(canvas_input_path));
  require(after_canvas_text.board.has_value(), "canvas interaction fixture still has a board");
  require(!after_canvas_text.board->texts.empty(),
          "canvas interaction text placement writes a board text object");
  require(after_canvas_text.board->texts.back().text == "AGENT TEXT",
          "canvas interaction text placement writes the requested label");

  const std::filesystem::path workflow_path = writeProjectFixture();
  ReviewWindow workflow_window;
  workflow_window.loadProjectPath(workflow_path);
  workflow_window.show();
  QApplication::processEvents();

  const QString current_tool =
      workflow_window.runAgentUiQueryJson("ui.current_tool", "{}");
  require(contains(current_tool, "\"ok\":true"), "agent current-tool query routes");
  require(contains(current_tool, "\"mode\":\"default\""),
          "agent current-tool query reports default mode");

  workflow_window.runAgentUiQueryJson("ui.click", "{\"id\":\"action:add_tracks\"}");
  const QString armed_tool =
      workflow_window.runAgentUiQueryJson("ui.current_tool", "{}");
  require(contains(armed_tool, "\"mode\":\"route_track\""),
          "agent current-tool query reports an armed route tool");
  const QString cancel_tool =
      workflow_window.runAgentUiQueryJson("ui.cancel_tool", "{}");
  require(contains(cancel_tool, "\"ok\":true"), "agent cancel-tool query routes");
  require(contains(cancel_tool, "\"performed\":true"),
          "agent cancel-tool sends Escape through the GUI path");
  require(contains(cancel_tool, "\"mode\":\"default\""),
          "agent cancel-tool returns to default mode");

  const QString workflow_via =
      workflow_window.runAgentUiQueryJson("ui.place_via", "{\"x_mm\":14,\"y_mm\":10}");
  require(contains(workflow_via, "\"ok\":true"), "agent place-via workflow routes");
  require(contains(workflow_via, "\"performed\":true"),
          "agent place-via workflow places through viewport input");
  require(contains(workflow_via, "\"via_count\":1"),
          "agent place-via workflow reports updated via count");
  require(contains(workflow_window.uiMapJson(), "\"id\":\"canvas_object:V1\""),
          "agent place-via workflow exposes the placed via");

  const QString workflow_track = workflow_window.runAgentUiQueryJson(
      "ui.route_track", "{\"start_x_mm\":8,\"start_y_mm\":9,\"end_x_mm\":18,\"end_y_mm\":12}");
  require(contains(workflow_track, "\"performed\":true"),
          "agent route-track workflow places through viewport input");
  require(contains(workflow_track, "\"track_count\":1"),
          "agent route-track workflow reports updated track count");

  const QString workflow_zone = workflow_window.runAgentUiQueryJson(
      "ui.add_zone", "{\"start_x_mm\":2,\"start_y_mm\":2,\"end_x_mm\":20,\"end_y_mm\":12}");
  require(contains(workflow_zone, "\"performed\":true"),
          "agent add-zone workflow places through viewport input");
  require(contains(workflow_zone, "\"zone_count\":1"),
          "agent add-zone workflow reports updated zone count");

  const QString workflow_keepout = workflow_window.runAgentUiQueryJson(
      "ui.add_keepout", "{\"start_x_mm\":22,\"start_y_mm\":8,\"end_x_mm\":28,\"end_y_mm\":14}");
  require(contains(workflow_keepout, "\"performed\":true"),
          "agent add-keepout workflow places through viewport input");
  require(contains(workflow_keepout, "\"keepout_count\":1"),
          "agent add-keepout workflow reports updated keepout count");

  const QString workflow_graphic = workflow_window.runAgentUiQueryJson(
      "ui.draw_graphic", "{\"start_x_mm\":4,\"start_y_mm\":24,\"end_x_mm\":18,\"end_y_mm\":24}");
  require(contains(workflow_graphic, "\"performed\":true"),
          "agent draw-graphic workflow places through viewport input");
  require(contains(workflow_graphic, "\"graphic_count\":1"),
          "agent draw-graphic workflow reports updated graphic count");

  const QString workflow_text = workflow_window.runAgentUiQueryJson(
      "ui.place_text", "{\"x_mm\":8,\"y_mm\":22,\"text\":\"FLOW TEXT\"}");
  require(contains(workflow_text, "\"performed\":true"),
          "agent place-text workflow places through viewport input");
  require(contains(workflow_text, "\"text_count\":1"),
          "agent place-text workflow reports updated text count");
  const ccad::Project after_workflow_text = ccad::loadProjectJson(readFile(workflow_path));
  require(after_workflow_text.board.has_value(), "workflow fixture still has a board");
  require(!after_workflow_text.board->texts.empty(),
          "workflow text placement writes a board text object");
  require(after_workflow_text.board->texts.back().text == "FLOW TEXT",
          "workflow text placement writes the requested label");

  const QString workflow_delete =
      workflow_window.runAgentUiQueryJson("ui.delete_object", "{\"id\":\"V1\"}");
  require(contains(workflow_delete, "\"performed\":true"),
          "agent delete-object workflow deletes a selected object");
  require(contains(workflow_delete, "\"deleted_type\":\"via\""),
          "agent delete-object workflow reports deleted object type");
  require(!contains(workflow_window.uiMapJson(), "\"id\":\"canvas_object:V1\""),
          "agent delete-object workflow removes the via from the UI map");

  const std::filesystem::path evidence_path = writeProjectFixture();
  ReviewWindow evidence_window;
  evidence_window.loadProjectPath(evidence_path);
  evidence_window.show();
  QApplication::processEvents();
  const auto evidence_stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  const std::filesystem::path screenshot_path =
      std::filesystem::temp_directory_path() /
      ("ccad-ui-map-evidence-" + std::to_string(evidence_stamp) + ".png");
  const QString screenshot_payload =
      QString("{\"path\":%1}").arg(jsonStringLiteral(screenshot_path));
  const QString screenshot_dry_payload =
      QString("{\"path\":%1,\"dry_run\":true}").arg(jsonStringLiteral(screenshot_path));
  const QString screenshot_dry =
      evidence_window.runAgentUiQueryJson("ui.screenshot", screenshot_dry_payload);
  require(contains(screenshot_dry, "\"ok\":true"), "agent screenshot dry-run query routes");
  require(contains(screenshot_dry, "\"dry_run\":true"),
          "agent screenshot dry-run reports dry-run mode");
  require(contains(screenshot_dry, "\"performed\":false"),
          "agent screenshot dry-run does not write an image");
  require(!std::filesystem::exists(screenshot_path),
          "agent screenshot dry-run leaves the target path unwritten");
  const QString screenshot_capture =
      evidence_window.runAgentUiQueryJson("ui.screenshot", screenshot_payload);
  require(contains(screenshot_capture, "\"ok\":true"), "agent screenshot capture query routes");
  require(contains(screenshot_capture, "\"performed\":true"),
          "agent screenshot capture writes a PNG");
  require(contains(screenshot_capture, "\"width\":"),
          "agent screenshot capture reports pixel width");
  require(contains(screenshot_capture, "\"height\":"),
          "agent screenshot capture reports pixel height");
  require(std::filesystem::exists(screenshot_path),
          "agent screenshot capture creates the requested PNG");
  require(std::filesystem::file_size(screenshot_path) > 0,
          "agent screenshot capture creates a non-empty PNG");
  std::filesystem::remove(screenshot_path);

  const QString project_context =
      evidence_window.runAgentUiQueryJson("project.context", "{}");
  require(contains(project_context, "\"ok\":true"), "agent project-context query routes");
  require(contains(project_context, "\"project_id\":\"proj-ui-map\""),
          "agent project-context reports project id");
  require(contains(project_context, "\"project_name\":\"UI map\""),
          "agent project-context reports project name");
  require(contains(project_context, "\"has_board\":true"),
          "agent project-context reports board presence");
  require(contains(project_context, "\"active_pcb_layer_id\":\"F.Cu\""),
          "agent project-context reports active layer");
  require(contains(project_context, "\"active_pcb_net_id\":\"N1\""),
          "agent project-context reports active net");

  const QString project_counts =
      evidence_window.runAgentUiQueryJson("project.object_counts", "{}");
  require(contains(project_counts, "\"ok\":true"), "agent object-count query routes");
  require(contains(project_counts, "\"pad_count\":1"),
          "agent object-count query reports board pad count");
  require(contains(project_counts, "\"net_count\":2"),
          "agent object-count query reports project net count");

  const QString project_review =
      evidence_window.runAgentUiQueryJson("project.review", "{}");
  require(contains(project_review, "\"ok\":true"), "agent project-review query routes");
  require(contains(project_review, "\"diagnostic_count\":"),
          "agent project-review reports diagnostic count");
  require(contains(project_review, "\"status\":"),
          "agent project-review reports review status");

  const QString project_erc = evidence_window.runAgentUiQueryJson("project.erc", "{}");
  require(contains(project_erc, "\"ok\":true"), "agent project-erc query routes");
  require(contains(project_erc, "\"diagnostic_count\":"),
          "agent project-erc reports diagnostic count");
  require(contains(project_erc, "\"diagnostics\":["),
          "agent project-erc includes diagnostic evidence");

  const QString project_drc = evidence_window.runAgentUiQueryJson("project.drc", "{}");
  require(contains(project_drc, "\"ok\":true"), "agent project-drc query routes");
  require(contains(project_drc, "\"diagnostic_count\":"),
          "agent project-drc reports diagnostic count");
  require(contains(project_drc, "\"diagnostics\":["),
          "agent project-drc includes diagnostic evidence");

  const QString project_diagnostics =
      evidence_window.runAgentUiQueryJson("project.diagnostics", "{}");
  require(contains(project_diagnostics, "\"ok\":true"),
          "agent project-diagnostics query routes");
  require(contains(project_diagnostics, "\"erc_count\":"),
          "agent project-diagnostics reports ERC count");
  require(contains(project_diagnostics, "\"drc_count\":"),
          "agent project-diagnostics reports DRC count");
  require(contains(project_diagnostics, "\"error_count\":"),
          "agent project-diagnostics reports error count");

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
  const QString method_catalog_response = requestLine(socket, "{\"method\":\"agent.methods\"}");
  require(contains(method_catalog_response, "\"ok\":true"),
          "live server returns successful agent.methods");
  require(contains(method_catalog_response, "\"method\":\"ui.map\""),
          "live server agent.methods includes UI map query");
  require(contains(method_catalog_response, "\"method\":\"project.drc\""),
          "live server agent.methods includes DRC query");
  const QString method_schema_response =
      requestLine(socket, "{\"method\":\"agent.method_schema\",\"method_name\":\"ui.watch_delta\"}");
  require(contains(method_schema_response, "\"ok\":true"),
          "live server returns successful agent.method_schema");
  require(contains(method_schema_response, "\"found\":true"),
          "live server agent.method_schema finds watch-delta");
  require(contains(method_schema_response, "\"since_epoch\""),
          "live server agent.method_schema documents watch-delta input");
  const QString quickstart_response = requestLine(socket, "{\"method\":\"agent.quickstart\"}");
  require(contains(quickstart_response, "\"ok\":true"),
          "live server returns successful agent.quickstart");
  require(contains(quickstart_response, "\"workflow\":\"ui_map_agent_loop\""),
          "live server agent.quickstart reports workflow");
  const QString harness_context_response =
      requestLine(socket, "{\"method\":\"agent.harness_context\"}");
  require(contains(harness_context_response, "\"ok\":true"),
          "live server returns successful agent.harness_context");
  require(contains(harness_context_response, "\"harness_kind\":\"ccad_native_qt_agent_surface\""),
          "live server harness context identifies the native Qt surface");
  const QString tool_guide_response =
      requestLine(socket, "{\"method\":\"agent.tool_guide\",\"method_name\":\"ui.route_track\"}");
  require(contains(tool_guide_response, "\"ok\":true"),
          "live server returns successful agent.tool_guide");
  require(contains(tool_guide_response, "\"preferred_surface\""),
          "live server tool guide includes preferred surface guidance");
  const QString index_stats_response = requestLine(socket, "{\"method\":\"ui.index_stats\"}");
  require(contains(index_stats_response, "\"ok\":true"),
          "live server returns successful ui.index_stats");
  require(contains(index_stats_response, "\"cache_state\":\"fresh\""),
          "live server UI index stats report a fresh cache");
  const QString get_node_response =
      requestLine(socket, "{\"method\":\"ui.get_node\",\"id\":\"menu:file\"}");
  require(contains(get_node_response, "\"ok\":true"),
          "live server returns successful ui.get_node");
  require(contains(get_node_response, "\"lookup_kind\":\"id_index\""),
          "live server ui.get_node uses the ID index");
  require(contains(get_node_response, "\"id\":\"menu:file\""),
          "live server ui.get_node returns the requested menu node");
  const QString nodes_by_role_response =
      requestLine(socket, "{\"method\":\"ui.nodes_by_role\",\"role\":\"action\",\"limit\":80}");
  require(contains(nodes_by_role_response, "\"ok\":true"),
          "live server returns successful ui.nodes_by_role");
  require(contains(nodes_by_role_response, "\"lookup_kind\":\"role_index\""),
          "live server ui.nodes_by_role uses the role index");
  require(contains(nodes_by_role_response, "\"id\":\"action:add_footprint\""),
          "live server ui.nodes_by_role returns action nodes");
  const QString watch_delta_response = requestLine(
      socket, "{\"method\":\"ui.watch_delta\",\"since_epoch\":0,\"timeout_ms\":1,\"max_events\":2}");
  require(contains(watch_delta_response, "\"ok\":true"),
          "live server returns successful ui.watch_delta");
  require(contains(watch_delta_response, "\"event_count\":"),
          "live server ui.watch_delta reports event count");
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
  require(contains(nearest_response, "\"index_kind\":\"uniform_grid\""),
          "live server nearest canvas-object reports indexed lookup");
  const QString wait_delta_response =
      requestLine(socket, "{\"method\":\"ui.wait_for_delta\",\"since_epoch\":999999,\"timeout_ms\":1}");
  require(contains(wait_delta_response, "\"ok\":true"),
          "live server returns successful ui.wait_for_delta");
  require(contains(wait_delta_response, "\"changed\":false"),
          "live server wait-for-delta can return unchanged without closing the socket");
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
  const QString canvas_click_response =
      requestLine(socket, "{\"method\":\"ui.canvas_click\",\"x_mm\":8,\"y_mm\":9,\"dry_run\":true}");
  require(contains(canvas_click_response, "\"ok\":true"),
          "live server returns successful ui.canvas_click");
  require(contains(canvas_click_response, "\"dry_run\":true"),
          "live server ui.canvas_click supports dry run");
  const QString canvas_drag_response = requestLine(
      socket,
      "{\"method\":\"ui.canvas_drag\",\"start_x_mm\":8,\"start_y_mm\":9,\"end_x_mm\":12,\"end_y_mm\":12,\"dry_run\":true}");
  require(contains(canvas_drag_response, "\"ok\":true"),
          "live server returns successful ui.canvas_drag");
  require(contains(canvas_drag_response, "\"dry_run\":true"),
          "live server ui.canvas_drag supports dry run");
  const QString cancel_tool_response = requestLine(socket, "{\"method\":\"ui.cancel_tool\"}");
  require(contains(cancel_tool_response, "\"ok\":true"),
          "live server returns successful ui.cancel_tool");
  require(contains(cancel_tool_response, "\"performed\":true"),
          "live server cancel-tool query sends Escape");
  const QString current_tool_response = requestLine(socket, "{\"method\":\"ui.current_tool\"}");
  require(contains(current_tool_response, "\"ok\":true"),
          "live server returns successful ui.current_tool");
  require(contains(current_tool_response, "\"mode\":\"default\""),
          "live server current-tool query reports default mode after cancellation");
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
  const QString live_context_response = requestLine(socket, "{\"method\":\"project.context\"}");
  require(contains(live_context_response, "\"ok\":true"),
          "live server returns successful project.context");
  require(contains(live_context_response, "\"project_id\":\"proj-ui-map\""),
          "live server project.context reports project id");
  const QString live_counts_response =
      requestLine(socket, "{\"method\":\"project.object_counts\"}");
  require(contains(live_counts_response, "\"ok\":true"),
          "live server returns successful project.object_counts");
  require(contains(live_counts_response, "\"pad_count\":1"),
          "live server project.object_counts reports pad count");
  const QString live_diagnostics_response =
      requestLine(socket, "{\"method\":\"project.diagnostics\"}");
  require(contains(live_diagnostics_response, "\"ok\":true"),
          "live server returns successful project.diagnostics");
  require(contains(live_diagnostics_response, "\"drc_count\":"),
          "live server project.diagnostics reports DRC count");
  const QString live_screenshot_dry_response =
      requestLine(socket, "{\"method\":\"ui.screenshot\",\"dry_run\":true}");
  require(contains(live_screenshot_dry_response, "\"ok\":true"),
          "live server returns successful ui.screenshot dry-run");
  require(contains(live_screenshot_dry_response, "\"dry_run\":true"),
          "live server ui.screenshot reports dry-run mode");
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
