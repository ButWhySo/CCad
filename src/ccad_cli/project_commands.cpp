#include "ccad_cli/project_commands.hpp"

#include "ccad_cli/common.hpp"
#include "ccad_core/bom_export.hpp"
#include "ccad_core/pnp_export.hpp"
#include "ccad_core/drill_export.hpp"
#include "ccad_core/diff.hpp"
#include "ccad_core/drc.hpp"
#include "ccad_core/erc.hpp"
#include "ccad_core/review.hpp"
#include "ccad_core/serialize.hpp"
#include "ccad_core/transaction.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>

namespace ccad_cli {

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

  std::ofstream out_file(out);
  if (!out_file) {
    std::cerr << "Failed to open output file for writing: " << out << "\n";
    return 2;
  }
  out_file << bom;
  return 0;
}

int exportPnpCommand(const std::vector<std::string>& args) {
  auto opts = parseOptions(args, 1, {"--file", "--output"});
  auto file = requireOption(opts, "--file");
  auto out = requireOption(opts, "--output");

  ccad::Project project = loadProjectFile(file);
  std::string pnp = ccad::exportToPnpCsv(project);

  std::ofstream out_file(out);
  if (!out_file) {
    std::cerr << "Failed to open output file for writing: " << out << "\n";
    return 2;
  }
  out_file << pnp;
  return 0;
}

int exportDrillCommand(const std::vector<std::string>& args) {
  auto opts = parseOptions(args, 1, {"--file", "--output"});
  auto file = requireOption(opts, "--file");
  auto out = requireOption(opts, "--output");

  ccad::Project project = loadProjectFile(file);
  std::string drill = ccad::exportToDrillExcellon(project);

  std::ofstream out_file(out);
  if (!out_file) {
    std::cerr << "Failed to open output file for writing: " << out << "\n";
    return 2;
  }
  out_file << drill;
  return 0;
}

}  // namespace ccad_cli
