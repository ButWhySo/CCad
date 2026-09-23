#include "ccad_cli/sch_commands.hpp"

#include "ccad_cli/common.hpp"
#include "ccad_core/kicad_symbol_import.hpp"
#include "ccad_core/placement.hpp"
#include "ccad_core/annotate.hpp"
#include "ccad_core/autoplace_fields.hpp"
#include "ccad_core/junction_helpers.hpp"
#include "ccad_core/board_loader.hpp"
#include "ccad_core/json.hpp"
#include "ccad_core/schematic_collector.hpp"

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
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      const ccad::Symbol symbol =
          ccad::loadSymbolJsonFileWithLocalInheritance(requireOption(options, "--symbol"));
      const ccad::Point origin{
          .x = ccad::millimeters(requireDoubleOption(options, "--at-x-mm")),
          .y = ccad::millimeters(requireDoubleOption(options, "--at-y-mm")),
      };
      const double placement_rotation = optionDoubleOrDefault(options, "--rotation-deg", 0.0);

      ccad::placeComponent(project, symbol, requireOption(options, "--component"), origin,
                           placement_rotation);
      context.markDirty();

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
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::SchWire wire{
          .id = requireOption(options, "--id"),
          .start = {ccad::millimeters(requireDoubleOption(options, "--start-x-mm")),
                    ccad::millimeters(requireDoubleOption(options, "--start-y-mm"))},
          .end = {ccad::millimeters(requireDoubleOption(options, "--end-x-mm")),
                  ccad::millimeters(requireDoubleOption(options, "--end-y-mm"))},
          .net_id = options.contains("--net") ? options.at("--net") : ""
      };
      ccad::ensurePrimarySchematic(project).wires.push_back(wire);
      context.markDirty();
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
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::SchBus bus{
          .id = requireOption(options, "--id"),
          .start = {ccad::millimeters(requireDoubleOption(options, "--start-x-mm")),
                    ccad::millimeters(requireDoubleOption(options, "--start-y-mm"))},
          .end = {ccad::millimeters(requireDoubleOption(options, "--end-x-mm")),
                  ccad::millimeters(requireDoubleOption(options, "--end-y-mm"))},
          .bus_id = options.contains("--bus") ? options.at("--bus") : "",
          .net_ids = {},
      };
      ccad::ensurePrimarySchematic(project).buses.push_back(bus);
      context.markDirty();
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
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::SchLabel label{
          .id = requireOption(options, "--id"),
          .text = requireOption(options, "--text"),
          .net_id = options.contains("--net") ? options.at("--net") : "",
          .position = {ccad::millimeters(requireDoubleOption(options, "--at-x-mm")),
                       ccad::millimeters(requireDoubleOption(options, "--at-y-mm"))},
          .rotation_degrees = optionDoubleOrDefault(options, "--rotation-deg", 0.0),
          .type = options.count("--global") > 0 ? ccad::LabelType::Global : ccad::LabelType::Local
      };
      ccad::ensurePrimarySchematic(project).labels.push_back(label);
      context.markDirty();
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
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::SchPowerSymbol power{
          .id = requireOption(options, "--id"),
          .value = requireOption(options, "--value"),
          .net_id = options.contains("--net") ? options.at("--net") : "",
          .position = {ccad::millimeters(requireDoubleOption(options, "--at-x-mm")),
                       ccad::millimeters(requireDoubleOption(options, "--at-y-mm"))},
          .rotation_degrees = optionDoubleOrDefault(options, "--rotation-deg", 0.0)
      };
      ccad::ensurePrimarySchematic(project).power_symbols.push_back(power);
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file\n";
        return 2;
      }
      return 0;
    }

    if (subcommand == "annotate") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--algo", "--order", "--start"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      
      ccad::AnnotateOptions annotate_opts;
      if (options.count("--algo") && options.at("--algo") == "reset") {
        annotate_opts.algo = ccad::AnnotateAlgo::ResetAll;
      }
      if (options.count("--order") && options.at("--order") == "y") {
        annotate_opts.order = ccad::AnnotateOrder::SortY;
      }
      if (options.count("--start")) {
        annotate_opts.start_number = std::stoi(options.at("--start"));
      }

      ccad::annotateProject(project, annotate_opts);
      context.markDirty();

      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file\n";
        return 2;
      }
      return 0;
    }

    if (subcommand == "autoplace") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--no-collisions"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      
      ccad::AutoplaceOptions autoplace_opts;
      if (options.count("--no-collisions")) {
        autoplace_opts.avoid_collisions = false;
      }

      // Autoplace all schematics
      for (auto& sch : project.schematics) {
        ccad::autoplaceSchematicFields(sch, autoplace_opts);
      }
      context.markDirty();

      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file\n";
        return 2;
      }
      return 0;
    }

    if (subcommand == "fix-junctions") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();

      // Fix junctions in all schematics
      for (auto& sch : project.schematics) {
        ccad::fixSchematicJunctions(sch);
      }
      context.markDirty();

      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file\n";
        return 2;
      }
      return 0;
    }

    if (subcommand == "collect-items") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--scan-set", "--include-secondary"});
      const std::string file = requireOption(options, "--file");
      const std::string scan_set_name = options.contains("--scan-set") ? options.at("--scan-set") : "all_items";
      
      ccad::SchematicCollectorGuide guide;
      if (options.contains("--include-secondary")) {
        guide.include_secondary = requireOption(options, "--include-secondary") == "true";
      }

      ccad::Project project = loadProjectFile(file);
      const ccad::Schematic* sch = ccad::primarySchematic(project);
      if (!sch) {
        std::cerr << "project has no primary schematic\n";
        return 2;
      }

      const ccad::SchematicCollectorScanSet scan_set = ccad::parseSchematicCollectorScanSet(scan_set_name);
      const ccad::SchematicCollectorReport report = ccad::collectSchematicItems(*sch, scan_set, guide);

      std::ostringstream out;
      out << "{\n"
          << "  \"kicad_collector\": \"" << ccad::escapeJson(report.kicad_collector) << "\",\n"
          << "  \"parity_scope\": \"" << ccad::escapeJson(report.parity_scope) << "\",\n"
          << "  \"scan_set\": \"" << ccad::escapeJson(report.scan_set) << "\",\n"
          << "  \"collector_guide\": {\n"
          << "    \"include_secondary\": " << (guide.include_secondary ? "true" : "false") << "\n"
          << "  },\n"
          << "  \"kicad_scan_types\": [\n";
      for (std::size_t i = 0; i < report.kicad_scan_types.size(); ++i) {
        out << "    \"" << ccad::escapeJson(report.kicad_scan_types[i]) << "\""
            << (i + 1 == report.kicad_scan_types.size() ? "" : ",") << "\n";
      }
      out << "  ],\n"
          << "  \"unsupported_kicad_types\": [\n";
      for (std::size_t i = 0; i < report.unsupported_kicad_types.size(); ++i) {
        out << "    \"" << ccad::escapeJson(report.unsupported_kicad_types[i]) << "\""
            << (i + 1 == report.unsupported_kicad_types.size() ? "" : ",") << "\n";
      }
      out << "  ],\n"
          << "  \"summary\": {\n"
          << "    \"total\": " << report.candidates.size() << ",\n"
          << "    \"primary_count\": " << report.primary_count << ",\n"
          << "    \"secondary_count\": " << report.secondary_count << "\n"
          << "  },\n"
          << "  \"items\": [\n";
      for (std::size_t i = 0; i < report.candidates.size(); ++i) {
        const ccad::SchematicCollectorCandidate& c = report.candidates[i];
        out << "    {\n"
            << "      \"type\": \"" << ccad::escapeJson(c.type) << "\",\n"
            << "      \"id\": \"" << ccad::escapeJson(c.id) << "\",\n"
            << "      \"kicad_type\": \"" << ccad::escapeJson(c.kicad_type) << "\",\n"
            << "      \"collection_bucket\": \"" << ccad::escapeJson(c.collection_bucket) << "\",\n"
            << "      \"net_id\": \"" << ccad::escapeJson(c.net_id) << "\"\n"
            << "    }" << (i + 1 == report.candidates.size() ? "" : ",") << "\n";
      }
      out << "  ]\n"
          << "}\n";

      std::cout << out.str();
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
