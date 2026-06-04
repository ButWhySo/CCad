#include "agent_session.hpp"

#include "ccad_core/json.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace ccad_cli {

namespace {

struct AgentCheckpoint {
  std::string checkpoint_id;
  int sequence = 0;
  std::string kind;
  std::string summary;
  std::string artifact_path;
  std::string created_at;
  std::string resource_uri;
};

struct AgentSession {
  std::string session_id;
  std::string thread_id;
  std::string title;
  std::string project_path;
  std::string created_at;
  std::string updated_at;
  std::vector<AgentCheckpoint> checkpoints;
};

std::string jsonString(const std::string& value) {
  return "\"" + ccad::escapeJson(value) + "\"";
}

std::string jsonNullableString(const std::string& value) {
  if (value.empty()) {
    return "null";
  }
  return jsonString(value);
}

std::string readTextFile(const std::string& path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("failed to open agent session file: " + path);
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

void writeTextFile(const std::string& path, const std::string& content) {
  std::ofstream output(path);
  if (!output) {
    throw std::runtime_error("failed to write agent session file: " + path);
  }
  output << content;
  if (!output) {
    throw std::runtime_error("failed to finish writing agent session file: " + path);
  }
}

std::string extractStringValue(const std::string& json, const std::string& key) {
  const std::string search = "\"" + key + "\":";
  auto pos = json.find(search);
  if (pos == std::string::npos) {
    return "";
  }
  pos += search.length();
  while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' ||
                                 json[pos] == '\r')) {
    pos++;
  }
  if (pos >= json.length() || json[pos] != '"') {
    return "";
  }
  pos++;
  std::string value;
  bool escaped = false;
  for (; pos < json.length(); ++pos) {
    const char ch = json[pos];
    if (escaped) {
      value.push_back(ch);
      escaped = false;
      continue;
    }
    if (ch == '\\') {
      escaped = true;
      continue;
    }
    if (ch == '"') {
      break;
    }
    value.push_back(ch);
  }
  return value;
}

int extractIntValue(const std::string& json, const std::string& key) {
  const std::string search = "\"" + key + "\":";
  auto pos = json.find(search);
  if (pos == std::string::npos) {
    return 0;
  }
  pos += search.length();
  while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' ||
                                 json[pos] == '\r')) {
    pos++;
  }
  auto end = pos;
  while (end < json.length() && json[end] >= '0' && json[end] <= '9') {
    end++;
  }
  if (end == pos) {
    return 0;
  }
  return std::stoi(json.substr(pos, end - pos));
}

std::vector<AgentCheckpoint> parseCheckpoints(const std::string& json) {
  std::vector<AgentCheckpoint> checkpoints;
  const std::string search = "\"checkpoints\":[";
  auto pos = json.find(search);
  if (pos == std::string::npos) {
    return checkpoints;
  }
  pos += search.length();
  while (pos < json.length()) {
    auto object_start = json.find('{', pos);
    auto array_end = json.find(']', pos);
    if (array_end != std::string::npos &&
        (object_start == std::string::npos || array_end < object_start)) {
      break;
    }
    if (object_start == std::string::npos) {
      break;
    }
    auto object_end = json.find('}', object_start);
    if (object_end == std::string::npos) {
      throw std::runtime_error("malformed checkpoint object in agent session file");
    }
    const std::string object = json.substr(object_start, object_end - object_start + 1);
    AgentCheckpoint checkpoint;
    checkpoint.checkpoint_id = extractStringValue(object, "checkpoint_id");
    checkpoint.sequence = extractIntValue(object, "sequence");
    checkpoint.kind = extractStringValue(object, "kind");
    checkpoint.summary = extractStringValue(object, "summary");
    checkpoint.artifact_path = extractStringValue(object, "artifact_path");
    checkpoint.created_at = extractStringValue(object, "created_at");
    checkpoint.resource_uri = extractStringValue(object, "resource_uri");
    if (!checkpoint.checkpoint_id.empty()) {
      checkpoints.push_back(checkpoint);
    }
    pos = object_end + 1;
  }
  return checkpoints;
}

