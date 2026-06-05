#pragma once

#include <string>
#include <vector>

namespace ccad {

struct AgentCommandPolicy {
  std::vector<std::string> args;
  bool read_only = true;
  bool mutates_project = false;
  bool mutates_files = false;
  bool requires_allow_read = true;
  bool requires_allow_write = false;
  bool approval_required = false;
  bool dry_run_supported = true;
  bool dry_run = false;
  bool would_execute = true;
  std::string risk_level = "low";
  std::string approval_reason;
  std::string decision = "allow_read";
};

AgentCommandPolicy classifyAgentCommandPolicy(const std::vector<std::string>& args,
                                              bool dry_run);
std::string agentPolicySchemaJson();
std::string agentCommandPolicyJson(const std::vector<std::string>& args, bool dry_run);
std::string agentPolicyApprovalMessage(const AgentCommandPolicy& policy);

}  // namespace ccad
