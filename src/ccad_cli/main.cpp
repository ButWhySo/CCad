#include "ccad_core/diff.hpp"
#include "ccad_core/erc.hpp"
#include "ccad_core/json.hpp"
#include "ccad_core/model.hpp"
#include "ccad_core/review.hpp"
#include "ccad_core/serialize.hpp"
#include "ccad_core/transaction.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace {

void printUsage(std::ostream& out) {
  out << "Usage:\n"
      << "  ccad init --name <name> --out <path>\n"
      << "  ccad validate <path>\n"
      << "  ccad inspect <path>\n"
      << "  ccad diff <before> <after>\n";
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
        << "      \"code\": \"" << ccad::escapeJson(diagnostic.code) << "\",\n"
        << "      \"message\": \"" << ccad::escapeJson(diagnostic.message) << "\",\n"
        << "      \"object_id\": \"" << ccad::escapeJson(diagnostic.object_id) << "\",\n"
        << "      \"severity\": \"" << ccad::escapeJson(diagnostic.severity) << "\"\n"
        << "    }" << (i + 1 == diagnostics.size() ? "" : ",") << '\n';
  }
  out << "  ]\n}\n";
  return out.str();
}

ccad::Project loadProjectFile(const std::string& path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("failed to open project file: " + path);
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  return ccad::loadProjectJson(buffer.str());
}

std::string reviewJson(const ccad::ProjectReview& review) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"project\": {\n";
  out << "    \"id\": \"" << ccad::escapeJson(review.project_id) << "\",\n";
  out << "    \"name\": \"" << ccad::escapeJson(review.project_name) << "\"\n";
  out << "  },\n";
  out << "  \"counts\": {\n";
  out << "    \"components\": " << review.component_count << ",\n";
  out << "    \"constraints\": " << review.constraint_count << ",\n";
  out << "    \"layers\": " << review.layer_count << ",\n";
  out << "    \"nets\": " << review.net_count << "\n";
  out << "  },\n";
  out << "  \"board\": {\n";
  out << "    \"has_board\": " << (review.has_board ? "true" : "false") << ",\n";
  out << "    \"width_nm\": " << review.board_width_nm << ",\n";
  out << "    \"height_nm\": " << review.board_height_nm << "\n";
  out << "  },\n";
  out << "  \"status\": \"" << ccad::escapeJson(review.status) << "\",\n";
  out << "  \"diagnostics\": [\n";
  for (std::size_t i = 0; i < review.diagnostics.size(); ++i) {
    const ccad::Diagnostic& diagnostic = review.diagnostics.at(i);
    out << "    {\n"
        << "      \"code\": \"" << ccad::escapeJson(diagnostic.code) << "\",\n"
        << "      \"message\": \"" << ccad::escapeJson(diagnostic.message) << "\",\n"
        << "      \"object_id\": \"" << ccad::escapeJson(diagnostic.object_id) << "\",\n"
        << "      \"severity\": \"" << ccad::escapeJson(diagnostic.severity) << "\"\n"
        << "    }" << (i + 1 == review.diagnostics.size() ? "" : ",") << '\n';
  }
  out << "  ]\n";
  out << "}\n";
  return out.str();
}

int initCommand(const std::vector<std::string>& args) {
  std::string name;
  std::filesystem::path out_path;
  std::optional<double> width_mm;
  std::optional<double> height_mm;

  for (std::size_t i = 0; i < args.size(); ++i) {
    if (args.at(i) == "--name" && i + 1 < args.size()) {
      name = args.at(++i);
    } else if (args.at(i) == "--out" && i + 1 < args.size()) {
      out_path = args.at(++i);
    } else if (args.at(i) == "--width-mm" && i + 1 < args.size()) {
      width_mm = std::stod(args.at(++i));
    } else if (args.at(i) == "--height-mm" && i + 1 < args.size()) {
      height_mm = std::stod(args.at(++i));
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
  if (width_mm.has_value() != height_mm.has_value()) {
    std::cerr << "init requires both --width-mm and --height-mm when creating a board\n";
    return 2;
  }
  if (width_mm.has_value() && height_mm.has_value()) {
    project.board = ccad::Board{
        .outline = ccad::Rect{
            .origin = ccad::Point{.x = ccad::nanometers(0), .y = ccad::nanometers(0)},
            .size = ccad::Size{.width = ccad::millimeters(*width_mm),
                                .height = ccad::millimeters(*height_mm)},
        },
        .layers = {ccad::Layer{.id = "F.Cu", .name = "Front copper", .kind = "copper", .visible = true},
                   ccad::Layer{.id = "B.Cu", .name = "Back copper", .kind = "copper", .visible = true}},
    };
  }

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

  try {
    const ccad::Project project = loadProjectFile(args.at(0));
    const std::vector<ccad::Diagnostic> diagnostics = ccad::runErc(project);
    std::cout << diagnosticsJson(diagnostics);
    return hasError(diagnostics) ? 1 : 0;
  } catch (const std::exception& error) {
    std::cerr << "failed to parse project file: " << error.what() << '\n';
    return 2;
  }
}

int inspectCommand(const std::vector<std::string>& args) {
  if (args.size() != 1) {
    std::cerr << "inspect requires exactly one project path\n";
    return 2;
  }

  try {
    const ccad::Project project = loadProjectFile(args.at(0));
    std::cout << reviewJson(ccad::buildReview(project));
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "failed to inspect project file: " << error.what() << '\n';
    return 2;
  }
}

int diffCommand(const std::vector<std::string>& args) {
  if (args.size() != 2) {
    std::cerr << "diff requires before and after project paths\n";
    return 2;
  }

  try {
    const ccad::Project before = loadProjectFile(args.at(0));
    const ccad::Project after = loadProjectFile(args.at(1));
    std::cout << ccad::dumpProjectDiffJson(ccad::diffProjects(before, after));
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "failed to diff project files: " << error.what() << '\n';
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
  if (command == "inspect") {
    return inspectCommand(args);
  }
  if (command == "diff") {
    return diffCommand(args);
  }

  std::cerr << "unknown command: " << command << '\n';
  printUsage(std::cerr);
  return 2;
}
