#include "agent_commands.hpp"

#include "agent_observability_config.hpp"
#include "agent_kicad_evidence.hpp"
#include "agent_policy.hpp"
#include "agent_provider_config.hpp"
#include "agent_session.hpp"
#include "app.hpp"
#include "ccad_core/json.hpp"
#include "common.hpp"
#include "agent_orchestrator_cli.hpp"

#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace ccad_cli {

namespace {

// Extremely basic handwritten parser for newline-delimited JSON-RPC requests.
// We only support {"jsonrpc":"2.0", "method":"...", "params":{...}, "id":...}
// For `execute`, params should have `"args": ["...", "..."]`

std::string extractStringValue(const std::string& json, const std::string& key) {
  std::string search = "\"" + key + "\":";
  auto pos = json.find(search);
  if (pos == std::string::npos) return "";
  pos += search.length();
  while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
  if (pos < json.length() && json[pos] == '"') {
    pos++;
    std::string value;
    bool escaped = false;
    for (; pos < json.length(); ++pos) {
      const char c = json[pos];
      if (escaped) {
        if (c == 'n') value += '\n';
        else if (c == 'r') value += '\r';
        else if (c == 't') value += '\t';
        else if (c == '"' || c == '\\' || c == '/') value += c;
        else { value += '\\'; value += c; }
        escaped = false;
      } else if (c == '\\') {
        escaped = true;
      } else if (c == '"') {
        return value;
      } else {
        value += c;
      }
    }
  }
  return "";
}

std::string extractRawValue(const std::string& json, const std::string& key) {
  std::string search = "\"" + key + "\":";
  auto pos = json.find(search);
  if (pos == std::string::npos) return "";
  pos += search.length();
  while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
  auto end = json.find_first_of(",}", pos);
  if (end != std::string::npos) {
    std::string val = json.substr(pos, end - pos);
    // trim right whitespace
    while (!val.empty() && (val.back() == ' ' || val.back() == '\t')) {
      val.pop_back();
    }
    return val;
  }
  return "";
}

std::string extractRequestId(const std::string& json) {
  const std::string search = "\"id\":";
  const auto found = json.rfind(search);
  if (found == std::string::npos) return "";
  std::size_t pos = found + search.size();
  while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
  if (pos < json.size() && json[pos] == '"') {
    const auto end = json.find('"', pos + 1);
    return end == std::string::npos ? "" : json.substr(pos, end - pos + 1);
  }
  const auto end = json.find_first_of(",}", pos);
  return json.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
}

std::vector<std::string> extractStringArray(const std::string& json, const std::string& key) {
  std::vector<std::string> result;
  std::string search = "\"" + key + "\":";
  auto pos = json.find(search);
  if (pos == std::string::npos) return result;
  pos += search.length();
  while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
  if (pos < json.length() && json[pos] == '[') {
    pos++;
    while (pos < json.length() && json[pos] != ']') {
      while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == ',')) pos++;
      if (pos < json.length() && json[pos] == '"') {
        pos++;
        auto end = json.find("\"", pos);
        if (end != std::string::npos) {
          result.push_back(json.substr(pos, end - pos));
          pos = end + 1;
        } else {
          break;
        }
      } else {
        break;
      }
    }
  }
  return result;
}

std::string formatError(const std::string& id, int code, const std::string& message) {
  std::ostringstream out;
  out << "{\"jsonrpc\": \"2.0\", \"error\": {\"code\": " << code << ", \"message\": \"" << ccad::escapeJson(message) << "\"}";
  if (!id.empty()) {
    out << ", \"id\": " << id;
  } else {
    out << ", \"id\": null";
  }
  out << "}";
  return out.str();
}

std::string formatSuccess(const std::string& id, const std::string& resultJson) {
  std::ostringstream out;
  out << "{\"jsonrpc\": \"2.0\", \"result\": " << resultJson << ", \"id\": " << id << "}";
  return out.str();
}

std::string agentMethodEntryJson(const std::string& method, const std::string& category,
                                 const std::string& title, const bool read_only,
                                 const bool mutates_ui, const bool mutates_project) {
  std::ostringstream out;
  out << "{\"method\":\"" << ccad::escapeJson(method) << "\","
      << "\"category\":\"" << ccad::escapeJson(category) << "\","
      << "\"title\":\"" << ccad::escapeJson(title) << "\","
      << "\"read_only\":" << (read_only ? "true" : "false") << ","
      << "\"mutates_ui\":" << (mutates_ui ? "true" : "false") << ","
      << "\"mutates_project\":" << (mutates_project ? "true" : "false") << "}";
  return out.str();
}

std::string optionOrEmpty(const std::map<std::string, std::string>& options,
                          const std::string& key) {
  const auto found = options.find(key);
  if (found == options.end()) {
    return "";
  }
  return found->second;
}

std::string agentProtocolCatalogJson() {
  const std::vector<std::string> methods = {
      agentMethodEntryJson("agent.methods", "agent", "List Agent Methods", true, false, false),
      agentMethodEntryJson("agent.quickstart", "agent", "Agent Quickstart", true, false, false),
      agentMethodEntryJson("agent.harness_context", "agent", "Harness Context", true, false, false),
      agentMethodEntryJson("agent.state", "agent", "Workspace State", true, false, false),
      agentMethodEntryJson("agent.workspace_state", "agent", "Workspace State Alias", true, false,
                           false),
      agentMethodEntryJson("agent.tasks", "agent", "Task State", true, false, false),
      agentMethodEntryJson("agent.evidence", "agent", "Evidence State", true, false, false),
      agentMethodEntryJson("agent.approvals", "agent", "Approval State", true, false, false),
      agentMethodEntryJson("agent.session_schema", "agent", "Session Schema", true, false, false),
      agentMethodEntryJson("agent.session_state", "agent", "Session State", true, false, false),
      agentMethodEntryJson("agent.replay_manifest", "agent", "Replay Manifest", true, false,
                           false),
      agentMethodEntryJson("agent.policy_schema", "agent", "Policy Schema", true, false, false),
      agentMethodEntryJson("agent.policy_check", "agent", "Policy Check", true, false, false),
      agentMethodEntryJson("agent.run_profile", "agent", "Run Profile", true, false, false),
      agentMethodEntryJson("agent.safety_policy", "agent", "Safety Policy", true, false, false),
      agentMethodEntryJson("agent.provider_policy", "agent", "Provider Policy", true, false, false),
      agentMethodEntryJson("agent.provider_config_schema", "agent", "Provider Config Schema", true,
                           false, false),
      agentMethodEntryJson("agent.provider_config_template", "agent", "Provider Config Template",
                           true, false, false),
      agentMethodEntryJson("agent.provider_status", "agent", "Provider Status", true, false,
                           false),
      agentMethodEntryJson("agent.observability_config", "agent", "Observability Config", true, false,
                           false),
      agentMethodEntryJson("agent.trace_export_schema", "agent", "Trace Export Schema", true, false,
                           false),
      agentMethodEntryJson("agent.trace_export_template", "agent", "Trace Export Template", true,
                           false, false),
      agentMethodEntryJson("agent.trace_redaction_policy", "agent", "Trace Redaction Policy", true,
                           false, false),
      agentMethodEntryJson("agent.trace_export_dry_run", "agent", "Trace Export Dry Run", true,
                           false, false),
      agentMethodEntryJson("agent.kicad_evidence_schema", "agent", "KiCad Evidence Schema", true,
                           false, false),
      agentMethodEntryJson("agent.kicad_evidence_plan", "agent", "KiCad Evidence Plan", true,
                           false, false),
      agentMethodEntryJson("agent.kicad_evidence_dry_run", "agent", "KiCad Evidence Dry Run", true,
                           false, false),
      agentMethodEntryJson("agent.kicad_evidence_run", "agent", "KiCad Evidence Run", false,
                           false, false),
      agentMethodEntryJson("agent.pcb_api_schema", "agent", "PCB API Parity Schema", true, false,
                           false),
      agentMethodEntryJson("agent.evidence_manifest_schema", "agent", "Evidence Manifest Schema",
                           true, false, false),
      agentMethodEntryJson("agent.tool_guide", "agent", "Tool Guide", true, false, false),
      agentMethodEntryJson("ccad_execute", "cli_tool", "Execute CCad CLI", false, false, true)};
  std::ostringstream out;
  out << "{\"schema_version\":1,\"catalog_kind\":\"ccad_agent_protocol\","
      << "\"surface\":\"headless_cli\",\"method_count\":" << methods.size() << ",\"methods\":[";
  for (std::size_t i = 0; i < methods.size(); ++i) {
    if (i > 0) {
      out << ',';
    }
    out << methods[i];
  }
  out << "],\"reference_model\":\"KiCad-style named actions plus MCP-style tool schemas for LLM-native use\"}";
  return out.str();
}

