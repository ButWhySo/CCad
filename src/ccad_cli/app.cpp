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
      CommandHelp{.name = "lib export-footprint",
                  .summary = "Export CCad footprint JSON to a KiCad .kicad_mod footprint",
                  .usage = "ccad lib export-footprint --in <path.json> --out <path.kicad_mod>"},
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
      CommandHelp{.name = "pcb add-layer",
                  .summary = "Append one board layer to a board project",
                  .usage = "ccad pcb add-layer --file <path> --id <id> --name <name> "
                           "--kind <kind> [--visible true|false]"},
      CommandHelp{.name = "pcb add-standard-layers",
                  .summary = "Append missing KiCad standard PCB layers to a board project",
                  .usage = "ccad pcb add-standard-layers --file <path>"},
      CommandHelp{.name = "pcb set-layer",
                  .summary = "Update one board layer's metadata",
                  .usage = "ccad pcb set-layer --file <path> --id <id> --name <name> "
                           "--kind <kind> --visible true|false"},
      CommandHelp{.name = "pcb get-object",
                  .summary = "Emit one board layer or physical object by stable ID as JSON",
                  .usage = "ccad pcb get-object --file <path> --id <id>"},
      CommandHelp{.name = "pcb list-objects",
                  .summary = "List board layer and physical object IDs as compact JSON",
                  .usage = "ccad pcb list-objects --file <path> [--type <type>]"},
      CommandHelp{.name = "pcb list-nets",
                  .summary = "List physical board net usage counts as compact JSON",
                  .usage = "ccad pcb list-nets --file <path>"},
      CommandHelp{.name = "pcb list-route-requests",
                  .summary = "List board route-request intent records as compact JSON",
                  .usage = "ccad pcb list-route-requests --file <path>"},
      CommandHelp{.name = "pcb route-status",
                  .summary = "Report open, partial, and completed route-assistance work",
                  .usage = "ccad pcb route-status --file <path>"},
      CommandHelp{.name = "pcb export-route-job",
                  .summary = "Export a compact route-assistance job as JSON",
                  .usage = "ccad pcb export-route-job --file <path> [--request-id <id>]"},
      CommandHelp{.name = "pcb remove-layer",
                  .summary = "Remove one unused board layer by stable ID",
                  .usage = "ccad pcb remove-layer --file <path> --id <id>"},
      CommandHelp{.name = "pcb set-layer-visibility",
                  .summary = "Set one board layer visibility flag",
                  .usage = "ccad pcb set-layer-visibility --file <path> --id <id> "
                           "--visible true|false"},
      CommandHelp{.name = "pcb set-rules",
                  .summary = "Set board-level physical DRC rule defaults",
                  .usage = "ccad pcb set-rules --file <path> --copper-clearance-mm <n> "
                           "--min-track-width-mm <n> --min-via-annular-ring-mm <n>"},
      CommandHelp{.name = "pcb set-outline",
                  .summary = "Set the rectangular board outline",
                  .usage = "ccad pcb set-outline --file <path> --x-mm <n> --y-mm <n> "
                           "--width-mm <n> --height-mm <n>"},
      CommandHelp{.name = "pcb add-pad",
                  .summary = "Append one rectangular pad to a board project",
                  .usage = "ccad pcb add-pad --file <path> --id <id> --component <id> "
                           "--pin <name> --net <id> --layer <id> --x-mm <n> --y-mm <n> "
                           "--width-mm <n> --height-mm <n>"},
      CommandHelp{.name = "pcb set-pad",
                  .summary = "Update one existing pad metadata and rotation",
                  .usage = "ccad pcb set-pad --file <path> --id <id> --component <id> "
                           "--pin <name> --net <id> --layer <id> --rotation-deg <n>"},
      CommandHelp{.name = "pcb add-via",
                  .summary = "Append one via to a board project",
                  .usage = "ccad pcb add-via --file <path> --id <id> --net <id> --x-mm <n> "
                           "--y-mm <n> --diameter-mm <n> --drill-mm <n>"},
      CommandHelp{.name = "pcb set-via",
                  .summary = "Update one existing via geometry",
                  .usage = "ccad pcb set-via --file <path> --id <id> --diameter-mm <n> "
                           "--drill-mm <n> [--net <id>]"},
      CommandHelp{.name = "pcb add-track",
                  .summary = "Append one straight track segment to a board project",
                  .usage = "ccad pcb add-track --file <path> --id <id> --net <id> --layer <id> "
                           "--start-x-mm <n> --start-y-mm <n> --end-x-mm <n> "
                           "--end-y-mm <n> --width-mm <n>"},
      CommandHelp{.name = "pcb add-route-request",
                  .summary = "Append one route-assistance request to a board project",
                  .usage = "ccad pcb add-route-request --file <path> --id <id> --net <id> "
                           "--from <object-id> --to <object-id> --preferred-layer <id> "
                           "--policy <name> --width-mm <n>"},
      CommandHelp{.name = "pcb set-route-request",
                  .summary = "Update one existing route-assistance request",
                  .usage = "ccad pcb set-route-request --file <path> --id <id> --net <id> "
                           "--from <object-id> --to <object-id> --preferred-layer <id> "
                           "--policy <name> --width-mm <n>"},
      CommandHelp{.name = "pcb remove-route-request",
                  .summary = "Remove one route-assistance request by stable ID",
                  .usage = "ccad pcb remove-route-request --file <path> --id <id>"},
      CommandHelp{.name = "pcb apply-route-segment",
                  .summary = "Apply one routed track segment for a route request",
                  .usage = "ccad pcb apply-route-segment --file <path> --request-id <id> "
                           "--track-id <id> [--layer <id>] --start-x-mm <n> --start-y-mm <n> "
                           "--end-x-mm <n> --end-y-mm <n> [--complete true|false]"},
      CommandHelp{.name = "pcb apply-route-polyline",
                  .summary = "Apply a routed polyline as multiple route-result track segments",
                  .usage = "ccad pcb apply-route-polyline --file <path> --request-id <id> "
                           "--track-prefix <id> [--layer <id>] --points-mm "
                           "<x,y;x,y;...> [--complete true|false]"},
      CommandHelp{.name = "pcb set-track",
                  .summary = "Update one existing track segment geometry",
                  .usage = "ccad pcb set-track --file <path> --id <id> --start-x-mm <n> "
                           "--start-y-mm <n> --end-x-mm <n> --end-y-mm <n> --width-mm <n> "
                           "[--net <id>] [--layer <id>]"},
      CommandHelp{.name = "pcb add-keepout",
                  .summary = "Append one rectangular keepout to a board project",
                  .usage = "ccad pcb add-keepout --file <path> --id <id> --kind <kind> "
                           "--x-mm <n> --y-mm <n> --width-mm <n> --height-mm <n>"},
      CommandHelp{.name = "pcb add-placement-region",
                  .summary = "Append one rectangular placement region to a board project",
                  .usage = "ccad pcb add-placement-region --file <path> --id <id> "
                           "--kind <kind> --x-mm <n> --y-mm <n> --width-mm <n> "
                           "--height-mm <n>"},
      CommandHelp{.name = "pcb set-region-kind",
                  .summary = "Update one keepout or placement region kind",
                  .usage = "ccad pcb set-region-kind --file <path> --id <id> --kind <kind>"},
      CommandHelp{.name = "pcb remove-object",
                  .summary = "Remove one physical board object by stable ID",
                  .usage = "ccad pcb remove-object --file <path> --id <id>"},
      CommandHelp{.name = "pcb export-kicad",
                  .summary = "Export the board project to a KiCad S-expression (.kicad_pcb) file",
                  .usage = "ccad pcb export-kicad --file <path> --output <path>"},
      CommandHelp{.name = "pcb export-dsn",
                  .summary = "Export the board project to a Specctra DSN file",
                  .usage = "ccad pcb export-dsn --file <path> --output <path>"},
      CommandHelp{.name = "pcb move-object",
                  .summary = "Move one physical board object by stable ID",
                  .usage = "ccad pcb move-object --file <path> --id <id> --x-mm <n> "
                           "--y-mm <n>"},
      CommandHelp{.name = "pcb resize-object",
                  .summary = "Resize one physical board object by stable ID",
                  .usage = "ccad pcb resize-object --file <path> --id <id> --width-mm <n> "
                           "--height-mm <n>"},
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
