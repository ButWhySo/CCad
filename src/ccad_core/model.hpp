#pragma once

#include "ccad_core/geometry.hpp"

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
};

struct NetMember {
  std::string component_id;
  std::string pin_name;
};

struct Net {
  std::string id;
  std::vector<NetMember> members;
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
};

struct Pad {
  std::string id;
  std::string component_id;
  std::string pin_name;
  std::string net_id;
  std::string layer_id;
  Point position;
  double rotation_degrees = 0.0;
  Size size;
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
};

struct Project {
  int schema_version = 1;
  std::string id;
  std::string name;
  std::optional<Board> board;
  std::vector<Component> components;
  std::vector<Net> nets;
  std::vector<Constraint> constraints;
};

}  // namespace ccad

