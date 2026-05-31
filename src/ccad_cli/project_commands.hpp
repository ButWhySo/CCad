#pragma once

#include <string>
#include <vector>

namespace ccad_cli {

int initCommand(const std::vector<std::string>& args);
int validateCommand(const std::vector<std::string>& args);
int drcCommand(const std::vector<std::string>& args);
int inspectCommand(const std::vector<std::string>& args);
int diffCommand(const std::vector<std::string>& args);
int exportBomCommand(const std::vector<std::string>& args);

}  // namespace ccad_cli
