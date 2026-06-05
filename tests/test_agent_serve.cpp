#include "ccad_cli/agent_commands.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

void assertContains(const std::string& content, const std::string& pattern, const std::string& label) {
  if (content.find(pattern) == std::string::npos) {
    std::cerr << "FAIL " << label << "\n  pattern not found: " << pattern << '\n';
    std::exit(1);
  }
}

std::string tempSessionPath(const std::string& name) {
  const std::filesystem::path path = std::filesystem::temp_directory_path() / name;
  std::filesystem::remove(path);
  return path.string();
}

void testPing() {
  std::istringstream in("{\"jsonrpc\": \"2.0\", \"method\": \"ping\", \"id\": 1}\n");
  std::ostringstream out;

  auto oldCin = std::cin.rdbuf(in.rdbuf());
  auto oldCout = std::cout.rdbuf(out.rdbuf());

  std::vector<std::string> args = {"serve", "--allow-read"};
  int result = ccad_cli::agentCommand(args);

  std::cin.rdbuf(oldCin);
  std::cout.rdbuf(oldCout);

  if (result != 0) {
    std::cerr << "FAIL testPing exited with " << result << "\n";
    std::exit(1);
  }

  assertContains(out.str(), "\"result\": \"pong\"", "has pong result");
  assertContains(out.str(), "\"id\": 1", "has request id");
}

void testExecute() {
  std::istringstream in("{\"jsonrpc\": \"2.0\", \"method\": \"execute\", \"params\": {\"args\": [\"help\", \"--format\", \"json\"]}, \"id\": 2}\n");
  std::ostringstream out;

  auto oldCin = std::cin.rdbuf(in.rdbuf());
  auto oldCout = std::cout.rdbuf(out.rdbuf());

  std::vector<std::string> args = {"serve", "--allow-read"};
  int result = ccad_cli::agentCommand(args);

  std::cin.rdbuf(oldCin);
  std::cout.rdbuf(oldCout);

  if (result != 0) {
    std::cerr << "FAIL testExecute exited with " << result << "\n";
    std::exit(1);
  }

  assertContains(out.str(), "\"id\": 2", "has request id 2");
  assertContains(out.str(), "\"exit_code\": 0", "has exit_code 0");
  assertContains(out.str(), "\\\"commands\\\": [", "has stdout escaped JSON");
}

void testMCPInitialize() {
  std::istringstream in("{\"jsonrpc\": \"2.0\", \"method\": \"initialize\", \"id\": 3}\n");
  std::ostringstream out;
  auto oldCin = std::cin.rdbuf(in.rdbuf());
  auto oldCout = std::cout.rdbuf(out.rdbuf());
  std::vector<std::string> args = {"serve", "--allow-read"};
  int result = ccad_cli::agentCommand(args);
  std::cin.rdbuf(oldCin);
  std::cout.rdbuf(oldCout);
  if (result != 0) std::exit(1);
  assertContains(out.str(), "\"protocolVersion\":", "has protocolVersion");
}

void testMCPToolsList() {
  std::istringstream in("{\"jsonrpc\": \"2.0\", \"method\": \"tools/list\", \"id\": 4}\n");
  std::ostringstream out;
  auto oldCin = std::cin.rdbuf(in.rdbuf());
  auto oldCout = std::cout.rdbuf(out.rdbuf());
  std::vector<std::string> args = {"serve", "--allow-read"};
  int result = ccad_cli::agentCommand(args);
  std::cin.rdbuf(oldCin);
  std::cout.rdbuf(oldCout);
  if (result != 0) std::exit(1);
  assertContains(out.str(), "\"name\": \"ccad_execute\"", "has ccad_execute tool");
}

void testMCPToolsCall() {
  std::istringstream in("{\"jsonrpc\": \"2.0\", \"method\": \"tools/call\", \"params\": {\"name\": \"ccad_execute\", \"arguments\": {\"args\": [\"help\", \"--format\", \"json\"]}}, \"id\": 5}\n");
  std::ostringstream out;
  auto oldCin = std::cin.rdbuf(in.rdbuf());
  auto oldCout = std::cout.rdbuf(out.rdbuf());
  std::vector<std::string> args = {"serve", "--allow-read"};
  int result = ccad_cli::agentCommand(args);
  std::cin.rdbuf(oldCin);
  std::cout.rdbuf(oldCout);
  if (result != 0) std::exit(1);
  assertContains(out.str(), "\"content\": [", "has content array");
  assertContains(out.str(), "\\\"commands\\\": [", "has stdout escaped JSON");
  assertContains(out.str(), "\"isError\": false", "has isError false");
}

