#include "ccad_cli/app.hpp"

#include "ccad_cli/lib_commands.hpp"
#include "ccad_cli/pcb_commands.hpp"
#include "ccad_cli/project_commands.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace ccad_cli {
namespace {

void printUsage(std::ostream& out) {
  out << "Usage:\n"
      << "  ccad init --name <name> --out <path> [--width-mm <n> --height-mm <n>]\n"
      << "  ccad validate <path>\n"
      << "  ccad drc <path>\n"
      << "  ccad inspect <path>\n"
      << "  ccad diff <before> <after>\n"
      << "  ccad lib import-footprint --in <path.kicad_mod> --out <path.json>\n"
      << "  ccad pcb place-footprint --file <path> --footprint <path.json> --component <id> "
         "--at-x-mm <n> --at-y-mm <n> --layer <id> [--rotation-deg <n>]\n"
      << "  ccad pcb add-pad --file <path> --id <id> --component <id> --pin <name> "
         "--net <id> --layer <id> --x-mm <n> --y-mm <n> --width-mm <n> --height-mm <n>\n"
      << "  ccad pcb add-via --file <path> --id <id> --net <id> --x-mm <n> --y-mm <n> "
         "--diameter-mm <n> --drill-mm <n>\n"
      << "  ccad pcb add-track --file <path> --id <id> --net <id> --layer <id> "
         "--start-x-mm <n> --start-y-mm <n> --end-x-mm <n> --end-y-mm <n> --width-mm <n>\n";
}

}  // namespace

int run(int argc, char** argv) {
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

}  // namespace ccad_cli
