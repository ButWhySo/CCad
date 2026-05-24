#include "ccad_cli/pcb_commands.hpp"

#include "ccad_cli/common.hpp"
#include "ccad_core/json.hpp"

#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

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

bool parseRequiredVisibleOption(const std::map<std::string, std::string>& options) {
  requireOption(options, "--visible");
  return parseVisibleOption(options);
}

void requireLayerUnused(const ccad::Board& board, const std::string& id) {
  for (const ccad::Pad& pad : board.pads) {
    if (pad.layer_id == id) {
      throw std::runtime_error("layer is referenced by pad: " + pad.id);
    }
  }
  for (const ccad::TrackSegment& track : board.tracks) {
    if (track.layer_id == id) {
      throw std::runtime_error("layer is referenced by track: " + track.id);
    }
  }
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

template <typename T>
bool eraseById(std::vector<T>& items, const std::string& id) {
  for (auto it = items.begin(); it != items.end(); ++it) {
    if (it->id == id) {
      items.erase(it);
      return true;
    }
  }
  return false;
}

void writePointJson(std::ostream& out, const ccad::Point& point, const int indent) {
  const std::string pad(static_cast<std::size_t>(indent), ' ');
  out << pad << "\"x_nm\": " << point.x.nanometers << ",\n";
  out << pad << "\"y_nm\": " << point.y.nanometers;
}

void writeSizeJson(std::ostream& out, const ccad::Size& size, const int indent) {
  const std::string pad(static_cast<std::size_t>(indent), ' ');
  out << pad << "\"width_nm\": " << size.width.nanometers << ",\n";
  out << pad << "\"height_nm\": " << size.height.nanometers;
}

std::string layerObjectJson(const ccad::Layer& layer) {
  std::ostringstream out;
  out << "{\n"
      << "  \"object\": {\n"
      << "    \"type\": \"layer\",\n"
      << "    \"id\": \"" << ccad::escapeJson(layer.id) << "\",\n"
      << "    \"name\": \"" << ccad::escapeJson(layer.name) << "\",\n"
      << "    \"kind\": \"" << ccad::escapeJson(layer.kind) << "\",\n"
      << "    \"visible\": " << (layer.visible ? "true" : "false") << "\n"
      << "  }\n"
      << "}\n";
  return out.str();
}

std::string padObjectJson(const ccad::Pad& pad) {
  std::ostringstream out;
  out << "{\n"
      << "  \"object\": {\n"
      << "    \"type\": \"pad\",\n"
      << "    \"id\": \"" << ccad::escapeJson(pad.id) << "\",\n"
      << "    \"component_id\": \"" << ccad::escapeJson(pad.component_id) << "\",\n"
      << "    \"pin_name\": \"" << ccad::escapeJson(pad.pin_name) << "\",\n"
      << "    \"net_id\": \"" << ccad::escapeJson(pad.net_id) << "\",\n"
      << "    \"layer_id\": \"" << ccad::escapeJson(pad.layer_id) << "\",\n"
      << "    \"position\": {\n";
  writePointJson(out, pad.position, 6);
  out << "\n    },\n"
      << "    \"rotation_degrees\": " << pad.rotation_degrees << ",\n"
      << "    \"size\": {\n";
  writeSizeJson(out, pad.size, 6);
  out << "\n    }\n"
      << "  }\n"
      << "}\n";
  return out.str();
}

std::string viaObjectJson(const ccad::Via& via) {
  std::ostringstream out;
  out << "{\n"
      << "  \"object\": {\n"
      << "    \"type\": \"via\",\n"
      << "    \"id\": \"" << ccad::escapeJson(via.id) << "\",\n"
      << "    \"net_id\": \"" << ccad::escapeJson(via.net_id) << "\",\n"
      << "    \"position\": {\n";
  writePointJson(out, via.position, 6);
  out << "\n    },\n"
      << "    \"diameter_nm\": " << via.diameter.nanometers << ",\n"
      << "    \"drill_nm\": " << via.drill.nanometers << "\n"
      << "  }\n"
      << "}\n";
  return out.str();
}

std::string trackObjectJson(const ccad::TrackSegment& track) {
  std::ostringstream out;
  out << "{\n"
      << "  \"object\": {\n"
      << "    \"type\": \"track\",\n"
      << "    \"id\": \"" << ccad::escapeJson(track.id) << "\",\n"
      << "    \"net_id\": \"" << ccad::escapeJson(track.net_id) << "\",\n"
      << "    \"layer_id\": \"" << ccad::escapeJson(track.layer_id) << "\",\n"
      << "    \"start\": {\n";
  writePointJson(out, track.start, 6);
  out << "\n    },\n"
      << "    \"end\": {\n";
  writePointJson(out, track.end, 6);
  out << "\n    },\n"
      << "    \"width_nm\": " << track.width.nanometers << "\n"
      << "  }\n"
      << "}\n";
  return out.str();
}

std::string regionObjectJson(const std::string& type, const std::string& id,
                             const std::string& kind, const ccad::Rect& area) {
  std::ostringstream out;
  out << "{\n"
      << "  \"object\": {\n"
      << "    \"type\": \"" << ccad::escapeJson(type) << "\",\n"
      << "    \"id\": \"" << ccad::escapeJson(id) << "\",\n"
      << "    \"kind\": \"" << ccad::escapeJson(kind) << "\",\n"
      << "    \"area\": {\n"
      << "      \"x_nm\": " << area.origin.x.nanometers << ",\n"
      << "      \"y_nm\": " << area.origin.y.nanometers << ",\n"
      << "      \"width_nm\": " << area.size.width.nanometers << ",\n"
      << "      \"height_nm\": " << area.size.height.nanometers << "\n"
      << "    }\n"
      << "  }\n"
      << "}\n";
  return out.str();
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

    if (subcommand == "set-layer") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--name", "--kind", "--visible"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const std::string name = requireOption(options, "--name");
      const std::string kind = requireOption(options, "--kind");
      const bool visible = parseRequiredVisibleOption(options);
      if (kind != "copper") {
        requireLayerUnused(board, id);
      }
      bool updated = false;
      for (ccad::Layer& layer : board.layers) {
        if (layer.id == id) {
          layer.name = name;
          layer.kind = kind;
          layer.visible = visible;
          updated = true;
          break;
        }
      }
      if (!updated) {
        throw std::runtime_error("unknown layer: " + id);
      }
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "get-object") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id"});
      const std::string file = requireOption(options, "--file");
      const ccad::Project project = loadProjectFile(file);
      if (!project.board.has_value()) {
        throw std::runtime_error("project has no board");
      }
      const ccad::Board& board = *project.board;
      const std::string id = requireOption(options, "--id");
      for (const ccad::Layer& layer : board.layers) {
        if (layer.id == id) {
          std::cout << layerObjectJson(layer);
          return 0;
        }
      }
      for (const ccad::Pad& pad : board.pads) {
        if (pad.id == id) {
          std::cout << padObjectJson(pad);
          return 0;
        }
      }
      for (const ccad::Via& via : board.vias) {
        if (via.id == id) {
          std::cout << viaObjectJson(via);
          return 0;
        }
      }
      for (const ccad::TrackSegment& track : board.tracks) {
        if (track.id == id) {
          std::cout << trackObjectJson(track);
          return 0;
        }
      }
      for (const ccad::Keepout& keepout : board.keepouts) {
        if (keepout.id == id) {
          std::cout << regionObjectJson("keepout", keepout.id, keepout.kind, keepout.area);
          return 0;
        }
      }
      for (const ccad::PlacementRegion& region : board.placement_regions) {
        if (region.id == id) {
          std::cout << regionObjectJson("placement_region", region.id, region.kind, region.area);
          return 0;
        }
      }
      throw std::runtime_error("unknown board object: " + id);
    }

    if (subcommand == "remove-layer") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      requireLayerUnused(board, id);
      bool removed = false;
      for (auto it = board.layers.begin(); it != board.layers.end(); ++it) {
        if (it->id == id) {
          board.layers.erase(it);
          removed = true;
          break;
        }
      }
      if (!removed) {
        throw std::runtime_error("unknown layer: " + id);
      }
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "set-layer-visibility") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--visible"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      bool updated = false;
      for (ccad::Layer& layer : board.layers) {
        if (layer.id == id) {
          layer.visible = parseRequiredVisibleOption(options);
          updated = true;
          break;
        }
      }
      if (!updated) {
        throw std::runtime_error("unknown layer: " + id);
      }
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
      requireCopperLayer(board, layer_id);
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

    if (subcommand == "set-pad") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--component", "--pin", "--net", "--layer",
                                 "--rotation-deg"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const std::string layer_id = requireOption(options, "--layer");
      const double rotation_degrees = requireDoubleOption(options, "--rotation-deg");
      requireCopperLayer(board, layer_id);
      bool updated = false;
      for (ccad::Pad& pad : board.pads) {
        if (pad.id == id) {
          requireRotatedRectInsideBoard(board, pad.position, pad.size, rotation_degrees, "pad");
          pad.component_id = requireOption(options, "--component");
          pad.pin_name = requireOption(options, "--pin");
          pad.net_id = requireOption(options, "--net");
          pad.layer_id = layer_id;
          pad.rotation_degrees = rotation_degrees;
          updated = true;
          break;
        }
      }
      if (!updated) {
        throw std::runtime_error("unknown pad: " + id);
      }
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

    if (subcommand == "set-via") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--diameter-mm", "--drill-mm", "--net"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const ccad::Length diameter = requirePositiveMillimeters(options, "--diameter-mm");
      const ccad::Length drill = requirePositiveMillimeters(options, "--drill-mm");
      if (drill.nanometers > diameter.nanometers) {
        throw std::runtime_error("via drill must be less than or equal to diameter");
      }
      bool updated = false;
      for (ccad::Via& via : board.vias) {
        if (via.id == id) {
          requirePointWithMarginInsideBoard(board, via.position,
                                            ccad::nanometers(diameter.nanometers / 2), "via");
          if (options.contains("--net")) {
            via.net_id = requireOption(options, "--net");
          }
          via.diameter = diameter;
          via.drill = drill;
          updated = true;
          break;
        }
      }
      if (!updated) {
        throw std::runtime_error("unknown via: " + id);
      }
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
      requireCopperLayer(board, layer_id);
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

    if (subcommand == "set-track") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--start-x-mm", "--start-y-mm",
                                 "--end-x-mm", "--end-y-mm", "--width-mm", "--net",
                                 "--layer"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      if (options.contains("--layer")) {
        requireCopperLayer(board, requireOption(options, "--layer"));
      }
      const ccad::Point start{
          .x = requirePositiveMillimeters(options, "--start-x-mm"),
          .y = requirePositiveMillimeters(options, "--start-y-mm"),
      };
      const ccad::Point end{
          .x = requirePositiveMillimeters(options, "--end-x-mm"),
          .y = requirePositiveMillimeters(options, "--end-y-mm"),
      };
      const ccad::Length width = requirePositiveMillimeters(options, "--width-mm");
      const ccad::Length half_width = ccad::nanometers(width.nanometers / 2);
      requirePointWithMarginInsideBoard(board, start, half_width, "track start");
      requirePointWithMarginInsideBoard(board, end, half_width, "track end");
      bool updated = false;
      for (ccad::TrackSegment& track : board.tracks) {
        if (track.id == id) {
          if (options.contains("--net")) {
            track.net_id = requireOption(options, "--net");
          }
          if (options.contains("--layer")) {
            track.layer_id = requireOption(options, "--layer");
          }
          track.start = start;
          track.end = end;
          track.width = width;
          updated = true;
          break;
        }
      }
      if (!updated) {
        throw std::runtime_error("unknown track: " + id);
      }
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

    if (subcommand == "set-region-kind") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--kind"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const std::string kind = requireOption(options, "--kind");
      bool updated = false;
      for (ccad::Keepout& keepout : board.keepouts) {
        if (keepout.id == id) {
          keepout.kind = kind;
          updated = true;
          break;
        }
      }
      for (ccad::PlacementRegion& region : board.placement_regions) {
        if (region.id == id) {
          region.kind = kind;
          updated = true;
          break;
        }
      }
      if (!updated) {
        throw std::runtime_error("unknown region: " + id);
      }
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "remove-object") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const bool removed = eraseById(board.pads, id) || eraseById(board.vias, id) ||
                           eraseById(board.tracks, id) || eraseById(board.keepouts, id) ||
                           eraseById(board.placement_regions, id);
      if (!removed) {
        throw std::runtime_error("unknown physical object: " + id);
      }
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "move-object") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--x-mm", "--y-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const ccad::Point position{
          .x = requirePositiveMillimeters(options, "--x-mm"),
          .y = requirePositiveMillimeters(options, "--y-mm"),
      };

      bool moved = false;
      for (ccad::Pad& pad : board.pads) {
        if (pad.id == id) {
          requireRotatedRectInsideBoard(board, position, pad.size, pad.rotation_degrees, "pad");
          pad.position = position;
          moved = true;
          break;
        }
      }
      for (ccad::Via& via : board.vias) {
        if (via.id == id) {
          requirePointWithMarginInsideBoard(board, position,
                                            ccad::nanometers(via.diameter.nanometers / 2),
                                            "via");
          via.position = position;
          moved = true;
          break;
        }
      }
      for (ccad::Keepout& keepout : board.keepouts) {
        if (keepout.id == id) {
          const ccad::Rect moved_area{.origin = position, .size = keepout.area.size};
          requireRectInsideBoard(board, moved_area, "keepout area");
          keepout.area.origin = position;
          moved = true;
          break;
        }
      }
      for (ccad::PlacementRegion& region : board.placement_regions) {
        if (region.id == id) {
          const ccad::Rect moved_area{.origin = position, .size = region.area.size};
          requireRectInsideBoard(board, moved_area, "placement region area");
          region.area.origin = position;
          moved = true;
          break;
        }
      }
      if (!moved) {
        throw std::runtime_error("unknown movable physical object: " + id);
      }
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "resize-object") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--width-mm", "--height-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const ccad::Size size{.width = requirePositiveMillimeters(options, "--width-mm"),
                            .height = requirePositiveMillimeters(options, "--height-mm")};

      bool resized = false;
      for (ccad::Pad& pad : board.pads) {
        if (pad.id == id) {
          requireRotatedRectInsideBoard(board, pad.position, size, pad.rotation_degrees, "pad");
          pad.size = size;
          resized = true;
          break;
        }
      }
      for (ccad::Keepout& keepout : board.keepouts) {
        if (keepout.id == id) {
          const ccad::Rect resized_area{.origin = keepout.area.origin, .size = size};
          requireRectInsideBoard(board, resized_area, "keepout area");
          keepout.area.size = size;
          resized = true;
          break;
        }
      }
      for (ccad::PlacementRegion& region : board.placement_regions) {
        if (region.id == id) {
          const ccad::Rect resized_area{.origin = region.area.origin, .size = size};
          requireRectInsideBoard(board, resized_area, "placement region area");
          region.area.size = size;
          resized = true;
          break;
        }
      }
      if (!resized) {
        throw std::runtime_error("unknown resizable physical object: " + id);
      }
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
      requireCopperLayer(board, layer_id);
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