void testAgentMethodsCommand() {
  std::ostringstream out;
  auto oldCout = std::cout.rdbuf(out.rdbuf());
  std::vector<std::string> args = {"methods"};
  int result = ccad_cli::agentCommand(args);
  std::cout.rdbuf(oldCout);
  if (result != 0) {
    std::cerr << "FAIL testAgentMethodsCommand exited with " << result << "\n";
    std::exit(1);
  }
  assertContains(out.str(), "\"catalog_kind\":\"ccad_agent_protocol\"",
                 "agent methods command prints protocol catalog");
  assertContains(out.str(), "\"method\":\"agent.harness_context\"",
                 "agent methods command includes harness context");
  assertContains(out.str(), "\"method\":\"agent.state\"",
                 "agent methods command includes headless workspace state");
  assertContains(out.str(), "\"method\":\"agent.tasks\"",
                 "agent methods command includes headless task state");
  assertContains(out.str(), "\"method\":\"agent.evidence\"",
                 "agent methods command includes headless evidence state");
  assertContains(out.str(), "\"method\":\"agent.approvals\"",
                 "agent methods command includes headless approval state");
  assertContains(out.str(), "\"method\":\"agent.session_schema\"",
                 "agent methods command includes durable session schema");
  assertContains(out.str(), "\"method\":\"agent.session_state\"",
                 "agent methods command includes durable session state");
  assertContains(out.str(), "\"method\":\"agent.replay_manifest\"",
                 "agent methods command includes replay manifest");
  assertContains(out.str(), "\"method\":\"agent.policy_schema\"",
                 "agent methods command includes policy schema");
  assertContains(out.str(), "\"method\":\"agent.policy_check\"",
                 "agent methods command includes policy check");
  assertContains(out.str(), "\"method\":\"agent.provider_config_schema\"",
                 "agent methods command includes provider config schema");
  assertContains(out.str(), "\"method\":\"agent.provider_config_template\"",
                 "agent methods command includes provider config template");
  assertContains(out.str(), "\"method\":\"agent.provider_status\"",
                 "agent methods command includes provider readiness status");
  assertContains(out.str(), "\"method\":\"agent.observability_config\"",
                 "agent methods command includes observability config");
  assertContains(out.str(), "\"method\":\"agent.evidence_manifest_schema\"",
                 "agent methods command includes evidence manifest schema");
}

void testAgentMetadataCommands() {
  std::ostringstream out;
  auto oldCout = std::cout.rdbuf(out.rdbuf());
  std::vector<std::string> args = {"harness-context"};
  int result = ccad_cli::agentCommand(args);
  std::cout.rdbuf(oldCout);
  if (result != 0) {
    std::cerr << "FAIL testAgentMetadataCommands exited with " << result << "\n";
    std::exit(1);
  }
  assertContains(out.str(), "\"harness_kind\":\"ccad_headless_cli_agent_surface\"",
                 "CLI harness context identifies the headless surface");
  assertContains(out.str(), "\"session_state\"", "CLI harness context includes session state");
  assertContains(out.str(), "\"pending_diagnostics\"",
                 "CLI harness context includes pending diagnostics");

  out.str("");
  out.clear();
  oldCout = std::cout.rdbuf(out.rdbuf());
  std::vector<std::string> schema_args = {"evidence-manifest-schema"};
  result = ccad_cli::agentCommand(schema_args);
  std::cout.rdbuf(oldCout);
  if (result != 0) {
    std::cerr << "FAIL testAgentMetadataCommands evidence schema exited with " << result << "\n";
    std::exit(1);
  }
  assertContains(out.str(), "\"manifest_kind\":\"ccad_agent_evidence_manifest\"",
                 "CLI evidence manifest schema reports kind");
  assertContains(out.str(), "\"evidence_cards\"",
                 "CLI evidence manifest schema documents evidence cards");
  assertContains(out.str(), "\"card_fields\"",
                 "CLI evidence manifest schema documents card fields");
  assertContains(out.str(), "\"artifact_path\"",
                 "CLI evidence manifest schema documents artifact paths");
  assertContains(out.str(), "\"trace_id\"",
                 "CLI evidence manifest schema includes trace-ready fields");
}

