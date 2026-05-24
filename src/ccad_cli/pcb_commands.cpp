#include "ccad_cli/pcb_commands.hpp"

#include "ccad_cli/common.hpp"

#include <iostream>
#include <map>
#include <stdexcept>

namespace ccad_cli {
namespace {

std::string netIdForPin(const ccad::Project& project, const std::string& component_id,
                        const std::string& pin_name) {
  for (const ccad::Net& net : project.nets) {
    for (const ccad::NetMember& member : net.members) {
      if (member.component_id == component_id && member.pin_name == pin_name) {
        return net.id;
      }
    }
  }
  return "";
}

bool parseVisibleOption(const std::map<std::string, std::string>& options) {
  if (!options.contains("--visible")) {
    return true;
  }
  const std::string value = requireOption(options, "--visible");
  if (value == "true") {
    return true;
  }
  if (value == "false") {
    return false;
  }
  throw std::runtime_error("--visible must be true or false");
}

ccad::Length requireMillimeters(const std::map<std::string, std::string>& options,
                                const std::string& key) {
  return ccad::millimeters(requireDoubleOption(options, key));
}

void requireBoardObjectsInsideOutline(const ccad::Board& board) {
  for (const ccad::Pad& pad : board.pads) {
    requireRotatedRectInsideBoard(board, pad.position, pad.size, pad.rotation_degrees,
                                  "pad " + pad.id);
  }
  for (const ccad::Via& via : board.vias) {
    requirePointWithMarginInsideBoard(board, via.position,
                                      ccad::nanometers(via.diameter.nanometers / 2),
                                      "via " + via.id);
  }
  for (const ccad::TrackSegment& track : board.tracks) {
    const ccad::Length half_width = ccad::nanometers(track.width.nanometers / 2);
    requirePointWithMarginInsideBoard(board, track.start, half_width, "track " + track.id);
    requirePointWithMarginInsideBoard(board, track.end, half_width, "track " + track.id);
  }
  for (const ccad::Keepout& keepout : board.keepouts) {
    requireRectInsideBoard(board, keepout.area, "keepout " + keepout.id);
  }
  for (const ccad::PlacementRegion& region : board.placement_regions) {
    requireRectInsideBoard(board, region.area, "placement region " + region.id);
  }
}

}  // namespace

int pcbCommand(const std::vector<std::string>& args) {
  if (args.empty()) {
    std::cerr << "pcb requires a subcommand\n";
    return 2;
  }

  try {
    const std::string& subcommand = args.at(0);
    if (subcommand == "add-layer") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--name", "--kind", "--visible"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const std::string name = requireOption(options, "--name");
      const std::string kind = requireOption(options, "--kind");
      requireUniqueLayerId(board, id);
      board.layers.push_back(ccad::Layer{
          .id = id,
          .name = name,
          .kind = kind,
          .visible = parseVisibleOption(options),
      });
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "set-rules") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--copper-clearance-mm", "--min-track-width-mm",
                                 "--min-via-annular-ring-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      board.design_rules = ccad::DesignRules{
          .copper_clearance = requirePositiveMillimeters(options, "--copper-clearance-mm"),
          .min_track_width = requirePositiveMillimeters(options, "--min-track-width-mm"),
          .min_via_annular_ring =
              requirePositiveMillimeters(options, "--min-via-annular-ring-mm"),
      };
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "set-outline") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--x-mm", "--y-mm", "--width-mm", "--height-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      const ccad::Rect previous_outline = board.outline;
      board.outline = ccad::Rect{
          .origin = ccad::Point{.x = requireMillimeters(options, "--x-mm"),
                                .y = requireMillimeters(options, "--y-mm")},
          .size = ccad::Size{.width = requirePositiveMillimeters(options, "--width-mm"),
                             .height = requirePositiveMillimeters(options, "--height-mm")},
      };
      try {
        requireBoardObjectsInsideOutline(board);
      } catch (...) {
        board.outline = previous_outline;
        throw;
      }
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

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
      requireUniquePhysicalObjectId(board, id);
      requireLayer(board, layer_id);
      const ccad::Point position{
          .x = requirePositiveMillimeters(options, "--x-mm"),
          .y = requirePositiveMillimeters(options, "--y-mm"),
      };
      requireInsideBoard(board, position, "pad position");
      const ccad::Size size{.width = requirePositiveMillimeters(options, "--width-mm"),
                            .height = requirePositiveMillimeters(options, "--height-mm")};
      requireCenteredRectInsideBoard(board, position, size, "pad");
      board.pads.push_back(ccad::Pad{
          .id = id,
          .component_id = requireOption(options, "--component"),
          .pin_name = requireOption(options, "--pin"),
          .net_id = requireOption(options, "--net"),
          .layer_id = layer_id,
          .position = position,
          .size = size,
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
      requireUniquePhysicalObjectId(board, id);
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
      requirePointWithMarginInsideBoard(board, position,
                                        ccad::nanometers(diameter.nanometers / 2), "via");
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
      requireUniquePhysicalObjectId(board, id);
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
      const ccad::Length width = requirePositiveMillimeters(options, "--width-mm");
      const ccad::Length half_width = ccad::nanometers(width.nanometers / 2);
      requirePointWithMarginInsideBoard(board, start, half_width, "track start");
      requirePointWithMarginInsideBoard(board, end, half_width, "track end");
      board.tracks.push_back(ccad::TrackSegment{
          .id = id,
          .net_id = requireOption(options, "--net"),
          .layer_id = layer_id,
          .start = start,
          .end = end,
          .width = width,
      });
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "add-keepout") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--kind", "--x-mm", "--y-mm", "--width-mm",
                                 "--height-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      requireUniqueKeepoutId(board, id);
      requireUniquePhysicalObjectId(board, id);
      const ccad::Rect area{
          .origin = ccad::Point{.x = requirePositiveMillimeters(options, "--x-mm"),
                                .y = requirePositiveMillimeters(options, "--y-mm")},
          .size = ccad::Size{.width = requirePositiveMillimeters(options, "--width-mm"),
                             .height = requirePositiveMillimeters(options, "--height-mm")},
      };
      requireRectInsideBoard(board, area, "keepout area");
      board.keepouts.push_back(ccad::Keepout{
          .id = id,
          .kind = requireOption(options, "--kind"),
          .area = area,
      });
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "add-placement-region") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--kind", "--x-mm", "--y-mm", "--width-mm",
                                 "--height-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      requireUniquePlacementRegionId(board, id);
      requireUniquePhysicalObjectId(board, id);
      const ccad::Rect area{
          .origin = ccad::Point{.x = requirePositiveMillimeters(options, "--x-mm"),
                                .y = requirePositiveMillimeters(options, "--y-mm")},
          .size = ccad::Size{.width = requirePositiveMillimeters(options, "--width-mm"),
                             .height = requirePositiveMillimeters(options, "--height-mm")},
      };
      requireRectInsideBoard(board, area, "placement region area");
      board.placement_regions.push_back(ccad::PlacementRegion{
          .id = id,
          .kind = requireOption(options, "--kind"),
          .area = area,
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
        requireUniquePhysicalObjectId(board, pad_id);
        const ccad::Point placed_position =
            rotateAndTranslate(footprint_pad.position, origin, placement_rotation);
        requireInsideBoard(board, placed_position, "footprint pad position");
        requireRotatedRectInsideBoard(board, placed_position, footprint_pad.size,
                                      footprint_pad.rotation_degrees + placement_rotation,
                                      "footprint pad");
      }

      for (const ccad::FootprintPad& footprint_pad : footprint.pads) {
        const ccad::Point placed_position =
            rotateAndTranslate(footprint_pad.position, origin, placement_rotation);
        board.pads.push_back(ccad::Pad{
            .id = component_id + "." + footprint_pad.number,
            .component_id = component_id,
            .pin_name = footprint_pad.number,
            .net_id = netIdForPin(project, component_id, footprint_pad.number),
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
