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

    std::cerr << "unknown sch subcommand: " << subcommand << '\n';
    return 2;
  } catch (const std::exception& error) {
    std::cerr << "failed to mutate schematic project: " << error.what() << '\n';
    return 2;
  }
}

}  // namespace ccad_cli
