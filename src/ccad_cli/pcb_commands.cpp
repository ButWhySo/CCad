#include "ccad_cli/pcb_commands.hpp"

#include "ccad_cli/common.hpp"

#include <iostream>
#include <map>
#include <stdexcept>

namespace ccad_cli {

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

    if (subcommand == "place-footprint") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--footprint", "--component", "--at-x-mm", "--at-y-mm",
                                 "--layer", "--rotation-deg"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      const ccad::Footprint footprint = loadFootprintFile(requireOption(options, "--footprint"));
      if (footprint.pads.empty()) {
        throw std::runtime_error("footprint has no pads");
      }
      const std::string component_id = requireOption(options, "--component");
      const std::string layer_id = requireOption(options, "--layer");
      requireLayer(board, layer_id);
      const ccad::Point origin{
          .x = requirePositiveMillimeters(options, "--at-x-mm"),
          .y = requirePositiveMillimeters(options, "--at-y-mm"),
      };
      const double placement_rotation = optionDoubleOrDefault(options, "--rotation-deg", 0.0);

      for (const ccad::FootprintPad& footprint_pad : footprint.pads) {
        const std::string pad_id = component_id + "." + footprint_pad.number;
        requireUniquePadId(board, pad_id);
        const ccad::Point placed_position =
            rotateAndTranslate(footprint_pad.position, origin, placement_rotation);
        requireInsideBoard(board, placed_position, "footprint pad position");
      }

      for (const ccad::FootprintPad& footprint_pad : footprint.pads) {
        const ccad::Point placed_position =
            rotateAndTranslate(footprint_pad.position, origin, placement_rotation);
        board.pads.push_back(ccad::Pad{
            .id = component_id + "." + footprint_pad.number,
            .component_id = component_id,
            .pin_name = footprint_pad.number,
            .net_id = "",
            .layer_id = layer_id,
            .position = placed_position,
            .rotation_degrees = footprint_pad.rotation_degrees + placement_rotation,
            .size = footprint_pad.size,
        });
      }

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

}  // namespace ccad_cli