AgentSession parseSession(const std::string& json) {
  AgentSession session;
  session.session_id = extractStringValue(json, "session_id");
  session.thread_id = extractStringValue(json, "thread_id");
  session.title = extractStringValue(json, "title");
  session.project_path = extractStringValue(json, "project_path");
  session.created_at = extractStringValue(json, "created_at");
  session.updated_at = extractStringValue(json, "updated_at");
  session.checkpoints = parseCheckpoints(json);
  if (session.session_id.empty()) {
    throw std::runtime_error("agent session file is missing session_id");
  }
  if (session.thread_id.empty()) {
    session.thread_id = session.session_id;
  }
  return session;
}

std::string checkpointResourceUri(const std::string& session_id, const std::string& checkpoint_id) {
  return "ccad-agent-checkpoint:" + session_id + "/" + checkpoint_id;
}

std::string sessionResourceUri(const std::string& session_id) {
  return "ccad-agent-session:" + session_id;
}

std::string dumpCheckpointJson(const AgentCheckpoint& checkpoint) {
  std::ostringstream out;
  out << "{\"checkpoint_id\":" << jsonString(checkpoint.checkpoint_id) << ","
      << "\"sequence\":" << checkpoint.sequence << ","
      << "\"kind\":" << jsonString(checkpoint.kind) << ","
      << "\"summary\":" << jsonString(checkpoint.summary) << ","
      << "\"artifact_path\":" << jsonNullableString(checkpoint.artifact_path) << ","
      << "\"created_at\":" << jsonString(checkpoint.created_at) << ","
      << "\"resource_uri\":" << jsonString(checkpoint.resource_uri) << "}";
  return out.str();
}

std::string dumpSessionJson(const AgentSession& session) {
  std::ostringstream out;
  out << "{\"schema_version\":1,"
      << "\"session_kind\":\"ccad_agent_session\","
      << "\"session_id\":" << jsonString(session.session_id) << ","
      << "\"thread_id\":" << jsonString(session.thread_id) << ","
      << "\"title\":" << jsonString(session.title) << ","
      << "\"project_path\":" << jsonNullableString(session.project_path) << ","
      << "\"created_at\":" << jsonString(session.created_at) << ","
      << "\"updated_at\":" << jsonString(session.updated_at) << ","
      << "\"durability\":\"local_json_checkpoint_file\","
      << "\"provider_configured\":false,"
      << "\"trace_export_configured\":false,"
      << "\"resource_uri\":" << jsonString(sessionResourceUri(session.session_id)) << ","
      << "\"checkpoint_count\":" << session.checkpoints.size() << ","
      << "\"checkpoints\":[";
  for (std::size_t i = 0; i < session.checkpoints.size(); ++i) {
    if (i > 0) {
      out << ',';
    }
    out << dumpCheckpointJson(session.checkpoints[i]);
  }
  out << "]}";
  return out.str();
}

std::string dumpReplayManifestJson(const AgentSession& session) {
  const std::string latest = session.checkpoints.empty() ? "" : session.checkpoints.back().checkpoint_id;
  std::ostringstream out;
  out << "{\"schema_version\":1,"
      << "\"manifest_kind\":\"ccad_agent_replay_manifest\","
      << "\"session_id\":" << jsonString(session.session_id) << ","
      << "\"thread_id\":" << jsonString(session.thread_id) << ","
      << "\"session_resource_uri\":" << jsonString(sessionResourceUri(session.session_id)) << ","
      << "\"checkpoint_count\":" << session.checkpoints.size() << ","
      << "\"latest_checkpoint_id\":" << jsonNullableString(latest) << ","
      << "\"replayable\":" << (session.checkpoints.empty() ? "false" : "true") << ","
      << "\"checkpoints\":[";
  for (std::size_t i = 0; i < session.checkpoints.size(); ++i) {
    if (i > 0) {
      out << ',';
    }
    out << dumpCheckpointJson(session.checkpoints[i]);
  }
  out << "]}";
  return out.str();
}

}  // namespace

