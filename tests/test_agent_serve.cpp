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
  assertContains(out.str(), "\"method\":\"agent.trace_export_schema\"",
                 "agent methods command includes trace export schema");
  assertContains(out.str(), "\"method\":\"agent.trace_export_template\"",
                 "agent methods command includes trace export template");
  assertContains(out.str(), "\"method\":\"agent.trace_redaction_policy\"",
                 "agent methods command includes trace redaction policy");
  assertContains(out.str(), "\"method\":\"agent.trace_export_dry_run\"",
                 "agent methods command includes trace export dry run");
  assertContains(out.str(), "\"method\":\"agent.kicad_evidence_schema\"",
                 "agent methods command includes KiCad evidence schema");
  assertContains(out.str(), "\"method\":\"agent.kicad_evidence_plan\"",
                 "agent methods command includes KiCad evidence plan");
  assertContains(out.str(), "\"method\":\"agent.kicad_evidence_dry_run\"",
                 "agent methods command includes KiCad evidence dry run");
  assertContains(out.str(), "\"method\":\"agent.kicad_evidence_run\"",
                 "agent methods command includes guarded KiCad evidence run");
  assertContains(out.str(), "\"method\":\"agent.pcb_api_schema\"",
                 "agent methods command includes PCB API parity schema");
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

  out.str("");
  out.clear();
  oldCout = std::cout.rdbuf(out.rdbuf());
  std::vector<std::string> pcb_api_args = {"pcb-api-schema"};
  result = ccad_cli::agentCommand(pcb_api_args);
  std::cout.rdbuf(oldCout);
  if (result != 0) {
    std::cerr << "FAIL testAgentMetadataCommands PCB API schema exited with " << result << "\n";
    std::exit(1);
  }
  assertContains(out.str(), "\"schema_kind\":\"ccad_agent_pcb_api_schema\"",
                 "PCB API schema reports schema kind");
  assertContains(out.str(), "F:/kicad_src/pcbnew/api/api_handler_pcb.cpp",
                 "PCB API schema references the audited KiCad PCB handler");
  assertContains(out.str(), "\"kicad_handler\":\"GetConnectedItems\"",
                 "PCB API schema includes KiCad connected-items handler");
  assertContains(out.str(), "\"ccad_command\":\"pcb list-connected\"",
                 "PCB API schema maps connected items to the CCad CLI command");
  assertContains(out.str(), "\"kicad_handler\":\"GetItemsByNet\"",
                 "PCB API schema includes KiCad items-by-net handler");
  assertContains(out.str(), "\"ccad_command\":\"pcb list-by-net\"",
                 "PCB API schema maps items by net to the CCad CLI command");
  assertContains(out.str(), "\"kicad_handler\":\"GetBoardStackup\"",
                 "PCB API schema includes KiCad board stackup handler");
  assertContains(out.str(), "\"ccad_command\":\"pcb get-board-stackup\"",
                 "PCB API schema maps stackup to the CCad CLI command");
  assertContains(out.str(), "\"kicad_handler\":\"GetBoardDesignRules\"",
                 "PCB API schema includes KiCad board design rules handler");
  assertContains(out.str(), "\"ccad_command\":\"pcb get-rules\"",
                 "PCB API schema maps design rules to the CCad CLI command");
  assertContains(out.str(), "\"connectivity_scope\":\"net_equivalent_first_slice\"",
                 "PCB API schema documents first-slice connectivity scope");
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

