#include "ccad_cli/app.hpp"

#include "ccad_cli/lib_commands.hpp"
#include "ccad_cli/pcb_commands.hpp"
#include "ccad_cli/project_commands.hpp"
#include "ccad_core/json.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace ccad_cli {
namespace {

struct CommandHelp {
  std::string name;
  std::string summary;
  std::string usage;
};

const std::vector<CommandHelp>& commandHelp() {
  static const std::vector<CommandHelp> commands{
      CommandHelp{.name = "init",
                  .summary = "Create a CCad project file",
                  .usage = "ccad init --name <name> --out <path> [--width-mm <n> --height-mm <n>]"},
      CommandHelp{.name = "validate",
                  .summary = "Run logical ERC diagnostics and emit JSON",
                  .usage = "ccad validate <path>"},
      CommandHelp{.name = "drc",
                  .summary = "Run physical board diagnostics and emit JSON",
                  .usage = "ccad drc <path>"},
      CommandHelp{.name = "inspect",
                  .summary = "Emit human-review project summary JSON",
                  .usage = "ccad inspect <path>"},
      CommandHelp{.name = "diff",
                  .summary = "Emit machine-readable diff JSON for two project files",
                  .usage = "ccad diff <before> <after>"},
      CommandHelp{.name = "lib import-footprint",
                  .summary = "Import one KiCad .kicad_mod footprint as CCad footprint JSON",
                  .usage = "ccad lib import-footprint --in <path.kicad_mod> --out <path.json>"},
      CommandHelp{.name = "lib catalog-info",
                  .summary = "Summarize a local CCad library catalog as JSON",
                  .usage = "ccad lib catalog-info --catalog <path.ccad-library.json>"},
      CommandHelp{.name = "lib catalog-find",
                  .summary = "Find one local CCad library catalog item by stable ID",
                  .usage = "ccad lib catalog-find --catalog <path.ccad-library.json> --id <id>"},
      CommandHelp{.name = "lib catalog-search",
                  .summary = "Search local CCad library catalog items",
                  .usage = "ccad lib catalog-search --catalog <path.ccad-library.json> "
                           "--query <text> [--kind <kind>]"},
      CommandHelp{.name = "lib catalog-validate",
                  .summary = "Validate local CCad library catalog metadata",
                  .usage = "ccad lib catalog-validate --catalog <path.ccad-library.json> "
                           "[--root <native-library-root>]"},
      CommandHelp{.name = "pcb place-footprint",
                  .summary = "Place imported footprint pads onto a board",
                  .usage = "ccad pcb place-footprint --file <path> --footprint <path.json> "
                           "--component <id> --at-x-mm <n> --at-y-mm <n> --layer <id> "
                           "[--rotation-deg <n>]"},
      CommandHelp{.name = "pcb add-pad",
                  .summary = "Append one rectangular pad to a board project",
                  .usage = "ccad pcb add-pad --file <path> --id <id> --component <id> "
                           "--pin <name> --net <id> --layer <id> --x-mm <n> --y-mm <n> "
                           "--width-mm <n> --height-mm <n>"},
      CommandHelp{.name = "pcb add-via",
                  .summary = "Append one via to a board project",
                  .usage = "ccad pcb add-via --file <path> --id <id> --net <id> --x-mm <n> "
                           "--y-mm <n> --diameter-mm <n> --drill-mm <n>"},
      CommandHelp{.name = "pcb add-track",
                  .summary = "Append one straight track segment to a board project",
                  .usage = "ccad pcb add-track --file <path> --id <id> --net <id> --layer <id> "
                           "--start-x-mm <n> --start-y-mm <n> --end-x-mm <n> "
                           "--end-y-mm <n> --width-mm <n>"},
      CommandHelp{.name = "pcb add-keepout",
                  .summary = "Append one rectangular keepout to a board project",
                  .usage = "ccad pcb add-keepout --file <path> --id <id> --kind <kind> "
                           "--x-mm <n> --y-mm <n> --width-mm <n> --height-mm <n>"},
  };
  return commands;
}

void printUsage(std::ostream& out) {
  out << "Usage:\n"
      << "  ccad help [--format json]\n";
  for (const CommandHelp& command : commandHelp()) {
    out << "  " << command.usage << '\n';
  }
}

std::string helpJson() {
  std::ostringstream out;
  out << "{\n  \"commands\": [\n";
  const std::vector<CommandHelp>& commands = commandHelp();
  for (std::size_t i = 0; i < commands.size(); ++i) {
    const CommandHelp& command = commands.at(i);
    out << "    {\n"
        << "      \"name\": \"" << ccad::escapeJson(command.name) << "\",\n"
        << "      \"summary\": \"" << ccad::escapeJson(command.summary) << "\",\n"
        << "      \"usage\": \"" << ccad::escapeJson(command.usage) << "\"\n"
        << "    }" << (i + 1 == commands.size() ? "" : ",") << '\n';
  }
  out << "  ]\n}\n";
  return out.str();
}

int helpCommand(const std::vector<std::string>& args) {
  if (args.empty()) {
    printUsage(std::cout);
    return 0;
  }
  if (args.size() == 2 && args.at(0) == "--format" && args.at(1) == "json") {
    std::cout << helpJson();
    return 0;
  }
  std::cerr << "help supports only --format json\n";
  return 2;
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
  if (command == "help") {
    return helpCommand(args);
  }
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
