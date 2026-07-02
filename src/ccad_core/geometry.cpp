#include "ccad_core/geometry.hpp"

#include <algorithm>
#include <cmath>

namespace ccad {

Length nanometers(const std::int64_t value) {
  return Length{.nanometers = value};
}

Length millimeters(const double value) {
  return Length{.nanometers = static_cast<std::int64_t>(std::llround(value * 1000000.0))};
}

Length mils(const double value) {
  return Length{.nanometers = static_cast<std::int64_t>(std::llround(value * 25400.0))};
}

Point maxPoint(const Rect& rect) {
  return Point{
      .x = nanometers(rect.origin.x.nanometers + rect.size.width.nanometers),
      .y = nanometers(rect.origin.y.nanometers + rect.size.height.nanometers),
  };
}

double toMillimeters(Length l) {
  return static_cast<double>(l.nanometers) / 1000000.0;
}

Length fromMillimeters(double mm) {
  return millimeters(mm);
}

double distancePoints(Point a, Point b) {
  double dx = static_cast<double>(a.x.nanometers - b.x.nanometers);
  double dy = static_cast<double>(a.y.nanometers - b.y.nanometers);
  return std::sqrt(dx * dx + dy * dy);
}

BoundingBox rectBoundingBox(const Rect& rect) {
  BoundingBox bb;
  bb.min = rect.origin;
  bb.max = maxPoint(rect);
  bb.valid = true;
  return bb;
}

BoundingBox expandBoundingBox(const BoundingBox& box, int64_t margin) {
  if (!box.valid) return box;
  BoundingBox result;
  result.min.x = nanometers(box.min.x.nanometers - margin);
  result.min.y = nanometers(box.min.y.nanometers - margin);
  result.max.x = nanometers(box.max.x.nanometers + margin);
  result.max.y = nanometers(box.max.y.nanometers + margin);
  result.valid = true;
  return result;
}

BoundingBox mergeBoundingBoxes(const BoundingBox& a, const BoundingBox& b) {
  if (!a.valid) return b;
  if (!b.valid) return a;
  BoundingBox result;
  result.min.x = nanometers(std::min(a.min.x.nanometers, b.min.x.nanometers));
  result.min.y = nanometers(std::min(a.min.y.nanometers, b.min.y.nanometers));
  result.max.x = nanometers(std::max(a.max.x.nanometers, b.max.x.nanometers));
  result.max.y = nanometers(std::max(a.max.y.nanometers, b.max.y.nanometers));
  result.valid = true;
  return result;
}

bool pointInsideRect(Point p, Point rectMin, Point rectMax) {
  return p.x.nanometers >= rectMin.x.nanometers &&
         p.x.nanometers <= rectMax.x.nanometers &&
         p.y.nanometers >= rectMin.y.nanometers &&
         p.y.nanometers <= rectMax.y.nanometers;
}

double distancePointToSegment(Point p, Point segStart, Point segEnd) {
  double px = static_cast<double>(p.x.nanometers);
  double py = static_cast<double>(p.y.nanometers);
  double ax = static_cast<double>(segStart.x.nanometers);
  double ay = static_cast<double>(segStart.y.nanometers);
  double bx = static_cast<double>(segEnd.x.nanometers);
  double by = static_cast<double>(segEnd.y.nanometers);

  double dx = bx - ax;
  double dy = by - ay;
  double lenSq = dx * dx + dy * dy;

  if (lenSq < 1e-12) {
    // Degenerate segment (zero length)
    double ddx = px - ax;
    double ddy = py - ay;
    return std::sqrt(ddx * ddx + ddy * ddy);
  }

  double t = ((px - ax) * dx + (py - ay) * dy) / lenSq;
  t = std::max(0.0, std::min(1.0, t));

  double projX = ax + t * dx;
  double projY = ay + t * dy;

  double ddx = px - projX;
  double ddy = py - projY;
  return std::sqrt(ddx * ddx + ddy * ddy);
}

}  // namespace ccad