void testAgentObservabilityConfigCommands() {
  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"trace-export-schema"};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentObservabilityConfigCommands schema exited with " << result
                << "\n";
      std::exit(1);
    }
    assertContains(out.str(), "\"schema_kind\":\"ccad_agent_trace_export_schema\"",
                   "trace export schema reports schema kind");
    assertContains(out.str(), "\"OTEL_EXPORTER_OTLP_ENDPOINT\"",
                   "trace export schema documents OTLP endpoint env var");
    assertContains(out.str(), "\"OTEL_EXPORTER_OTLP_HEADERS\"",
                   "trace export schema documents OTLP headers env var");
    assertContains(out.str(), "\"langfuse\"", "trace export schema includes Langfuse backend");
    assertContains(out.str(), "\"gen_ai\"", "trace export schema references GenAI conventions");
    assertContains(out.str(), "\"export_enabled\":false",
                   "trace export schema keeps export disabled");
  }

  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"trace-export-template"};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentObservabilityConfigCommands template exited with " << result
                << "\n";
      std::exit(1);
    }
    assertContains(out.str(), "\"template_kind\":\"ccad_agent_trace_export_template\"",
                   "trace export template reports template kind");
    assertContains(out.str(), "\"secret_values_present\":false",
                   "trace export template contains no secret values");
    assertContains(out.str(), "\"default_export_enabled\":false",
                   "trace export template defaults export off");
    assertContains(out.str(), "\"endpoint_env\":\"OTEL_EXPORTER_OTLP_ENDPOINT\"",
                   "trace export template uses OTLP endpoint env var");
    assertContains(out.str(), "\"headers_env\":\"OTEL_EXPORTER_OTLP_HEADERS\"",
                   "trace export template stores header env var name only");
  }

  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"trace-redaction-policy"};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentObservabilityConfigCommands redaction exited with " << result
                << "\n";
      std::exit(1);
    }
    assertContains(out.str(), "\"policy_kind\":\"ccad_agent_trace_redaction_policy\"",
                   "trace redaction policy reports policy kind");
    assertContains(out.str(), "\"export_prompt_content_by_default\":false",
                   "trace redaction policy defaults prompt export off");
    assertContains(out.str(), "\"export_tool_payloads_by_default\":false",
                   "trace redaction policy defaults tool payload export off");
    assertContains(out.str(), "\"export_screenshots_by_default\":false",
                   "trace redaction policy defaults screenshot export off");
    assertContains(out.str(), "\"export_design_files_by_default\":false",
                   "trace redaction policy defaults design file export off");
    assertContains(out.str(), "\"secret_values\":\"never\"",
                   "trace redaction policy refuses secret values");
  }

  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"trace-export-dry-run"};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentObservabilityConfigCommands dry-run exited with " << result
                << "\n";
      std::exit(1);
    }
    assertContains(out.str(), "\"dry_run_kind\":\"ccad_agent_trace_export_dry_run\"",
                   "trace export dry run reports dry-run kind");
    assertContains(out.str(), "\"network_probe_performed\":false",
                   "trace export dry run performs no network probe");
    assertContains(out.str(), "\"would_export\":false",
                   "trace export dry run does not export by default");
    assertContains(out.str(), "\"redaction_applied\":true",
                   "trace export dry run applies redaction");
    assertContains(out.str(), "\"headers_value\":\"redacted\"",
                   "trace export dry run redacts header values");
  }
}