void testAgentProviderConfigCommands() {
  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"provider-config-schema"};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentProviderConfigCommands schema exited with " << result << "\n";
      std::exit(1);
    }
    assertContains(out.str(), "\"schema_kind\":\"ccad_agent_provider_config_schema\"",
                   "provider config schema reports schema kind");
    assertContains(out.str(), "\"providers\"", "provider config schema lists providers");
    assertContains(out.str(), "\"OPENAI_API_KEY\"",
                   "provider config schema documents OpenAI env var");
    assertContains(out.str(), "\"ANTHROPIC_API_KEY\"",
                   "provider config schema documents Anthropic env var");
    assertContains(out.str(), "\"GEMINI_API_KEY\"",
                   "provider config schema documents Gemini env var");
    assertContains(out.str(), "\"GOOGLE_API_KEY\"",
                   "provider config schema documents Google Gemini alias env var");
    assertContains(out.str(), "\"project_file_secret_storage\":false",
                   "provider config schema refuses project-file secrets");
    assertContains(out.str(), "\"secret_value_policy\":\"never_emit_secret_values\"",
                   "provider config schema forbids secret emission");
  }

  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"provider-config-template"};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentProviderConfigCommands template exited with " << result
                << "\n";
      std::exit(1);
    }
    assertContains(out.str(), "\"template_kind\":\"ccad_agent_provider_config_template\"",
                   "provider config template reports template kind");
    assertContains(out.str(), "\"provider_execution\":false",
                   "provider config template does not enable execution");
    assertContains(out.str(), "\"secret_values_present\":false",
                   "provider config template does not contain secret values");
    assertContains(out.str(), "\"CCAD_OPENAI_COMPATIBLE_BASE_URL\"",
                   "provider config template includes OpenAI-compatible base URL env var");
    assertContains(out.str(), "\"CCAD_LOCAL_MODEL_BASE_URL\"",
                   "provider config template includes local model base URL env var");
    assertContains(out.str(), "\"GEMINI_API_KEY\"",
                   "provider config template includes Gemini API key env var");
  }

  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"provider-status"};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentProviderConfigCommands status exited with " << result << "\n";
      std::exit(1);
    }
    assertContains(out.str(), "\"status_kind\":\"ccad_agent_provider_status\"",
                   "provider status reports status kind");
    assertContains(out.str(), "\"env_value_redaction\":\"presence_only\"",
                   "provider status reports presence only");
    assertContains(out.str(), "\"provider_configured\"",
                   "provider status reports aggregate configuration");
    assertContains(out.str(), "\"secret_values_present\":false",
                   "provider status does not contain secret values");
    assertContains(out.str(), "\"provider_execution\":false",
                   "provider status does not run providers");
  }
}

void testAgentWorkspaceParityCommands() {
  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"state"};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentWorkspaceParityCommands state exited with " << result << "\n";
      std::exit(1);
    }
    assertContains(out.str(), "\"workspace_kind\":\"ccad_agent_workspace_state\"",
                   "agent state reports workspace state kind");
    assertContains(out.str(), "\"surface\":\"headless_cli\"",
                   "agent state identifies headless CLI surface");
    assertContains(out.str(), "\"panel_layout\":\"headless_cli_workspace\"",
                   "agent state reports headless layout contract");
    assertContains(out.str(), "\"task_state\":\"Task idle\"",
                   "agent state carries task summary");
    assertContains(out.str(), "\"approval_last_decision\":\"none\"",
                   "agent state carries approval decision");
  }

  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"tasks"};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentWorkspaceParityCommands tasks exited with " << result << "\n";
      std::exit(1);
    }
    assertContains(out.str(), "\"state_kind\":\"ccad_agent_tasks\"",
                   "agent tasks reports task state kind");
    assertContains(out.str(), "\"tasks\":[]", "agent tasks starts with no durable tasks");
    assertContains(out.str(), "\"durable_store\":\"not_configured\"",
                   "agent tasks does not pretend persistence exists");
  }

  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"evidence"};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentWorkspaceParityCommands evidence exited with " << result << "\n";
      std::exit(1);
    }
    assertContains(out.str(), "\"state_kind\":\"ccad_agent_evidence\"",
                   "agent evidence reports evidence state kind");
    assertContains(out.str(), "\"evidence\":[]", "agent evidence starts empty");
    assertContains(out.str(), "\"manifest_schema_method\":\"agent.evidence_manifest_schema\"",
                   "agent evidence points to manifest schema");
  }

  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"approvals"};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentWorkspaceParityCommands approvals exited with " << result
                << "\n";
      std::exit(1);
    }
    assertContains(out.str(), "\"state_kind\":\"ccad_agent_approvals\"",
                   "agent approvals reports approval state kind");
    assertContains(out.str(), "\"pending\":[]", "agent approvals starts empty");
    assertContains(out.str(), "\"last_decision\":\"none\"",
                   "agent approvals reports no decision yet");
  }
}

