#include "agent_commands.hpp"

#include "app.hpp"
#include "ccad_core/json.hpp"

#include <iostream>
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

std::string agentProtocolCatalogJson() {
  const std::vector<std::string> methods = {
      agentMethodEntryJson("agent.methods", "agent", "List Agent Methods", true, false, false),
      agentMethodEntryJson("agent.quickstart", "agent", "Agent Quickstart", true, false, false),
      agentMethodEntryJson("agent.harness_context", "agent", "Harness Context", true, false, false),
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
         "\"first_methods\":[\"agent.methods\",\"agent.harness_context\",\"agent.tool_guide\",\"tools/list\"],"
         "\"screenshot_rule\":\"GUI screenshots must use the project visual-validation harness with beep and current settle waits\","
         "\"unsafe_rule\":\"Write commands require explicit --allow-write in agent serve and direct human approval when policy requires it\"}";
}

std::string agentHarnessContextJson() {
  return "{\"schema_version\":1,\"harness_kind\":\"ccad_headless_cli_agent_surface\","
         "\"session_state\":{\"project_path\":null,\"active_view\":\"headless\","
         "\"ui_epoch\":null,\"selected_object_ids\":[],\"provider_configured\":false,"
         "\"last_verified_visual_artifact\":null,\"transaction_id\":null},"
         "\"pending_diagnostics\":{\"erc_count\":0,\"drc_count\":0,\"error_count\":0,\"warning_count\":0},"
         "\"capabilities\":[\"agent.methods\",\"agent.tool_guide\",\"ccad_execute\",\"mcp_stdio\"],"
         "\"visual_validation_policy\":{\"single_preview_wait_seconds\":7,"
         "\"multi_action_initial_wait_seconds\":5,\"multi_action_step_wait_ms\":800,"
         "\"beep_before_gui_test\":true}}";
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
    std::cerr << "Usage: ccad agent <serve|methods|quickstart|harness-context|run-profile|safety-policy|provider-policy|observability-config|evidence-manifest-schema|tool-guide>\n";
    return 1;
  }

  if (args[0] != "serve") {
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
    std::cerr << "Usage: ccad agent <serve|methods|quickstart|harness-context|run-profile|safety-policy|provider-policy|observability-config|evidence-manifest-schema|tool-guide>\n";
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
