#include "ccad_cli/project_commands.hpp"

#include "ccad_cli/common.hpp"
#include "ccad_core/board_loader.hpp"
#include "ccad_core/bom_export.hpp"
#include "ccad_core/board_loader.hpp"
#include "ccad_core/diff.hpp"
#include "ccad_core/drc.hpp"
#include "ccad_core/erc.hpp"
#include "ccad_core/json.hpp"
#include "ccad_core/review.hpp"
#include "ccad_core/serialize.hpp"
#include "ccad_core/transaction.hpp"
#include "ccad_core/filesystem_u8.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>

namespace ccad_cli {

int initCommand(const std::vector<std::string>& args) {
  std::string name;
  std::string out_path;
  std::optional<double> width_mm;
  std::optional<double> height_mm;

  for (std::size_t i = 0; i < args.size(); ++i) {
    if (args.at(i) == "--name" && i + 1 < args.size()) {
      name = args.at(++i);
    } else if (args.at(i) == "--out" && i + 1 < args.size()) {
      out_path = args.at(++i);
    } else if (args.at(i) == "--width-mm" && i + 1 < args.size()) {
      width_mm = parsePositiveDouble(args.at(++i));
      if (!width_mm.has_value()) {
        std::cerr << "--width-mm must be a positive number\n";
        return 2;
      }
    } else if (args.at(i) == "--height-mm" && i + 1 < args.size()) {
      height_mm = parsePositiveDouble(args.at(++i));
      if (!height_mm.has_value()) {
        std::cerr << "--height-mm must be a positive number\n";
        return 2;
      }
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
    project.boards.push_back(ccad::Board{
        .outline = ccad::Rect{
            .origin = ccad::Point{.x = ccad::nanometers(0), .y = ccad::nanometers(0)},
            .size = ccad::Size{.width = ccad::millimeters(*width_mm),
                                .height = ccad::millimeters(*height_mm)},
        },
        .design_rules = ccad::DesignRules{},
        .layers = {ccad::Layer{.id = "F.Cu", .name = "Front copper", .kind = "copper", .visible = true},
                   ccad::Layer{.id = "B.Cu", .name = "Back copper", .kind = "copper", .visible = true}},
        .footprints = {},
        .placement_regions = {},
        .keepouts = {},
        .pads = {},
        .vias = {},
        .tracks = {},
        .track_arcs = {},
        .graphics = {},
        .texts = {},
        .dimensions = {},
        .groups = {},
        .barcodes = {},
        .reference_images = {},
        .tables = {},
        .targets = {},
        .zones = {},
        .route_requests = {},
        .teardrops = {},
    });
  }

  std::ofstream output(ccad::u8ToPath(out_path));
  if (!output) {
    std::cerr << "failed to open output file: " << out_path << '\n';
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
    ccad::HeadlessBoardContext context;
    context.loadFile(args.at(0));
    const std::vector<ccad::Diagnostic> diagnostics = ccad::runErc(context.project());
    std::cout << diagnosticsJson(diagnostics);
    return hasError(diagnostics) ? 1 : 0;
  } catch (const std::exception& error) {
    std::cerr << "failed to parse project file: " << error.what() << '\n';
    return 2;
  }
}

int drcCommand(const std::vector<std::string>& args) {
  if (args.size() != 1) {
    std::cerr << "drc requires exactly one project path\n";
    return 2;
  }

  try {
    const ccad::Project project = loadProjectFile(args.at(0));
    const std::vector<ccad::Diagnostic> diagnostics = ccad::runDrc(project);
    std::cout << diagnosticsJson(diagnostics);
    return hasError(diagnostics) ? 1 : 0;
  } catch (const std::exception& error) {
    std::cerr << "failed to run drc: " << error.what() << '\n';
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

int exportBomCommand(const std::vector<std::string>& args) {
  auto opts = parseOptions(args, 1, {"--file", "--output"});
  auto file = requireOption(opts, "--file");
  auto out = requireOption(opts, "--output");

  ccad::Project project = loadProjectFile(file);
  std::string bom = ccad::exportToBomCsv(project);

  std::ofstream out_file(ccad::u8ToPath(out));
  if (!out_file) {
    std::cerr << "Failed to open output file for writing: " << out << "\n";
    return 2;
  }
  out_file << bom;
  return 0;
}

std::string textVariablesJson(const ccad::Project& project) {
  std::ostringstream out;
  out << "{\n"
      << "  \"kicad_handler\": \"GetTextVariables\",\n"
      << "  \"kicad_project_class\": \"PROJECT\",\n"
      << "  \"parity_scope\": \"project_text_variables_first_slice\",\n"
      << "  \"variables\": {\n";
  std::size_t index = 0;
  for (const auto& [key, value] : project.text_variables) {
    out << "    \"" << ccad::escapeJson(key) << "\": \""
        << ccad::escapeJson(value) << "\""
        << (++index == project.text_variables.size() ? "" : ",") << '\n';
  }
  out << "  }\n"
      << "}\n";
  return out.str();
}

int setTextVariableCommand(const std::vector<std::string>& args) {
  const auto options = parseOptions(args, 1, {"--file", "--key", "--value"});
  const std::string file = requireOption(options, "--file");
  const std::string key = requireOption(options, "--key");
  const std::string value = requireOption(options, "--value");

  if (key.empty()) {
    throw std::runtime_error("--key must not be empty");
  }

  ccad::HeadlessBoardContext context;
  context.loadFile(file);
  ccad::Project& project = context.project();
  project.text_variables[key] = value;
  context.markDirty();
  if (!writeProjectFile(file, project)) {
    throw std::runtime_error("failed to write project file: " + file);
  }
  return 0;
}

int listTextVariablesCommand(const std::vector<std::string>& args) {
  const auto options = parseOptions(args, 1, {"--file"});
  const std::string file = requireOption(options, "--file");
  const ccad::Project project = loadProjectFile(file);
  std::cout << textVariablesJson(project);
  return 0;
}

int projectCommand(const std::vector<std::string>& args) {
  if (args.empty()) {
    std::cerr << "missing project subcommand\n";
    return 2;
  }
  const std::string subcommand = args.at(0);
  if (subcommand == "export-bom") {
    return exportBomCommand(args);
  }
  if (subcommand == "set-text-variable") {
    return setTextVariableCommand(args);
  }
  if (subcommand == "list-text-variables") {
    return listTextVariablesCommand(args);
  }
  std::cerr << "unknown project subcommand: " << subcommand << '\n';
  return 2;
}

}  // namespace ccad_cli
