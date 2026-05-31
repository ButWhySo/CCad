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

int main() {
  try {
    testPing();
    testExecute();
    std::cout << "PASS agent serve\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "FAIL exception: " << e.what() << "\n";
    return 1;
  }
}
