#include "ccad_core/drc.hpp"

#include <algorithm>
#include <cmath>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ccad {
namespace {

Diagnostic makeDiagnostic(const std::string& code, const std::string& message,
                          const std::string& object_id) {
  return Diagnostic{
      .severity = "error",
      .code = code,
      .message = message,
      .object_id = object_id,
  };
}

Diagnostic makeWarning(const std::string& code, const std::string& message,
                       const std::string& object_id) {
  return Diagnostic{
      .severity = "warning",
      .code = code,
      .message = message,
      .object_id = object_id,
  };
}

bool hasLayer(const Board& board, const std::string& layer_id) {
  for (const Layer& layer : board.layers) {
    if (layer.id == layer_id) {
      return true;
    }
  }
  return false;
}

bool hasNet(const Project& project, const std::string& net_id) {
  for (const Net& net : project.nets) {
    if (net.id == net_id) {
      return true;
    }
  }
  return false;
}

bool containsPoint(const Board& board, const Point& point) {
  const Point min = board.outline.origin;
  const Point max = maxPoint(board.outline);
  return point.x.nanometers >= min.x.nanometers && point.x.nanometers <= max.x.nanometers &&
         point.y.nanometers >= min.y.nanometers && point.y.nanometers <= max.y.nanometers;
}

long double distanceToBoardEdge(const Board& board, const Point& point) {
  const std::int64_t min_x = board.outline.origin.x.nanometers;
  const std::int64_t min_y = board.outline.origin.y.nanometers;
  const Point max = maxPoint(board.outline);
  const std::int64_t max_x = max.x.nanometers;
  const std::int64_t max_y = max.y.nanometers;
  const std::int64_t left = point.x.nanometers - min_x;
  const std::int64_t right = max_x - point.x.nanometers;
  const std::int64_t bottom = point.y.nanometers - min_y;
  const std::int64_t top = max_y - point.y.nanometers;
  const std::int64_t nearest = std::min(std::min(left, right), std::min(bottom, top));
  return static_cast<long double>(nearest);
}

bool rectContainsPoint(const Rect& rect, const Point& point) {
  const Point max = maxPoint(rect);
  return point.x.nanometers >= rect.origin.x.nanometers && point.x.nanometers <= max.x.nanometers &&
         point.y.nanometers >= rect.origin.y.nanometers && point.y.nanometers <= max.y.nanometers;
}

std::vector<Point> rectCorners(const Rect& rect) {
  const Point top_left = rect.origin;
  const Point bottom_right = maxPoint(rect);
  const Point top_right{.x = bottom_right.x, .y = top_left.y};
  const Point bottom_left{.x = top_left.x, .y = bottom_right.y};
  return {top_left, top_right, bottom_right, bottom_left};
}

int orientation(const Point& a, const Point& b, const Point& c) {
  const long double ab_x = static_cast<long double>(b.x.nanometers - a.x.nanometers);
  const long double ab_y = static_cast<long double>(b.y.nanometers - a.y.nanometers);
  const long double ac_x = static_cast<long double>(c.x.nanometers - a.x.nanometers);
  const long double ac_y = static_cast<long double>(c.y.nanometers - a.y.nanometers);
  const long double cross = (ab_x * ac_y) - (ab_y * ac_x);
  if (cross > 0) {
    return 1;
  }
  if (cross < 0) {
    return -1;
  }
  return 0;
}

bool pointOnSegment(const Point& a, const Point& b, const Point& point) {
  if (orientation(a, b, point) != 0) {
    return false;
  }
  const std::int64_t min_x = std::min(a.x.nanometers, b.x.nanometers);
  const std::int64_t max_x = std::max(a.x.nanometers, b.x.nanometers);
  const std::int64_t min_y = std::min(a.y.nanometers, b.y.nanometers);
  const std::int64_t max_y = std::max(a.y.nanometers, b.y.nanometers);
  return point.x.nanometers >= min_x && point.x.nanometers <= max_x &&
         point.y.nanometers >= min_y && point.y.nanometers <= max_y;
}

bool segmentsIntersect(const Point& a, const Point& b, const Point& c, const Point& d) {
  const int o1 = orientation(a, b, c);
  const int o2 = orientation(a, b, d);
  const int o3 = orientation(c, d, a);
  const int o4 = orientation(c, d, b);
  if (o1 != o2 && o3 != o4) {
    return true;
  }
  return (o1 == 0 && pointOnSegment(a, b, c)) || (o2 == 0 && pointOnSegment(a, b, d)) ||
         (o3 == 0 && pointOnSegment(c, d, a)) || (o4 == 0 && pointOnSegment(c, d, b));
}

bool segmentIntersectsRect(const Point& start, const Point& end, const Rect& rect) {
  if (rectContainsPoint(rect, start) || rectContainsPoint(rect, end)) {
    return true;
  }

  const Point top_left = rect.origin;
  const Point bottom_right = maxPoint(rect);
  const Point top_right{.x = bottom_right.x, .y = top_left.y};
  const Point bottom_left{.x = top_left.x, .y = bottom_right.y};
  return segmentsIntersect(start, end, top_left, top_right) ||
         segmentsIntersect(start, end, top_right, bottom_right) ||
         segmentsIntersect(start, end, bottom_right, bottom_left) ||
         segmentsIntersect(start, end, bottom_left, top_left);
}

bool isPositive(const Length& length) {
  return length.nanometers > 0;
}

constexpr std::int64_t kDefaultMinTrackWidthNm = 150000;   // 0.15 mm
constexpr std::int64_t kDefaultMinAnnularRingNm = 100000;  // 0.10 mm

bool samePoint(const Point& left, const Point& right) {
  return left.x.nanometers == right.x.nanometers && left.y.nanometers == right.y.nanometers;
}

long double distanceBetweenPoints(const Point& left, const Point& right) {
  const long double dx = static_cast<long double>(left.x.nanometers - right.x.nanometers);
  const long double dy = static_cast<long double>(left.y.nanometers - right.y.nanometers);
  return std::hypotl(dx, dy);
}

long double distancePointToSegment(const Point& point, const Point& start, const Point& end) {
  const long double px = static_cast<long double>(point.x.nanometers);
  const long double py = static_cast<long double>(point.y.nanometers);
  const long double sx = static_cast<long double>(start.x.nanometers);
  const long double sy = static_cast<long double>(start.y.nanometers);
  const long double ex = static_cast<long double>(end.x.nanometers);
  const long double ey = static_cast<long double>(end.y.nanometers);
  const long double dx = ex - sx;
  const long double dy = ey - sy;
  const long double length_squared = (dx * dx) + (dy * dy);
  if (length_squared == 0.0L) {
    return distanceBetweenPoints(point, start);
  }
  const long double raw_t = (((px - sx) * dx) + ((py - sy) * dy)) / length_squared;
  const long double t = std::clamp(raw_t, 0.0L, 1.0L);
  const long double closest_x = sx + (t * dx);
  const long double closest_y = sy + (t * dy);
  return std::hypotl(px - closest_x, py - closest_y);
}

long double distanceBetweenSegments(const Point& first_start, const Point& first_end,
                                    const Point& second_start, const Point& second_end) {
  if (segmentsIntersect(first_start, first_end, second_start, second_end)) {
    return 0.0L;
  }
  return std::min({distancePointToSegment(first_start, second_start, second_end),
                   distancePointToSegment(first_end, second_start, second_end),
                   distancePointToSegment(second_start, first_start, first_end),
                   distancePointToSegment(second_end, first_start, first_end)});
}

std::vector<Point> padCorners(const Pad& pad) {
  constexpr long double pi = 3.141592653589793238462643383279502884L;
  const long double radians = (static_cast<long double>(pad.rotation_degrees) * pi) / 180.0L;
  const long double cosine = std::cos(radians);
  const long double sine = std::sin(radians);
  const long double center_x = static_cast<long double>(pad.position.x.nanometers);
  const long double center_y = static_cast<long double>(pad.position.y.nanometers);
  const long double half_width = static_cast<long double>(pad.size.width.nanometers) / 2.0L;
  const long double half_height = static_cast<long double>(pad.size.height.nanometers) / 2.0L;

  std::vector<Point> corners;
  corners.reserve(4);
  for (const auto& local : {std::pair<long double, long double>{-half_width, -half_height},
                            std::pair<long double, long double>{half_width, -half_height},
                            std::pair<long double, long double>{half_width, half_height},
                            std::pair<long double, long double>{-half_width, half_height}}) {
    const long double rotated_x = center_x + (local.first * cosine) - (local.second * sine);
    const long double rotated_y = center_y + (local.first * sine) + (local.second * cosine);
    corners.push_back(Point{.x = nanometers(static_cast<std::int64_t>(std::llround(rotated_x))),
                            .y = nanometers(static_cast<std::int64_t>(std::llround(rotated_y)))});
  }
  return corners;
}

bool pointInPolygon(const Point& point, const std::vector<Point>& polygon) {
  bool inside = false;
  for (std::size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
    const long double ix = static_cast<long double>(polygon.at(i).x.nanometers);
    const long double iy = static_cast<long double>(polygon.at(i).y.nanometers);
    const long double jx = static_cast<long double>(polygon.at(j).x.nanometers);
    const long double jy = static_cast<long double>(polygon.at(j).y.nanometers);
    const long double px = static_cast<long double>(point.x.nanometers);
    const long double py = static_cast<long double>(point.y.nanometers);
    if (pointOnSegment(polygon.at(j), polygon.at(i), point)) {
      return true;
    }
    const bool crosses_y = (iy > py) != (jy > py);
    if (crosses_y && px < (((jx - ix) * (py - iy)) / (jy - iy)) + ix) {
      inside = !inside;
    }
  }
  return inside;
}

bool segmentIntersectsPolygon(const Point& start, const Point& end,
                              const std::vector<Point>& polygon) {
  if (pointInPolygon(start, polygon) || pointInPolygon(end, polygon)) {
    return true;
  }
  for (std::size_t i = 0; i < polygon.size(); ++i) {
    const Point& edge_start = polygon.at(i);
    const Point& edge_end = polygon.at((i + 1) % polygon.size());
    if (segmentsIntersect(start, end, edge_start, edge_end)) {
      return true;
    }
  }
  return false;
}

Rect inflateRect(const Rect& rect, std::int64_t margin_nm) {
  const std::int64_t min_x = rect.origin.x.nanometers - margin_nm;
  const std::int64_t min_y = rect.origin.y.nanometers - margin_nm;
  const std::int64_t width_nm = rect.size.width.nanometers + (2 * margin_nm);
  const std::int64_t height_nm = rect.size.height.nanometers + (2 * margin_nm);
  return Rect{.origin = Point{.x = nanometers(min_x), .y = nanometers(min_y)},
              .size = Size{.width = nanometers(width_nm), .height = nanometers(height_nm)}};
}

bool polygonIntersectsRect(const std::vector<Point>& polygon, const Rect& rect) {
  const std::vector<Point> rect_polygon = rectCorners(rect);
  for (std::size_t i = 0; i < polygon.size(); ++i) {
    const Point& edge_start = polygon.at(i);
    const Point& edge_end = polygon.at((i + 1) % polygon.size());
    if (segmentIntersectsRect(edge_start, edge_end, rect)) {
      return true;
    }
  }
  for (const Point& point : polygon) {
    if (rectContainsPoint(rect, point)) {
      return true;
    }
  }
  for (const Point& rect_point : rect_polygon) {
    if (pointInPolygon(rect_point, polygon)) {
      return true;
    }
  }
  return false;
}

long double distancePointToRect(const Point& point, const Rect& rect) {
  const Point rect_max = maxPoint(rect);
  const long double px = static_cast<long double>(point.x.nanometers);
  const long double py = static_cast<long double>(point.y.nanometers);
  const long double min_x = static_cast<long double>(rect.origin.x.nanometers);
  const long double min_y = static_cast<long double>(rect.origin.y.nanometers);
  const long double max_x = static_cast<long double>(rect_max.x.nanometers);
  const long double max_y = static_cast<long double>(rect_max.y.nanometers);
  const long double clamped_x = std::clamp(px, min_x, max_x);
  const long double clamped_y = std::clamp(py, min_y, max_y);
  return std::hypotl(px - clamped_x, py - clamped_y);
}

long double distancePointToPolygon(const Point& point, const std::vector<Point>& polygon) {
  if (pointInPolygon(point, polygon)) {
    return 0.0L;
  }
  long double distance = distancePointToSegment(point, polygon.at(0), polygon.at(1));
  for (std::size_t i = 1; i < polygon.size(); ++i) {
    distance = std::min(distance, distancePointToSegment(point, polygon.at(i),
                                                        polygon.at((i + 1) % polygon.size())));
  }
  return distance;
}

long double distanceSegmentToPolygon(const Point& start, const Point& end,
                                     const std::vector<Point>& polygon) {
  if (segmentIntersectsPolygon(start, end, polygon)) {
    return 0.0L;
  }
  long double distance = distanceBetweenSegments(start, end, polygon.at(0), polygon.at(1));
  for (std::size_t i = 1; i < polygon.size(); ++i) {
    distance =
        std::min(distance, distanceBetweenSegments(start, end, polygon.at(i),
                                                  polygon.at((i + 1) % polygon.size())));
  }
  return distance;
}

long double distanceBetweenPolygons(const std::vector<Point>& first,
                                    const std::vector<Point>& second) {
  for (std::size_t i = 0; i < first.size(); ++i) {
    if (pointInPolygon(first.at(i), second)) {
      return 0.0L;
    }
  }
  for (std::size_t i = 0; i < second.size(); ++i) {
    if (pointInPolygon(second.at(i), first)) {
      return 0.0L;
    }
  }

  long double distance = distanceBetweenSegments(first.at(0), first.at(1), second.at(0),
                                                 second.at(1));
  for (std::size_t first_index = 0; first_index < first.size(); ++first_index) {
    for (std::size_t second_index = 0; second_index < second.size(); ++second_index) {
      distance = std::min(
          distance,
          distanceBetweenSegments(first.at(first_index),
                                  first.at((first_index + 1) % first.size()),
                                  second.at(second_index),
                                  second.at((second_index + 1) % second.size())));
    }
  }
  return distance;
}

bool sameNonEmptyNet(const std::string& left, const std::string& right) {
  return !left.empty() && left == right;
}

bool differentNonEmptyNets(const std::string& left, const std::string& right) {
  return !left.empty() && !right.empty() && left != right;
}

bool isCopperLayer(const Board& board, const std::string& layer_id) {
  for (const Layer& layer : board.layers) {
    if (layer.id == layer_id) {
      return layer.kind == "copper";
    }
  }
  return false;
}

bool shareCopperLayer(const Board& board, const std::string& left_layer,
                      const std::string& right_layer) {
  return left_layer == right_layer && isCopperLayer(board, left_layer);
}

void addClearanceDiagnostic(std::vector<Diagnostic>& diagnostics, const std::string& checked_id,
                            const std::string& other_id) {
  diagnostics.push_back(makeDiagnostic("COPPER_CLEARANCE",
                                       "Different-net copper is closer than default clearance to " +
                                           other_id,
                                       checked_id));
}

bool endpointTouchesSameNetPrimitive(const Board& board, const TrackSegment& source_track,
                                     const Point& endpoint) {
  if (source_track.net_id.empty()) {
    return true;
  }

  for (const Pad& pad : board.pads) {
    if (pad.net_id == source_track.net_id &&
        pointInPolygon(endpoint, padCorners(pad))) {
      return true;
    }
  }
  for (const Via& via : board.vias) {
    if (via.net_id == source_track.net_id &&
        distanceBetweenPoints(via.position, endpoint) <=
            (static_cast<long double>(via.diameter.nanometers) / 2.0L)) {
      return true;
    }
  }
  for (const TrackSegment& track : board.tracks) {
    if (track.id == source_track.id || track.net_id != source_track.net_id ||
        track.layer_id != source_track.layer_id) {
      continue;
    }
    if (samePoint(track.start, endpoint) || samePoint(track.end, endpoint) ||
        pointOnSegment(track.start, track.end, endpoint)) {
      return true;
    }
  }
  return false;
}

void checkPads(const Project& project, const Board& board, std::vector<Diagnostic>& diagnostics) {
  std::set<std::string> ids;
  for (const Pad& pad : board.pads) {
    if (!ids.insert(pad.id).second) {
      diagnostics.push_back(
          makeDiagnostic("DUPLICATE_PAD_ID", "Pad ID appears more than once", pad.id));
    }
    if (!hasLayer(board, pad.layer_id)) {
      diagnostics.push_back(
          makeDiagnostic("UNKNOWN_PAD_LAYER", "Pad references an unknown layer", pad.id));
    }
    if (!containsPoint(board, pad.position)) {
      diagnostics.push_back(
          makeDiagnostic("PAD_OUTSIDE_BOARD", "Pad position is outside board outline", pad.id));
    }
    if (!isPositive(pad.size.width) || !isPositive(pad.size.height)) {
      diagnostics.push_back(
          makeDiagnostic("INVALID_PAD_SIZE", "Pad width and height must be positive", pad.id));
    } else {
      const std::vector<Point> corners = padCorners(pad);
      bool all_corners_inside = true;
      for (const Point& corner : corners) {
        if (!containsPoint(board, corner)) {
          all_corners_inside = false;
          break;
        }
      }
      if (!all_corners_inside) {
        diagnostics.push_back(makeDiagnostic(
            "PAD_GEOMETRY_OUTSIDE_BOARD", "Pad geometry extends outside board outline", pad.id));
      }
    }
    if (pad.net_id.empty()) {
      diagnostics.push_back(makeWarning("UNCONNECTED_PAD", "Pad has no assigned net", pad.id));
    } else if (!hasNet(project, pad.net_id)) {
      diagnostics.push_back(
          makeDiagnostic("UNKNOWN_PAD_NET", "Pad references an unknown net", pad.id));
    }
    for (const Keepout& keepout : board.keepouts) {
      if (polygonIntersectsRect(padCorners(pad), keepout.area)) {
        diagnostics.push_back(
            makeDiagnostic("PAD_IN_KEEPOUT", "Pad geometry intersects keepout " + keepout.id,
                           pad.id));
      }
    }
  }
}

void checkVias(const Project& project, const Board& board, std::vector<Diagnostic>& diagnostics) {
  std::set<std::string> ids;
  for (const Via& via : board.vias) {
    if (!ids.insert(via.id).second) {
      diagnostics.push_back(
          makeDiagnostic("DUPLICATE_VIA_ID", "Via ID appears more than once", via.id));
    }
    if (!containsPoint(board, via.position)) {
      diagnostics.push_back(
          makeDiagnostic("VIA_OUTSIDE_BOARD", "Via position is outside board outline", via.id));
    }
    if (via.net_id.empty()) {
      diagnostics.push_back(makeWarning("UNCONNECTED_VIA", "Via has no assigned net", via.id));
    } else if (!hasNet(project, via.net_id)) {
      diagnostics.push_back(
          makeDiagnostic("UNKNOWN_VIA_NET", "Via references an unknown net", via.id));
    }
    if (!isPositive(via.diameter) || !isPositive(via.drill)) {
      diagnostics.push_back(makeDiagnostic("INVALID_VIA_SIZE",
                                           "Via diameter and drill must be positive", via.id));
    } else {
      const std::int64_t radius_nm = via.diameter.nanometers / 2;
      const Point min{.x = nanometers(via.position.x.nanometers - radius_nm),
                      .y = nanometers(via.position.y.nanometers - radius_nm)};
      const Point max{.x = nanometers(via.position.x.nanometers + radius_nm),
                      .y = nanometers(via.position.y.nanometers + radius_nm)};
      if (!containsPoint(board, min) || !containsPoint(board, max)) {
        diagnostics.push_back(makeDiagnostic(
            "VIA_GEOMETRY_OUTSIDE_BOARD", "Via copper geometry extends outside board outline",
            via.id));
      }
    }
    if (via.drill.nanometers > via.diameter.nanometers) {
      diagnostics.push_back(makeDiagnostic("VIA_DRILL_TOO_LARGE",
                                           "Via drill must be less than or equal to diameter",
                                           via.id));
    }
    const std::int64_t annular_ring = (via.diameter.nanometers - via.drill.nanometers) / 2;
    if (annular_ring < kDefaultMinAnnularRingNm) {
      diagnostics.push_back(
          makeDiagnostic("VIA_ANNULAR_RING_TOO_SMALL",
                         "Via annular ring is below default minimum of 0.10 mm", via.id));
    }
    for (const Keepout& keepout : board.keepouts) {
      const long double radius = static_cast<long double>(via.diameter.nanometers) / 2.0L;
      if (distancePointToRect(via.position, keepout.area) <= radius) {
        diagnostics.push_back(
            makeDiagnostic("VIA_IN_KEEPOUT", "Via geometry intersects keepout " + keepout.id,
                           via.id));
      }
    }
  }
}

void checkTracks(const Project& project, const Board& board, std::vector<Diagnostic>& diagnostics) {
  std::set<std::string> ids;
  for (const TrackSegment& track : board.tracks) {
    if (!ids.insert(track.id).second) {
      diagnostics.push_back(
          makeDiagnostic("DUPLICATE_TRACK_ID", "Track ID appears more than once", track.id));
    }
    if (!hasLayer(board, track.layer_id)) {
      diagnostics.push_back(
          makeDiagnostic("UNKNOWN_TRACK_LAYER", "Track references an unknown layer", track.id));
    }
    if (track.net_id.empty()) {
      diagnostics.push_back(
          makeWarning("UNCONNECTED_TRACK", "Track has no assigned net", track.id));
    } else if (!hasNet(project, track.net_id)) {
      diagnostics.push_back(
          makeDiagnostic("UNKNOWN_TRACK_NET", "Track references an unknown net", track.id));
    }
    if (!containsPoint(board, track.start)) {
      diagnostics.push_back(
          makeDiagnostic("TRACK_START_OUTSIDE_BOARD", "Track start is outside board outline",
                         track.id));
    }
    if (!containsPoint(board, track.end)) {
      diagnostics.push_back(makeDiagnostic("TRACK_END_OUTSIDE_BOARD",
                                           "Track end is outside board outline", track.id));
    }
    if (!isPositive(track.width)) {
      diagnostics.push_back(
          makeDiagnostic("INVALID_TRACK_WIDTH", "Track width must be positive", track.id));
    } else {
      if (track.width.nanometers < kDefaultMinTrackWidthNm) {
        diagnostics.push_back(makeDiagnostic(
            "TRACK_TOO_NARROW", "Track width is below default minimum of 0.15 mm", track.id));
      }
      const long double half_width = static_cast<long double>(track.width.nanometers) / 2.0L;
      if (distanceToBoardEdge(board, track.start) < half_width ||
          distanceToBoardEdge(board, track.end) < half_width) {
        diagnostics.push_back(makeDiagnostic(
            "TRACK_GEOMETRY_OUTSIDE_BOARD",
            "Track copper geometry extends outside board outline", track.id));
      }
    }
    if (samePoint(track.start, track.end)) {
      diagnostics.push_back(
          makeDiagnostic("ZERO_LENGTH_TRACK", "Track start and end must be different", track.id));
    }
    if (!endpointTouchesSameNetPrimitive(board, track, track.start) ||
        !endpointTouchesSameNetPrimitive(board, track, track.end)) {
      diagnostics.push_back(makeWarning("UNCONNECTED_TRACK_ENDPOINT",
                                        "Track endpoint does not touch a same-net pad, via, or track",
                                        track.id));
    }
    for (const Keepout& keepout : board.keepouts) {
      const std::int64_t half_width_nm = track.width.nanometers / 2;
      const Rect inflated_keepout = inflateRect(keepout.area, half_width_nm);
      if (rectContainsPoint(inflated_keepout, track.start) ||
          rectContainsPoint(inflated_keepout, track.end)) {
        diagnostics.push_back(makeDiagnostic(
            "TRACK_ENDPOINT_IN_KEEPOUT", "Track endpoint is inside keepout " + keepout.id,
            track.id));
      } else if (segmentIntersectsRect(track.start, track.end, inflated_keepout)) {
        diagnostics.push_back(makeDiagnostic(
            "TRACK_CROSSES_KEEPOUT", "Track segment crosses keepout " + keepout.id, track.id));
      }
    }
  }
}

void checkProjectNets(const Project& project, std::vector<Diagnostic>& diagnostics) {
  std::set<std::string> ids;
  for (const Net& net : project.nets) {
    if (net.id.empty()) {
      diagnostics.push_back(
          makeDiagnostic("INVALID_NET_ID", "Net ID must not be empty", net.id));
      continue;
    }
    if (!ids.insert(net.id).second) {
      diagnostics.push_back(
          makeDiagnostic("DUPLICATE_NET_ID", "Net ID appears more than once", net.id));
    }
  }
}

void checkLayers(const Board& board, std::vector<Diagnostic>& diagnostics) {
  std::set<std::string> ids;
  for (const Layer& layer : board.layers) {
    if (layer.id.empty()) {
      diagnostics.push_back(
          makeDiagnostic("INVALID_LAYER_ID", "Layer ID must not be empty", layer.id));
    }
    if (layer.name.empty()) {
      diagnostics.push_back(
          makeDiagnostic("INVALID_LAYER_NAME", "Layer name must not be empty", layer.id));
    }
    if (layer.kind.empty()) {
      diagnostics.push_back(
          makeDiagnostic("INVALID_LAYER_KIND", "Layer kind must not be empty", layer.id));
    }
    if (!ids.insert(layer.id).second) {
      diagnostics.push_back(
          makeDiagnostic("DUPLICATE_LAYER_ID", "Layer ID appears more than once", layer.id));
    }
  }
}

void checkKeepouts(const Board& board, std::vector<Diagnostic>& diagnostics) {
  std::set<std::string> ids;
  for (const Keepout& keepout : board.keepouts) {
    if (keepout.id.empty()) {
      diagnostics.push_back(makeDiagnostic("INVALID_KEEPOUT_ID",
                                           "Keepout ID must not be empty", keepout.id));
    }
    if (!ids.insert(keepout.id).second) {
      diagnostics.push_back(makeDiagnostic("DUPLICATE_KEEPOUT_ID",
                                           "Keepout ID appears more than once", keepout.id));
    }

    if (!isPositive(keepout.area.size.width) || !isPositive(keepout.area.size.height)) {
      diagnostics.push_back(makeDiagnostic("INVALID_KEEPOUT_SIZE",
                                           "Keepout width and height must be positive",
                                           keepout.id));
      continue;
    }

    if (keepout.kind != "placement" && keepout.kind != "routing") {
      diagnostics.push_back(makeDiagnostic("UNKNOWN_KEEPOUT_KIND",
                                           "Keepout kind must be placement or routing",
                                           keepout.id));
    }

    const Point min = keepout.area.origin;
    const Point max = maxPoint(keepout.area);
    if (!containsPoint(board, min) || !containsPoint(board, max)) {
      diagnostics.push_back(makeDiagnostic("KEEPOUT_OUTSIDE_BOARD",
                                           "Keepout area is outside board outline", keepout.id));
    }
  }
}

void checkCopperClearance(const Board& board, std::vector<Diagnostic>& diagnostics) {
  constexpr std::int64_t default_clearance_nm = 200000;
  constexpr long double default_clearance = static_cast<long double>(default_clearance_nm);

  for (std::size_t left = 0; left < board.pads.size(); ++left) {
    for (std::size_t right = left + 1; right < board.pads.size(); ++right) {
      const Pad& left_pad = board.pads.at(left);
      const Pad& right_pad = board.pads.at(right);
      if (sameNonEmptyNet(left_pad.net_id, right_pad.net_id) ||
          !differentNonEmptyNets(left_pad.net_id, right_pad.net_id) ||
          !shareCopperLayer(board, left_pad.layer_id, right_pad.layer_id)) {
        continue;
      }
      if (distanceBetweenPolygons(padCorners(left_pad), padCorners(right_pad)) <
          default_clearance) {
        addClearanceDiagnostic(diagnostics, right_pad.id, left_pad.id);
      }
    }
  }

  for (std::size_t left = 0; left < board.tracks.size(); ++left) {
    for (std::size_t right = left + 1; right < board.tracks.size(); ++right) {
      const TrackSegment& left_track = board.tracks.at(left);
      const TrackSegment& right_track = board.tracks.at(right);
      if (sameNonEmptyNet(left_track.net_id, right_track.net_id) ||
          !differentNonEmptyNets(left_track.net_id, right_track.net_id) ||
          !shareCopperLayer(board, left_track.layer_id, right_track.layer_id)) {
        continue;
      }
      const long double edge_distance =
          distanceBetweenSegments(left_track.start, left_track.end, right_track.start,
                                  right_track.end) -
          (static_cast<long double>(left_track.width.nanometers) / 2.0L) -
          (static_cast<long double>(right_track.width.nanometers) / 2.0L);
      if (edge_distance < default_clearance) {
        addClearanceDiagnostic(diagnostics, right_track.id, left_track.id);
      }
    }
  }

  for (const Pad& pad : board.pads) {
    for (const TrackSegment& track : board.tracks) {
      if (sameNonEmptyNet(pad.net_id, track.net_id) ||
          !differentNonEmptyNets(pad.net_id, track.net_id) ||
          !shareCopperLayer(board, pad.layer_id, track.layer_id)) {
        continue;
      }
      const long double edge_distance =
          distanceSegmentToPolygon(track.start, track.end, padCorners(pad)) -
          (static_cast<long double>(track.width.nanometers) / 2.0L);
      if (edge_distance < default_clearance) {
        addClearanceDiagnostic(diagnostics, track.id, pad.id);
      }
    }
  }

  for (std::size_t left = 0; left < board.vias.size(); ++left) {
    for (std::size_t right = left + 1; right < board.vias.size(); ++right) {
      const Via& left_via = board.vias.at(left);
      const Via& right_via = board.vias.at(right);
      if (sameNonEmptyNet(left_via.net_id, right_via.net_id) ||
          !differentNonEmptyNets(left_via.net_id, right_via.net_id)) {
        continue;
      }
      const long double edge_distance =
          distanceBetweenPoints(left_via.position, right_via.position) -
          (static_cast<long double>(left_via.diameter.nanometers) / 2.0L) -
          (static_cast<long double>(right_via.diameter.nanometers) / 2.0L);
      if (edge_distance < default_clearance) {
        addClearanceDiagnostic(diagnostics, right_via.id, left_via.id);
      }
    }
  }

  for (const Via& via : board.vias) {
    for (const Pad& pad : board.pads) {
      if (sameNonEmptyNet(via.net_id, pad.net_id) ||
          !differentNonEmptyNets(via.net_id, pad.net_id) || !isCopperLayer(board, pad.layer_id)) {
        continue;
      }
      const long double edge_distance =
          distancePointToPolygon(via.position, padCorners(pad)) -
          (static_cast<long double>(via.diameter.nanometers) / 2.0L);
      if (edge_distance < default_clearance) {
        addClearanceDiagnostic(diagnostics, via.id, pad.id);
      }
    }
    for (const TrackSegment& track : board.tracks) {
      if (sameNonEmptyNet(via.net_id, track.net_id) ||
          !differentNonEmptyNets(via.net_id, track.net_id) ||
          !isCopperLayer(board, track.layer_id)) {
        continue;
      }
      const long double edge_distance =
          distancePointToSegment(via.position, track.start, track.end) -
          (static_cast<long double>(via.diameter.nanometers) / 2.0L) -
          (static_cast<long double>(track.width.nanometers) / 2.0L);
      if (edge_distance < default_clearance) {
        addClearanceDiagnostic(diagnostics, via.id, track.id);
      }
    }
  }
}

}  // namespace

std::vector<Diagnostic> runDrc(const Project& project) {
  std::vector<Diagnostic> diagnostics;
  if (!project.board.has_value()) {
    return diagnostics;
  }

  checkProjectNets(project, diagnostics);
  const Board& board = *project.board;
  checkLayers(board, diagnostics);
  checkPads(project, board, diagnostics);
  checkVias(project, board, diagnostics);
  checkTracks(project, board, diagnostics);
  checkKeepouts(board, diagnostics);
  checkCopperClearance(board, diagnostics);
  return diagnostics;
}

}  // namespace ccad
