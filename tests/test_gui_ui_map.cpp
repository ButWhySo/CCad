#include <QApplication>
#include <QAction>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMenu>
#include <QScrollArea>
#include <QTemporaryDir>
#include <QTimer>

#include <fstream>
#include <sstream>

#include "ccad_cli/app.hpp"
#include "ccad_core/geometry.hpp"
#include "ccad_core/model.hpp"
#include "ccad_core/serialize.hpp"
#include "ccad_gui/agent_panel.hpp"
#include "ccad_gui/agent_settings_dialog.hpp"
#include "ccad_gui/review_window.hpp"

namespace {

bool writeProject(const QString& path, const ccad::Project& project) {
  std::ofstream out(path.toStdString(), std::ios::binary);
  out << ccad::dumpProjectJson(project);
  return static_cast<bool>(out);
}

ccad::Project readProject(const QString& path) {
  std::ifstream input(path.toStdString(), std::ios::binary);
  std::ostringstream contents;
  contents << input.rdbuf();
  return ccad::loadProjectJson(contents.str());
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

ccad::Project diagnosticProject() {
  ccad::Project project = boardOnlyProject();
  project.id = "proj-diagnostics-context";
  ccad::TrackSegment track;
  track.id = "T_BAD";
  track.layer_id = "F.Cu";
  track.start = {ccad::millimeters(3.0), ccad::millimeters(3.0)};
  track.end = track.start;
  track.width = ccad::millimeters(0.25);
  project.boards.front().tracks.push_back(track);
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
  const QString diagnostic_path = temp_dir.filePath("diagnostic-context.ccad.json");
  if (!writeProject(empty_path, emptyProject()) ||
      !writeProject(board_path, boardOnlyProject()) ||
      !writeProject(diagnostic_path, diagnosticProject())) {
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

  auto* agent_panel = dynamic_cast<AgentPanel*>(window.findChild<QWidget*>("agentPanel"));
  auto* history_menu = agent_panel == nullptr
                           ? nullptr
                           : agent_panel->findChild<QMenu*>("menu:agent_conversation_history");
  if (history_menu == nullptr) return 20;
  QAction* history_item = history_menu->addAction("Saved thread");
  history_item->setObjectName("action:conversation_contract-thread");
  history_item->setProperty("ccadConversationAction", true);
  bool history_item_triggered = false;
  QObject::connect(history_item, &QAction::triggered, &window,
                   [&history_item_triggered]() { history_item_triggered = true; });
  history_menu->popup(QPoint(300, 240));
  QCoreApplication::processEvents();
  const QJsonObject history_map = QJsonDocument::fromJson(window.uiMapJson().toUtf8())
                                      .object();
  QJsonObject history_node;
  for (const QJsonValue& value : history_map.value("nodes").toArray()) {
    const QJsonObject node = value.toObject();
    if (node.value("id").toString() == history_item->objectName()) {
      history_node = node;
      break;
    }
  }
  const QJsonObject history_target = QJsonDocument::fromJson(
      window.uiTargetJsonById(history_item->objectName()).toUtf8()).object();
  if (history_node.value("role").toString() != "action" ||
      !history_node.value("visible").toBool() ||
      !history_target.value("found").toBool()) {
    return 21;
  }
  const QJsonObject history_click = QJsonDocument::fromJson(
      window.uiClickJson(history_item->objectName(), false, false).toUtf8()).object();
  if (!history_click.value("performed").toBool() || !history_item_triggered) return 22;
  history_menu->hide();

  const QString methods = window.runAgentUiQueryJson("agent.methods", "{}");
  const QString context = window.runAgentUiQueryJson("project.context", "{}");
  const QString state = window.runAgentUiQueryJson("project.state", "{}");
  if (!methods.contains("project.state") || !context.contains("typed_state") ||
      !context.contains("Front copper") || !context.contains("U1") ||
      !state.contains("binary_payloads_excluded") || !state.contains("GND")) {
    return 13;
  }
  const QJsonObject inspect_catalog = QJsonDocument::fromJson(
      methods.toUtf8()).object().value("result").toObject();
  const QJsonObject inspect_schema = [&inspect_catalog]() {
    const QJsonArray entries = inspect_catalog.value("methods").toArray();
    for (const QJsonValue& value : entries) {
      const QJsonObject entry = value.toObject();
      if (entry.value("method").toString() == "project.inspect") return entry;
    }
    return QJsonObject{};
  }();
  if (!inspect_schema.value("read_only").toBool() ||
      inspect_schema.value("inputSchema").toObject().value("properties").toObject()
          .value("max_objects").toObject().value("type").toString() != "integer" ||
      !inspect_schema.value("inputSchema").toObject().value("properties").toObject()
          .contains("bbox") ||
      !inspect_schema.value("inputSchema").toObject().value("properties").toObject()
          .contains("max_tokens")) {
    return 32;
  }
  const QJsonObject summary_only_inspect = QJsonDocument::fromJson(
      window.runAgentUiQueryJson("project.inspect", "{}").toUtf8()).object()
      .value("result").toObject();
  if (summary_only_inspect.value("inspection_mode").toString() != "summary" ||
      !summary_only_inspect.value("sections").toObject().isEmpty()) return 47;
  const QJsonObject inspect = QJsonDocument::fromJson(window.runAgentUiQueryJson(
      "project.inspect",
      "{\"scope\":\"schematic\",\"sections\":[\"components\"],"
      "\"object_ids\":[\"U1\"],\"refdes\":[\"U1\"],"
      "\"object_types\":[\"component\"],\"max_objects\":8,\"max_bytes\":4096}")
      .toUtf8()).object()
      .value("result").toObject();
  const QJsonObject inspect_sections = inspect.value("sections").toObject();
  const QJsonArray inspected_components = inspect_sections.value("schematic").toObject()
      .value("components").toArray();
  const QJsonObject context_revision = QJsonDocument::fromJson(context.toUtf8())
      .object().value("result").toObject();
  if (inspect.value("scope").toString() != "schematic" ||
      inspect.value("project_revision").toString() !=
          context_revision.value("design_revision").toString() ||
      inspect.value("digest").toString().isEmpty() ||
      inspected_components.size() != 1 ||
      inspected_components.at(0).toObject().value("reference").toString() != "U1") {
    return 33;
  }
  if (inspect.value("serialized_bytes").toInt() !=
      QJsonDocument(inspect).toJson(QJsonDocument::Compact).size()) {
    return 40;
  }
  const QJsonObject revision_request{{"scope", "schematic"},
                                    {"sections", QJsonArray{"components"}},
                                    {"object_ids", QJsonArray{"U1"}},
                                    {"max_objects", 8}, {"max_bytes", 4096},
                                    {"if_revision", inspect.value("project_revision")}};
  const QJsonObject repeated_envelope = QJsonDocument::fromJson(
      window.runAgentUiQueryJson("project.inspect",
          QString::fromUtf8(QJsonDocument(revision_request).toJson(QJsonDocument::Compact)))
          .toUtf8()).object();
  if (!repeated_envelope.value("ok").toBool()) return 43;
  const QJsonObject repeated_inspect = repeated_envelope.value("result").toObject();
  if (repeated_inspect.value("project_revision").toString() !=
      inspect.value("project_revision").toString()) return 44;
  if (repeated_inspect.value("digest").toString() != inspect.value("digest").toString()) {
    return 36;
  }
  const QJsonObject layer_inspect = QJsonDocument::fromJson(window.runAgentUiQueryJson(
      "project.inspect", "{\"scope\":\"pcb\",\"sections\":[\"board.layers\"],"
      "\"layer_ids\":[\"F.Cu\"]}").toUtf8()).object().value("result").toObject();
  const QJsonArray inspected_layers = layer_inspect.value("sections").toObject()
      .value("pcb").toObject().value("layers").toArray();
  if (inspected_layers.size() != 1 ||
      inspected_layers.at(0).toObject().value("id").toString() != "F.Cu" ||
      layer_inspect.value("section_counts").toObject().value("board.layers")
          .toObject().value("filtered").toInt() != 1) {
    return 38;
  }
  const QJsonObject net_inspect = QJsonDocument::fromJson(window.runAgentUiQueryJson(
      "project.inspect", "{\"scope\":\"nets\",\"net_ids\":[\"GND\"]}")
      .toUtf8()).object().value("result").toObject();
  const QJsonArray inspected_nets = net_inspect.value("sections").toObject()
      .value("schematic").toObject().value("nets").toArray();
  if (inspected_nets.size() != 1 ||
      inspected_nets.at(0).toObject().value("id").toString() != "GND") {
    return 39;
  }
  const QJsonObject stale_inspect = QJsonDocument::fromJson(
      window.runAgentUiQueryJson("project.inspect",
          "{\"scope\":\"pcb\",\"if_revision\":\"stale\"}").toUtf8()).object();
  if (stale_inspect.value("ok").toBool() ||
      stale_inspect.value("reason").toString() != "revision_mismatch") {
    return 37;
  }
  const QJsonObject bounded_envelope = QJsonDocument::fromJson(window.runAgentUiQueryJson(
      "project.inspect", "{\"scope\":\"project\",\"sections\":[\"board.layers\"],"
      "\"max_objects\":1,"
      "\"max_bytes\":4096}").toUtf8()).object();
  if (!bounded_envelope.value("ok").toBool()) return 45;
  const QJsonObject bounded_inspect = bounded_envelope.value("result").toObject();
  if (bounded_inspect.value("omissions").toArray().isEmpty()) return 35;
  bool has_follow_up_ids = false;
  for (const QJsonValue& omission : bounded_inspect.value("omissions").toArray()) {
    has_follow_up_ids = has_follow_up_ids ||
        !omission.toObject().value("follow_up_object_ids").toArray().isEmpty();
  }
  if (!has_follow_up_ids) return 48;
  if (bounded_inspect.value("serialized_bytes").toInt() > 4096) return 46;
  const QJsonObject invalid_inspect = QJsonDocument::fromJson(
      window.runAgentUiQueryJson("project.inspect", "{\"scope\":\"invalid\"}")
          .toUtf8()).object();
  if (invalid_inspect.value("ok").toBool() ||
      invalid_inspect.value("reason").toString() != "unsupported_scope") {
    return 34;
  }
  const QJsonObject fractional_limit = QJsonDocument::fromJson(
      window.runAgentUiQueryJson("project.inspect", "{\"max_objects\":1.5}")
          .toUtf8()).object();
  if (fractional_limit.value("ok").toBool() ||
      fractional_limit.value("reason").toString() != "invalid_snapshot_limit") {
    return 42;
  }
  QJsonObject context_result = QJsonDocument::fromJson(context.toUtf8()).object()
                                   .value("result").toObject();
  if (context_result.value("active_editor").toString() != "pcb" ||
      context_result.value("active_view").toString() != "pcb") {
    return 23;
  }
  const QJsonObject active_layer = QJsonDocument::fromJson(
      window.runAgentUiQueryJson("ui.active_layer", "{}").toUtf8()).object()
      .value("result").toObject();
  const QJsonArray available_layers = active_layer.value("layers").toArray();
  bool front_layer_found = false;
  bool back_layer_found = false;
  for (const QJsonValue& value : available_layers) {
    const QJsonObject layer = value.toObject();
    front_layer_found = front_layer_found ||
        (layer.value("id").toString() == "F.Cu" &&
         layer.value("name").toString() == "Front copper" &&
         layer.value("kind").toString() == "copper");
    back_layer_found = back_layer_found ||
        (layer.value("id").toString() == "B.Cu" &&
         layer.value("visible").toBool());
  }
  if (active_layer.value("active_layer_id").toString() != "F.Cu" ||
      available_layers.size() != 2 || !front_layer_found || !back_layer_found) {
    return 30;
  }
  const QJsonObject active_net = QJsonDocument::fromJson(
      window.runAgentUiQueryJson("ui.active_net", "{}").toUtf8()).object()
      .value("result").toObject();
  const QJsonArray available_nets = active_net.value("nets").toArray();
  if (available_nets.size() != 1 ||
      available_nets.at(0).toObject().value("id").toString() != "GND" ||
      active_net.value("active_net_id").toString() != "GND") {
    return 31;
  }
  const QString select_schematic = window.runAgentUiQueryJson(
      "ui.select_canvas_object",
      "{\"id\":\"U1\",\"canvas\":\"canvas:schematic\"}");
  const QJsonObject schematic_selection_result = QJsonDocument::fromJson(
      select_schematic.toUtf8()).object().value("result").toObject();
  if (!schematic_selection_result.value("performed").toBool()) return 24;
  const QJsonObject unsupported_canvas = QJsonDocument::fromJson(
      window.runAgentUiQueryJson("ui.select_canvas_object",
          "{\"id\":\"U1\",\"canvas\":\"canvas:unknown\"}")
          .toUtf8()).object().value("result").toObject();
  if (unsupported_canvas.value("performed").toBool() ||
      unsupported_canvas.value("reason").toString() != "unsupported_canvas") return 29;
  const QJsonObject selected_context = QJsonDocument::fromJson(
      window.runAgentUiQueryJson("project.context", "{}").toUtf8()).object()
      .value("result").toObject();
  const QJsonObject selection = selected_context.value("selection").toObject();
  const QJsonArray selected_items = selection.value("items").toArray();
  if (selected_context.value("active_editor").toString() != "schematic" ||
      selected_context.value("active_view").toString() != "schematic" ||
      selection.value("canvas").toString() != "canvas:schematic" ||
      selected_items.size() != 1 ||
      selected_items.at(0).toObject().value("object_id").toString() != "U1") {
    return 25;
  }
  const QJsonObject selected_snapshot = QJsonDocument::fromJson(
      window.runAgentUiQueryJson("project.inspect",
          "{\"scope\":\"selection\",\"sections\":[\"components\"]}")
          .toUtf8()).object().value("result").toObject();
  const QJsonArray selected_components = selected_snapshot.value("sections").toObject()
      .value("schematic").toObject().value("components").toArray();
  if (selected_components.size() != 1 ||
      selected_components.at(0).toObject().value("reference").toString() != "U1") {
    return 41;
  }
  const QJsonObject selected = QJsonDocument::fromJson(
      window.runAgentUiQueryJson("ui.get_selection", "{}").toUtf8()).object()
      .value("result").toObject();
  if (selected.value("canvas").toString() != "canvas:schematic" ||
      selected.value("items").toArray().size() != 1) {
    return 26;
  }
  const QJsonObject select_catalog_entry = [&]() {
    for (const QJsonValue& value : QJsonDocument::fromJson(methods.toUtf8())
                                       .object().value("result").toObject()
                                       .value("methods").toArray()) {
      const QJsonObject entry = value.toObject();
      if (entry.value("method").toString() == "ui.select_canvas_object") return entry;
    }
    return QJsonObject{};
  }();
  if (!select_catalog_entry.value("inputSchema").toObject()
           .value("properties").toObject().contains("canvas")) return 27;
  window.uiClickJson("tab:pcb", false, false);
  context_result = QJsonDocument::fromJson(
      window.runAgentUiQueryJson("project.context", "{}").toUtf8()).object()
      .value("result").toObject();
  if (context_result.value("active_editor").toString() != "pcb" ||
      context_result.value("selection").toObject().value("canvas").toString() !=
          "canvas:pcb") {
    return 28;
  }

  window.loadProjectPath(diagnostic_path.toStdString());
  const QJsonObject undersized_token_budget = QJsonDocument::fromJson(
      window.runAgentUiQueryJson("project.inspect",
          "{\"scope\":\"pcb\",\"sections\":[\"board.tracks\"],"
          "\"bbox\":[2.8,2.8,3.2,3.2],\"max_tokens\":1024}").toUtf8())
      .object();
  if (undersized_token_budget.value("ok").toBool() ||
      undersized_token_budget.value("reason").toString() !=
          "snapshot_metadata_exceeds_byte_limit") return 54;
  const QJsonObject spatial_hit = QJsonDocument::fromJson(
      window.runAgentUiQueryJson("project.inspect",
          "{\"scope\":\"pcb\",\"sections\":[\"board.tracks\"],"
          "\"bbox\":[2.8,2.8,3.2,3.2],\"max_tokens\":4096}").toUtf8())
      .object().value("result").toObject();
  const QJsonArray spatial_tracks = spatial_hit.value("sections").toObject()
      .value("pcb").toObject().value("tracks").toArray();
  if (spatial_hit.value("inspection_mode").toString() != "filtered_objects" ||
      spatial_tracks.size() != 1 ||
      spatial_tracks.at(0).toObject().value("id").toString() != "T_BAD" ||
      spatial_hit.value("bbox_semantics").toString() !=
          "inclusive_axis_aligned_bounds_intersection" ||
      spatial_hit.value("serialized_bytes").toInt() >
          spatial_hit.value("token_budget").toInt()) return 49;
  const QJsonObject spatial_miss_envelope = QJsonDocument::fromJson(
      window.runAgentUiQueryJson("project.inspect",
          "{\"scope\":\"pcb\",\"sections\":[\"board.tracks\"],"
          "\"bbox\":[8,8,9,9]}").toUtf8()).object();
  const QJsonObject spatial_miss = spatial_miss_envelope.value("result").toObject();
  if (!spatial_miss_envelope.value("ok").toBool() ||
      !spatial_miss.value("sections").toObject().value("pcb").toObject()
           .value("tracks").toArray().isEmpty() ||
      spatial_miss.value("section_counts").toObject().value("board.tracks")
          .toObject().value("filtered").toInt() != 1) return 50;
  const QJsonObject spatial_boundary = QJsonDocument::fromJson(
      window.runAgentUiQueryJson("project.inspect",
          "{\"scope\":\"pcb\",\"sections\":[\"board.tracks\"],"
          "\"bbox\":[3.125,3.125,4,4]}").toUtf8()).object()
      .value("result").toObject();
  if (spatial_boundary.value("sections").toObject().value("pcb").toObject()
          .value("tracks").toArray().size() != 1) return 53;
  const QJsonObject invalid_bbox = QJsonDocument::fromJson(
      window.runAgentUiQueryJson("project.inspect",
          "{\"bbox\":[4,4,3,5]}").toUtf8()).object();
  if (invalid_bbox.value("ok").toBool() ||
      invalid_bbox.value("reason").toString() != "invalid_bbox") return 51;
  const QJsonObject fractional_tokens = QJsonDocument::fromJson(
      window.runAgentUiQueryJson("project.inspect",
          "{\"max_tokens\":1024.5}").toUtf8()).object();
  if (fractional_tokens.value("ok").toBool() ||
      fractional_tokens.value("reason").toString() != "invalid_snapshot_limit") return 52;
  const QJsonObject diagnostic_context = QJsonDocument::fromJson(
      window.runAgentUiQueryJson("project.context", "{}").toUtf8()).object()
      .value("result").toObject();
  const QJsonArray project_diagnostics = diagnostic_context.value(
      "project_diagnostics").toArray();
  bool linked_drc_present = false;
  for (const QJsonValue& value : project_diagnostics) {
    const QJsonObject diagnostic = value.toObject();
    if (diagnostic.value("engine").toString() == "drc" &&
        diagnostic.value("code").toString() == "ZERO_LENGTH_TRACK" &&
        diagnostic.value("object_id").toString() == "T_BAD") {
      linked_drc_present = true;
      break;
    }
  }
  if (!linked_drc_present ||
      diagnostic_context.value("project_diagnostic_count").toInt() <
          project_diagnostics.size()) {
    return 19;
  }
  window.loadProjectPath(board_path.toStdString());

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
  const QJsonObject native_counts = [&]() {
    for (const QJsonValue& value : registry_methods) {
      const QJsonObject entry = value.toObject();
      if (entry.value("method").toString() == "project.object_counts") return entry;
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
      native_counts.value("inputSchema").toObject()
              .value("additionalProperties").toBool(true) ||
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
  auto* undo_action = window.findChild<QAction*>("action:undo");
  auto* redo_action = window.findChild<QAction*>("action:redo");
  if (undo_action == nullptr || redo_action == nullptr || !undo_action->isEnabled() ||
      redo_action->isEnabled()) {
    return 23;
  }
  undo_action->trigger();
  if (!undo_action->isEnabled() || !redo_action->isEnabled() ||
      readProject(board_path).boards.front().graphics.size() != 0) {
    return 24;
  }
  redo_action->trigger();
  if (!undo_action->isEnabled() || redo_action->isEnabled() ||
      readProject(board_path).boards.front().graphics.size() != 1) {
    return 25;
  }
  const QString add_text = window.runAgentUiQueryJson(
      "ui.place_text", "{\"x_mm\":2,\"y_mm\":8,\"text\":\"AGENT\"}");
  if (!add_text.contains("\"performed\":true") ||
      !add_text.contains("\"text_count\":1") || redo_action->isEnabled() ||
      readProject(board_path).boards.front().graphics.size() != 1 ||
      readProject(board_path).boards.front().texts.size() != 1) {
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
  auto* personalisation_scroll = settings.findChild<QScrollArea*>("scroll:personalisation");
  auto* personalisation_page = settings.findChild<QWidget*>("personalisationPage");
  if (personalisation_scroll == nullptr || personalisation_page == nullptr ||
      !personalisation_scroll->styleSheet().contains("#0d1117") ||
      !personalisation_page->styleSheet().contains("#0d1117")) {
    return 21;
  }
  auto* categories = settings.findChild<QListWidget*>("control:categoryList");
  if (categories == nullptr || categories->count() <= 2) {
    return 18;
  }
  categories->setCurrentRow(2);
  QApplication::processEvents();
  auto* semantic_endpoint = settings.findChild<QLineEdit*>("control:semanticMemoryEndpoint");
  auto* semantic_model = settings.findChild<QLineEdit*>("control:semanticMemoryModel");
  if (semantic_endpoint == nullptr || semantic_model == nullptr) {
    return 19;
  }
  const QString typed_endpoint = window.runAgentUiQueryJson(
      "ui.type_text",
      "{\"id\":\"control:semanticMemoryEndpoint\",\"text\":\"http://127.0.0.1:11435\"}");
  const QString typed_model = window.runAgentUiQueryJson(
      "ui.type_text",
      "{\"id\":\"control:semanticMemoryModel\",\"text\":\"embeddinggemma:latest\"}");
  if (!typed_endpoint.contains("\"performed\":true") ||
      !typed_model.contains("\"performed\":true") ||
      semantic_endpoint->text() != "http://127.0.0.1:11435" ||
      semantic_model->text() != "embeddinggemma:latest") {
    return 20;
  }
  auto* public_key = settings.findChild<QLineEdit*>("control:langfusePublicKeyInput");
  auto* secret_key = settings.findChild<QLineEdit*>("control:langfuseSecretKeyInput");
  if (public_key == nullptr || secret_key == nullptr) {
    return 11;
  }
  public_key->setText("public-ui-value-must-not-leak");
  secret_key->setText("secret-ui-value-must-not-leak");
  QLineEdit memory_title;
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
