#include "ccad_cli/sch_commands.hpp"

#include "ccad_cli/common.hpp"
#include "ccad_core/kicad_symbol_import.hpp"
#include "ccad_core/placement.hpp"

#include <iostream>
#include <map>
#include <stdexcept>

namespace ccad_cli {

int schCommand(const std::vector<std::string>& args) {
  if (args.empty()) {
    std::cerr << "sch requires a subcommand\n";
    return 2;
  }

  try {
    const std::string& subcommand = args.at(0);
    if (subcommand == "place-symbol") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--symbol", "--component", "--at-x-mm", "--at-y-mm",
                                 "--rotation-deg"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      const ccad::Symbol symbol =
          ccad::loadSymbolJsonFileWithLocalInheritance(requireOption(options, "--symbol"));
      const ccad::Point origin{
          .x = ccad::millimeters(requireDoubleOption(options, "--at-x-mm")),
          .y = ccad::millimeters(requireDoubleOption(options, "--at-y-mm")),
      };
      const double placement_rotation = optionDoubleOrDefault(options, "--rotation-deg", 0.0);

      ccad::placeComponent(project, symbol, requireOption(options, "--component"), origin,
                           placement_rotation);

      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "add-wire") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--start-x-mm", "--start-y-mm", "--end-x-mm", "--end-y-mm", "--net"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::WireSegment wire{
          .id = requireOption(options, "--id"),
          .start = {ccad::millimeters(requireDoubleOption(options, "--start-x-mm")),
                    ccad::millimeters(requireDoubleOption(options, "--start-y-mm"))},
          .end = {ccad::millimeters(requireDoubleOption(options, "--end-x-mm")),
                  ccad::millimeters(requireDoubleOption(options, "--end-y-mm"))},
          .net_id = options.contains("--net") ? options.at("--net") : ""
      };
      ccad::ensurePrimarySchematic(project).wires.push_back(wire);
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file\n";
        return 2;
      }
      return 0;
    }

    if (subcommand == "add-bus") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--start-x-mm", "--start-y-mm", "--end-x-mm", "--end-y-mm", "--bus"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::BusSegment bus{
          .id = requireOption(options, "--id"),
          .start = {ccad::millimeters(requireDoubleOption(options, "--start-x-mm")),
                    ccad::millimeters(requireDoubleOption(options, "--start-y-mm"))},
          .end = {ccad::millimeters(requireDoubleOption(options, "--end-x-mm")),
                  ccad::millimeters(requireDoubleOption(options, "--end-y-mm"))},
          .bus_id = options.contains("--bus") ? options.at("--bus") : ""
      };
      ccad::ensurePrimarySchematic(project).buses.push_back(bus);
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file\n";
        return 2;
      }
      return 0;
    }

    if (subcommand == "add-label") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--text", "--net", "--at-x-mm", "--at-y-mm", "--rotation-deg", "--global"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Label label{
          .id = requireOption(options, "--id"),
          .text = requireOption(options, "--text"),
          .net_id = options.contains("--net") ? options.at("--net") : "",
          .position = {ccad::millimeters(requireDoubleOption(options, "--at-x-mm")),
                       ccad::millimeters(requireDoubleOption(options, "--at-y-mm"))},
          .rotation_degrees = optionDoubleOrDefault(options, "--rotation-deg", 0.0),
          .global = options.count("--global") > 0
      };
      ccad::ensurePrimarySchematic(project).labels.push_back(label);
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file\n";
        return 2;
      }
      return 0;
    }

    if (subcommand == "add-power") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--value", "--net", "--at-x-mm", "--at-y-mm", "--rotation-deg"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::PowerSymbol power{
          .id = requireOption(options, "--id"),
          .value = requireOption(options, "--value"),
          .net_id = options.contains("--net") ? options.at("--net") : "",
          .position = {ccad::millimeters(requireDoubleOption(options, "--at-x-mm")),
                       ccad::millimeters(requireDoubleOption(options, "--at-y-mm"))},
          .rotation_degrees = optionDoubleOrDefault(options, "--rotation-deg", 0.0)
      };
      ccad::ensurePrimarySchematic(project).power_symbols.push_back(power);
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file\n";
        return 2;
      }
      return 0;
    }

    std::cerr << "unknown sch subcommand: " << subcommand << '\n';
    return 2;
  } catch (const std::exception& error) {
    std::cerr << "failed to mutate schematic project: " << error.what() << '\n';
    return 2;
  }
}

}  // namespace ccad_cli
