#pragma once

#include "ccad_core/footprint.hpp"
#include "ccad_core/model.hpp"
#include "ccad_core/symbol.hpp"

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
  std::vector<std::string> layers;
  std::string type;
  std::string shape;
  double x_units = 0.0;
  double y_units = 0.0;
  double width_units = 0.0;
  double height_units = 0.0;
  double rotation_degrees = 0.0;
  double drill_units = 0.0;
  std::optional<double> roundrect_rratio = std::nullopt;
  std::optional<double> chamfer_ratio = std::nullopt;
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
  std::string source_route_request_id;
  double start_x_units = 0.0;
  double start_y_units = 0.0;
  double end_x_units = 0.0;
  double end_y_units = 0.0;
  double width_units = 0.0;
};

struct CanvasRouteRequest {
  std::string id;
  std::string net_id;
  std::string from_object_id;
  std::string to_object_id;
  std::string preferred_layer_id;
  std::string policy;
  std::int64_t width_nm = 0;
  std::size_t routed_segment_count = 0;
};

struct CanvasComponent {
  std::string id;
  std::string part;
  double x_units = 0.0;
  double y_units = 0.0;
  double rotation_degrees = 0.0;
  bool selected = false;
  bool has_symbol_graphics = false;
};

struct CanvasWire {
  std::string id;
  std::string net_id;
  double start_x_units = 0.0;
  double start_y_units = 0.0;
  double end_x_units = 0.0;
  double end_y_units = 0.0;
};

struct CanvasLabel {
  std::string id;
  std::string text;
  std::string net_id;
  double x_units = 0.0;
  double y_units = 0.0;
  double rotation_degrees = 0.0;
  bool global = false;
};

struct CanvasPowerSymbol {
  std::string id;
  std::string value;
  std::string net_id;
  double x_units = 0.0;
  double y_units = 0.0;
  double rotation_degrees = 0.0;
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

struct CanvasLine {
  std::string id;
  std::string layer_id;
  double start_x_units = 0.0;
  double start_y_units = 0.0;
  double end_x_units = 0.0;
  double end_y_units = 0.0;
  double width_units = 0.0;
};

struct CanvasArc {
  std::string id;
  std::string layer_id;
  double start_x_units = 0.0;
  double start_y_units = 0.0;
  double mid_x_units = 0.0;
  double mid_y_units = 0.0;
  double end_x_units = 0.0;
  double end_y_units = 0.0;
  double width_units = 0.0;
};

struct CanvasCircle {
  std::string id;
  std::string layer_id;
  double center_x_units = 0.0;
  double center_y_units = 0.0;
  double radius_units = 0.0;
  double width_units = 0.0;
  std::string fill_type;
};

struct CanvasPolygon {
  std::string id;
  std::string layer_id;
  std::vector<double> pts_x_units;
  std::vector<double> pts_y_units;
  double width_units = 0.0;
  std::string fill_type;
};

struct CanvasZone {
  std::string id;
  std::string name;
  std::string net_id;
  std::vector<std::string> layer_ids;
  std::vector<double> pts_x_units;
  std::vector<double> pts_y_units;
  int priority = 0;
  double clearance_units = 0.0;
  double min_thickness_units = 0.0;
  bool fill_enabled = true;
  std::string pad_connection;
};

struct CanvasText {
  std::string id;
  std::string layer_id;
  std::string text;
  double x_units = 0.0;
  double y_units = 0.0;
  double rotation_degrees = 0.0;
  double size_x_units = 0.0;
  double size_y_units = 0.0;
};

struct CanvasScene {
  bool has_board = false;
  std::int64_t board_width_nm = 0;
  std::int64_t board_height_nm = 0;
  double board_origin_x_units = 0.0;
  double board_origin_y_units = 0.0;
  double view_width_units = 0.0;
  double view_height_units = 0.0;
  std::vector<CanvasLayer> layers;
  std::vector<CanvasPlacementRegion> placement_regions;
  std::vector<CanvasKeepout> keepouts;
  std::vector<CanvasPad> pads;
  std::vector<CanvasVia> vias;
  std::vector<CanvasTrack> tracks;
  std::vector<CanvasRouteRequest> route_requests;
  
  std::vector<CanvasLine> lines;
  std::vector<CanvasArc> arcs;
  std::vector<CanvasCircle> circles;
  std::vector<CanvasPolygon> polygons;
  std::vector<CanvasText> texts;
  std::vector<CanvasZone> zones;

  std::vector<CanvasComponent> components;
  std::vector<CanvasWire> wires;
  std::vector<CanvasLabel> labels;
  std::vector<CanvasPowerSymbol> power_symbols;
};

CanvasScene buildCanvasScene(const Project& project);
CanvasScene buildSchematicScene(const Project& project);
CanvasScene buildCanvasScene(const Footprint& footprint);
CanvasScene buildCanvasScene(const Symbol& symbol);

}  // namespace ccad