std::string agentQuickstartJson() {
  return "{\"schema_version\":1,\"workflow\":\"ccad_cli_agent_loop\","
         "\"summary\":\"Discover methods, inspect project state with CLI commands, run deterministic tools, and use GUI map or screenshots only when visual proof is required.\","
         "\"first_methods\":[\"agent.methods\",\"agent.state\",\"agent.session_schema\","
         "\"agent.policy_schema\",\"agent.policy_check\",\"agent.session_state\","
         "\"agent.replay_manifest\",\"agent.provider_config_schema\","
         "\"agent.provider_config_template\",\"agent.provider_status\","
         "\"agent.trace_export_schema\",\"agent.trace_export_dry_run\",\"agent.tasks\","
         "\"agent.kicad_evidence_schema\",\"agent.kicad_evidence_plan\","
         "\"agent.kicad_evidence_dry_run\",\"agent.pcb_api_schema\",\"agent.evidence\","
         "\"agent.approvals\",\"agent.harness_context\","
         "\"agent.tool_guide\",\"tools/list\"],"
         "\"screenshot_rule\":\"GUI screenshots must use the project visual-validation harness with beep and current settle waits\","
         "\"unsafe_rule\":\"Write commands require explicit --allow-write in agent serve and direct human approval when policy requires it\"}";
}

std::string agentHarnessContextJson() {
  return "{\"schema_version\":1,\"harness_kind\":\"ccad_headless_cli_agent_surface\","
         "\"session_state\":{\"project_path\":null,\"active_view\":\"headless\","
         "\"ui_epoch\":null,\"selected_object_ids\":[],\"provider_configured\":false,"
         "\"last_verified_visual_artifact\":null,\"transaction_id\":null},"
         "\"pending_diagnostics\":{\"erc_count\":0,\"drc_count\":0,\"error_count\":0,\"warning_count\":0},"
         "\"capabilities\":[\"agent.methods\",\"agent.state\",\"agent.session_schema\","
         "\"agent.policy_schema\",\"agent.policy_check\",\"agent.session_state\","
         "\"agent.replay_manifest\",\"agent.provider_config_schema\","
         "\"agent.provider_config_template\",\"agent.provider_status\","
         "\"agent.trace_export_schema\",\"agent.trace_export_dry_run\","
         "\"agent.kicad_evidence_schema\",\"agent.kicad_evidence_plan\","
         "\"agent.kicad_evidence_dry_run\",\"agent.pcb_api_schema\",\"agent.tasks\",\"agent.evidence\","
         "\"agent.approvals\",\"agent.tool_guide\",\"ccad_execute\","
         "\"mcp_stdio\"],"
         "\"visual_validation_policy\":{\"single_preview_wait_seconds\":7,"
         "\"multi_action_initial_wait_seconds\":5,\"multi_action_step_wait_ms\":800,"
         "\"beep_before_gui_test\":true}}";
}