std::string agentSessionSchemaJson() {
  return "{\"schema_version\":1,\"schema_kind\":\"ccad_agent_session_schema\","
         "\"file_extension\":\".ccad-agent-session.json\","
         "\"session_fields\":[\"session_id\",\"thread_id\",\"title\",\"project_path\","
         "\"created_at\",\"updated_at\",\"durability\",\"resource_uri\","
         "\"checkpoint_count\",\"checkpoints\"],"
         "\"checkpoint_fields\":[\"checkpoint_id\",\"sequence\",\"kind\",\"summary\","
         "\"artifact_path\",\"created_at\",\"resource_uri\"],"
         "\"provider_fields_are_forbidden\":[\"api_key\",\"secret\",\"token\"],"
         "\"resource_uri\":\"ccad-agent-session:<session_id>\"}";
}

std::string createAgentSessionFile(const std::string& output_path,
                                   const std::string& session_id,
                                   const std::string& title,
                                   const std::string& project_path,
                                   const std::string& created_at) {
  if (output_path.empty()) {
    throw std::runtime_error("missing required option: --out");
  }
  if (session_id.empty()) {
    throw std::runtime_error("missing required option: --session-id");
  }
  AgentSession session;
  session.session_id = session_id;
  session.thread_id = session_id;
  session.title = title.empty() ? "CCad Agent Session" : title;
  session.project_path = project_path;
  session.created_at = created_at.empty() ? "unspecified" : created_at;
  session.updated_at = session.created_at;
  writeTextFile(output_path, dumpSessionJson(session));

  std::ostringstream out;
  out << "{\"schema_version\":1,\"created\":true,"
      << "\"session_path\":" << jsonString(std::filesystem::path(output_path).string()) << ","
      << "\"session_id\":" << jsonString(session.session_id) << ","
      << "\"thread_id\":" << jsonString(session.thread_id) << ","
      << "\"checkpoint_count\":0,"
      << "\"resource_uri\":" << jsonString(sessionResourceUri(session.session_id)) << "}";
  return out.str();
}

std::string loadAgentSessionFileJson(const std::string& session_path) {
  if (session_path.empty()) {
    throw std::runtime_error("missing required option: --session");
  }
  return dumpSessionJson(parseSession(readTextFile(session_path)));
}

std::string appendAgentCheckpointFile(const std::string& session_path,
                                      const std::string& checkpoint_id,
                                      const std::string& kind,
                                      const std::string& summary,
                                      const std::string& artifact_path,
                                      const std::string& created_at) {
  if (checkpoint_id.empty()) {
    throw std::runtime_error("missing required option: --checkpoint-id");
  }
  AgentSession session = parseSession(readTextFile(session_path));
  for (const AgentCheckpoint& existing : session.checkpoints) {
    if (existing.checkpoint_id == checkpoint_id) {
      throw std::runtime_error("duplicate checkpoint id: " + checkpoint_id);
    }
  }
  AgentCheckpoint checkpoint;
  checkpoint.checkpoint_id = checkpoint_id;
  checkpoint.sequence = static_cast<int>(session.checkpoints.size()) + 1;
  checkpoint.kind = kind.empty() ? "checkpoint" : kind;
  checkpoint.summary = summary;
  checkpoint.artifact_path = artifact_path;
  checkpoint.created_at = created_at.empty() ? "unspecified" : created_at;
  checkpoint.resource_uri = checkpointResourceUri(session.session_id, checkpoint.checkpoint_id);
  session.checkpoints.push_back(checkpoint);
  session.updated_at = checkpoint.created_at;
  writeTextFile(session_path, dumpSessionJson(session));

  std::ostringstream out;
  out << "{\"schema_version\":1,\"checkpoint_added\":true,"
      << "\"session_id\":" << jsonString(session.session_id) << ","
      << "\"checkpoint_id\":" << jsonString(checkpoint.checkpoint_id) << ","
      << "\"sequence\":" << checkpoint.sequence << ","
      << "\"checkpoint_count\":" << session.checkpoints.size() << ","
      << "\"resource_uri\":" << jsonString(checkpoint.resource_uri) << "}";
  return out.str();
}

std::string agentReplayManifestForFile(const std::string& session_path) {
  if (session_path.empty()) {
    throw std::runtime_error("missing required option: --session");
  }
  return dumpReplayManifestJson(parseSession(readTextFile(session_path)));
}

}  // namespace ccad_cli
