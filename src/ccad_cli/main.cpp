#include "ccad_core/erc.hpp"
#include "ccad_core/model.hpp"
#include "ccad_core/serialize.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string escapeJson(const std::string& value) {
  std::string out;
  for (const char ch : value) {
    if (ch == '"' || ch == '\\') {
      out.push_back('\\');
    }
    out.push_back(ch);
  }
  return out;
}

void printUsage(std::ostream& out) {
  out << "Usage:\n"
      << "  ccad init --name <name> --out <path>\n"
      << "  ccad validate <path>\n";
}

bool hasError(const std::vector<ccad::Diagnostic>& diagnostics) {
  for (const ccad::Diagnostic& diagnostic : diagnostics) {
    if (diagnostic.severity == "error") {
      return true;
    }
  }
  return false;
}

std::string diagnosticsJson(const std::vector<ccad::Diagnostic>& diagnostics) {
  std::ostringstream out;
  out << "{\n  \"diagnostics\": [\n";
  for (std::size_t i = 0; i < diagnostics.size(); ++i) {
    const ccad::Diagnostic& diagnostic = diagnostics.at(i);
    out << "    {\n"
        << "      \"code\": \"" << escapeJson(diagnostic.code) << "\",\n"
        << "      \"message\": \"" << escapeJson(diagnostic.message) << "\",\n"
        << "      \"object_id\": \"" << escapeJson(diagnostic.object_id) << "\",\n"
        << "      \"severity\": \"" << escapeJson(diagnostic.severity) << "\"\n"
        << "    }" << (i + 1 == diagnostics.size() ? "" : ",") << '\n';
  }
  out << "  ]\n}\n";
  return out.str();
}

int initCommand(const std::vector<std::string>& args) {
  std::string name;
  std::filesystem::path out_path;

  for (std::size_t i = 0; i < args.size(); ++i) {
    if (args.at(i) == "--name" && i + 1 < args.size()) {
      name = args.at(++i);
    } else if (args.at(i) == "--out" && i + 1 < args.size()) {
      out_path = args.at(++i);
    } else {
      std::cerr << "unknown or incomplete init argument: " << args.at(i) << '\n';
      return 2;
    }
  }

  if (name.empty() || out_path.empty()) {
    std::cerr << "init requires --name and --out\n";
    return 2;
  }

  ccad::Project project;
  project.id = "proj-" + name;
  project.name = name;

  std::ofstream output(out_path);
  if (!output) {
    std::cerr << "failed to open output file: " << out_path.string() << '\n';
    return 2;
  }
  output << ccad::dumpProjectJson(project);
  return 0;
}

int validateCommand(const std::vector<std::string>& args) {
  if (args.size() != 1) {
    std::cerr << "validate requires exactly one project path\n";
    return 2;
  }

  std::ifstream input(args.at(0));
  if (!input) {
    std::cerr << "failed to open project file: " << args.at(0) << '\n';
    return 2;
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();

  try {
    const ccad::Project project = ccad::loadProjectJson(buffer.str());
    const std::vector<ccad::Diagnostic> diagnostics = ccad::runErc(project);
    std::cout << diagnosticsJson(diagnostics);
    return hasError(diagnostics) ? 1 : 0;
  } catch (const std::exception& error) {
    std::cerr << "failed to parse project file: " << error.what() << '\n';
    return 2;
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    printUsage(std::cerr);
    return 2;
  }

  std::vector<std::string> args;
  for (int i = 2; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }

  const std::string command = argv[1];
  if (command == "init") {
    return initCommand(args);
  }
  if (command == "validate") {
    return validateCommand(args);
  }

  std::cerr << "unknown command: " << command << '\n';
  printUsage(std::cerr);
  return 2;
}