std::string agentPcbApiSchemaJson() {
  struct Mapping {
    const char* handler;
    const char* command;
    const char* status;
    const char* scope;
  };

  struct EnumGroup {
    const char* api_enum;
    const char* kicad_native;
    std::vector<const char*> values;
    const char* ccad_status;
    const char* compatibility_note;
  };

  const std::vector<Mapping> mappings = {
      {"RunAction", "ui.trigger_safe", "gui_only_first_slice", "non_headless_tool_action"},
      {"GetOpenDocuments", "inspect", "first_slice", "loaded_project_summary"},
      {"SaveDocument", "project_mutation_commands", "available", "commands_write_project_file"},
      {"SaveCopyOfDocument", "", "gap", "copy_document_api_missing"},
      {"RevertDocument", "", "gap", "revert_document_api_missing"},
      {"GetItems", "pcb list-objects", "available", "board_object_query"},
      {"GetItemsById", "pcb get-object", "available", "single_board_object_query"},
      {"GetSelection", "ui.map / ui.get_node", "gui_only_first_slice", "selection_state_not_persisted_headless"},
      {"ClearSelection", "ui.cancel_tool", "gui_only_first_slice", "selection_mutation_gui_only"},
      {"AddToSelection", "", "gap", "multi_selection_mutation_missing"},
      {"RemoveFromSelection", "", "gap", "multi_selection_mutation_missing"},
      {"GetBoardStackup", "pcb get-board-stackup", "first_slice", "default_stackup_first_slice"},
      {"GetBoardEnabledLayers", "pcb list-enabled-layers", "first_slice", "enabled_layers_from_project"},
      {"SetBoardEnabledLayers", "pcb add-layer / pcb remove-layer", "first_slice", "layer_registry_mutation"},
      {"GetGraphicsDefaults", "", "gap", "graphics_defaults_model_missing"},
      {"GetBoardDesignRules", "pcb get-rules", "first_slice", "minimum_constraints"},
      {"SetBoardDesignRules", "pcb set-rules", "first_slice", "minimum_constraints"},
      {"GetCustomDesignRules", "", "gap", "custom_rule_language_missing"},
      {"SetCustomDesignRules", "", "gap", "custom_rule_language_missing"},
      {"GetBoundingBox", "pcb get-outline", "first_slice", "rectangular_board_outline"},
      {"GetPadShapeAsPolygon", "", "gap", "pad_polygon_geometry_missing"},
      {"CheckPadstackPresenceOnLayers", "pcb list-objects", "first_slice", "resolved_pad_layer_sets"},
      {"ExpandTextVariables", "pcb expand-text-variables", "first_slice",
       "project_text_variable_expansion_first_slice"},
      {"GetBoardOrigin", "pcb get-outline", "first_slice", "outline_origin"},
      {"SetBoardOrigin", "pcb set-outline", "first_slice", "outline_origin"},
      {"GetBoardLayerName", "pcb get-layer-name", "available", "layer_name_lookup"},
      {"InteractiveMoveItems", "ui.route_track / ui.place_via", "gui_only_first_slice", "tool_specific_move_paths"},
      {"GetNets", "pcb list-nets", "available", "physical_net_summary"},
      {"GetConnectedItems", "pcb list-connected", "first_slice", "net_equivalent_first_slice"},
      {"GetItemsByNet", "pcb list-by-net", "first_slice", "net_equivalent_first_slice"},
      {"GetItemsByNetClass", "", "gap", "netclass_model_missing"},
      {"GetNetClassForNets", "", "gap", "netclass_model_missing"},
      {"RefillZones", "pcb refill-zones", "first_slice", "deterministic_contour_refill"},
      {"SaveDocumentToString", "", "gap", "document_string_export_api_missing"},
      {"SaveSelectionToString", "", "gap", "selection_serialization_missing"},
      {"ParseAndCreateItemsFromString", "", "gap", "clipboard_parse_create_api_missing"},
      {"GetVisibleLayers", "pcb list-visible-layers", "first_slice", "visible_layers_from_project"},
      {"SetVisibleLayers", "pcb set-layer-visibility", "first_slice", "visible_layers_from_project"},
      {"GetActiveLayer", "ui.current_tool / control:active_pcb_layer", "gui_only_first_slice", "active_layer_gui_state"},
      {"SetActiveLayer", "ui.set_active_layer", "gui_only_first_slice", "active_layer_gui_state"},
      {"GetBoardEditorAppearanceSettings", "ui.map", "first_slice", "appearance_state_sparse"},
      {"SetBoardEditorAppearanceSettings", "", "gap", "appearance_settings_mutation_missing"},
      {"InjectDrcError", "", "gap", "diagnostic_injection_missing"},
      {"RunBoardJobExport3D", "", "gap", "3d_export_missing"},
      {"RunBoardJobExportRender", "", "gap", "render_export_missing"},
      {"RunBoardJobExportSvg", "", "gap", "svg_plot_export_missing"},
      {"RunBoardJobExportDxf", "", "gap", "dxf_plot_export_missing"},
      {"RunBoardJobExportPdf", "", "gap", "pdf_plot_export_missing"},
      {"RunBoardJobExportPs", "", "gap", "postscript_plot_export_missing"},
      {"RunBoardJobExportGerbers", "agent kicad-evidence-plan --kind pcb-export-gerbers", "evidence_plan_only", "external_kicad_cli_plan"},
      {"RunBoardJobExportDrill", "pcb export-drill", "first_slice", "excellon_drill_export"},
      {"RunBoardJobExportPosition", "pcb export-pnp", "first_slice", "position_export"},
      {"RunBoardJobExportGencad", "", "gap", "gencad_export_missing"},
      {"RunBoardJobExportIpc2581", "agent kicad-evidence-plan --kind pcb-export-ipc2581", "evidence_plan_only", "external_kicad_cli_plan"},
      {"RunBoardJobExportIpcD356", "", "gap", "ipcd356_export_missing"},
      {"RunBoardJobExportODB", "agent kicad-evidence-plan --kind pcb-export-odb", "evidence_plan_only", "external_kicad_cli_plan"},
      {"RunBoardJobExportStats", "", "gap", "pcb_stats_export_missing"},
      {"GetPageSettings", "", "gap", "page_settings_model_missing"},
      {"SetPageSettings", "", "gap", "page_settings_model_missing"}};

  const std::vector<EnumGroup> enum_groups = {
      {"types::PadType",
       "PAD_ATTRIB",
       {"PT_PTH", "PT_SMD", "PT_EDGE_CONNECTOR", "PT_NPTH"},
       "first_slice",
       "CCad stores pad type strings and round-trips imported KiCad pad attributes through pad metadata."},
      {"types::DrillShape",
       "PAD_DRILL_SHAPE",
       {"DS_CIRCLE", "DS_OBLONG", "DS_UNDEFINED"},
       "first_slice",
       "CCad currently models circular drill diameter directly and keeps richer drill shape parity as metadata/backlog."},
      {"types::PadStackShape",
       "PAD_SHAPE",
       {"PSS_CIRCLE",
        "PSS_RECTANGLE",
        "PSS_OVAL",
        "PSS_TRAPEZOID",
        "PSS_ROUNDRECT",
        "PSS_CHAMFEREDRECT",
        "PSS_CUSTOM"},
       "first_slice",
       "CCad renders common pad shapes and records custom pad-shape parity as a remaining polygon-geometry gap."},
      {"types::PadStackType",
       "PADSTACK::MODE",
       {"PST_NORMAL", "PST_FRONT_INNER_BACK", "PST_CUSTOM"},
       "gap",
       "KiCad padstack mode needs a richer per-layer padstack model than CCad currently has."},
      {"types::ViaType",
       "VIATYPE",
       {"VT_THROUGH", "VT_BLIND", "VT_BURIED", "VT_MICRO"},
       "first_slice",
       "CCad stores through vias today; blind, buried, and microvia behavior remain model and DRC backlog."},
      {"types::ZoneConnectionStyle",
       "ZONE_CONNECTION",
       {"ZCS_INHERITED", "ZCS_NONE", "ZCS_THERMAL", "ZCS_FULL", "ZCS_PTH_THERMAL"},
       "first_slice",
       "CCad stores zone pad-connection intent but does not yet run KiCad-equivalent zone-fill thermal geometry."},
      {"CustomRuleConstraintType",
       "DRC_CONSTRAINT_T",
       {"CRCT_CLEARANCE",
        "CRCT_CREEPAGE",
        "CRCT_HOLE_CLEARANCE",
        "CRCT_EDGE_CLEARANCE",
        "CRCT_TRACK_WIDTH",
        "CRCT_ANNULAR_WIDTH",
        "CRCT_DIFF_PAIR_GAP",
        "CRCT_MAX_UNCOUPLED",
        "CRCT_VIA_COUNT",
        "CRCT_TRACK_ANGLE",
        "CRCT_VIA_DANGLING",
        "CRCT_NET_CHAIN_RETURN_PATH"},
       "gap",
       "CCad has minimum board rules only; KiCad custom-rule language and full constraint taxonomy are backlog."},
      {"DrillFormat",
       "JOB_EXPORT_PCB_DRILL::DRILL_FORMAT",
       {"DF_EXCELLON", "DF_GERBER"},
       "first_slice",
       "CCad exposes Excellon drill export and tracks KiCad Gerber-drill parity for export expansion."},
      {"DrillMapFormat",
       "JOB_EXPORT_PCB_DRILL::MAP_FORMAT",
       {"DMF_POSTSCRIPT", "DMF_GERBER_X2", "DMF_DXF", "DMF_SVG", "DMF_PDF"},
       "gap",
       "Drill map plotting formats are not yet implemented in CCad exports."},
      {"PositionFormat",
       "JOB_EXPORT_PCB_POS::FORMAT",
       {"PF_ASCII", "PF_CSV", "PF_GERBER"},
       "first_slice",
       "CCad exposes a first position export and needs KiCad option parity in later exporter work."},
      {"Ipc2581Version",
       "JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION",
       {"IPC2581V_B", "IPC2581V_C"},
       "evidence_plan_only",
       "CCad currently routes IPC-2581 through guarded KiCad evidence planning, not a native exporter."},
      {"OdbCompression",
       "JOB_EXPORT_PCB_ODB::ODB_COMPRESSION",
       {"ODBC_NONE", "ODBC_ZIP", "ODBC_TGZ"},
       "evidence_plan_only",
       "CCad currently routes ODB++ through guarded KiCad evidence planning, not a native exporter."},
      {"DrcErrorType",
       "PCB_DRC_CODE",
       {"DRCET_UNCONNECTED_ITEMS",
        "DRCET_CLEARANCE",
        "DRCET_TRACKS_CROSSING",
        "DRCET_DANGLING_VIA",
        "DRCET_DANGLING_TRACK",
        "DRCET_TRACK_WIDTH",
        "DRCET_ANNULAR_WIDTH",
        "DRCET_VIA_DIAMETER",
        "DRCET_PADSTACK",
        "DRCET_INVALID_OUTLINE",
        "DRCET_NET_CONFLICT",
        "DRCET_DIFF_PAIR_GAP_OUT_OF_RANGE",
        "DRCET_TRACK_ON_POST_MACHINED_LAYER"},
       "first_slice",
       "CCad reports its own diagnostic codes today and needs a translation layer for KiCad-equivalent DRC evidence."}};

  std::ostringstream out;
  out << "{\"schema_version\":1,"
      << "\"schema_kind\":\"ccad_agent_pcb_api_schema\","
      << "\"surface\":\"headless_cli\","
      << "\"source_reference\":\"F:/kicad_src/pcbnew/api/api_handler_pcb.cpp\","
      << "\"source_header\":\"F:/kicad_src/pcbnew/api/api_handler_pcb.h\","
      << "\"source_files_read\":[\"F:/kicad_src/pcbnew/api/api_handler_pcb.cpp\","
      << "\"F:/kicad_src/pcbnew/api/api_handler_pcb.h\"],"
      << "\"next_file\":\"F:/kicad_src/pcbnew/api/api_pcb_enums.cpp\","
      << "\"external_behavior_reference\":\"KiCad PCB editor API_HANDLER_PCB handlers\","
      << "\"ledger_status\":\"api_handler_pcb_cpp_registered_handlers_read\","
      << "\"enum_ledger_status\":\"api_pcb_enums_cpp_read\","
      << "\"source_enum_reference\":\"F:/kicad_src/pcbnew/api/api_pcb_enums.cpp\","
      << "\"context_ledger_status\":\"board_context_h_cpp_read\","
      << "\"source_context_reference\":\"F:/kicad_src/pcbnew/api/board_context.h\","
      << "\"headless_context_ledger_status\":\"headless_board_context_h_cpp_read\","
      << "\"source_headless_context_reference\":\"F:/kicad_src/pcbnew/api/headless_board_context.h\","
      << "\"api_folder_status\":\"complete_first_audit\","
      << "\"handler_count\":" << mappings.size() << ','
      << "\"connectivity_scope\":\"net_equivalent_first_slice\","
      << "\"limitations\":[\"CCad currently groups connectable objects by explicit net_id\","
      << "\"Full KiCad connectivity graph traversal still needs a kernel connectivity engine\","
      << "\"GUI-only handlers require the Qt UI-map surface until a kernel transaction analogue exists\"],"
      << "\"status_legend\":{\"available\":\"headless command covers the current handler shape\","
      << "\"first_slice\":\"partial compatible surface with documented limits\","
      << "\"gui_only_first_slice\":\"currently available only through native GUI agent methods\","
      << "\"evidence_plan_only\":\"planned through guarded KiCad CLI evidence tooling\","
      << "\"gap\":\"no complete CCad analogue yet\"},"
      << "\"enum_reference\":{\"source\":\"F:/kicad_src/pcbnew/api/api_pcb_enums.cpp\","
      << "\"next_file\":\"F:/kicad_src/pcbnew/api/board_context.cpp\","
      << "\"groups\":[";
  for (std::size_t i = 0; i < enum_groups.size(); ++i) {
    const EnumGroup& group = enum_groups.at(i);
    if (i > 0) {
      out << ',';
    }
    out << "{\"api_enum\":\"" << ccad::escapeJson(group.api_enum) << "\","
        << "\"kicad_native\":\"" << ccad::escapeJson(group.kicad_native) << "\","
        << "\"ccad_status\":\"" << ccad::escapeJson(group.ccad_status) << "\","
        << "\"compatibility_note\":\"" << ccad::escapeJson(group.compatibility_note)
        << "\",\"values\":[";
    for (std::size_t j = 0; j < group.values.size(); ++j) {
      if (j > 0) {
        out << ',';
      }
      out << '"' << ccad::escapeJson(group.values.at(j)) << '"';
    }
    out << "]}";
  }
  out << "]},"
      << "\"context_reference\":{\"source_header\":\"F:/kicad_src/pcbnew/api/board_context.h\","
      << "\"source_impl\":\"F:/kicad_src/pcbnew/api/board_context.cpp\","
      << "\"next_file\":\"F:/kicad_src/pcbnew/api/headless_board_context.cpp\","
      << "\"context_status\":\"split_context_gap\","
      << "\"ccad_analogue\":\"headless_cli_project_file_context_and_qt_review_window_context\","
      << "\"methods\":[\"GetBoard\",\"Prj\",\"GetToolManager\",\"GetKiway\","
      << "\"GetCurrentFileName\",\"CanAcceptApiCommands\",\"SaveBoard\",\"SavePcbCopy\"],"
      << "\"compatibility_note\":\"KiCad isolates PCB API handlers behind BOARD_CONTEXT so GUI and headless callers share one handler surface; CCad currently splits this between CLI file commands, JSON-RPC agent commands, and ReviewWindow UI-map methods, so a shared board-session context remains backlog.\"},"
      << "\"headless_context_reference\":{\"source_header\":\"F:/kicad_src/pcbnew/api/headless_board_context.h\","
      << "\"source_impl\":\"F:/kicad_src/pcbnew/api/headless_board_context.cpp\","
      << "\"kicad_class\":\"HEADLESS_BOARD_CONTEXT\","
      << "\"command_acceptance\":\"always_true_headless\","
      << "\"ownership\":[\"owned_board\",\"borrowed_project\",\"owned_tool_manager\","
      << "\"borrowed_settings\",\"optional_kiway\"],"
      << "\"lifecycle\":[\"board_set_project\",\"tool_environment_setup\","
      << "\"project_linkage_teardown\",\"save_board\",\"save_pcb_copy_optional_project\"],"
      << "\"ccad_status\":\"gap\","
      << "\"compatibility_note\":\"KiCad headless PCB API calls hold a live board/project/tool context and can save the active board or a copy. CCad commands currently reload and rewrite project files per command, so a reusable headless board-session context is the next architectural gap before full API-handler parity.\"},"
      << "\"mappings\":[";
  for (std::size_t i = 0; i < mappings.size(); ++i) {
    const Mapping& mapping = mappings.at(i);
    if (i > 0) {
      out << ',';
    }
    out << "{\"kicad_handler\":\"" << ccad::escapeJson(mapping.handler) << "\",";
    if (mapping.command[0] != '\0') {
      out << "\"ccad_command\":\"" << ccad::escapeJson(mapping.command) << "\",";
    }
    out << "\"status\":\"" << ccad::escapeJson(mapping.status) << "\","
        << "\"scope\":\"" << ccad::escapeJson(mapping.scope) << "\"";
    if (std::string(mapping.scope) == "net_equivalent_first_slice") {
      out << ",\"connectivity_scope\":\"net_equivalent_first_slice\","
          << "\"object_types\":[\"pad\",\"via\",\"track\",\"zone\"]";
    }
    out << '}';
  }
  out << "],"
      << "\"agent_usage\":[\"Call agent.pcb_api_schema before PCB API parity work\","
      << "\"Use status gap entries as the next implementation backlog\","
      << "\"Use pcb list-connected for selected-object neighborhood context\","
      << "\"Use pcb list-by-net for same-net board inspection\"]}";
  return out.str();
}