void testAgentKiCadEvidenceCommands() {
  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"kicad-evidence-schema"};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentKiCadEvidenceCommands schema exited with " << result
                << "\n";
      std::exit(1);
    }
    assertContains(out.str(), "\"schema_kind\":\"ccad_agent_kicad_evidence_schema\"",
                   "KiCad evidence schema reports schema kind");
    assertContains(out.str(), "\"tool\":\"kicad-cli\"",
                   "KiCad evidence schema identifies the external tool");
    assertContains(out.str(), "\"pcb-drc\"", "KiCad evidence schema includes PCB DRC");
    assertContains(out.str(), "\"sch-erc\"", "KiCad evidence schema includes schematic ERC");
    assertContains(out.str(), "\"pcb-export-gerbers\"",
                   "KiCad evidence schema includes Gerber export");
    assertContains(out.str(), "\"pcb-export-drill\"",
                   "KiCad evidence schema includes drill export");
    assertContains(out.str(), "\"pcb-export-pos\"",
                   "KiCad evidence schema includes placement export");
    assertContains(out.str(), "\"pcb-export-ipc2581\"",
                   "KiCad evidence schema includes IPC-2581 export");
    assertContains(out.str(), "\"pcb-export-odb\"",
                   "KiCad evidence schema includes ODB++ export");
  }

  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"kicad-evidence-plan",
                                     "--kind",
                                     "pcb-drc",
                                     "--input",
                                     "board.kicad_pcb",
                                     "--output",
                                     "artifacts/kicad/drc.json",
                                     "--format",
                                     "json",
                                     "--units",
                                     "mm",
                                     "--severity",
                                     "all",
                                     "--exit-code-violations"};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentKiCadEvidenceCommands plan exited with " << result
                << "\n";
      std::exit(1);
    }
    assertContains(out.str(), "\"plan_kind\":\"ccad_agent_kicad_evidence_plan\"",
                   "KiCad evidence plan reports plan kind");
    assertContains(out.str(), "\"command_kind\":\"pcb-drc\"",
                   "KiCad evidence plan carries the requested kind");
    assertContains(out.str(), "\"command\":[\"kicad-cli\",\"pcb\",\"drc\"",
                   "KiCad evidence plan builds a structured command array");
    assertContains(out.str(), "\"--format\",\"json\"",
                   "KiCad evidence plan includes KiCad report format");
    assertContains(out.str(), "\"--units\",\"mm\"",
                   "KiCad evidence plan includes KiCad units");
    assertContains(out.str(), "\"--severity-all\"",
                   "KiCad evidence plan maps severity all to KiCad flag");
    assertContains(out.str(), "\"--exit-code-violations\"",
                   "KiCad evidence plan includes violation exit-code flag");
    assertContains(out.str(), "\"artifact_path\":\"artifacts/kicad/drc.json\"",
                   "KiCad evidence plan records output artifact path");
  }

  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"kicad-evidence-dry-run",
                                     "--kind",
                                     "pcb-export-gerbers",
                                     "--input",
                                     "board.kicad_pcb",
                                     "--output",
                                     "artifacts/fab/gerbers",
                                     "--layers",
                                     "F.Cu,B.Cu",
                                     "--kicad-cli",
                                     "Z:/missing/kicad-cli.exe"};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentKiCadEvidenceCommands dry-run exited with " << result
                << "\n";
      std::exit(1);
    }
    assertContains(out.str(), "\"dry_run_kind\":\"ccad_agent_kicad_evidence_dry_run\"",
                   "KiCad evidence dry run reports dry-run kind");
    assertContains(out.str(), "\"would_execute\":false",
                   "KiCad evidence dry run does not execute");
    assertContains(out.str(), "\"executable_configured\":true",
                   "KiCad evidence dry run sees explicit executable path");
    assertContains(out.str(), "\"executable_found\":false",
                   "KiCad evidence dry run reports missing executable");
    assertContains(out.str(), "\"network_access\":false",
                   "KiCad evidence dry run declares no network access");
    assertContains(out.str(), "\"project_file_secret_storage\":false",
                   "KiCad evidence dry run keeps secrets out of project files");
  }

  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"kicad-evidence-run",
                                     "--kind",
                                     "sch-erc",
                                     "--input",
                                     "root.kicad_sch",
                                     "--output",
                                     "artifacts/kicad/erc.json",
                                     "--format",
                                     "json"};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentKiCadEvidenceCommands run guard exited with " << result
                << "\n";
      std::exit(1);
    }
    assertContains(out.str(), "\"run_kind\":\"ccad_agent_kicad_evidence_run\"",
                   "KiCad evidence run reports run kind");
    assertContains(out.str(), "\"executed\":false",
                   "KiCad evidence run refuses execution without explicit flag");
    assertContains(out.str(), "\"reason\":\"execute_flag_required\"",
                   "KiCad evidence run explains missing execute flag");
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

  {
    std::ostringstream out;
    auto oldCout = std::cout.rdbuf(out.rdbuf());
    std::vector<std::string> args = {"policy-check",
                                     "--",
                                     "agent",
                                     "kicad-evidence-run",
                                     "--kind",
                                     "pcb-drc",
                                     "--input",
                                     "board.kicad_pcb",
                                     "--output",
                                     "artifacts/kicad/drc.json",
                                     "--execute"};
    int result = ccad_cli::agentCommand(args);
    std::cout.rdbuf(oldCout);
    if (result != 0) {
      std::cerr << "FAIL testAgentPolicyCommands KiCad evidence policy exited with " << result
                << "\n";
      std::exit(1);
    }
    assertContains(out.str(), "\"mutates_files\":true",
                   "agent policy-check marks KiCad evidence run as file-writing");
    assertContains(out.str(), "\"approval_required\":true",
                   "agent policy-check requires approval for KiCad evidence run");
    assertContains(out.str(), "\"approval_reason\":\"external_process_file_write\"",
                   "agent policy-check reports external process write reason");
    assertContains(out.str(), "\"decision\":\"approval_required\"",
                   "agent policy-check blocks KiCad evidence run until approval");
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

void testAgentObservabilityConfigJsonRpc() {
  std::istringstream in(
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.trace_export_schema\", \"id\": 24}\n"
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.trace_export_template\", \"id\": 25}\n"
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.trace_redaction_policy\", \"id\": 26}\n"
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.trace_export_dry_run\", \"id\": 27}\n"
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.tool_guide\", \"params\": {\"method_name\": \"agent.trace_export_schema\"}, \"id\": 28}\n");
  std::ostringstream out;
  auto oldCin = std::cin.rdbuf(in.rdbuf());
  auto oldCout = std::cout.rdbuf(out.rdbuf());
  std::vector<std::string> args = {"serve", "--allow-read"};
  int result = ccad_cli::agentCommand(args);
  std::cin.rdbuf(oldCin);
  std::cout.rdbuf(oldCout);
  if (result != 0) {
    std::cerr << "FAIL testAgentObservabilityConfigJsonRpc exited with " << result << "\n";
    std::exit(1);
  }
  assertContains(out.str(), "\"id\": 24", "has trace schema request id");
  assertContains(out.str(), "\"schema_kind\":\"ccad_agent_trace_export_schema\"",
                 "JSON-RPC exposes trace export schema");
  assertContains(out.str(), "\"id\": 25", "has trace template request id");
  assertContains(out.str(), "\"template_kind\":\"ccad_agent_trace_export_template\"",
                 "JSON-RPC exposes trace export template");
  assertContains(out.str(), "\"id\": 26", "has trace redaction request id");
  assertContains(out.str(), "\"policy_kind\":\"ccad_agent_trace_redaction_policy\"",
                 "JSON-RPC exposes trace redaction policy");
  assertContains(out.str(), "\"id\": 27", "has trace dry-run request id");
  assertContains(out.str(), "\"dry_run_kind\":\"ccad_agent_trace_export_dry_run\"",
                 "JSON-RPC exposes trace export dry run");
  assertContains(out.str(), "\"id\": 28", "has trace schema tool guide request id");
  assertContains(out.str(), "\"method\":\"agent.trace_export_schema\"",
                 "JSON-RPC tool guide finds trace export schema");
  assertContains(out.str(), "\"preferred_surface\":\"headless_cli_observability_config\"",
                 "JSON-RPC tool guide points trace config to the headless observability surface");
}

void testAgentKiCadEvidenceJsonRpc() {
  std::istringstream in(
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.kicad_evidence_schema\", \"id\": 29}\n"
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.kicad_evidence_plan\", \"params\": {\"kind\": \"pcb-drc\", \"input_path\": \"board.kicad_pcb\", \"output_path\": \"artifacts/kicad/drc.json\", \"format\": \"json\", \"units\": \"mm\", \"severity\": \"all\", \"exit_code_violations\": true}, \"id\": 30}\n"
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.kicad_evidence_dry_run\", \"params\": {\"kind\": \"pcb-export-drill\", \"input_path\": \"board.kicad_pcb\", \"output_path\": \"artifacts/fab/drill\", \"format\": \"excellon\"}, \"id\": 31}\n"
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.tool_guide\", \"params\": {\"method_name\": \"agent.kicad_evidence_plan\"}, \"id\": 32}\n"
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.kicad_evidence_run\", \"params\": {\"kind\": \"pcb-drc\", \"input_path\": \"board.kicad_pcb\", \"output_path\": \"artifacts/kicad/drc.json\", \"execute\": true}, \"id\": 33}\n"
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.pcb_api_schema\", \"id\": 34}\n"
      "{\"jsonrpc\": \"2.0\", \"method\": \"agent.tool_guide\", \"params\": {\"method_name\": \"agent.pcb_api_schema\"}, \"id\": 35}\n");
  std::ostringstream out;
  auto oldCin = std::cin.rdbuf(in.rdbuf());
  auto oldCout = std::cout.rdbuf(out.rdbuf());
  std::vector<std::string> args = {"serve", "--allow-read"};
  int result = ccad_cli::agentCommand(args);
  std::cin.rdbuf(oldCin);
  std::cout.rdbuf(oldCout);
  if (result != 0) {
    std::cerr << "FAIL testAgentKiCadEvidenceJsonRpc exited with " << result << "\n";
    std::exit(1);
  }
  assertContains(out.str(), "\"id\": 29", "has KiCad evidence schema request id");
  assertContains(out.str(), "\"schema_kind\":\"ccad_agent_kicad_evidence_schema\"",
                 "JSON-RPC exposes KiCad evidence schema");
  assertContains(out.str(), "\"id\": 30", "has KiCad evidence plan request id");
  assertContains(out.str(), "\"plan_kind\":\"ccad_agent_kicad_evidence_plan\"",
                 "JSON-RPC exposes KiCad evidence plan");
  assertContains(out.str(), "\"--severity-all\"",
                 "JSON-RPC KiCad evidence plan maps severity flag");
  assertContains(out.str(), "\"id\": 31", "has KiCad evidence dry-run request id");
  assertContains(out.str(), "\"dry_run_kind\":\"ccad_agent_kicad_evidence_dry_run\"",
                 "JSON-RPC exposes KiCad evidence dry run");
  assertContains(out.str(), "\"id\": 32", "has KiCad evidence tool-guide request id");
  assertContains(out.str(), "\"method\":\"agent.kicad_evidence_plan\"",
                 "JSON-RPC tool guide finds KiCad evidence plan");
  assertContains(out.str(), "\"preferred_surface\":\"headless_cli_kicad_evidence\"",
                 "JSON-RPC tool guide points KiCad evidence to the headless surface");
  assertContains(out.str(), "\"id\": 33", "has guarded KiCad evidence run request id");
  assertContains(out.str(), "\"code\": -32604",
                 "read-only JSON-RPC denies executing KiCad evidence run");
  assertContains(out.str(), "external_process_file_write",
                 "KiCad evidence run denial includes external process write reason");
  assertContains(out.str(), "\"id\": 34", "has PCB API schema request id");
  assertContains(out.str(), "\"schema_kind\":\"ccad_agent_pcb_api_schema\"",
                 "JSON-RPC exposes PCB API schema");
  assertContains(out.str(), "\"kicad_handler\":\"GetConnectedItems\"",
                 "JSON-RPC PCB API schema includes connected-items mapping");
  assertContains(out.str(), "\"id\": 35", "has PCB API schema tool-guide request id");
  assertContains(out.str(), "\"method\":\"agent.pcb_api_schema\"",
                 "JSON-RPC tool guide finds PCB API schema");
  assertContains(out.str(), "\"preferred_surface\":\"headless_cli_pcb_api_parity\"",
                 "JSON-RPC tool guide points PCB API schema to the headless parity surface");
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

void testAgentToolCallRpc() {
  std::istringstream in(
      "{\"jsonrpc\":\"2.0\",\"method\":\"agent.tool_call\",\"params\":{\"name\":\"project.context\"},\"id\":36}\n"
      "{\"jsonrpc\":\"2.0\",\"method\":\"agent.tool_call\",\"params\":{\"name\":\"no.such.tool\"},\"id\":37}\n"
      "{\"jsonrpc\":\"2.0\",\"method\":\"agent.tool_call\",\"params\":{\"name\":\"pcb.add-via\",\"id\":\"nested\"},\"id\":38}\n");
  std::ostringstream out;
  auto oldCin = std::cin.rdbuf(in.rdbuf());
  auto oldCout = std::cout.rdbuf(out.rdbuf());
  std::vector<std::string> args = {"serve", "--allow-read"};
  int result = ccad_cli::agentCommand(args);
  std::cin.rdbuf(oldCin);
  std::cout.rdbuf(oldCout);
  if (result != 0) std::exit(1);
  assertContains(out.str(), "\"id\": 36", "tool call preserves request id");
  assertContains(out.str(), "\"has_board\":false", "context tool returns live empty context");
  assertContains(out.str(), "\"id\": 37", "unknown tool preserves request id");
  assertContains(out.str(), "-32601", "unknown tool returns method error");
  assertContains(out.str(), "\"id\": 38", "nested tool id does not replace request id");
  assertContains(out.str(), "-32604", "mutation tool requires write approval");

  std::istringstream write_in(
      "{\"jsonrpc\":\"2.0\",\"method\":\"agent.tool_call\",\"params\":{\"name\":\"pcb.add-via\"},\"id\":39}\n");
  std::ostringstream write_out;
  oldCin = std::cin.rdbuf(write_in.rdbuf());
  oldCout = std::cout.rdbuf(write_out.rdbuf());
  args = {"serve", "--allow-read", "--allow-write"};
  result = ccad_cli::agentCommand(args);
  std::cin.rdbuf(oldCin);
  std::cout.rdbuf(oldCout);
  if (result != 0) std::exit(1);
  assertContains(write_out.str(), "\"id\": 39", "missing-parameter request preserves id");
  assertContains(write_out.str(), "missing_parameter", "missing required tool parameter is explicit");

  std::ostringstream drc_out;
  std::istringstream drc_in(
      "{\"jsonrpc\":\"2.0\",\"method\":\"agent.tool_call\",\"params\":{\"name\":\"pcb.drc\"},\"id\":40}\n");
  oldCin = std::cin.rdbuf(drc_in.rdbuf());
  oldCout = std::cout.rdbuf(drc_out.rdbuf());
  args = {"serve", "--allow-read"};
  result = ccad_cli::agentCommand(args);
  std::cin.rdbuf(oldCin);
  std::cout.rdbuf(oldCout);
  if (result != 0) std::exit(1);
  assertContains(drc_out.str(), "\"id\": 40", "drc request preserves id");
  assertContains(drc_out.str(), "missing_parameter", "drc requires file before dispatch");

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
    testAgentObservabilityConfigCommands();
    testAgentKiCadEvidenceCommands();
    testAgentWorkspaceParityCommands();
    testAgentSessionCheckpointCommands();
    testAgentPolicyCommands();
    testAgentMetadataJsonRpc();
    testAgentWorkspaceParityJsonRpc();
    testAgentSessionJsonRpc();
    testAgentPolicyJsonRpc();
    testAgentProviderConfigJsonRpc();
    testAgentObservabilityConfigJsonRpc();
    testAgentKiCadEvidenceJsonRpc();
    testAgentServePermissionGatesReportApproval();
    testAgentToolCallRpc();
    std::cout << "PASS agent serve\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "FAIL exception: " << e.what() << "\n";
    return 1;
  }
}
