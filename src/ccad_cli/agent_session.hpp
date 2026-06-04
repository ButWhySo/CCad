#pragma once

#include <string>

namespace ccad_cli {

std::string agentSessionSchemaJson();
std::string createAgentSessionFile(const std::string& output_path,
                                   const std::string& session_id,
                                   const std::string& title,
                                   const std::string& project_path,
                                   const std::string& created_at);
std::string loadAgentSessionFileJson(const std::string& session_path);
std::string appendAgentCheckpointFile(const std::string& session_path,
                                      const std::string& checkpoint_id,
                                      const std::string& kind,
                                      const std::string& summary,
                                      const std::string& artifact_path,
                                      const std::string& created_at);
std::string agentReplayManifestForFile(const std::string& session_path);

}  // namespace ccad_cli