std::string agentWorkspaceStateJson() {
  return "{\"schema_version\":1,\"workspace_kind\":\"ccad_agent_workspace_state\","
         "\"surface\":\"headless_cli\",\"panel_layout\":\"headless_cli_workspace\","
         "\"session\":{\"session_id\":null,\"session_title\":\"Headless CLI Agent\","
         "\"model_label\":\"none\",\"mode_label\":\"local read-only\"},"
         "\"project\":null,\"ui_epoch\":null,"
         "\"workspace_context\":{\"active_view\":\"headless\",\"active_layer\":null,"
         "\"active_net\":null,\"interaction_mode\":\"none\"},"
         "\"goal\":\"\",\"task_state\":\"Task idle\",\"tasks\":[],"
         "\"evidence_count\":0,\"evidence\":[],"
         "\"approval_pending_count\":0,\"approval_request\":\"\",\"approval_input\":\"\","
         "\"approval_status\":\"No pending approval\",\"approval_last_decision\":\"none\","
         "\"command_input\":\"\",\"staged_command\":\"\","
         "\"durable_store\":\"not_configured\",\"provider_configured\":false,"
         "\"trace_export_configured\":false}";
}

std::string agentTasksJson() {
  return "{\"schema_version\":1,\"state_kind\":\"ccad_agent_tasks\","
         "\"surface\":\"headless_cli\",\"durable_store\":\"not_configured\","
         "\"current_goal\":\"\",\"task_count\":0,\"tasks\":[],"
         "\"persistence_note\":\"Sprint 192 is read-only CLI parity; Sprint 193 adds durable session files\","
         "\"next_step\":\"sprint_193_durable_session_schema\"}";
}

