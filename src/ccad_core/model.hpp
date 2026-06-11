#pragma once

#include "ccad_core/geometry.hpp"
#include "ccad_core/symbol.hpp"

#include <optional>
#include <string>
#include <vector>

namespace ccad {

struct Pin {
  std::string name;
  std::string kind;
};

struct Component {
  std::string id;
  std::string part;
  std::vector<Pin> pins;
  Point position;
  double rotation_degrees = 0.0;
  std::optional<Symbol> symbol = std::nullopt;
};

struct NetMember {
  std::string component_id;
  std::string pin_name;
};

struct Net {
  std::string id;
  std::vector<NetMember> members;
};

struct WireSegment {
  std::string id;
  Point start;
  Point end;
  std::string net_id;
};

struct Label {
  std::string id;
  std::string text;
  std::string net_id;
  Point position;
  double rotation_degrees = 0.0;
  bool global = false;
};

struct PowerSymbol {
  std::string id;
  std::string value; // e.g. "GND", "+5V"
  std::string net_id;
  Point position;
  double rotation_degrees = 0.0;
};

struct Constraint {
  std::string id;
  std::string kind;
  std::string target;
  std::string value;
};

struct Layer {
  std::string id;
  std::string name;
  std::string kind;
  bool visible = true;
};

struct DesignRules {
  Length copper_clearance = millimeters(0.20);
  Length min_track_width = millimeters(0.15);
  Length min_via_annular_ring = millimeters(0.10);
  Length min_connection = millimeters(0.0);
  Length min_via_diameter = millimeters(0.50);
  Length min_through_hole_drill = millimeters(0.30);
  Length min_microvia_diameter = millimeters(0.20);
  Length min_microvia_drill = millimeters(0.10);
  Length min_hole_to_hole = millimeters(0.25);
  Length hole_clearance = millimeters(0.25);
  Length copper_edge_clearance = millimeters(0.50);
  Length silk_clearance = millimeters(0.0);
  Length min_groove_width = millimeters(0.0);
  Length solder_mask_expansion = millimeters(0.0);
  Length solder_mask_min_width = millimeters(0.0);
  Length solder_mask_to_copper_clearance = millimeters(0.0);
  Length solder_paste_margin = millimeters(0.0);
  double solder_paste_margin_ratio = 0.0;
  Length board_thickness = millimeters(1.60);
  bool use_height_for_length_calcs = true;
  bool tent_vias_front = true;
  bool tent_vias_back = true;
  bool cover_vias_front = false;
  bool cover_vias_back = false;
  bool plug_vias_front = false;
  bool plug_vias_back = false;
  bool cap_vias = false;
  bool fill_vias = false;
};

struct Pad {
  std::string id;
  std::string component_id;
  std::string pin_name;
  std::string net_id;
  std::vector<std::string> layers;
  std::string type;
  std::string shape;
  Point position;
  double rotation_degrees = 0.0;
  Size size;
  std::optional<Length> drill;
  std::optional<double> roundrect_rratio = std::nullopt;
  std::optional<double> chamfer_ratio = std::nullopt;
};

struct Via {
  std::string id;
  std::string net_id;
  Point position;
  Length diameter;
  Length drill;
};

struct TrackSegment {
  std::string id;
  std::string net_id;
  std::string layer_id;
  Point start;
  Point end;
  Length width;
  std::string source_route_request_id;
};

struct BoardGraphic {
  std::string id;
  std::string kind;
  std::string layer_id;
  Point start;
  Point end;
  Length width;
};

struct BoardText {
  std::string id;
  std::string layer_id;
  std::string text;
  Point position;
  double rotation_degrees = 0.0;
  Size size;
};

struct BoardZone {
  std::string id;
  std::string name;
  std::string net_id;
  std::vector<std::string> layer_ids;
  std::vector<Point> outline;
  int priority = 0;
  Length clearance;
  Length min_thickness;
  bool fill_enabled = true;
  std::string pad_connection;
};

struct RouteRequest {
  std::string id;
  std::string net_id;
  std::string from_object_id;
  std::string to_object_id;
  std::string preferred_layer_id;
  std::string policy;
  Length width;
};

struct Keepout {
  std::string id;
  std::string kind;
  Rect area;
};

struct PlacementRegion {
  std::string id;
  std::string kind;
  Rect area;
};

struct Board {
  Rect outline;
  DesignRules design_rules;
  std::vector<Layer> layers;
  std::vector<PlacementRegion> placement_regions;
  std::vector<Keepout> keepouts;
  std::vector<Pad> pads;
  std::vector<Via> vias;
  std::vector<TrackSegment> tracks;
  std::vector<BoardGraphic> graphics;
  std::vector<BoardText> texts;
  std::vector<BoardZone> zones;
  std::vector<RouteRequest> route_requests;
};

struct Schematic {
  std::string id;
  std::string name;
  std::vector<Component> components;
  std::vector<Net> nets;
  std::vector<WireSegment> wires;
  std::vector<Label> labels;
  std::vector<PowerSymbol> power_symbols;
  std::vector<Constraint> constraints;
};

struct Project {
  int schema_version = 2;
  std::string id;
  std::string name;
  std::vector<Board> boards;
  std::vector<Schematic> schematics;
};

inline const Board* primaryBoard(const Project& project) {
  return project.boards.empty() ? nullptr : &project.boards.front();
}

inline Board* primaryBoard(Project& project) {
  return project.boards.empty() ? nullptr : &project.boards.front();
}

inline const Schematic* primarySchematic(const Project& project) {
  return project.schematics.empty() ? nullptr : &project.schematics.front();
}

inline Schematic* primarySchematic(Project& project) {
  return project.schematics.empty() ? nullptr : &project.schematics.front();
}

inline Schematic& ensurePrimarySchematic(Project& project) {
  if (project.schematics.empty()) {
    project.schematics.push_back(Schematic{});
  }
  return project.schematics.front();
}

}  // namespace ccad
