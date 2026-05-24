#pragma once

#include "ccad_core/model.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace ccad {

struct CanvasLayer {
  std::string id;
  std::string name;
  std::string kind;
  bool visible = true;
};

struct CanvasPad {
  std::string id;
  std::string net_id;
  std::string layer_id;
  double x_units = 0.0;
  double y_units = 0.0;
  double width_units = 0.0;
  double height_units = 0.0;
  double rotation_degrees = 0.0;
};

struct CanvasVia {
  std::string id;
  std::string net_id;
  double x_units = 0.0;
  double y_units = 0.0;
  double diameter_units = 0.0;
  double drill_units = 0.0;
};

struct CanvasTrack {
  std::string id;
  std::string net_id;
  std::string layer_id;
  double start_x_units = 0.0;
  double start_y_units = 0.0;
  double end_x_units = 0.0;
  double end_y_units = 0.0;
  double width_units = 0.0;
};

struct CanvasKeepout {
  std::string id;
  std::string kind;
  double x_units = 0.0;
  double y_units = 0.0;
  double width_units = 0.0;
  double height_units = 0.0;
};

struct CanvasPlacementRegion {
  std::string id;
  std::string kind;
  double x_units = 0.0;
  double y_units = 0.0;
  double width_units = 0.0;
  double height_units = 0.0;
};

struct CanvasScene {
  bool has_board = false;
  std::int64_t board_width_nm = 0;
  std::int64_t board_height_nm = 0;
  double view_width_units = 0.0;
  double view_height_units = 0.0;
  std::vector<CanvasLayer> layers;
  std::vector<CanvasPlacementRegion> placement_regions;
  std::vector<CanvasKeepout> keepouts;
  std::vector<CanvasPad> pads;
  std::vector<CanvasVia> vias;
  std::vector<CanvasTrack> tracks;
};

CanvasScene buildCanvasScene(const Project& project);

}  // namespace ccad

