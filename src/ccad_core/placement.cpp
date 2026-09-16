#include "ccad_core/placement.hpp"
#include "ccad_core/layers.hpp"

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace ccad {

namespace {

Point rotateAndTranslate(const Point& local, const Point& origin, double rotation_degrees) {
  constexpr double pi = 3.14159265358979323846;
  const double radians = rotation_degrees * pi / 180.0;
  const double cos_theta = std::cos(radians);
  const double sin_theta = std::sin(radians);
  const double local_x = static_cast<double>(local.x.nanometers);
  const double local_y = static_cast<double>(local.y.nanometers);
  return Point{
      .x = nanometers(origin.x.nanometers +
                      static_cast<std::int64_t>(std::llround((local_x * cos_theta) -
                                                             (local_y * sin_theta)))),
      .y = nanometers(origin.y.nanometers +
                      static_cast<std::int64_t>(std::llround((local_x * sin_theta) +
                                                             (local_y * cos_theta)))),
  };
}

void requireInsideBoard(const Board& board, const Point point, const std::string& label) {
  const Point min = board.outline.origin;
  const Point max = maxPoint(board.outline);
  if (point.x.nanometers < min.x.nanometers || point.x.nanometers > max.x.nanometers ||
      point.y.nanometers < min.y.nanometers || point.y.nanometers > max.y.nanometers) {
    throw std::runtime_error(label + " is outside board outline");
  }
}

void requireRotatedRectInsideBoard(const Board& board, const Point center, const Size size,
                                   double rotation_degrees, const std::string& label) {
  constexpr double pi = 3.14159265358979323846;
  const double radians = rotation_degrees * pi / 180.0;
  const double cos_theta = std::cos(radians);
  const double sin_theta = std::sin(radians);
  const double half_width = static_cast<double>(size.width.nanometers) / 2.0;
  const double half_height = static_cast<double>(size.height.nanometers) / 2.0;
  const double center_x = static_cast<double>(center.x.nanometers);
  const double center_y = static_cast<double>(center.y.nanometers);

  for (const auto& local : {std::pair<double, double>{-half_width, -half_height},
                            std::pair<double, double>{half_width, -half_height},
                            std::pair<double, double>{half_width, half_height},
                            std::pair<double, double>{-half_width, half_height}}) {
    const Point corner{
        .x = nanometers(static_cast<std::int64_t>(
            std::llround(center_x + (local.first * cos_theta) - (local.second * sin_theta)))),
        .y = nanometers(static_cast<std::int64_t>(
            std::llround(center_y + (local.first * sin_theta) + (local.second * cos_theta))))};
    requireInsideBoard(board, corner, label + " corner");
  }
}

void requireCopperLayer(const Board& board, const std::string& layer_id) {
  for (const Layer& layer : board.layers) {
    if (layer.id == layer_id) {
      if (layer.kind != "copper") {
        throw std::runtime_error("layer is not copper: " + layer_id);
      }
      return;
    }
  }
  throw std::runtime_error("unknown layer: " + layer_id);
}

void requireUniquePhysicalObjectId(const Board& board, const std::string& id) {
  for (const Pad& pad : board.pads) {
    if (pad.id == id) throw std::runtime_error("duplicate physical object id: " + id);
  }
  for (const Via& via : board.vias) {
    if (via.id == id) throw std::runtime_error("duplicate physical object id: " + id);
  }
  for (const TrackSegment& track : board.tracks) {
    if (track.id == id) throw std::runtime_error("duplicate physical object id: " + id);
  }
  for (const Keepout& keepout : board.keepouts) {
    if (keepout.id == id) throw std::runtime_error("duplicate physical object id: " + id);
  }
  for (const PlacementRegion& region : board.placement_regions) {
    if (region.id == id) throw std::runtime_error("duplicate physical object id: " + id);
  }
}

void requireUniquePadId(const Board& board, const std::string& id) {
  for (const Pad& pad : board.pads) {
    if (pad.id == id) throw std::runtime_error("duplicate pad id: " + id);
  }
}

std::string netIdForPin(const Project& project, const std::string& component_id,
                        const std::string& pin_name) {
  const Schematic* schematic = primarySchematic(project);
  if (schematic == nullptr) {
    return "";
  }
  for (const Net& net : schematic->nets) {
    for (const NetMember& member : net.members) {
      if (member.component_id == component_id && member.pin_name == pin_name) {
        return net.id;
      }
    }
  }
  return "";
}

std::string valueForPlacedFootprint(const Project& project, const std::string& component_id,
                                    const std::optional<std::string>& explicit_value) {
  if (explicit_value.has_value()) {
    return *explicit_value;
  }
  const Schematic* schematic = primarySchematic(project);
  if (schematic == nullptr) {
    return "";
  }
  for (const SchSymbol& component : schematic->symbols) {
    if (component.id == component_id) {
      return component.lib_id;
    }
  }
  return "";
}

}  // namespace

void placeFootprint(Project& project, const Footprint& footprint, const std::string& component_id,
                    const Point& origin, double rotation_deg, const std::string& layer_id,
                    std::optional<std::string> value, bool exclude_from_bom) {
  if (project.boards.empty()) {
    throw std::runtime_error("project has no board");
  }
  Board& board = project.boards[0];
  
  if (footprint.pads.empty()) {
    throw std::runtime_error("footprint has no pads");
  }
  
  requireCopperLayer(board, layer_id);

  // Validation pass
  for (const FootprintPad& footprint_pad : footprint.pads) {
    const std::string pad_id = component_id + "." + footprint_pad.number;
    requireUniquePadId(board, pad_id);
    requireUniquePhysicalObjectId(board, pad_id);
    const Point placed_position =
        rotateAndTranslate(footprint_pad.position, origin, rotation_deg);
    requireInsideBoard(board, placed_position, "footprint pad position");
    requireRotatedRectInsideBoard(board, placed_position, footprint_pad.size,
                                  footprint_pad.rotation_degrees + rotation_deg,
                                  "footprint pad");
  }

  // Mutation pass
  for (const FootprintPad& footprint_pad : footprint.pads) {
    const Point placed_position =
        rotateAndTranslate(footprint_pad.position, origin, rotation_deg);
    std::vector<std::string> pad_layers = footprint_pad.layers;
    if (pad_layers.empty()) {
      pad_layers.push_back(layer_id);
    } else {
      if (layer_id.starts_with("B.")) {
        for (std::string& l : pad_layers) {
          if (l.starts_with("F.")) {
            l.replace(0, 2, "B.");
          } else if (l.starts_with("B.")) {
            l.replace(0, 2, "F.");
          }
        }
      }
      pad_layers = expandKiCadLayerSet(pad_layers, board);
    }

    Pad pad{
        .id = component_id + "." + footprint_pad.number,
        .component_id = component_id,
        .pin_name = footprint_pad.number,
        .net_id = netIdForPin(project, component_id, footprint_pad.number),
        .type = footprint_pad.type,
        .position = placed_position,
        .rotation_degrees = footprint_pad.rotation_degrees + rotation_deg,
        .pin_type = footprint_pad.pin_type,
        .pad_to_die_length = footprint_pad.pad_to_die_length,
        .pad_to_die_delay = footprint_pad.pad_to_die_delay,
        .locked = false,
        .padstack = Padstack{},
        .teardrops_enabled = false,
    };
    pad.padstack.layer_set = pad_layers;
    pad.padstack.drill.size.width = footprint_pad.drill.value_or(Length{});
    pad.padstack.drill.size.height = footprint_pad.drill_height.value_or(footprint_pad.drill.value_or(Length{}));
    if (footprint_pad.drill_shape.value_or("") == "oval") {
      pad.padstack.drill.shape = DrillShape::Oval;
    } else if (footprint_pad.drill.has_value()) {
      pad.padstack.drill.shape = DrillShape::Circle;
    }
    if (footprint_pad.secondary_drill.has_value()) {
        PadstackDrillProps drill_props{};
        drill_props.size.width = *footprint_pad.secondary_drill;
        drill_props.size.height = *footprint_pad.secondary_drill;
        pad.padstack.secondary_drill = drill_props;
    }
    if (footprint_pad.tertiary_drill.has_value()) {
        PadstackDrillProps drill_props{};
        drill_props.size.width = *footprint_pad.tertiary_drill;
        drill_props.size.height = *footprint_pad.tertiary_drill;
        pad.padstack.tertiary_drill = drill_props;
    }
    if (footprint_pad.backdrilled) {
        pad.padstack.back_post_machining.mode = "backdrill";
    }
    if (footprint_pad.front_post_machining.has_value()) {
        pad.padstack.front_post_machining.mode = "mill";
        pad.padstack.front_post_machining.size = *footprint_pad.front_post_machining;
    }
    if (footprint_pad.back_post_machining.has_value()) {
        pad.padstack.back_post_machining.mode = "mill";
        pad.padstack.back_post_machining.size = *footprint_pad.back_post_machining;
    }
    
    PadShape pad_shape = PadShape::Circle;
    if (footprint_pad.shape == "rect") pad_shape = PadShape::Rectangle;
    else if (footprint_pad.shape == "oval") pad_shape = PadShape::Oval;
    else if (footprint_pad.shape == "trapezoid") pad_shape = PadShape::Trapezoid;
    else if (footprint_pad.shape == "roundrect") pad_shape = PadShape::RoundRect;
    else if (footprint_pad.shape == "chamfered_rect") pad_shape = PadShape::ChamferedRect;
    else if (footprint_pad.shape == "custom") pad_shape = PadShape::Custom;
    
    PadstackShapeProps shape_props{};
    shape_props.shape = pad_shape;
    shape_props.size = footprint_pad.size;
    shape_props.roundrect_rratio = footprint_pad.roundrect_rratio.value_or(0.0);
    shape_props.chamfer_ratio = footprint_pad.chamfer_ratio.value_or(0.0);
    
    PadstackCopperLayerProps layer_props{};
    layer_props.shape = shape_props;
    pad.padstack.copper_props["top"] = layer_props;
    
    board.pads.push_back(pad);
  }

  board.footprints.push_back(BoardFootprint{
      .reference = component_id,
      .value = valueForPlacedFootprint(project, component_id, value),
      .footprint_name = footprint.name,
      .layer_id = layer_id,
      .position = origin,
      .rotation_degrees = rotation_deg,
      .exclude_from_bom = footprint.exclude_from_bom || exclude_from_bom,
      .locked = false,
      .front_courtyard = {},
      .back_courtyard = {},
  });
}

void placeComponent(Project& project, const Symbol& symbol, const std::string& component_id,
                    const Point& origin, double rotation_deg) {
  Schematic& schematic = ensurePrimarySchematic(project);
  for (const SchSymbol& comp : schematic.symbols) {
    if (comp.id == component_id) {
      throw std::runtime_error("duplicate component id: " + component_id);
    }
  }

  SchSymbol comp;
  comp.id = component_id;
  comp.lib_id = symbol.name;
  comp.position = origin;
  comp.rotation_degrees = rotation_deg;
  comp.symbol = symbol;
  
  for (const SymbolPin& pin : symbol.pins) {
    comp.pins.push_back(SchPin{
        .id = "",
        .name = pin.name,
        .number = pin.number,
        .electrical_type = pin.electrical_type,
        .shape = pin.shape,
        .orientation = pin.orientation,
        .position = Point{nanometers(0), nanometers(0)},
        .length = Length{0},
        .name_text_size = Length{0},
        .num_text_size = Length{0},
        .visible = true,
    });
  }
  schematic.symbols.push_back(comp);
}

void moveFootprint(Project& project, const std::string& component_id, const Point& delta) {
  if (project.boards.empty()) {
    throw std::runtime_error("project has no board");
  }
  for (auto& pad : project.boards[0].pads) {
    if (pad.component_id == component_id) {
      pad.position.x.nanometers += delta.x.nanometers;
      pad.position.y.nanometers += delta.y.nanometers;
    }
  }
}

}  // namespace ccad
