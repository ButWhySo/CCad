#include "agent_commands.hpp"

#include "agent_session.hpp"
#include "app.hpp"
#include "ccad_core/json.hpp"
#include "common.hpp"

#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

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
    auto end = json.find("\"", pos);
    if (end != std::string::npos) {
      return json.substr(pos, end - pos);
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
      agentMethodEntryJson("agent.run_profile", "agent", "Run Profile", true, false, false),
      agentMethodEntryJson("agent.safety_policy", "agent", "Safety Policy", true, false, false),
      agentMethodEntryJson("agent.provider_policy", "agent", "Provider Policy", true, false, false),
      agentMethodEntryJson("agent.observability_config", "agent", "Observability Config", true, false,
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
         "\"agent.session_state\",\"agent.replay_manifest\",\"agent.tasks\",\"agent.evidence\","
         "\"agent.approvals\",\"agent.harness_context\",\"agent.tool_guide\",\"tools/list\"],"
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
         "\"agent.session_state\",\"agent.replay_manifest\",\"agent.tasks\",\"agent.evidence\","
         "\"agent.approvals\",\"agent.tool_guide\",\"ccad_execute\",\"mcp_stdio\"],"
         "\"visual_validation_policy\":{\"single_preview_wait_seconds\":7,"
         "\"multi_action_initial_wait_seconds\":5,\"multi_action_step_wait_ms\":800,"
         "\"beep_before_gui_test\":true}}";
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
         "\"project_file_secret_storage\":false}";
}

std::string agentObservabilityConfigJson() {
  return "{\"schema_version\":1,\"config_kind\":\"ccad_agent_observability_config\","
         "\"status\":\"disabled_until_user_configured\",\"protocol\":\"opentelemetry\","
         "\"otel_backend_options\":[\"langfuse\",\"otlp_http\",\"otlp_grpc\",\"local_collector\"],"
         "\"span_plan\":[\"agent.run\",\"prompt.assembly\",\"model.call\",\"tool.call\","
         "\"gui.map_query\",\"gui.screenshot_capture\",\"project.drc\",\"project.erc\","
         "\"file.write\",\"retry\",\"interrupt\",\"final_verification\"],"
         "\"redaction_policy\":{\"export_design_files_by_default\":false,"
         "\"export_screenshots_by_default\":false,\"export_prompt_content_by_default\":false}}";
}

std::string agentEvidenceManifestSchemaJson() {
  return "{\"schema_version\":1,\"manifest_kind\":\"ccad_agent_evidence_manifest\","
         "\"required_fields\":[\"sprint_id\",\"project_path\",\"source_references\","
         "\"tool_calls\",\"screenshots\",\"drc_reports\",\"erc_reports\",\"design_artifacts\","
         "\"decisions\",\"redactions\"],"
         "\"artifact_fields\":[\"kind\",\"path\",\"created_at\",\"sha256\",\"producer_method\","
         "\"project_id\",\"transaction_id\",\"ui_epoch\"]}";
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
                     method == "agent.observability_config" ||
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
  if (command == "run-profile" || command == "run_profile") return agentRunProfileJson();
  if (command == "safety-policy" || command == "safety_policy") return agentSafetyPolicyJson();
  if (command == "provider-policy" || command == "provider_policy") return agentProviderPolicyJson();
  if (command == "observability-config" || command == "observability_config") {
    return agentObservabilityConfigJson();
  }
  if (command == "evidence-manifest-schema" || command == "evidence_manifest_schema") {
    return agentEvidenceManifestSchemaJson();
  }
  if (command == "tool-guide" || command == "tool_guide") return agentToolGuideJson(method_name);
  return "";
}

}  // namespace

int agentCommand(const std::vector<std::string>& args) {
  if (args.empty()) {
    std::cerr << "Usage: ccad agent <serve|methods|quickstart|harness-context|state|tasks|evidence|approvals|session-schema|session-new|session-state|checkpoint-add|replay|run-profile|safety-policy|provider-policy|observability-config|evidence-manifest-schema|tool-guide>\n";
    return 1;
  }

  if (args[0] != "serve") {
    try {
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
    std::cerr << "Usage: ccad agent <serve|methods|quickstart|harness-context|state|tasks|evidence|approvals|session-schema|session-new|session-state|checkpoint-add|replay|run-profile|safety-policy|provider-policy|observability-config|evidence-manifest-schema|tool-guide>\n";
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

    std::string id = extractRawValue(line, "id");
    std::string method = extractStringValue(line, "method");

    if (method == "ping") {
      std::cout << formatSuccess(id, "\"pong\"") << "\n";
      std::cout.flush();
    } else if (method == "initialize") {
      std::string res = "{\"protocolVersion\": \"2024-11-05\", \"capabilities\": {\"tools\": {}}, \"serverInfo\": {\"name\": \"ccad\", \"version\": \"1.0.0\"}}";
      std::cout << formatSuccess(id, res) << "\n";
      std::cout.flush();
    } else if (method == "tools/list") {
      std::string res = "{\"tools\": [{\"name\": \"ccad_execute\", \"description\": \"Execute ccad CLI commands\", \"inputSchema\": {\"type\": \"object\", \"properties\": {\"args\": {\"type\": \"array\", \"items\": {\"type\": \"string\"}}}, \"required\": [\"args\"]}}]}";
      std::cout << formatSuccess(id, res) << "\n";
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
    } else if (method == "agent.run_profile") {
      std::cout << formatSuccess(id, agentRunProfileJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.safety_policy") {
      std::cout << formatSuccess(id, agentSafetyPolicyJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.provider_policy") {
      std::cout << formatSuccess(id, agentProviderPolicyJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.observability_config") {
      std::cout << formatSuccess(id, agentObservabilityConfigJson()) << "\n";
      std::cout.flush();
    } else if (method == "agent.evidence_manifest_schema") {
      std::cout << formatSuccess(id, agentEvidenceManifestSchemaJson()) << "\n";
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
    } else if (method == "execute" || method == "tools/call") {
      std::vector<std::string> cmdArgs = extractStringArray(line, "args");
      if (cmdArgs.empty()) {
        std::cout << formatError(id, -32602, "Invalid params: args required") << "\n";
      } else {
        bool is_write = false;
        const std::string& top_cmd = cmdArgs[0];
        if (top_cmd == "pcb" || top_cmd == "lib") {
          if (cmdArgs.size() >= 2) {
            const std::string& sub = cmdArgs[1];
            if (sub == "get-object" || sub == "list-objects" || sub == "list-nets" ||
                sub == "list-route-requests" || sub == "route-status" || sub == "export-route-job" ||
                sub == "export-kicad" || sub == "export-dsn" || sub == "export-footprint" ||
                sub == "catalog-info" || sub == "catalog-find" || sub == "catalog-search" ||
                sub == "catalog-validate") {
              is_write = false;
            } else {
              is_write = true;
            }
          }
        } else if (top_cmd == "init") {
          is_write = true;
        } else {
          is_write = false;
        }

        if ((is_write && !allow_write) || (!is_write && !allow_read)) {
          std::cout << formatError(id, -32604, "Permission denied") << "\n";
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
