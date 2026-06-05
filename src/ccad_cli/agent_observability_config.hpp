#pragma once

#include <string>

namespace ccad_cli {

std::string agentObservabilityConfigJson();
std::string agentTraceExportSchemaJson();
std::string agentTraceExportTemplateJson();
std::string agentTraceRedactionPolicyJson();
std::string agentTraceExportDryRunJson();

}  // namespace ccad_cli
