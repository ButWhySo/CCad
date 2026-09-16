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

long double cross(const Point& a, const Point& b, const Point& c) {
  return static_cast<long double>(b.x.nanometers - a.x.nanometers) *
             (c.y.nanometers - a.y.nanometers) -
         static_cast<long double>(b.y.nanometers - a.y.nanometers) *
             (c.x.nanometers - a.x.nanometers);
}

bool onSegment(const Point& a, const Point& b, const Point& p) {
  return cross(a, b, p) == 0.0L &&
         p.x.nanometers >= std::min(a.x.nanometers, b.x.nanometers) &&
         p.x.nanometers <= std::max(a.x.nanometers, b.x.nanometers) &&
         p.y.nanometers >= std::min(a.y.nanometers, b.y.nanometers) &&
         p.y.nanometers <= std::max(a.y.nanometers, b.y.nanometers);
}

bool segmentsIntersect(const Point& a, const Point& b, const Point& c, const Point& d) {
  const long double ab_c = cross(a, b, c);
  const long double ab_d = cross(a, b, d);
  const long double cd_a = cross(c, d, a);
  const long double cd_b = cross(c, d, b);
  if (((ab_c > 0) != (ab_d > 0)) && ((cd_a > 0) != (cd_b > 0))) return true;
  return onSegment(a, b, c) || onSegment(a, b, d) || onSegment(c, d, a) ||
         onSegment(c, d, b);
}

bool blockedByHole(const Point& start, const Point& end,
                  const std::vector<std::vector<Point>>& holes) {
  return std::any_of(holes.begin(), holes.end(), [&](const auto& hole) {
    if (hole.size() < 3) return false;
    if (pointInContour(start, hole) || pointInContour(end, hole)) return true;
    for (std::size_t i = 0; i < hole.size(); ++i)
      if (segmentsIntersect(start, end, hole[i], hole[(i + 1) % hole.size()])) return true;
    return false;
  });
}

bool padOnZoneLayer(const BoardZone& zone, const Pad& pad) {
  if (zone.layer_ids.empty() || pad.padstack.layer_set.empty()) return true;
  return std::any_of(zone.layer_ids.begin(), zone.layer_ids.end(), [&](const std::string& zone_layer) {
    return std::any_of(pad.padstack.layer_set.begin(), pad.padstack.layer_set.end(),
                       [&](const std::string& pad_layer) {
                         return pad_layer == zone_layer ||
                                (pad_layer == "*.Cu" && zone_layer.ends_with(".Cu"));
                       });
  });
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
  for (const auto& hole : zone.holes)
    if (hole.size() >= 3 && pointInContour(pad_center, hole)) return spokes;
  const int64_t min_x = std::min_element(outer.begin(), outer.end(),
      [](const Point& a, const Point& b) { return a.x.nanometers < b.x.nanometers; })->x.nanometers;
  const int64_t max_x = std::max_element(outer.begin(), outer.end(),
      [](const Point& a, const Point& b) { return a.x.nanometers < b.x.nanometers; })->x.nanometers;
  const int64_t min_y = std::min_element(outer.begin(), outer.end(),
      [](const Point& a, const Point& b) { return a.y.nanometers < b.y.nanometers; })->y.nanometers;
  const int64_t max_y = std::max_element(outer.begin(), outer.end(),
      [](const Point& a, const Point& b) { return a.y.nanometers < b.y.nanometers; })->y.nanometers;
  const int64_t start = pad_radius.nanometers + gap.nanometers;
  const auto add = [&](Point spoke_start, Point spoke_end) {
    if (!blockedByHole(spoke_start, spoke_end, zone.holes))
      spokes.push_back({spoke_start, spoke_end, spoke_width});
  };
  if (pad_center.x.nanometers + start < max_x)
    add({nanometers(pad_center.x.nanometers + start), pad_center.y},
        {nanometers(max_x), pad_center.y});
  if (pad_center.x.nanometers - start > min_x)
    add({nanometers(pad_center.x.nanometers - start), pad_center.y},
        {nanometers(min_x), pad_center.y});
  if (pad_center.y.nanometers + start < max_y)
    add({pad_center.x, nanometers(pad_center.y.nanometers + start)},
        {pad_center.x, nanometers(max_y)});
  if (pad_center.y.nanometers - start > min_y)
    add({pad_center.x, nanometers(pad_center.y.nanometers - start)},
        {pad_center.x, nanometers(min_y)});
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

ZoneFillResult calculateZoneFill(const BoardZone& zone, const std::vector<Pad>& pads) {
  return calculateZoneFill(zone, pads, nanometers(1), nanometers(1));
}

ZoneFillResult calculateZoneFill(const BoardZone& zone, const std::vector<Pad>& pads,
                                 Length gap, Length spoke_width) {
  ZoneFillResult result = calculateZoneFill(zone);
  if (!result.filled || zone.pad_connection == "direct" || zone.pad_connection == "solid" ||
      zone.pad_connection == "none")
    return result;
  for (const Pad& pad : pads) {
    if (pad.net_id != zone.net_id || pad.net_id.empty() || !padOnZoneLayer(zone, pad)) continue;
    int64_t width = 0;
    for (const auto& [layer, props] : pad.padstack.copper_props) {
      (void)layer;
      width = std::max(props.shape.size.width.nanometers, props.shape.size.height.nanometers);
      if (width > 0) break;
    }
    const int64_t radius = width / 2;
    if (radius <= 0) continue;
    const auto spokes = buildRectangularThermalSpokes(
        zone, pad.position, nanometers(radius), gap, spoke_width);
    result.thermal_spokes.insert(result.thermal_spokes.end(), spokes.begin(), spokes.end());
  }
  return result;
}
}  // namespace ccad