void testAgentSessionCheckpointCommands() {
  const std::string session_path = tempSessionPath("ccad-agent-session-test.ccad-agent-session.json");
  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"session-schema"};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentSessionCheckpointCommands schema exited with " << result
                << "\n";
      std::exit(1);
    }
    assertContains(out.str(), "\"schema_kind\":\"ccad_agent_session_schema\"",
                   "agent session schema reports schema kind");
    assertContains(out.str(), "\"checkpoint_fields\"",
                   "agent session schema reports checkpoint fields");
    assertContains(out.str(), "\"resource_uri\"",
                   "agent session schema includes MCP-ready resource URI field");
  }

  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"session-new",
                                     "--out",
                                     session_path,
                                     "--session-id",
                                     "sess-test",
                                     "--title",
                                     "Bridge run",
                                     "--project",
                                     "demo.ccad.json",
                                     "--created-at",
                                     "2026-06-05T00:00:00Z"};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentSessionCheckpointCommands session-new exited with " << result
                << "\n";
      std::exit(1);
    }
    assertContains(out.str(), "\"created\":true", "agent session-new reports file creation");
    assertContains(out.str(), "\"session_id\":\"sess-test\"", "agent session-new reports id");
    if (!std::filesystem::exists(session_path)) {
      std::cerr << "FAIL session file was not created\n";
      std::exit(1);
    }
  }

  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"session-state", "--session", session_path};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentSessionCheckpointCommands session-state exited with " << result
                << "\n";
      std::exit(1);
    }
    assertContains(out.str(), "\"session_kind\":\"ccad_agent_session\"",
                   "agent session-state reads session file");
    assertContains(out.str(), "\"thread_id\":\"sess-test\"",
                   "agent session-state preserves thread id");
    assertContains(out.str(), "\"checkpoint_count\":0",
                   "new session starts with no checkpoints");
  }

  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"checkpoint-add",
                                     "--session",
                                     session_path,
                                     "--checkpoint-id",
                                     "cp-001",
                                     "--kind",
                                     "planning",
                                     "--summary",
                                     "Plan bridge rectifier checks",
                                     "--artifact",
                                     "artifacts/screenshots/bridge.png",
                                     "--created-at",
                                     "2026-06-05T00:01:00Z"};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentSessionCheckpointCommands checkpoint-add exited with " << result
                << "\n";
      std::exit(1);
    }
    assertContains(out.str(), "\"checkpoint_added\":true",
                   "agent checkpoint-add reports append");
    assertContains(out.str(), "\"sequence\":1", "agent checkpoint-add assigns sequence");
  }

  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"session-state", "--session", session_path};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentSessionCheckpointCommands session-state after append exited with "
                << result << "\n";
      std::exit(1);
    }
    assertContains(out.str(), "\"checkpoint_count\":1",
                   "agent session-state sees appended checkpoint");
    assertContains(out.str(), "\"checkpoint_id\":\"cp-001\"",
                   "agent session-state includes checkpoint id");
    assertContains(out.str(), "\"resource_uri\":\"ccad-agent-checkpoint:sess-test/cp-001\"",
                   "agent session-state includes checkpoint resource URI");
  }

  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"replay", "--session", session_path};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentSessionCheckpointCommands replay exited with " << result
                << "\n";
      std::exit(1);
    }
    assertContains(out.str(), "\"manifest_kind\":\"ccad_agent_replay_manifest\"",
                   "agent replay returns manifest");
    assertContains(out.str(), "\"replayable\":true", "agent replay marks manifest replayable");
    assertContains(out.str(), "\"latest_checkpoint_id\":\"cp-001\"",
                   "agent replay reports latest checkpoint");
  }
}

