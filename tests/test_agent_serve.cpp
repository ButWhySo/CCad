#include "ccad_cli/agent_commands.hpp"

#include <iostream>
#include <sstream>
#include <string>

void assertContains(const std::string& content, const std::string& pattern, const std::string& label) {
  if (content.find(pattern) == std::string::npos) {
    std::cerr << "FAIL " << label << "\n  pattern not found: " << pattern << '\n';
    std::exit(1);
  }
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
  assertContains(out.str(), "\"method\":\"agent.observability_config\"",
                 "agent methods command includes observability config");
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

int main() {
  try {
    testPing();
    testExecute();
    testMCPInitialize();
    testMCPToolsList();
    testMCPToolsCall();
    testAgentMethodsCommand();
    testAgentMetadataCommands();
    testAgentMetadataJsonRpc();
    std::cout << "PASS agent serve\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "FAIL exception: " << e.what() << "\n";
    return 1;
  }
}
