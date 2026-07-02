#pragma once

#include <cstdint>

namespace ccad {

struct Length {
  std::int64_t nanometers = 0;
};

struct Point {
  Length x;
  Length y;
};

struct Size {
  Length width;
  Length height;
};

struct Rect {
  Point origin;
  Size size;
};

struct BoundingBox {
  Point min;
  Point max;
  bool valid = false;
};

Length nanometers(std::int64_t value);
Length millimeters(double value);
Length mils(double value);
Point maxPoint(const Rect& rect);

// Unit conversion helpers
double toMillimeters(Length l);
Length fromMillimeters(double mm);

// Geometry computation helpers
double distancePoints(Point a, Point b);
BoundingBox rectBoundingBox(const Rect& rect);
BoundingBox expandBoundingBox(const BoundingBox& box, int64_t margin);
BoundingBox mergeBoundingBoxes(const BoundingBox& a, const BoundingBox& b);
bool pointInsideRect(Point p, Point rectMin, Point rectMax);
double distancePointToSegment(Point p, Point segStart, Point segEnd);

}  // namespace ccad

