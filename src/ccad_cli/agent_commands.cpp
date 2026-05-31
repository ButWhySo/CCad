#include "agent_commands.hpp"

#include "app.hpp"
#include "ccad_core/json.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>

namespace ccad_cli {

namespace {

// Extremely basic handwritten parser for newline-delimited JSON-RPC requests.
// We only support {"jsonrpc":"2.0", "method":"...", "params":{...}, "id":...}
// For `execute`, params should have `"args": ["...", "..."]`

std::string extractStringValue(const std::string& json, const std::string& key) {
  std::string search = "\"" + key + "\":";
  auto pos = json.find(search);
  if (pos == std::string::npos) return "";
  pos += search.length();
  while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
  if (pos < json.length() && json[pos] == '"') {
    pos++;
    auto end = json.find("\"", pos);
    if (end != std::string::npos) {
      return json.substr(pos, end - pos);
    }
  }
  return "";
}

std::string extractRawValue(const std::string& json, const std::string& key) {
  std::string search = "\"" + key + "\":";
  auto pos = json.find(search);
  if (pos == std::string::npos) return "";
  pos += search.length();
  while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
  auto end = json.find_first_of(",}", pos);
  if (end != std::string::npos) {
    std::string val = json.substr(pos, end - pos);
    // trim right whitespace
    while (!val.empty() && (val.back() == ' ' || val.back() == '\t')) {
      val.pop_back();
    }
    return val;
  }
  return "";
}

std::vector<std::string> extractStringArray(const std::string& json, const std::string& key) {
  std::vector<std::string> result;
  std::string search = "\"" + key + "\":";
  auto pos = json.find(search);
  if (pos == std::string::npos) return result;
  pos += search.length();
  while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
  if (pos < json.length() && json[pos] == '[') {
    pos++;
    while (pos < json.length() && json[pos] != ']') {
      while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == ',')) pos++;
      if (pos < json.length() && json[pos] == '"') {
        pos++;
        auto end = json.find("\"", pos);
        if (end != std::string::npos) {
          result.push_back(json.substr(pos, end - pos));
          pos = end + 1;
        } else {
          break;
        }
      } else {
        break;
      }
    }
  }
  return result;
}

std::string formatError(const std::string& id, int code, const std::string& message) {
  std::ostringstream out;
  out << "{\"jsonrpc\": \"2.0\", \"error\": {\"code\": " << code << ", \"message\": \"" << ccad::escapeJson(message) << "\"}";
  if (!id.empty()) {
    out << ", \"id\": " << id;
  } else {
    out << ", \"id\": null";
  }
  out << "}";
  return out.str();
}

std::string formatSuccess(const std::string& id, const std::string& resultJson) {
  std::ostringstream out;
  out << "{\"jsonrpc\": \"2.0\", \"result\": " << resultJson << ", \"id\": " << id << "}";
  return out.str();
}

}  // namespace

int agentCommand(const std::vector<std::string>& args) {
  if (args.size() != 1 || args[0] != "serve") {
    std::cerr << "Usage: ccad agent serve\n";
    return 1;
  }

  std::string line;
  while (std::getline(std::cin, line)) {
    if (line.empty()) continue;

    std::string jsonrpc = extractStringValue(line, "jsonrpc");
    if (jsonrpc != "2.0") {
      std::cout << formatError("", -32600, "Invalid Request") << "\n";
      continue;
    }

    std::string id = extractRawValue(line, "id");
    std::string method = extractStringValue(line, "method");

    if (method == "ping") {
      std::cout << formatSuccess(id, "\"pong\"") << "\n";
      std::cout.flush();
    } else if (method == "execute") {
      std::vector<std::string> cmdArgs = extractStringArray(line, "args");
      if (cmdArgs.empty()) {
        std::cout << formatError(id, -32602, "Invalid params: args required") << "\n";
      } else {
        // Create argv
        std::vector<std::string> fullArgs;
        fullArgs.push_back("ccad");
        for (const auto& a : cmdArgs) {
          fullArgs.push_back(a);
        }

        std::vector<char*> argv;
        for (auto& a : fullArgs) {
          argv.push_back(&a[0]);
        }

        std::ostringstream capturedOut;
        std::ostringstream capturedErr;
        std::streambuf* oldCout = std::cout.rdbuf(capturedOut.rdbuf());
        std::streambuf* oldCerr = std::cerr.rdbuf(capturedErr.rdbuf());

        int exitCode = 0;
        try {
          exitCode = run(static_cast<int>(argv.size()), argv.data());
        } catch (...) {
          exitCode = 1;
        }

        std::cout.rdbuf(oldCout);
        std::cerr.rdbuf(oldCerr);

        std::ostringstream res;
        res << "{\"exit_code\": " << exitCode << ", "
            << "\"stdout\": \"" << ccad::escapeJson(capturedOut.str()) << "\", "
            << "\"stderr\": \"" << ccad::escapeJson(capturedErr.str()) << "\"}";

        std::cout << formatSuccess(id, res.str()) << "\n";
        std::cout.flush();
      }
    } else {
      std::cout << formatError(id, -32601, "Method not found") << "\n";
      std::cout.flush();
    }
  }

  return 0;
}

}  // namespace ccad_cli
