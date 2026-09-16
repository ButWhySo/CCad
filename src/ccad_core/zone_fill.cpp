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
}  // namespace

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
  result.filled = true;
  const int64_t clearance = std::max<int64_t>(0, zone.clearance.nanometers);
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
    result.contours.push_back(std::move(fill_hole));
    result.area_square_nanometers -= static_cast<int64_t>(std::llround(std::abs(signedArea(result.contours.back()))));
  }
  if (result.area_square_nanometers < 0) result.area_square_nanometers = 0;
  return result;
}
}  // namespace ccad