std::string agentEvidenceJson() {
  return "{\"schema_version\":1,\"state_kind\":\"ccad_agent_evidence\","
         "\"surface\":\"headless_cli\",\"durable_store\":\"not_configured\","
         "\"evidence_count\":0,\"evidence\":[],"
         "\"manifest_schema_method\":\"agent.evidence_manifest_schema\","
         "\"artifact_policy\":{\"screenshots_require_visual_harness\":true,"
         "\"drc_and_erc_reports_are_structured_artifacts\":true,"
         "\"secrets_must_not_enter_evidence\":true}}";
}

std::string agentApprovalsJson() {
  return "{\"schema_version\":1,\"state_kind\":\"ccad_agent_approvals\","
         "\"surface\":\"headless_cli\",\"durable_store\":\"not_configured\","
         "\"pending_count\":0,\"pending\":[],\"last_decision\":\"none\","
         "\"approval_required_actions\":[\"delete_file\",\"overwrite_project\","
         "\"fabrication_export\",\"release_gerbers\",\"remote_model_upload\","
         "\"broad_filesystem_action\",\"external_network_action\"],"
         "\"outcomes\":[\"accept\",\"decline\",\"cancel\"]}";
}

std::string agentRunProfileJson() {
  return "{\"schema_version\":1,\"profile_kind\":\"ccad_agent_run_profile\","
         "\"durability_reference\":\"langgraph\","
         "\"loop\":[\"plan\",\"act\",\"observe\",\"verify\",\"repair_or_stop\"],"
         "\"retry_policy\":{\"default_max_attempts\":3,\"backoff\":\"exponential_with_jitter\"},"
         "\"circuit_breaker\":{\"max_repeated_same_failure\":3,\"max_consecutive_tool_errors\":5},"
         "\"stop_conditions\":[\"human_approval_required\",\"circuit_breaker_open\",\"verification_gate_failed_after_retries\"]}";
}

std::string agentSafetyPolicyJson() {
  return "{\"schema_version\":1,\"policy_kind\":\"ccad_agent_safety_policy\","
         "\"no_project_file_execution\":true,\"design_files_are_data\":true,"
         "\"secret_storage\":\"env_or_os_credential_store_only\","
         "\"human_approval_required\":[\"delete_file\",\"overwrite_project\",\"fabrication_export\","
         "\"release_gerbers\",\"remote_model_upload\",\"broad_filesystem_action\","
         "\"external_network_action\"],\"approval_response\":\"approval_required\"}";
}

std::string agentProviderPolicyJson() {
  return "{\"schema_version\":1,\"policy_kind\":\"ccad_agent_provider_policy\","
         "\"preferred_model_access\":\"byok\","
         "\"allowed_paths\":[\"official_api\",\"openai_compatible_api\",\"anthropic_api\","
         "\"google_gemini_api\",\"local_model_server\",\"future_user_installed_connector\"],"
         "\"disallowed_paths\":[\"no_consumer_web_ui_automation\"],"
         "\"secret_storage\":\"environment_or_os_credential_store\","
         "\"project_file_secret_storage\":false,"
         "\"configuration_schema_method\":\"agent.provider_config_schema\","
         "\"configuration_template_method\":\"agent.provider_config_template\","
         "\"status_method\":\"agent.provider_status\"}";
}

std::string agentEvidenceManifestSchemaJson() {
  return "{\"schema_version\":1,\"manifest_kind\":\"ccad_agent_evidence_manifest\","
         "\"required_fields\":[\"sprint_id\",\"project_path\",\"source_references\","
         "\"tool_calls\",\"screenshots\",\"drc_reports\",\"erc_reports\",\"design_artifacts\","
         "\"decisions\",\"redactions\"],"
         "\"artifact_fields\":[\"kind\",\"path\",\"created_at\",\"sha256\",\"producer_method\","
         "\"project_id\",\"transaction_id\",\"ui_epoch\"],"
         "\"evidence_cards\":{\"producer\":\"AgentPanel::pinEvidence\","
         "\"workspace_state_field\":\"evidence_cards\",\"bounded_count\":8,"
         "\"inline_payload_policy\":\"metadata_and_summaries_only\"},"
         "\"card_fields\":[\"id\",\"kind\",\"title\",\"summary\",\"method\",\"artifact_path\","
         "\"created_at\",\"trace_id\",\"span_id\",\"source\",\"diagnostic_count\","
         "\"error_count\",\"warning_count\",\"drc_count\",\"erc_count\",\"width\",\"height\"],"
         "\"card_kinds\":[\"tool_result\",\"screenshot\",\"drc_report\",\"erc_report\","
         "\"diagnostics_report\",\"review_report\"]}";
}

std::string preferredSurfaceForMethod(const std::string& method) {
  if (method == "agent.state" || method == "agent.workspace_state" || method == "agent.tasks" ||
      method == "agent.evidence" || method == "agent.approvals") {
    return "headless_cli_workspace_state";
  }
  if (method == "agent.session_schema" || method == "agent.session_state" ||
      method == "agent.replay_manifest") {
    return "local_agent_session_file";
  }
  if (method == "agent.policy_schema" || method == "agent.policy_check") {
    return "headless_cli_policy_gate";
  }
  if (method == "agent.provider_config_schema" ||
      method == "agent.provider_config_template" ||
      method == "agent.provider_status") {
    return "headless_cli_provider_config";
  }
  if (method == "agent.observability_config" || method == "agent.trace_export_schema" ||
      method == "agent.trace_export_template" || method == "agent.trace_redaction_policy" ||
      method == "agent.trace_export_dry_run") {
    return "headless_cli_observability_config";
  }
  if (method == "agent.kicad_evidence_schema" || method == "agent.kicad_evidence_plan" ||
      method == "agent.kicad_evidence_dry_run" || method == "agent.kicad_evidence_run") {
    return "headless_cli_kicad_evidence";
  }
  if (method == "agent.pcb_api_schema") {
    return "headless_cli_pcb_api_parity";
  }
  if (method.rfind("agent.", 0) == 0) {
    return "read_only_protocol_metadata";
  }
  if (method == "ccad_execute") {
    return "headless_cli";
  }
  return "kernel_or_cli_first";
}

