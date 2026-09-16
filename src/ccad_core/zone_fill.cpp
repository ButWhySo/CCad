#include "zone_fill.hpp"

#include <algorithm>
#include <cmath>

namespace ccad {
namespace {
std::vector<Point> offsetRectangle(const std::vector<Point>& contour, const int64_t offset) {
  if (contour.size() != 4) return {};
  const int64_t min_x = std::min_element(contour.begin(), contour.end(),
      [](const Point& a, const Point& b) { return a.x.nanometers < b.x.nanometers; })->x.nanometers;
  const int64_t max_x = std::max_element(contour.begin(), contour.end(),
      [](const Point& a, const Point& b) { return a.x.nanometers < b.x.nanometers; })->x.nanometers;
  const int64_t min_y = std::min_element(contour.begin(), contour.end(),
      [](const Point& a, const Point& b) { return a.y.nanometers < b.y.nanometers; })->y.nanometers;
  const int64_t max_y = std::max_element(contour.begin(), contour.end(),
      [](const Point& a, const Point& b) { return a.y.nanometers < b.y.nanometers; })->y.nanometers;
  if (offset > 0 && (max_x - min_x <= 2 * offset || max_y - min_y <= 2 * offset)) return {};
  for (const Point& point : contour)
    if (!((point.x.nanometers == min_x || point.x.nanometers == max_x) &&
          (point.y.nanometers == min_y || point.y.nanometers == max_y))) return {};
  return {{nanometers(min_x + offset), nanometers(min_y + offset)},
          {nanometers(max_x - offset), nanometers(min_y + offset)},
          {nanometers(max_x - offset), nanometers(max_y - offset)},
          {nanometers(min_x + offset), nanometers(max_y - offset)}};
}

long double signedArea(const std::vector<Point>& contour) {
  long double twice_area = 0.0L;
  for (std::size_t i = 0; i < contour.size(); ++i) {
    const Point& a = contour[i];
    const Point& b = contour[(i + 1) % contour.size()];
    twice_area += static_cast<long double>(a.x.nanometers) * b.y.nanometers -
                  static_cast<long double>(b.x.nanometers) * a.y.nanometers;
  }
  return twice_area / 2.0L;
}

bool pointInContour(const Point& point, const std::vector<Point>& contour) {
  bool inside = false;
  for (std::size_t i = 0, j = contour.size() - 1; i < contour.size(); j = i++) {
    const auto& a = contour[i];
    const auto& b = contour[j];
    const bool crosses = ((a.y.nanometers > point.y.nanometers) != (b.y.nanometers > point.y.nanometers));
    if (crosses) {
      const long double x = static_cast<long double>(b.x.nanometers - a.x.nanometers) *
                                (point.y.nanometers - a.y.nanometers) /
                                static_cast<long double>(b.y.nanometers - a.y.nanometers) + a.x.nanometers;
      if (static_cast<long double>(point.x.nanometers) < x) inside = !inside;
    }
  }
  return inside;
}
}  // namespace

std::vector<ZoneThermalSpoke> buildRectangularThermalSpokes(
    const BoardZone& zone, const Point& pad_center, Length pad_radius,
    Length gap, Length spoke_width) {
  std::vector<ZoneThermalSpoke> spokes;
  if (zone.outline.size() != 4 || pad_radius.nanometers < 0 || gap.nanometers < 0 ||
      spoke_width.nanometers <= 0) return spokes;
  const auto outer = offsetRectangle(zone.outline, 0);
  if (outer.empty() || !pointInContour(pad_center, outer)) return spokes;
  const int64_t min_x = std::min_element(outer.begin(), outer.end(),
      [](const Point& a, const Point& b) { return a.x.nanometers < b.x.nanometers; })->x.nanometers;
  const int64_t max_x = std::max_element(outer.begin(), outer.end(),
      [](const Point& a, const Point& b) { return a.x.nanometers < b.x.nanometers; })->x.nanometers;
  const int64_t min_y = std::min_element(outer.begin(), outer.end(),
      [](const Point& a, const Point& b) { return a.y.nanometers < b.y.nanometers; })->y.nanometers;
  const int64_t max_y = std::max_element(outer.begin(), outer.end(),
      [](const Point& a, const Point& b) { return a.y.nanometers < b.y.nanometers; })->y.nanometers;
  const int64_t start = pad_radius.nanometers + gap.nanometers;
  if (pad_center.x.nanometers + start < max_x)
    spokes.push_back({{nanometers(pad_center.x.nanometers + start), pad_center.y},
                      {nanometers(max_x), pad_center.y}, spoke_width});
  if (pad_center.x.nanometers - start > min_x)
    spokes.push_back({{nanometers(pad_center.x.nanometers - start), pad_center.y},
                      {nanometers(min_x), pad_center.y}, spoke_width});
  if (pad_center.y.nanometers + start < max_y)
    spokes.push_back({{pad_center.x, nanometers(pad_center.y.nanometers + start)},
                      {pad_center.x, nanometers(max_y)}, spoke_width});
  if (pad_center.y.nanometers - start > min_y)
    spokes.push_back({{pad_center.x, nanometers(pad_center.y.nanometers - start)},
                      {pad_center.x, nanometers(min_y)}, spoke_width});
  return spokes;
}

ZoneFillResult calculateZoneFill(const BoardZone& zone) {
  ZoneFillResult result;
  if (!zone.fill_enabled) {
    result.diagnostics.push_back("zone fill disabled");
    return result;
  }
  if (zone.outline.size() < 3) {
    result.diagnostics.push_back("zone outline requires at least three points");
    return result;
  }
  if (zone.clearance.nanometers < 0) {
    result.diagnostics.push_back("zone clearance must be non-negative");
    return result;
  }
  result.filled = true;
  const int64_t clearance = zone.clearance.nanometers;
  std::vector<Point> outer = clearance == 0 ? zone.outline : offsetRectangle(zone.outline, clearance);
  if (outer.empty()) {
    result.filled = false;
    result.diagnostics.push_back("clearance knockout supports axis-aligned rectangles only");
    return result;
  }
  result.contours.push_back(std::move(outer));
  result.area_square_nanometers = static_cast<int64_t>(std::llround(std::abs(signedArea(result.contours.front()))));
  for (const auto& hole : zone.holes) {
    if (hole.size() < 3) {
      result.filled = false;
      result.contours.clear();
      result.area_square_nanometers = 0;
      result.diagnostics.push_back("zone hole requires at least three points");
      return result;
    }
    std::vector<Point> fill_hole = clearance == 0 ? hole : offsetRectangle(hole, -clearance);
    if (fill_hole.empty()) {
      result.filled = false;
      result.contours.clear();
      result.area_square_nanometers = 0;
      result.diagnostics.push_back("clearance knockout supports axis-aligned rectangular holes only");
      return result;
    }
    if (!pointInContour(fill_hole.front(), result.contours.front())) {
      result.filled = false;
      result.contours.clear();
      result.area_square_nanometers = 0;
      result.diagnostics.push_back("zone hole must be contained by outer contour");
      return result;
    }
    result.contours.push_back(std::move(fill_hole));
    result.area_square_nanometers -= static_cast<int64_t>(std::llround(std::abs(signedArea(result.contours.back()))));
  }
  if (result.area_square_nanometers < 0) result.area_square_nanometers = 0;
  return result;
}
}  // namespace ccad