void testAgentPolicyCommands() {
  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"policy-schema"};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentPolicyCommands policy-schema exited with " << result << "\n";
      std::exit(1);
    }
    assertContains(out.str(), "\"schema_kind\":\"ccad_agent_policy_schema\"",
                   "agent policy schema reports schema kind");
    assertContains(out.str(), "\"approval_required\"", "agent policy schema has approval field");
    assertContains(out.str(), "\"dry_run_supported\"", "agent policy schema has dry-run field");
  }

  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"policy-check", "--", "help", "--format", "json"};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentPolicyCommands read policy exited with " << result << "\n";
      std::exit(1);
    }
    assertContains(out.str(), "\"policy_kind\":\"ccad_agent_command_policy\"",
                   "agent policy-check returns policy kind");
    assertContains(out.str(), "\"read_only\":true", "agent policy-check identifies read command");
    assertContains(out.str(), "\"requires_allow_read\":true",
                   "agent policy-check requires read permission");
    assertContains(out.str(), "\"decision\":\"allow_read\"",
                   "agent policy-check allows read command");
  }

  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"policy-check", "--", "pcb", "add-via", "--file", "board.ccad.json"};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentPolicyCommands write policy exited with " << result << "\n";
      std::exit(1);
    }
    assertContains(out.str(), "\"read_only\":false", "agent policy-check identifies write command");
    assertContains(out.str(), "\"mutates_project\":true",
                   "agent policy-check marks project mutation");
    assertContains(out.str(), "\"requires_allow_write\":true",
                   "agent policy-check requires write permission");
    assertContains(out.str(), "\"approval_required\":true",
                   "agent policy-check requires approval for write command");
    assertContains(out.str(), "\"decision\":\"approval_required\"",
                   "agent policy-check reports approval decision");
  }

  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"dry-run", "--", "pcb", "add-via", "--file", "board.ccad.json"};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentPolicyCommands dry-run exited with " << result << "\n";
      std::exit(1);
    }
    assertContains(out.str(), "\"dry_run\":true", "agent dry-run marks dry run");
    assertContains(out.str(), "\"would_execute\":false", "agent dry-run does not execute");
    assertContains(out.str(), "\"decision\":\"dry_run_only\"", "agent dry-run reports dry-run decision");
  }
}

void testAgentMetadataJsonRpc() {
  std::istringstream in(
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.harness_context\", \"id\": 6}\n"
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.tool_guide\", \"params\": {\"method_name\": \"agent.harness_context\"}, \"id\": 7}\n");
  std::ostringstream out;
  auto oldCin = std::cin.rdbuf(in.rdbuf());
  auto oldCout = std::cout.rdbuf(out.rdbuf());
  std::vector<std::string> args = {"serve", "--allow-read"};
  int result = ccad_cli::agentCommand(args);
  std::cin.rdbuf(oldCin);
  std::cout.rdbuf(oldCout);
  if (result != 0) {
    std::cerr << "FAIL testAgentMetadataJsonRpc exited with " << result << "\n";
    std::exit(1);
  }
  assertContains(out.str(), "\"id\": 6", "has harness context request id");
  assertContains(out.str(), "\"harness_kind\":\"ccad_headless_cli_agent_surface\"",
                 "JSON-RPC exposes headless harness context");
  assertContains(out.str(), "\"id\": 7", "has tool guide request id");
  assertContains(out.str(), "\"found\":true", "JSON-RPC tool guide finds known method");
  assertContains(out.str(), "\"preferred_surface\"",
                 "JSON-RPC tool guide includes preferred surface");
}