std::string agentToolGuideJson(const std::string& method) {
  const bool found = method == "agent.methods" || method == "agent.quickstart" ||
                     method == "agent.harness_context" || method == "agent.run_profile" ||
                     method == "agent.safety_policy" || method == "agent.provider_policy" ||
                     method == "agent.state" || method == "agent.workspace_state" ||
                     method == "agent.tasks" || method == "agent.evidence" ||
                     method == "agent.approvals" ||
                     method == "agent.session_schema" || method == "agent.session_state" ||
                     method == "agent.replay_manifest" ||
                     method == "agent.policy_schema" || method == "agent.policy_check" ||
                     method == "agent.provider_config_schema" ||
                     method == "agent.provider_config_template" ||
                     method == "agent.provider_status" ||
                     method == "agent.observability_config" ||
                     method == "agent.trace_export_schema" ||
                     method == "agent.trace_export_template" ||
                     method == "agent.trace_redaction_policy" ||
                     method == "agent.trace_export_dry_run" ||
                     method == "agent.kicad_evidence_schema" ||
                     method == "agent.kicad_evidence_plan" ||
                     method == "agent.kicad_evidence_dry_run" ||
                     method == "agent.kicad_evidence_run" ||
                     method == "agent.pcb_api_schema" ||
                     method == "agent.evidence_manifest_schema" || method == "agent.tool_guide" ||
                     method == "ccad_execute";
  std::ostringstream out;
  out << "{\"schema_version\":1,\"guide_kind\":\"ccad_agent_tool_guide\","
      << "\"method\":\"" << ccad::escapeJson(method) << "\",\"found\":"
      << (found ? "true" : "false");
  if (!found) {
    out << ",\"reason\":\"method_not_found\","
        << "\"recovery_loop\":[\"call agent.methods\",\"choose a known method\","
        << "\"retry with method_name\"]}";
    return out.str();
  }
  out << ",\"preferred_surface\":\"" << preferredSurfaceForMethod(method) << "\","
      << "\"verification\":[\"check JSON-RPC result or direct command exit code\","
      << "\"run project diagnostics after design mutations\","
      << "\"use GUI visual validation harness for visual proof\"],"
      << "\"recovery_loop\":[\"inspect agent.methods\",\"dry-run write tools when available\","
      << "\"retry transient failures\",\"stop on approval_required or circuit breaker\"]}";
  return out.str();
}

std::string agentMetadataJson(const std::string& command, const std::string& method_name = "") {
  if (command == "methods") return agentProtocolCatalogJson();
  if (command == "quickstart") return agentQuickstartJson();
  if (command == "harness-context" || command == "harness_context") return agentHarnessContextJson();
  if (command == "state" || command == "workspace-state" || command == "workspace_state") {
    return agentWorkspaceStateJson();
  }
  if (command == "tasks") return agentTasksJson();
  if (command == "evidence") return agentEvidenceJson();
  if (command == "approvals") return agentApprovalsJson();
  if (command == "session-schema" || command == "session_schema") return agentSessionSchemaJson();
  if (command == "policy-schema" || command == "policy_schema") return agentPolicySchemaJson();
  if (command == "run-profile" || command == "run_profile") return agentRunProfileJson();
  if (command == "safety-policy" || command == "safety_policy") return agentSafetyPolicyJson();
  if (command == "provider-policy" || command == "provider_policy") return agentProviderPolicyJson();
  if (command == "provider-config-schema" || command == "provider_config_schema") {
    return agentProviderConfigSchemaJson();
  }
  if (command == "provider-config-template" || command == "provider_config_template") {
    return agentProviderConfigTemplateJson();
  }
  if (command == "provider-status" || command == "provider_status") {
    return agentProviderStatusJson();
  }
  if (command == "observability-config" || command == "observability_config") {
    return agentObservabilityConfigJson();
  }
  if (command == "trace-export-schema" || command == "trace_export_schema") {
    return agentTraceExportSchemaJson();
  }
  if (command == "trace-export-template" || command == "trace_export_template") {
    return agentTraceExportTemplateJson();
  }
  if (command == "trace-redaction-policy" || command == "trace_redaction_policy") {
    return agentTraceRedactionPolicyJson();
  }
  if (command == "trace-export-dry-run" || command == "trace_export_dry_run") {
    return agentTraceExportDryRunJson();
  }
  if (command == "kicad-evidence-schema" || command == "kicad_evidence_schema") {
    return agentKiCadEvidenceSchemaJson();
  }
  if (command == "pcb-api-schema" || command == "pcb_api_schema") {
    return agentPcbApiSchemaJson();
  }
  if (command == "evidence-manifest-schema" || command == "evidence_manifest_schema") {
    return agentEvidenceManifestSchemaJson();
  }
  if (command == "tool-guide" || command == "tool_guide") return agentToolGuideJson(method_name);
  return "";
}

std::map<std::string, std::string> extractKiCadEvidenceOptions(const std::string& line) {
  std::map<std::string, std::string> options;
  const auto addString = [&options, &line](const std::string& key) {
    const std::string value = extractStringValue(line, key);
    if (!value.empty()) options.emplace(key, value);
  };
  const auto addBool = [&options, &line](const std::string& key) {
    if (extractRawValue(line, key) == "true") options.emplace(key, "true");
  };
  addString("kind");
  addString("input_path");
  addString("output_path");
  addString("format");
  addString("units");
  addString("severity");
  addString("layers");
  addString("common_layers");
  addString("side");
  addString("kicad_cli");
  addBool("exit_code_violations");
  addBool("schematic_parity");
  addBool("refill_zones");
  addBool("save_board");
  addBool("generate_map");
  addBool("generate_report");
  addBool("execute");
  return options;
}

}  // namespace

