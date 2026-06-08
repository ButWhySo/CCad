#pragma once

#include <string>
#include <vector>

namespace ccad_cli {

// Handle `ccad agent orchestrate`, `ccad agent plan`, etc.
int agentOrchestratorCommand(const std::vector<std::string>& args);

// Optionally, handle JSON-RPC routes for orchestrator
bool handleOrchestratorJsonRpc(const std::string& method,
                               const std::string& line,
                               const std::string& id,
                               bool allow_read,
                               bool allow_write);

} // namespace ccad_cli
