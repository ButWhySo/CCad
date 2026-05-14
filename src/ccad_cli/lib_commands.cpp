#include "ccad_cli/lib_commands.hpp"

#include "ccad_cli/common.hpp"
#include "ccad_core/kicad_footprint_import.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

namespace ccad_cli {

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

}  // namespace ccad_cli
