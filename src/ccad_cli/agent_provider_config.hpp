#pragma once

#include <string>
#include <vector>

namespace ccad_cli {

std::string agentProviderConfigSchemaJson();
std::string agentProviderConfigTemplateJson();
std::string agentProviderStatusJson();
int agentProviderCredentialCommand(const std::vector<std::string>& args);

}  // namespace ccad_cli