void testAgentWorkspaceParityJsonRpc() {
  std::istringstream in(
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.state\", \"id\": 8}\n"
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.workspace_state\", \"id\": 9}\n"
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.tasks\", \"id\": 10}\n"
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.evidence\", \"id\": 11}\n"
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.approvals\", \"id\": 12}\n"
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.tool_guide\", \"params\": {\"method_name\": \"agent.state\"}, \"id\": 13}\n");
  std::ostringstream out;
  auto oldCin = std::cin.rdbuf(in.rdbuf());
  auto oldCout = std::cout.rdbuf(out.rdbuf());
  std::vector<std::string> args = {"serve", "--allow-read"};
  int result = ccad_cli::agentCommand(args);
  std::cin.rdbuf(oldCin);
  std::cout.rdbuf(oldCout);
  if (result != 0) {
    std::cerr << "FAIL testAgentWorkspaceParityJsonRpc exited with " << result << "\n";
    std::exit(1);
  }
  assertContains(out.str(), "\"id\": 8", "has agent state request id");
  assertContains(out.str(), "\"workspace_kind\":\"ccad_agent_workspace_state\"",
                 "JSON-RPC exposes headless workspace state");
  assertContains(out.str(), "\"id\": 9", "has GUI-compatible workspace alias request id");
  assertContains(out.str(), "\"surface\":\"headless_cli\"",
                 "JSON-RPC workspace alias returns headless state");
  assertContains(out.str(), "\"id\": 10", "has agent tasks request id");
  assertContains(out.str(), "\"state_kind\":\"ccad_agent_tasks\"",
                 "JSON-RPC exposes task state");
  assertContains(out.str(), "\"id\": 11", "has agent evidence request id");
  assertContains(out.str(), "\"state_kind\":\"ccad_agent_evidence\"",
                 "JSON-RPC exposes evidence state");
  assertContains(out.str(), "\"id\": 12", "has agent approvals request id");
  assertContains(out.str(), "\"state_kind\":\"ccad_agent_approvals\"",
                 "JSON-RPC exposes approval state");
  assertContains(out.str(), "\"id\": 13", "has state tool guide request id");
  assertContains(out.str(), "\"method\":\"agent.state\"",
                 "JSON-RPC tool guide finds agent state");
  assertContains(out.str(), "\"preferred_surface\":\"headless_cli_workspace_state\"",
                 "JSON-RPC tool guide points state methods to the headless workspace surface");
}

void testAgentSessionJsonRpc() {
  const std::string session_path =
      tempSessionPath("ccad-agent-session-json-rpc-test.ccad-agent-session.json");
  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"session-new",
                                     "--out",
                                     session_path,
                                     "--session-id",
                                     "sess-rpc",
                                     "--title",
                                     "RPC run",
                                     "--created-at",
                                     "2026-06-05T00:02:00Z"};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentSessionJsonRpc setup exited with " << result << "\n";
      std::exit(1);
    }
  }

  std::istringstream in(
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.session_schema\", \"id\": 14}\n"
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.session_state\", \"params\": {\"session_path\": \"" +
      session_path +
      "\"}, \"id\": 15}\n"
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.replay_manifest\", \"params\": {\"session_path\": \"" +
      session_path + "\"}, \"id\": 16}\n");
  std::ostringstream out;
  auto oldCin = std::cin.rdbuf(in.rdbuf());
  auto oldCout = std::cout.rdbuf(out.rdbuf());
  std::vector<std::string> args = {"serve", "--allow-read"};
  int result = ccad_cli::agentCommand(args);
  std::cin.rdbuf(oldCin);
  std::cout.rdbuf(oldCout);
  if (result != 0) {
    std::cerr << "FAIL testAgentSessionJsonRpc exited with " << result << "\n";
    std::exit(1);
  }
  assertContains(out.str(), "\"id\": 14", "has session schema request id");
  assertContains(out.str(), "\"schema_kind\":\"ccad_agent_session_schema\"",
                 "JSON-RPC exposes session schema");
  assertContains(out.str(), "\"id\": 15", "has session state request id");
  assertContains(out.str(), "\"session_id\":\"sess-rpc\"", "JSON-RPC reads session file");
  assertContains(out.str(), "\"id\": 16", "has replay request id");
  assertContains(out.str(), "\"manifest_kind\":\"ccad_agent_replay_manifest\"",
                 "JSON-RPC exposes replay manifest");
}

void testAgentPolicyJsonRpc() {
  std::istringstream in(
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.policy_schema\", \"id\": 17}\n"
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.policy_check\", \"params\": {\"args\": [\"pcb\", \"add-via\", \"--file\", \"board.ccad.json\"], \"dry_run\": true}, \"id\": 18}\n");
  std::ostringstream out;
  auto oldCin = std::cin.rdbuf(in.rdbuf());
  auto oldCout = std::cout.rdbuf(out.rdbuf());
  std::vector<std::string> args = {"serve", "--allow-read"};
  int result = ccad_cli::agentCommand(args);
  std::cin.rdbuf(oldCin);
  std::cout.rdbuf(oldCout);
  if (result != 0) {
    std::cerr << "FAIL testAgentPolicyJsonRpc exited with " << result << "\n";
    std::exit(1);
  }
  assertContains(out.str(), "\"id\": 17", "has policy schema request id");
  assertContains(out.str(), "\"schema_kind\":\"ccad_agent_policy_schema\"",
                 "JSON-RPC exposes policy schema");
  assertContains(out.str(), "\"id\": 18", "has policy check request id");
  assertContains(out.str(), "\"approval_required\":true",
                 "JSON-RPC policy check reports approval required");
  assertContains(out.str(), "\"dry_run\":true", "JSON-RPC policy check accepts dry_run");
  assertContains(out.str(), "\"decision\":\"dry_run_only\"",
                 "JSON-RPC policy check reports dry-run decision");
}

