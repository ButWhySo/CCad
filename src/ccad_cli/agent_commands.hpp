#pragma once

#include <string>
#include <vector>

namespace ccad_cli {

// Dispatches `ccad agent serve`
int agentCommand(const std::vector<std::string>& args);

}  // namespace ccad_cli
