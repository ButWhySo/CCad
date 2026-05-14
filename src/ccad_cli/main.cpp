#include "ccad_core/diff.hpp"
#include "ccad_core/drc.hpp"
#include "ccad_core/erc.hpp"
#include "ccad_core/json.hpp"
#include "ccad_core/kicad_footprint_import.hpp"
#include "ccad_core/model.hpp"
#include "ccad_core/review.hpp"
#include "ccad_core/serialize.hpp"
#include "ccad_core/transaction.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace {

void printUsage(std::ostream& out) {
  out << "Usage:\n"
      << "  ccad init --name <name> --out <path> [--width-mm <n> --height-mm <n>]\n"
      << "  ccad validate <path>\n"
      << "  ccad drc <path>\n"
      << "  ccad inspect <path>\n"
      << "  ccad diff <before> <after>\n"
      << "  ccad lib import-footprint --in <path.kicad_mod> --out <path.json>\n"
      << "  ccad pcb add-pad --file <path> --id <id> --component <id> --pin <name> "
         "--net <id> --layer <id> --x-mm <n> --y-mm <n> --width-mm <n> --height-mm <n>\n"
      << "  ccad pcb add-via --file <path> --id <id> --net <id> --x-mm <n> --y-mm <n> "
         "--diameter-mm <n> --drill-mm <n>\n"
      << "  ccad pcb add-track --file <path> --id <id> --net <id> --layer <id> "
         "--start-x-mm <n> --start-y-mm <n> --end-x-mm <n> --end-y-mm <n> --width-mm <n>\n";
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

std::optional<double> parsePositiveDouble(const std::string& value) {
  try {
    std::size_t parsed = 0;
    const double number = std::stod(value, &parsed);
    if (parsed != value.size() || number <= 0.0) {
      return std::nullopt;
    }
    return number;
  } catch (const std::exception&) {
    return std::nullopt;
  }
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

bool writeProjectFile(const std::string& path, const ccad::Project& project) {
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << ccad::dumpProjectJson(project);
  return static_cast<bool>(output);
}

std::map<std::string, std::string> parseOptions(const std::vector<std::string>& args,
                                                const std::size_t start,
                                                const std::vector<std::string>& allowed) {
  std::map<std::string, std::string> options;
  for (std::size_t i = start; i < args.size(); i += 2) {
    if (i + 1 >= args.size()) {
      throw std::runtime_error("missing value for option: " + args.at(i));
    }
    const std::string& key = args.at(i);
    bool known = false;
    for (const std::string& allowed_key : allowed) {
      if (key == allowed_key) {
        known = true;
        break;
      }
    }
    if (!known) {
      throw std::runtime_error("unknown option: " + key);
    }
    if (options.contains(key)) {
      throw std::runtime_error("duplicate option: " + key);
    }
    options.emplace(key, args.at(i + 1));
  }
  return options;
}

std::string requireOption(const std::map<std::string, std::string>& options,
                          const std::string& key) {
  const auto found = options.find(key);
  if (found == options.end() || found->second.empty()) {
    throw std::runtime_error("missing required option: " + key);
  }
  return found->second;
}

ccad::Length requirePositiveMillimeters(const std::map<std::string, std::string>& options,
                                        const std::string& key) {
  const std::string value = requireOption(options, key);
  const std::optional<double> parsed = parsePositiveDouble(value);
  if (!parsed.has_value()) {
    throw std::runtime_error(key + " must be a positive number");
  }
  return ccad::millimeters(*parsed);
}

bool hasLayer(const ccad::Board& board, const std::string& layer_id) {
  for (const ccad::Layer& layer : board.layers) {
    if (layer.id == layer_id) {
      return true;
    }
  }
  return false;
}

bool containsPoint(const ccad::Board& board, const ccad::Point point) {
  const ccad::Point min = board.outline.origin;
  const ccad::Point max = ccad::maxPoint(board.outline);
  return point.x.nanometers >= min.x.nanometers && point.x.nanometers <= max.x.nanometers &&
         point.y.nanometers >= min.y.nanometers && point.y.nanometers <= max.y.nanometers;
}

ccad::Board& requireBoard(ccad::Project& project) {
  if (!project.board.has_value()) {
    throw std::runtime_error("project has no board");
  }
  return *project.board;
}

void requireLayer(const ccad::Board& board, const std::string& layer_id) {
  if (!hasLayer(board, layer_id)) {
    throw std::runtime_error("unknown layer: " + layer_id);
  }
}

void requireInsideBoard(const ccad::Board& board, const ccad::Point point,
                        const std::string& label) {
  if (!containsPoint(board, point)) {
    throw std::runtime_error(label + " is outside board outline");
  }
}

void requireUniquePadId(const ccad::Board& board, const std::string& id) {
  for (const ccad::Pad& pad : board.pads) {
    if (pad.id == id) {
      throw std::runtime_error("duplicate pad id: " + id);
    }
  }
}

void requireUniqueViaId(const ccad::Board& board, const std::string& id) {
  for (const ccad::Via& via : board.vias) {
    if (via.id == id) {
      throw std::runtime_error("duplicate via id: " + id);
    }
  }
}

void requireUniqueTrackId(const ccad::Board& board, const std::string& id) {
  for (const ccad::TrackSegment& track : board.tracks) {
    if (track.id == id) {
      throw std::runtime_error("duplicate track id: " + id);
    }
  }
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

int pcbCommand(const std::vector<std::string>& args) {
  if (args.empty()) {
    std::cerr << "pcb requires a subcommand\n";
    return 2;
  }

  try {
    const std::string& subcommand = args.at(0);
    if (subcommand == "add-pad") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--component", "--pin", "--net", "--layer",
                                 "--x-mm", "--y-mm", "--width-mm", "--height-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const std::string layer_id = requireOption(options, "--layer");
      requireUniquePadId(board, id);
      requireLayer(board, layer_id);
      const ccad::Point position{
          .x = requirePositiveMillimeters(options, "--x-mm"),
          .y = requirePositiveMillimeters(options, "--y-mm"),
      };
      requireInsideBoard(board, position, "pad position");
      board.pads.push_back(ccad::Pad{
          .id = id,
          .component_id = requireOption(options, "--component"),
          .pin_name = requireOption(options, "--pin"),
          .net_id = requireOption(options, "--net"),
          .layer_id = layer_id,
          .position = position,
          .size = ccad::Size{.width = requirePositiveMillimeters(options, "--width-mm"),
                             .height = requirePositiveMillimeters(options, "--height-mm")},
      });
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "add-via") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--net", "--x-mm", "--y-mm", "--diameter-mm",
                                 "--drill-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      requireUniqueViaId(board, id);
      const ccad::Point position{
          .x = requirePositiveMillimeters(options, "--x-mm"),
          .y = requirePositiveMillimeters(options, "--y-mm"),
      };
      requireInsideBoard(board, position, "via position");
      const ccad::Length diameter = requirePositiveMillimeters(options, "--diameter-mm");
      const ccad::Length drill = requirePositiveMillimeters(options, "--drill-mm");
      if (drill.nanometers > diameter.nanometers) {
        throw std::runtime_error("via drill must be less than or equal to diameter");
      }
      board.vias.push_back(ccad::Via{
          .id = id,
          .net_id = requireOption(options, "--net"),
          .position = position,
          .diameter = diameter,
          .drill = drill,
      });
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "add-track") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--net", "--layer", "--start-x-mm",
                                 "--start-y-mm", "--end-x-mm", "--end-y-mm", "--width-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const std::string layer_id = requireOption(options, "--layer");
      requireUniqueTrackId(board, id);
      requireLayer(board, layer_id);
      const ccad::Point start{
          .x = requirePositiveMillimeters(options, "--start-x-mm"),
          .y = requirePositiveMillimeters(options, "--start-y-mm"),
      };
      const ccad::Point end{
          .x = requirePositiveMillimeters(options, "--end-x-mm"),
          .y = requirePositiveMillimeters(options, "--end-y-mm"),
      };
      requireInsideBoard(board, start, "track start");
      requireInsideBoard(board, end, "track end");
      board.tracks.push_back(ccad::TrackSegment{
          .id = id,
          .net_id = requireOption(options, "--net"),
          .layer_id = layer_id,
          .start = start,
          .end = end,
          .width = requirePositiveMillimeters(options, "--width-mm"),
      });
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    std::cerr << "unknown pcb subcommand: " << subcommand << '\n';
    return 2;
  } catch (const std::exception& error) {
    std::cerr << "failed to mutate pcb project: " << error.what() << '\n';
    return 2;
  }
}

int libCommand(const std::vector<std::string>& args) {
  if (args.empty()) {
    std::cerr << "lib requires a subcommand\n";
    return 2;
  }

  try {
    const std::string& subcommand = args.at(0);
    if (subcommand == "import-footprint") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--in", "--out"});
      const std::string in_path = requireOption(options, "--in");
      const std::string out_path = requireOption(options, "--out");

      std::ifstream input(in_path);
      if (!input) {
        std::cerr << "failed to open footprint input file: " << in_path << '\n';
        return 2;
      }
      std::ostringstream buffer;
      buffer << input.rdbuf();
      const ccad::Footprint footprint = ccad::importKiCadFootprint(buffer.str());

      std::ofstream output(out_path);
      if (!output) {
        std::cerr << "failed to open footprint output file: " << out_path << '\n';
        return 2;
      }
      output << ccad::dumpFootprintJson(footprint);
      return static_cast<bool>(output) ? 0 : 2;
    }

    std::cerr << "unknown lib subcommand: " << subcommand << '\n';
    return 2;
  } catch (const std::exception& error) {
    std::cerr << "failed to import library data: " << error.what() << '\n';
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
  if (command == "drc") {
    return drcCommand(args);
  }
  if (command == "inspect") {
    return inspectCommand(args);
  }
  if (command == "diff") {
    return diffCommand(args);
  }
  if (command == "pcb") {
    return pcbCommand(args);
  }
  if (command == "lib") {
    return libCommand(args);
  }

  std::cerr << "unknown command: " << command << '\n';
  printUsage(std::cerr);
  return 2;
}
