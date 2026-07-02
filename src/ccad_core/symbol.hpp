#pragma once

#include "ccad_core/geometry.hpp"
#include "ccad_core/pin_type.hpp"

#include <optional>
#include <string>
#include <vector>

namespace ccad {

struct SymbolPin {
  std::string name;
  std::string number;
  ElectricalPinType electrical_type = ElectricalPinType::Unspecified;
  GraphicPinShape shape = GraphicPinShape::Line;
  Point position;
  PinOrientation orientation = PinOrientation::Right;
  Length length;
};

struct SymbolProperty {
  std::string name;
  std::string value;
  Point position;
  double rotation_degrees = 0.0;
  bool visible = true;
};

struct SymbolRectangle {
  Point start;
  Point end;
  Length stroke_width;
  std::string fill_type;
};

struct SymbolLine {
  Point start;
  Point end;
  Length stroke_width;
};

struct SymbolArc {
  Point start;
  Point end;
  Point center;
  Length stroke_width;
};

struct SymbolCircle {
  Point center;
  Length radius;
  Length stroke_width;
  std::string fill_type;
};

struct SymbolPolyline {
  std::vector<Point> points;
  Length stroke_width;
  std::string fill_type;
};

struct SymbolText {
  std::string text;
  Point position;
  double rotation_degrees = 0.0;
  Length size;
};

struct Symbol {
  std::string name;
  std::string extends;
  std::vector<SymbolProperty> properties;
  std::vector<SymbolPin> pins;
  std::vector<SymbolRectangle> rectangles;
  std::vector<SymbolLine> lines;
  std::vector<SymbolArc> arcs;
  std::vector<SymbolCircle> circles;
  std::vector<SymbolPolyline> polylines;
  std::vector<SymbolText> texts;
};

}  // namespace ccad
