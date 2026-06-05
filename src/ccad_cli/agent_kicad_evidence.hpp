#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace ccad_cli {

std::string agentKiCadEvidenceSchemaJson();
std::string agentKiCadEvidencePlanJson(const std::vector<std::string>& args,
                                       std::size_t start);
std::string agentKiCadEvidenceDryRunJson(const std::vector<std::string>& args,
                                         std::size_t start);
std::string agentKiCadEvidenceRunJson(const std::vector<std::string>& args,
                                      std::size_t start);
std::string agentKiCadEvidencePlanJson(const std::map<std::string, std::string>& options);
std::string agentKiCadEvidenceDryRunJson(const std::map<std::string, std::string>& options);
std::string agentKiCadEvidenceRunJson(const std::map<std::string, std::string>& options);

}  // namespace ccad_cli
