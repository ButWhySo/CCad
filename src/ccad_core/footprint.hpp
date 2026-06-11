#pragma once

#include "ccad_core/geometry.hpp"

#include <optional>
#include <string>
#include <vector>

namespace ccad {

struct FootprintPad {
  std::string number;
  std::string type;
  std::string shape;
  Point position;
  double rotation_degrees = 0.0;
  Size size;
  std::optional<Length> drill;
  std::optional<Length> secondary_drill = std::nullopt;
  std::optional<Length> tertiary_drill = std::nullopt;
  bool backdrilled = false;
  std::optional<Length> front_post_machining = std::nullopt;
  std::optional<Length> back_post_machining = std::nullopt;
  std::string pin_type;
  std::optional<Length> pad_to_die_length = std::nullopt;
  std::optional<double> pad_to_die_delay = std::nullopt;
  std::vector<std::string> layers;
  std::optional<double> roundrect_rratio = std::nullopt;
  std::optional<double> chamfer_ratio = std::nullopt;
};

struct FootprintLine {
  Point start;
  Point end;
  Length stroke_width;
  std::string layer;
};

struct FootprintArc {
  Point start;
  Point end;
  Point center;
  Length stroke_width;
  std::string layer;
};

struct FootprintCircle {
  Point center;
  Point end;
  Length stroke_width;
  std::string layer;
};

struct FootprintPolyline {
  std::vector<Point> points;
  Length stroke_width;
  std::string layer;
};

struct FootprintText {
  std::string type;
  std::string text;
  Point position;
  double rotation_degrees = 0.0;
  Length size_width;
  Length size_height;
  Length stroke_width;
  std::string layer;
};

struct FootprintModel3D {
  std::string path;
  Point offset; // Usually in mm, but we store in nm
  double offset_z = 0.0;
  double scale_x = 1.0;
  double scale_y = 1.0;
  double scale_z = 1.0;
  double rotate_x = 0.0;
  double rotate_y = 0.0;
  double rotate_z = 0.0;
};

struct Footprint {
  std::string name;
  std::vector<FootprintPad> pads;
  std::vector<FootprintLine> lines;
  std::vector<FootprintArc> arcs;
  std::vector<FootprintCircle> circles;
  std::vector<FootprintPolyline> polylines;
  std::vector<FootprintText> texts;
  std::vector<FootprintModel3D> models;
};

}  // namespace ccad