int agentCommand(const std::vector<std::string>& args) {
  if (args.empty()) {
    std::cerr << "Usage: ccad agent <serve|methods|quickstart|harness-context|state|tasks|evidence|approvals|session-schema|session-new|session-state|checkpoint-add|replay|policy-schema|policy-check|dry-run|run-profile|safety-policy|provider-policy|provider-config-schema|provider-config-template|provider-status|observability-config|trace-export-schema|trace-export-template|trace-redaction-policy|trace-export-dry-run|kicad-evidence-schema|kicad-evidence-plan|kicad-evidence-dry-run|kicad-evidence-run|evidence-manifest-schema|tool-guide>\n";
    return 1;
  }

  if (args[0] != "serve") {
    try {
      if (args[0] == "orchestrate" || args[0] == "plan" || args[0] == "orchestrator-schema") {
        return agentOrchestratorCommand(args);
      }
      if (args[0] == "session-new") {
        const std::map<std::string, std::string> options =
            parseOptions(args, 1, {"--out", "--session-id", "--title", "--project", "--created-at"});
        std::cout << createAgentSessionFile(requireOption(options, "--out"),
                                            requireOption(options, "--session-id"),
                                            optionOrEmpty(options, "--title"),
                                            optionOrEmpty(options, "--project"),
                                            optionOrEmpty(options, "--created-at"))
                  << "\n";
        return 0;
      }
      if (args[0] == "session-state") {
        const std::map<std::string, std::string> options =
            parseOptions(args, 1, {"--session"});
        std::cout << loadAgentSessionFileJson(requireOption(options, "--session")) << "\n";
        return 0;
      }
      if (args[0] == "checkpoint-add") {
        const std::map<std::string, std::string> options =
            parseOptions(args, 1, {"--session", "--checkpoint-id", "--kind", "--summary",
                                  "--artifact", "--created-at"});
        std::cout << appendAgentCheckpointFile(requireOption(options, "--session"),
                                               requireOption(options, "--checkpoint-id"),
                                               optionOrEmpty(options, "--kind"),
                                               optionOrEmpty(options, "--summary"),
                                               optionOrEmpty(options, "--artifact"),
                                               optionOrEmpty(options, "--created-at"))
                  << "\n";
        return 0;
      }
      if (args[0] == "replay") {
        const std::map<std::string, std::string> options =
            parseOptions(args, 1, {"--session"});
        std::cout << agentReplayManifestForFile(requireOption(options, "--session")) << "\n";
        return 0;
      }
      if (args[0] == "policy-check" || args[0] == "dry-run") {
        std::vector<std::string> command_args(args.begin() + 1, args.end());
        if (!command_args.empty() && command_args.front() == "--") {
          command_args.erase(command_args.begin());
        }
        std::cout << agentCommandPolicyJson(command_args, args[0] == "dry-run") << "\n";
        return 0;
      }
      if (args[0] == "kicad-evidence-plan") {
        std::cout << agentKiCadEvidencePlanJson(args, 1) << "\n";
        return 0;
      }
      if (args[0] == "kicad-evidence-dry-run") {
        std::cout << agentKiCadEvidenceDryRunJson(args, 1) << "\n";
        return 0;
      }
      if (args[0] == "kicad-evidence-run") {
        std::cout << agentKiCadEvidenceRunJson(args, 1) << "\n";
        return 0;
      }
    } catch (const std::exception& error) {
      std::cerr << error.what() << "\n";
      return 2;
    }

    std::string method_name;
    for (std::size_t i = 1; i + 1 < args.size(); ++i) {
      if (args[i] == "--method" || args[i] == "--method-name") {
        method_name = args[i + 1];
      }
    }
    const std::string metadata = agentMetadataJson(args[0], method_name);
    if (!metadata.empty()) {
      std::cout << metadata << "\n";
      return 0;
    }
    std::cerr << "Usage: ccad agent <serve|methods|quickstart|harness-context|state|tasks|evidence|approvals|session-schema|session-new|session-state|checkpoint-add|replay|policy-schema|policy-check|dry-run|run-profile|safety-policy|provider-policy|provider-config-schema|provider-config-template|provider-status|observability-config|trace-export-schema|trace-export-template|trace-redaction-policy|trace-export-dry-run|kicad-evidence-schema|kicad-evidence-plan|kicad-evidence-dry-run|kicad-evidence-run|evidence-manifest-schema|tool-guide>\n";
    return 1;
  }

  bool allow_read = false;
  bool allow_write = false;
  for (std::size_t i = 1; i < args.size(); ++i) {
    if (args[i] == "--allow-read") allow_read = true;
    else if (args[i] == "--allow-write") allow_write = true;
  }

  std::string line;
  while (std::getline(std::cin, line)) {
    if (line.empty()) continue;

    std::string jsonrpc = extractStringValue(line, "jsonrpc");
    if (jsonrpc != "2.0") {
      std::cout << formatError("", -32600, "Invalid Request") << "\n";
      continue;
    }

    std::string id = extractRequestId(line);
    std::string method = extractStringValue(line, "method");

    // MCP notifications have no request id and require no response. Keeping
    // them off stdout preserves strict stdio framing for real MCP clients.
    if (method.rfind("notifications/", 0) == 0) {
      continue;
    } else if (method == "ping") {
      std::cout << formatSuccess(id, "\"pong\"") << "\n";
      std::cout.flush();
    } else if (method == "initialize") {
      std::string res = "{\"protocolVersion\": \"2024-11-05\", \"capabilities\": {\"tools\": {}}, \"serverInfo\": {\"name\": \"ccad\", \"version\": \"1.0.0\"}}";
      std::cout << formatSuccess(id, res) << "\n";
      std::cout.flush();
    } else if (method == "tools/list") {
      std::string res = "{\"tools\": [{\"name\": \"ccad_execute\", \"description\": \"Execute guarded CCad CLI commands\", \"annotations\": {\"readOnlyHint\": false, \"destructiveHint\": true, \"openWorldHint\": false}, \"inputSchema\": {\"type\": \"object\", \"properties\": {\"args\": {\"type\": \"array\", \"items\": {\"type\": \"string\"}}}, \"required\": [\"args\"]}},";
      res += "{\"name\": \"ccad_harness_context\", \"description\": \"Read CCad agent harness contract and safety context\", \"annotations\": {\"readOnlyHint\": true, \"destructiveHint\": false, \"openWorldHint\": false}, \"inputSchema\": {\"type\": \"object\", \"properties\": {}}},";
      res += "{\"name\": \"ccad_workspace_state\", \"description\": \"Read provider-free CCad agent workspace state\", \"annotations\": {\"readOnlyHint\": true, \"destructiveHint\": false, \"openWorldHint\": false}, \"inputSchema\": {\"type\": \"object\", \"properties\": {}}},";
      res += "{\"name\": \"ccad_agent_methods\", \"description\": \"Read CCad agent JSON-RPC method catalog and safety metadata\", \"annotations\": {\"readOnlyHint\": true, \"destructiveHint\": false, \"openWorldHint\": false}, \"inputSchema\": {\"type\": \"object\", \"properties\": {}}}]}";
      std::cout << formatSuccess(id, res) << "\n";
      std::cout.flush();
    } else if (method == "tools/call" && extractStringValue(line, "name") == "ccad_harness_context") {
      const std::string result = agentHarnessContextJson();
      std::cout << formatSuccess(id, "{\"content\":[{\"type\":\"text\",\"text\":\"" +
                                      ccad::escapeJson(result) + "\"}],\"isError\":false}") << "\n";
      std::cout.flush();
    } else if (method == "tools/call" && extractStringValue(line, "name") == "ccad_workspace_state") {
      const std::string result = agentWorkspaceStateJson();
      std::cout << formatSuccess(id, "{\"content\":[{\"type\":\"text\",\"text\":\"" +
                                      ccad::escapeJson(result) + "\"}],\"isError\":false}") << "\n";
      std::cout.flush();
    } else if (method == "tools/call" && extractStringValue(line, "name") == "ccad_agent_methods") {
      const std::string result = agentProtocolCatalogJson();
      std::cout << formatSuccess(id, "{\"content\":[{\"type\":\"text\",\"text\":\"" +
                                      ccad::escapeJson(result) + "\"}],\"isError\":false}") << "\n";
      std::cout.flush();
    } else if (method == "agent.methods") {
      std::cout << formatSuccess(id, agentProtocolCatalogJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.quickstart") {
      std::cout << formatSuccess(id, agentQuickstartJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.harness_context") {
      std::cout << formatSuccess(id, agentHarnessContextJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.state" || method == "agent.workspace_state") {
      std::cout << formatSuccess(id, agentWorkspaceStateJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.tasks") {
      std::cout << formatSuccess(id, agentTasksJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.evidence") {
      std::cout << formatSuccess(id, agentEvidenceJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.approvals") {
      std::cout << formatSuccess(id, agentApprovalsJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.session_schema") {
      std::cout << formatSuccess(id, agentSessionSchemaJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.session_state") {
      const std::string session_path = extractStringValue(line, "session_path");
      if (session_path.empty()) {
        std::cout << formatError(id, -32602, "Invalid params: session_path required") << "\n";
      } else {
        try {
          std::cout << formatSuccess(id, loadAgentSessionFileJson(session_path)) << "\n";
        } catch (const std::exception& error) {
          std::cout << formatError(id, -32603, error.what()) << "\n";
        }
      }
      std::cout.flush();
    } else if (method == "agent.replay_manifest") {
      const std::string session_path = extractStringValue(line, "session_path");
      if (session_path.empty()) {
        std::cout << formatError(id, -32602, "Invalid params: session_path required") << "\n";
      } else {
        try {
          std::cout << formatSuccess(id, agentReplayManifestForFile(session_path)) << "\n";
        } catch (const std::exception& error) {
          std::cout << formatError(id, -32603, error.what()) << "\n";
        }
      }
      std::cout.flush();
    } else if (method == "agent.policy_schema") {
      std::cout << formatSuccess(id, agentPolicySchemaJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.policy_check") {
      const std::vector<std::string> command_args = extractStringArray(line, "args");
      const bool dry_run = extractRawValue(line, "dry_run") == "true";
      std::cout << formatSuccess(id, agentCommandPolicyJson(command_args, dry_run)) << "\n";
      std::cout.flush();
    } else if (method == "agent.run_profile") {
      std::cout << formatSuccess(id, agentRunProfileJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.safety_policy") {
      std::cout << formatSuccess(id, agentSafetyPolicyJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.provider_policy") {
      std::cout << formatSuccess(id, agentProviderPolicyJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.provider_config_schema") {
      std::cout << formatSuccess(id, agentProviderConfigSchemaJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.provider_config_template") {
      std::cout << formatSuccess(id, agentProviderConfigTemplateJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.provider_status") {
      std::cout << formatSuccess(id, agentProviderStatusJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.observability_config") {
      std::cout << formatSuccess(id, agentObservabilityConfigJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.trace_export_schema") {
      std::cout << formatSuccess(id, agentTraceExportSchemaJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.trace_export_template") {
      std::cout << formatSuccess(id, agentTraceExportTemplateJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.trace_redaction_policy") {
      std::cout << formatSuccess(id, agentTraceRedactionPolicyJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.trace_export_dry_run") {
      std::cout << formatSuccess(id, agentTraceExportDryRunJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.kicad_evidence_schema") {
      std::cout << formatSuccess(id, agentKiCadEvidenceSchemaJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.kicad_evidence_plan") {
      try {
        std::cout << formatSuccess(id, agentKiCadEvidencePlanJson(extractKiCadEvidenceOptions(line)))
                  << "\n";
      } catch (const std::exception& error) {
        std::cout << formatError(id, -32602, error.what()) << "\n";
      }
      std::cout.flush();
    } else if (method == "agent.kicad_evidence_dry_run") {
      try {
        std::cout << formatSuccess(id,
                                   agentKiCadEvidenceDryRunJson(extractKiCadEvidenceOptions(line)))
                  << "\n";
      } catch (const std::exception& error) {
        std::cout << formatError(id, -32602, error.what()) << "\n";
      }
      std::cout.flush();
    } else if (method == "agent.kicad_evidence_run") {
      try {
        const std::map<std::string, std::string> options = extractKiCadEvidenceOptions(line);
        if (optionOrEmpty(options, "execute") == "true" && !allow_write) {
          const AgentCommandPolicy policy =
              classifyAgentCommandPolicy({"agent", "kicad-evidence-run"}, false);
          std::cout << formatError(id, -32604, agentPolicyApprovalMessage(policy)) << "\n";
        } else {
          std::cout << formatSuccess(id, agentKiCadEvidenceRunJson(options)) << "\n";
        }
      } catch (const std::exception& error) {
        std::cout << formatError(id, -32602, error.what()) << "\n";
      }
      std::cout.flush();
    } else if (method == "agent.evidence_manifest_schema") {
      std::cout << formatSuccess(id, agentEvidenceManifestSchemaJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.pcb_api_schema") {
      std::cout << formatSuccess(id, agentPcbApiSchemaJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.tool_guide") {
      std::string method_name = extractStringValue(line, "method_name");
      if (method_name.empty()) {
        method_name = extractStringValue(line, "method");
        if (method_name == "agent.tool_guide") {
          method_name.clear();
        }
      }
      std::cout << formatSuccess(id, agentToolGuideJson(method_name)) << "\n";
      std::cout.flush();
    } else if (method == "agent.orchestrator_schema" || method == "agent.plan" || method == "agent.orchestrate" || method == "agent.tool_call") {
      handleOrchestratorJsonRpc(method, line, id, allow_read, allow_write);
    } else if (method == "execute" || method == "tools/call") {
      if (method == "tools/call") {
        const std::string tool_name = extractStringValue(line, "name");
        if (tool_name != "ccad_execute") {
          std::cout << formatError(id, -32601, "Tool not found: " + tool_name) << "\n";
          std::cout.flush();
          continue;
        }
      }
      std::vector<std::string> cmdArgs = extractStringArray(line, "args");
      if (cmdArgs.empty()) {
        std::cout << formatError(id, -32602, "Invalid params: args required") << "\n";
      } else {
        const AgentCommandPolicy policy = classifyAgentCommandPolicy(cmdArgs, false);
        if ((policy.requires_allow_write && !allow_write) ||
            (policy.requires_allow_read && !allow_read)) {
          std::cout << formatError(id, -32604, agentPolicyApprovalMessage(policy)) << "\n";
          std::cout.flush();
          continue;
        }

        // Create argv
        std::vector<std::string> fullArgs;
        fullArgs.push_back("ccad");
        for (const auto& a : cmdArgs) {
          fullArgs.push_back(a);
        }

        std::vector<char*> argv;
        for (auto& a : fullArgs) {
          argv.push_back(&a[0]);
        }

        std::ostringstream capturedOut;
        std::ostringstream capturedErr;
        std::streambuf* oldCout = std::cout.rdbuf(capturedOut.rdbuf());
        std::streambuf* oldCerr = std::cerr.rdbuf(capturedErr.rdbuf());

        int exitCode = 0;
        try {
          exitCode = run(static_cast<int>(argv.size()), argv.data());
        } catch (...) {
          exitCode = 1;
        }

        std::cout.rdbuf(oldCout);
        std::cerr.rdbuf(oldCerr);

        if (method == "tools/call") {
          std::ostringstream res;
          res << "{\"content\": [{\"type\": \"text\", \"text\": \"exit_code: " << exitCode 
              << "\\nstdout:\\n" << ccad::escapeJson(capturedOut.str()) 
              << "\\nstderr:\\n" << ccad::escapeJson(capturedErr.str()) << "\"}], "
              << "\"isError\": " << (exitCode == 0 ? "false" : "true") << "}";
          std::cout << formatSuccess(id, res.str()) << "\n";
        } else {
          std::ostringstream res;
          res << "{\"exit_code\": " << exitCode << ", "
              << "\"stdout\": \"" << ccad::escapeJson(capturedOut.str()) << "\", "
              << "\"stderr\": \"" << ccad::escapeJson(capturedErr.str()) << "\"}";
          std::cout << formatSuccess(id, res.str()) << "\n";
        }
        std::cout.flush();
      }
    } else {
      std::cout << formatError(id, -32601, "Method not found") << "\n";
      std::cout.flush();
    }
  }

  return 0;
}

}  // namespace ccad_cli
