#include "agent_policy.hpp"

#include "ccad_core/json.hpp"

#include <sstream>

namespace ccad {

namespace {

bool isOneOf(const std::string& value, const std::vector<std::string>& candidates) {
  for (const std::string& candidate : candidates) {
    if (value == candidate) {
      return true;
    }
  }
  return false;
}

void markRead(AgentCommandPolicy& policy) {
  policy.read_only = true;
  policy.mutates_project = false;
  policy.mutates_files = false;
  policy.requires_allow_read = true;
  policy.requires_allow_write = false;
  policy.approval_required = false;
  policy.risk_level = "low";
  policy.approval_reason.clear();
  policy.decision = policy.dry_run ? "dry_run_only" : "allow_read";
  policy.would_execute = !policy.dry_run;
}

void markProjectMutation(AgentCommandPolicy& policy, const std::string& reason) {
  policy.read_only = false;
  policy.mutates_project = true;
  policy.mutates_files = true;
  policy.requires_allow_read = false;
  policy.requires_allow_write = true;
  policy.approval_required = true;
  policy.risk_level = "high";
  policy.approval_reason = reason;
  policy.decision = policy.dry_run ? "dry_run_only" : "approval_required";
  policy.would_execute = false;
}

void markFileMutation(AgentCommandPolicy& policy, const std::string& reason) {
  policy.read_only = false;
  policy.mutates_project = false;
  policy.mutates_files = true;
  policy.requires_allow_read = false;
  policy.requires_allow_write = true;
  policy.approval_required = true;
  policy.risk_level = "medium";
  policy.approval_reason = reason;
  policy.decision = policy.dry_run ? "dry_run_only" : "approval_required";
  policy.would_execute = false;
}

std::string jsonString(const std::string& value) {
  return "\"" + escapeJson(value) + "\"";
}

std::string argsJson(const std::vector<std::string>& args) {
  std::ostringstream out;
  out << '[';
  for (std::size_t i = 0; i < args.size(); ++i) {
    if (i > 0) {
      out << ',';
    }
    out << jsonString(args[i]);
  }
  out << ']';
  return out.str();
}

std::string commandName(const std::vector<std::string>& args) {
  if (args.empty()) {
    return "";
  }
  if (args.size() == 1) {
    return args[0];
  }
  return args[0] + " " + args[1];
}

}  // namespace

AgentCommandPolicy classifyAgentCommandPolicy(const std::vector<std::string>& args,
                                              const bool dry_run) {
  AgentCommandPolicy policy;
  policy.args = args;
  policy.dry_run = dry_run;
  if (args.empty()) {
    policy.read_only = false;
    policy.requires_allow_read = false;
    policy.requires_allow_write = false;
    policy.approval_required = true;
    policy.risk_level = "medium";
    policy.approval_reason = "missing_command_args";
    policy.decision = "invalid_params";
    policy.would_execute = false;
    return policy;
  }

  const std::string& top = args[0];
  if (isOneOf(top, {"help", "validate", "drc", "inspect", "diff"})) {
    markRead(policy);
    return policy;
  }
  if (top == "init") {
    markFileMutation(policy, "file_write");
    return policy;
  }
  if (top == "project") {
    if (args.size() >= 2 && args[1] == "export-bom") {
      markFileMutation(policy, "file_write");
    } else if (args.size() >= 2 && args[1] == "set-text-variable") {
      markProjectMutation(policy, "project_mutation");
    } else {
      markRead(policy);
    }
    return policy;
  }
  if (top == "pcb") {
    const std::string sub = args.size() >= 2 ? args[1] : "";
    if (isOneOf(sub, {"drill-statistics", "board-statistics", "cleanup-actions",
                      "collect-items", "get-object",
                      "list-objects", "list-nets", "list-by-net", "calculate-net-bridges",
                      "list-connected", "list-route-requests", "route-status",
                      "export-route-job", "list-enabled-layers", "list-visible-layers",
                      "get-layer-name", "get-board-stackup", "get-rules", "get-outline",
                      "outline-polygon", "cross-probe", "expand-text-variables"})) {
      markRead(policy);
      return policy;
    }
    if (isOneOf(sub, {"export-board-bom", "export-kicad", "export-dsn", "export-pnp",
                      "export-drill"})) {
      markFileMutation(policy, "file_write");
      return policy;
    }
    markProjectMutation(policy, "project_mutation");
    return policy;
  }
  if (top == "sch" || top == "schematic") {
    if (args.size() >= 2 && args[1] == "collect-items") {
      markRead(policy);
      return policy;
    }
    markProjectMutation(policy, "project_mutation");
    return policy;
  }
  if (top == "lib") {
    const std::string sub = args.size() >= 2 ? args[1] : "";
    if (isOneOf(sub, {"catalog-info", "catalog-find", "catalog-search", "catalog-validate"})) {
      markRead(policy);
      return policy;
    }
    markFileMutation(policy, "file_write");
    return policy;
  }
  if (top == "agent") {
    const std::string sub = args.size() >= 2 ? args[1] : "";
    if (isOneOf(sub, {"session-new", "checkpoint-add"})) {
      markFileMutation(policy, "agent_session_write");
      return policy;
    }
    if (sub == "kicad-evidence-run") {
      markFileMutation(policy, "external_process_file_write");
      return policy;
    }
    markRead(policy);
    return policy;
  }

  policy.read_only = false;
  policy.requires_allow_read = false;
  policy.requires_allow_write = true;
  policy.approval_required = true;
  policy.risk_level = "medium";
  policy.approval_reason = "unknown_command";
  policy.decision = dry_run ? "dry_run_only" : "approval_required";
  policy.would_execute = false;
  return policy;
}

std::string agentPolicySchemaJson() {
  return "{\"schema_version\":1,\"schema_kind\":\"ccad_agent_policy_schema\","
         "\"fields\":[\"args\",\"command\",\"read_only\",\"mutates_project\",\"mutates_files\","
         "\"requires_allow_read\",\"requires_allow_write\",\"approval_required\","
         "\"approval_reason\",\"dry_run_supported\",\"dry_run\",\"would_execute\","
         "\"risk_level\",\"decision\"],"
         "\"decisions\":[\"allow_read\",\"approval_required\",\"dry_run_only\","
         "\"invalid_params\"],"
         "\"approval_reasons\":[\"project_mutation\",\"file_write\","
         "\"agent_session_write\",\"external_process_file_write\","
         "\"unknown_command\",\"missing_command_args\"],"
         "\"guardrail_reference\":\"tool_boundary_policy_check\"}";
}

std::string agentCommandPolicyJson(const std::vector<std::string>& args, const bool dry_run) {
  const AgentCommandPolicy policy = classifyAgentCommandPolicy(args, dry_run);
  std::ostringstream out;
  out << "{\"schema_version\":1,"
      << "\"policy_kind\":\"ccad_agent_command_policy\","
      << "\"args\":" << argsJson(policy.args) << ","
      << "\"command\":" << jsonString(commandName(policy.args)) << ","
      << "\"read_only\":" << (policy.read_only ? "true" : "false") << ","
      << "\"mutates_project\":" << (policy.mutates_project ? "true" : "false") << ","
      << "\"mutates_files\":" << (policy.mutates_files ? "true" : "false") << ","
      << "\"requires_allow_read\":" << (policy.requires_allow_read ? "true" : "false") << ","
      << "\"requires_allow_write\":" << (policy.requires_allow_write ? "true" : "false") << ","
      << "\"approval_required\":" << (policy.approval_required ? "true" : "false") << ","
      << "\"approval_reason\":" << jsonString(policy.approval_reason) << ","
      << "\"dry_run_supported\":" << (policy.dry_run_supported ? "true" : "false") << ","
      << "\"dry_run\":" << (policy.dry_run ? "true" : "false") << ","
      << "\"would_execute\":" << (policy.would_execute ? "true" : "false") << ","
      << "\"risk_level\":" << jsonString(policy.risk_level) << ","
      << "\"decision\":" << jsonString(policy.decision) << "}";
  return out.str();
}

std::string agentPolicyApprovalMessage(const AgentCommandPolicy& policy) {
  if (!policy.approval_required) {
    return "Permission denied";
  }
  return "Approval required: " + policy.approval_reason;
}

}  // namespace ccad