void testAgentProviderConfigJsonRpc() {
  std::istringstream in(
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.provider_config_schema\", \"id\": 20}\n"
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.provider_config_template\", \"id\": 21}\n"
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.provider_status\", \"id\": 22}\n"
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.tool_guide\", \"params\": {\"method_name\": \"agent.provider_config_schema\"}, \"id\": 23}\n");
  std::ostringstream out;
  auto oldCin = std::cin.rdbuf(in.rdbuf());
  auto oldCout = std::cout.rdbuf(out.rdbuf());
  std::vector<std::string> args = {"serve", "--allow-read"};
  int result = ccad_cli::agentCommand(args);
  std::cin.rdbuf(oldCin);
  std::cout.rdbuf(oldCout);
  if (result != 0) {
    std::cerr << "FAIL testAgentProviderConfigJsonRpc exited with " << result << "\n";
    std::exit(1);
  }
  assertContains(out.str(), "\"id\": 20", "has provider schema request id");
  assertContains(out.str(), "\"schema_kind\":\"ccad_agent_provider_config_schema\"",
                 "JSON-RPC exposes provider config schema");
  assertContains(out.str(), "\"id\": 21", "has provider template request id");
  assertContains(out.str(), "\"template_kind\":\"ccad_agent_provider_config_template\"",
                 "JSON-RPC exposes provider config template");
  assertContains(out.str(), "\"id\": 22", "has provider status request id");
  assertContains(out.str(), "\"status_kind\":\"ccad_agent_provider_status\"",
                 "JSON-RPC exposes provider status");
  assertContains(out.str(), "\"id\": 23", "has provider config tool guide request id");
  assertContains(out.str(), "\"method\":\"agent.provider_config_schema\"",
                 "JSON-RPC tool guide finds provider config schema");
  assertContains(out.str(), "\"preferred_surface\":\"headless_cli_provider_config\"",
                 "JSON-RPC tool guide points provider config to the headless config surface");
}

void testAgentServePermissionGatesReportApproval() {
  std::istringstream in(
      "{\"jsonrpc\": \"2.0\", \"method\": \"execute\", \"params\": {\"args\": [\"pcb\", \"add-via\", \"--file\", \"board.ccad.json\"]}, \"id\": 19}\n");
  std::ostringstream out;
  auto oldCin = std::cin.rdbuf(in.rdbuf());
  auto oldCout = std::cout.rdbuf(out.rdbuf());
  std::vector<std::string> args = {"serve", "--allow-read"};
  int result = ccad_cli::agentCommand(args);
  std::cin.rdbuf(oldCin);
  std::cout.rdbuf(oldCout);
  if (result != 0) {
    std::cerr << "FAIL testAgentServePermissionGatesReportApproval exited with " << result << "\n";
    std::exit(1);
  }
  assertContains(out.str(), "\"code\": -32604", "write command without write permission is denied");
  assertContains(out.str(), "Approval required", "write denial says approval is required");
  assertContains(out.str(), "project_mutation", "write denial includes approval reason");
}

int main() {
  try {
    testPing();
    testExecute();
    testMCPInitialize();
    testMCPToolsList();
    testMCPToolsCall();
    testAgentMethodsCommand();
    testAgentMetadataCommands();
    testAgentProviderConfigCommands();
    testAgentWorkspaceParityCommands();
    testAgentSessionCheckpointCommands();
    testAgentPolicyCommands();
    testAgentMetadataJsonRpc();
    testAgentWorkspaceParityJsonRpc();
    testAgentSessionJsonRpc();
    testAgentPolicyJsonRpc();
    testAgentProviderConfigJsonRpc();
    testAgentServePermissionGatesReportApproval();
    std::cout << "PASS agent serve\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "FAIL exception: " << e.what() << "\n";
    return 1;
  }
}
