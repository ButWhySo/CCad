#include "zone_fill.hpp"

#include <cmath>

namespace ccad {
namespace {
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
  result.contours.push_back(zone.outline);
  result.area_square_nanometers = static_cast<int64_t>(std::llround(std::abs(signedArea(zone.outline))));
  for (const auto& hole : zone.holes) {
    if (hole.size() < 3) {
      result.filled = false;
      result.contours.clear();
      result.area_square_nanometers = 0;
      result.diagnostics.push_back("zone hole requires at least three points");
      return result;
    }
    result.contours.push_back(hole);
    result.area_square_nanometers -= static_cast<int64_t>(std::llround(std::abs(signedArea(hole))));
  }
  if (result.area_square_nanometers < 0) result.area_square_nanometers = 0;
  return result;
}
}  // namespace ccad
